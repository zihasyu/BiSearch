/*
 * client_chunker.cc — Client-side: read files, cut chunks, AES-GCM encrypt,
 * push EncryptedChunkPackets onto the ThreadSafeQueue.
 *
 * Mirrors the original Chunker logic (FASTCDC/TAR/Fixed) but outputs
 * encrypted packets instead of TChunk_t into a MessageQueue.
 *
 * For simplicity this prototype supports only FASTCDC on plain files
 * (no MTar preprocessing).  TAR-aware chunking can be added later
 * by copying the Chunker::CutPointTarFast logic.
 */

#include "client_chunker.h"
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <openssl/evp.h>
#include <openssl/rand.h>

/* ---- Gear hash table (copied from original define.h) ---- */
static const uint32_t GEAR[] = {
    0x5C95C078,0x22408989,0x2D48A214,0x12842087,0x530F8AFB,0x474536B9,
    0x2963B4F1,0x44CB738B,0x4EA7403D,0x4D606B6E,0x074EC5D3,0x3AF39D18,
    0x726003CA,0x37A62A74,0x51A2F58E,0x7506358E,0x5D4AB128,0x4D4AE17B,
    0x41E85924,0x470C36F7,0x4741CBE1,0x01BB7F30,0x617C1DE3,0x2B0C3A1F,
    0x50C48F73,0x21A82D37,0x6095ACE0,0x419167A0,0x3CAF49B0,0x40CEA62D,
    0x66BC1C66,0x545E1DAD,0x2BFA77CD,0x6E85DA24,0x5FB0BDC5,0x652CFC29,
    0x3A0AE1AB,0x2837E0F3,0x6387B70E,0x13176012,0x4362C2BB,0x66D8F4B1,
    0x37FCE834,0x2C9CD386,0x21144296,0x627268A8,0x650DF537,0x2805D579,
    0x3B21EBBD,0x7357ED34,0x3F58B583,0x7150DDCA,0x7362225E,0x620A6070
};
static const size_t MIN_CHUNK = 4096;
static const size_t AVG_CHUNK = 8192;
static const size_t MAX_CHUNK = 16384;

static uint64_t cutpoint_fastcdc(const uint8_t* src, size_t len)
{
    uint32_t fp = 0;
    size_t i = std::min(len, MIN_CHUNK);
    uint32_t bits = (uint32_t)std::round(std::log2((double)AVG_CHUNK));
    uint32_t maskS = (1u << (bits + 1)) - 1;
    uint32_t maskL = (1u << (bits - 1)) - 1;
    size_t norm = std::min((size_t)(AVG_CHUNK - AVG_CHUNK / 2), len);
    for (; i < norm; i++) {
        fp = (fp >> 1) + GEAR[src[i] & 0x3F];
        if (!(fp & maskS)) return i + 1;
    }
    size_t maxn = std::min(len, MAX_CHUNK);
    for (; i < maxn; i++) {
        fp = (fp >> 1) + GEAR[src[i] & 0x3F];
        if (!(fp & maskL)) return i + 1;
    }
    return i;
}

/* ---- AES-128-GCM encrypt via OpenSSL (untrusted side) ---- */
static bool aes_gcm_encrypt(
    const uint8_t key[16],
    const uint8_t* plaintext, size_t plain_len,
    uint8_t iv_out[12],
    uint8_t mac_out[16],
    std::vector<uint8_t>& ciphertext_out)
{
    if (!RAND_bytes(iv_out, 12)) return false;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    ciphertext_out.resize(plain_len + 16);
    int len = 0, total = 0;
    bool ok = true;

    if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL)) { ok = false; goto done; }
    if (!EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv_out))              { ok = false; goto done; }
    if (!EVP_EncryptUpdate(ctx, ciphertext_out.data(), &len,
                           plaintext, (int)plain_len))                  { ok = false; goto done; }
    total = len;
    if (!EVP_EncryptFinal_ex(ctx, ciphertext_out.data() + total, &len)){ ok = false; goto done; }
    total += len;
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, mac_out))  { ok = false; goto done; }
    ciphertext_out.resize((size_t)total);

done:
    EVP_CIPHER_CTX_free(ctx);
    return ok;
}

/* ================================================================
 * ClientChunker::Run — called in the Client thread.
 * Reads each file, cuts chunks, encrypts, pushes to queue.
 * ================================================================ */
void ClientChunker::Run(
    const std::vector<std::string>& file_list,
    int num_versions,
    const uint8_t client_key[16],
    ThreadSafeQueue<EncryptedChunkPacket>& out_queue)
{
    static const size_t READ_BUF = 32 * 1024 * 1024;  /* 32 MiB */
    std::vector<uint8_t> read_buf(READ_BUF);

    for (int v = 0; v < num_versions; v++) {
        const std::string& path = file_list[(size_t)v];
        std::ifstream fin(path, std::ios::binary);
        if (!fin.is_open()) {
            fprintf(stderr, "[Client] Cannot open %s\n", path.c_str());
            continue;
        }

        size_t total_offset = 0;
        bool eof_reached = false;

        while (!eof_reached) {
            fin.read((char*)read_buf.data(), (std::streamsize)READ_BUF);
            eof_reached = fin.eof();
            size_t len = (size_t)fin.gcount();
            if (len == 0) break;

            size_t local = 0;
            while (local < len) {
                size_t remaining = len - local;
                if (remaining < MAX_CHUNK && !eof_reached) break;

                size_t cp = cutpoint_fastcdc(read_buf.data() + local, remaining);
                if (cp == 0) break;

                /* Build metadata */
                EncryptedChunkPacket pkt;
                memset(&pkt.meta, 0, sizeof(ChunkMeta_t));
                pkt.meta.chunk_size = cp;
                pkt.meta.name_exist = 1;
                /* In a full implementation, TAR header parsing would fill
                   parent_dir_hash, file_name_hash, mtime, typeflag, mode, etc.
                   For this prototype we zero-fill them. */

                /* Encrypt chunk */
                bool enc_ok = aes_gcm_encrypt(
                    client_key,
                    read_buf.data() + local, cp,
                    pkt.iv, pkt.mac,
                    pkt.encrypted_payload);

                if (!enc_ok) {
                    fprintf(stderr, "[Client] Encryption failed, skipping chunk.\n");
                    local += cp;
                    continue;
                }
                pkt.encrypted_len = pkt.encrypted_payload.size();

                out_queue.Push(std::move(pkt));
                local += cp;
            }

            total_offset += local;
            fin.seekg((std::streamoff)total_offset, std::ios::beg);
        }
        fin.close();

        /* Sentinel: push one packet with chunk_size == 0 to signal version end */
        EncryptedChunkPacket sentinel;
        memset(&sentinel.meta, 0, sizeof(ChunkMeta_t));
        sentinel.meta.chunk_size = 0;
        sentinel.encrypted_len   = 0;
        out_queue.Push(std::move(sentinel));
    }

    out_queue.SetDone();
}

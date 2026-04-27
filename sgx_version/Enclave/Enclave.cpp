/*
 * Enclave.cpp — ECALL entry points.
 *
 * All ECALLs declared in Enclave.edl are implemented here.
 * The Enclave holds a single global EnclaveBiSearch instance
 * whose lifetime spans all backup versions.
 */

#include "Enclave_t.h"   /* edger8r-generated trusted header */
#include "enclave_bisearch.h"
#include "enclave_datawrite.h"
#include "enclave_crypto.h"
#include "../include/sgx_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ---- Global Enclave state ---- */
static EnclaveBiSearch*   g_bisearch   = nullptr;
static EnclaveDataWrite*  g_datawrite  = nullptr;
static uint8_t            g_client_key[SGX_AES_GCM_KEY_SIZE] = {0};
/* Storage re-encryption key (different from client key for defence in depth) */
static uint8_t            g_storage_key[SGX_AES_GCM_KEY_SIZE] = {0};

/* Simple log helper via OCALL */
static void elog(const char* msg)
{
    ocall_print_string(msg);
}

/* ================================================================
 * ecall_init_enclave
 * ================================================================ */
int ecall_init_enclave(int method, double ratio, double accept_thr,
                       int turn_on_name, uint8_t* aes_key)
{
    if (g_bisearch || g_datawrite) return -1;   /* already initialised */

    memcpy(g_client_key, aes_key, SGX_AES_GCM_KEY_SIZE);

    /* Derive storage key as XOR with fixed diversifier (prototype only).
       In production, use proper key derivation via sgx_key_request. */
    for (int i = 0; i < SGX_AES_GCM_KEY_SIZE; i++)
        g_storage_key[i] = g_client_key[i] ^ 0xA5;

    if (method != SGX_METHOD_BISEARCH) {
        elog("[Enclave] Only BiSearch method supported in SGX prototype.\n");
        return -2;
    }

    g_datawrite = new EnclaveDataWrite();
    g_datawrite->SetStorageKey(g_storage_key);

    g_bisearch  = new EnclaveBiSearch(ratio, accept_thr, turn_on_name);
    g_bisearch->dataWrite_ = g_datawrite;

    elog("[Enclave] Initialised. Method=BiSearch.\n");
    return 0;
}

/* ================================================================
 * ecall_process_chunk
 * ================================================================ */
int ecall_process_chunk(uint8_t* encrypted_data, size_t enc_len,
                        ChunkMeta_t* meta, uint8_t* mac, uint8_t* iv)
{
    if (!g_bisearch) return -1;

    /* 1. Allocate plaintext buffer inside Enclave (trusted memory) */
    uint8_t* plaintext = (uint8_t*)malloc(meta->chunk_size);
    if (!plaintext) return -2;

    /* 2. Decrypt using the client AES-GCM key */
    sgx_status_t status = enclave_aes_gcm_decrypt(
        g_client_key,
        encrypted_data, enc_len,
        iv, mac,
        plaintext);

    if (status != SGX_SUCCESS) {
        free(plaintext);
        elog("[Enclave] Chunk decryption failed.\n");
        return -3;
    }

    /* 3. Build a trusted TChunk_t from the metadata + plaintext */
    TChunk_t chunk;
    memset(&chunk, 0, sizeof(TChunk_t));
    chunk.chunkSize     = meta->chunk_size;
    chunk.name          = meta->name_hash;
    chunk.parentDirName = meta->parent_dir_hash;
    chunk.fileName      = meta->file_name_hash;
    chunk.mtime         = meta->mtime;
    chunk.mode          = meta->mode;
    chunk.typeflag      = meta->typeflag;
    chunk.headerFlag    = (meta->header_flag != 0);
    chunk.cdcFlag       = (meta->cdc_flag    != 0);
    chunk.nameExist     = (meta->name_exist  != 0);
    chunk.uname         = std::string(meta->uname);
    chunk.gname         = std::string(meta->gname);
    chunk.linkname      = std::string(meta->linkname);
    chunk.chunkPtr      = plaintext;   /* ownership transferred to chunk */
    chunk.saveSize      = chunk.chunkSize;
    chunk.deltaFlag     = SGX_NO_DELTA;
    chunk.basechunkID   = -1;
    chunk.loadFromDisk  = false;

    /* 4. Run BiSearch dedup + delta + lz4 logic */
    return g_bisearch->ProcessChunk(chunk);
}

/* ================================================================
 * ecall_version_done
 * ================================================================ */
int ecall_version_done()
{
    if (!g_bisearch) return -1;
    g_bisearch->VersionDone();
    elog("[Enclave] Version done, container flushed.\n");
    return 0;
}

/* ================================================================
 * ecall_get_stats
 * ================================================================ */
int ecall_get_stats(MethodStats_t* stats)
{
    if (!g_bisearch || !stats) return -1;
    g_bisearch->GetStats(stats);
    return 0;
}

/* ================================================================
 * ecall_destroy
 * ================================================================ */
void ecall_destroy()
{
    delete g_bisearch;  g_bisearch  = nullptr;
    delete g_datawrite; g_datawrite = nullptr;
    elog("[Enclave] Resources freed.\n");
}

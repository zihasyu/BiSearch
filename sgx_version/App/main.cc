/*
 * main.cc — Program entry point for the SGX BiSearch prototype.
 *
 * Thread model:
 *   Thread 0 (Client): reads files, cuts chunks, AES-encrypts -> queue
 *   Thread 1 (Server): pops from queue, ECALLs into Enclave per chunk
 *
 * Usage:
 *   ./bisearch_sgx -i <input_dir> -n <num_versions>
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "App.h"
#include "client_chunker.h"
#include "../include/sgx_common.h"
#include "../include/secure_queue.h"
#include "sgx_urts.h"
#include "Enclave_u.h"

/* ---- Traverse input directory ---- */
static void traverse_dir(const std::string& root, std::vector<std::string>& files)
{
    DIR* dir = opendir(root.c_str());
    if (!dir) { perror(root.c_str()); return; }
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        std::string full = root + "/" + ent->d_name;
        if (ent->d_type == DT_REG)       files.push_back(full);
        else if (ent->d_type == DT_DIR)  traverse_dir(full, files);
    }
    closedir(dir);
}

/* ---- Hardcoded test key (all 0xAB) — replace with real KMS in production ---- */
static const uint8_t CLIENT_KEY[SGX_AES_GCM_KEY_SIZE] = {
    0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,
    0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,0xAB,0xAB
};

int main(int argc, char** argv)
{
    std::string input_dir;
    int num_versions = -1;

    int opt;
    while ((opt = getopt(argc, argv, "i:n:")) != -1) {
        if (opt == 'i') input_dir    = optarg;
        if (opt == 'n') num_versions = atoi(optarg);
    }

    if (input_dir.empty() || num_versions <= 0) {
        fprintf(stderr, "Usage: %s -i <input_dir> -n <num_versions>\n", argv[0]);
        return 1;
    }

    /* Collect input files */
    std::vector<std::string> file_list;
    traverse_dir(input_dir, file_list);
    std::sort(file_list.begin(), file_list.end());

    if ((int)file_list.size() < num_versions) {
        fprintf(stderr, "[App] Not enough files (%zu) for %d versions.\n",
                file_list.size(), num_versions);
        return 1;
    }

    /* Create output directory */
    mkdir("./sgx_Containers", 0755);

    /* ---- Initialise Enclave ---- */
    if (init_enclave() != 0) return 1;

    int ret = 0;
    sgx_status_t status = ecall_init_enclave(
        g_eid, &ret,
        SGX_METHOD_BISEARCH,
        0.1,   /* ratio β */
        0.0,   /* accept_threshold */
        1,     /* turn_on_name */
        (uint8_t*)CLIENT_KEY);

    if (status != SGX_SUCCESS || ret != 0) {
        fprintf(stderr, "[App] ecall_init_enclave failed: sgx=0x%x ret=%d\n",
                status, ret);
        destroy_enclave();
        return 1;
    }

    /* ---- Shared queue simulating the C/S network channel ---- */
    ThreadSafeQueue<EncryptedChunkPacket> channel(4096);

    auto t_start = std::chrono::high_resolution_clock::now();

    /* ---- Client thread ---- */
    std::thread client_thread([&]() {
        ClientChunker::Run(file_list, num_versions, CLIENT_KEY, channel);
    });

    /* ---- Server thread (untrusted host) ---- */
    std::thread server_thread([&]() {
        EncryptedChunkPacket pkt;
        while (channel.Pop(pkt)) {
            if (pkt.meta.chunk_size == 0) {
                /* Version-end sentinel */
                int r = 0;
                sgx_status_t s = ecall_version_done(g_eid, &r);
                if (s != SGX_SUCCESS || r != 0)
                    fprintf(stderr, "[Server] ecall_version_done error\n");
                continue;
            }

            int r = 0;
            sgx_status_t s = ecall_process_chunk(
                g_eid, &r,
                pkt.encrypted_payload.data(), pkt.encrypted_len,
                &pkt.meta,
                pkt.mac,
                pkt.iv);

            if (s != SGX_SUCCESS || r != 0)
                fprintf(stderr, "[Server] ecall_process_chunk error: sgx=0x%x r=%d\n", s, r);
        }
    });

    client_thread.join();
    server_thread.join();

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();

    /* ---- Retrieve and print stats ---- */
    MethodStats_t stats;
    memset(&stats, 0, sizeof(stats));
    ecall_get_stats(g_eid, &ret, &stats);

    printf("\n========== SGX BiSearch Results ==========\n");
    printf("Total time        : %.2f s\n", elapsed);
    printf("Logical chunks    : %lu\n", stats.logical_chunk_num);
    printf("Unique chunks     : %lu\n", stats.unique_chunk_num);
    printf("Base chunks       : %lu\n", stats.base_chunk_num);
    printf("Delta chunks      : %lu\n", stats.delta_chunk_num);
    printf("Logical size      : %.2f MiB\n", (double)stats.logical_chunk_size / 1048576.0);
    printf("Compressed size   : %.2f MiB\n", (double)stats.unique_chunk_size  / 1048576.0);
    if (stats.unique_chunk_size > 0)
        printf("Compression ratio : %.4f\n",
               (double)stats.logical_chunk_size / (double)stats.unique_chunk_size);
    printf("Throughput        : %.2f MiB/s\n",
           (double)stats.logical_chunk_size / elapsed / 1048576.0);
    printf("==========================================\n");

    /* ---- Cleanup ---- */
    ecall_destroy(g_eid);
    destroy_enclave();
    return 0;
}

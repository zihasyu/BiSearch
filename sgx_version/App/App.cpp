/*
 * App.cpp — Untrusted host: OCALL implementations + Enclave lifecycle.
 */

#include "App.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <fstream>
#include <iostream>

#include "sgx_urts.h"
#include "Enclave_u.h"   /* edger8r-generated untrusted header */

/* Global Enclave ID */
sgx_enclave_id_t g_eid = 0;

static const char ENCLAVE_FILENAME[] = "bisearch_enclave.signed.so";

int init_enclave()
{
    sgx_status_t ret = sgx_create_enclave(
        ENCLAVE_FILENAME,
        SGX_DEBUG_FLAG,   /* 1 = debug mode, 0 = production */
        NULL, NULL,
        &g_eid, NULL);

    if (ret != SGX_SUCCESS) {
        fprintf(stderr, "[App] sgx_create_enclave failed: 0x%x\n", ret);
        return -1;
    }
    printf("[App] Enclave created (eid=%lu).\n", g_eid);
    return 0;
}

void destroy_enclave()
{
    sgx_destroy_enclave(g_eid);
    g_eid = 0;
}

/* ================================================================
 * OCALL implementations
 * ================================================================ */

/*
 * ocall_write_container — persist one encrypted container blob to disk.
 * File naming: ./sgx_Containers/<container_id>
 */
void ocall_write_container(const uint8_t* data, size_t data_len, uint64_t container_id)
{
    std::string path = "./sgx_Containers/" + std::to_string(container_id);
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        fprintf(stderr, "[App] ocall_write_container: cannot open %s\n", path.c_str());
        return;
    }
    ofs.write(reinterpret_cast<const char*>(data), (std::streamsize)data_len);
    ofs.close();
    printf("[App] Container %lu written (%zu bytes).\n", container_id, data_len);
}

/*
 * ocall_print_string — forward Enclave log messages to stdout.
 */
void ocall_print_string(const char* str)
{
    fputs(str, stdout);
    fflush(stdout);
}

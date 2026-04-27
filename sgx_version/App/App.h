#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stddef.h>
#include "sgx_urts.h"

extern sgx_enclave_id_t g_eid;

int  init_enclave();
void destroy_enclave();

/* OCALL implementations — called by the Enclave */
void ocall_write_container(const uint8_t* data, size_t data_len, uint64_t container_id);
void ocall_print_string(const char* str);

#endif /* APP_H */

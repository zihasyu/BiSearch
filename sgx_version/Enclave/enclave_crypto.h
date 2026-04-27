#ifndef ENCLAVE_CRYPTO_H
#define ENCLAVE_CRYPTO_H

/*
 * Enclave-side cryptographic helpers.
 *
 * Uses Intel SGX SDK's sgx_tcrypto library so that all crypto operations
 * happen inside the trusted boundary without relying on OpenSSL.
 *
 * NOTE: This file will only compile against the SGX SDK headers.
 *       When you build on a non-SGX machine you can stub these out
 *       or skip compiling Enclave sources.
 */

#include <sgx_tcrypto.h>
#include <sgx_trts.h>
#include <stdint.h>
#include <string.h>
#include "../include/sgx_common.h"

/* ---------- SHA-256 ---------- */

static inline sgx_status_t enclave_sha256(
    const uint8_t* data, size_t data_len,
    uint8_t hash_out[SGX_CHUNK_HASH_SIZE])
{
    sgx_sha256_hash_t hash;
    sgx_status_t ret = sgx_sha256_msg(data, (uint32_t)data_len, &hash);
    if (ret == SGX_SUCCESS)
        memcpy(hash_out, hash, SGX_CHUNK_HASH_SIZE);
    return ret;
}

/* ---------- AES-128-GCM decrypt ---------- */

static inline sgx_status_t enclave_aes_gcm_decrypt(
    const uint8_t key[SGX_AES_GCM_KEY_SIZE],
    const uint8_t* ciphertext, size_t cipher_len,
    const uint8_t iv[SGX_AES_GCM_IV_SIZE],
    const uint8_t mac[SGX_AES_GCM_MAC_SIZE],
    uint8_t* plaintext_out)
{
    return sgx_rijndael128GCM_decrypt(
        (const sgx_aes_gcm_128bit_key_t*)key,
        ciphertext, (uint32_t)cipher_len,
        plaintext_out,
        iv, SGX_AES_GCM_IV_SIZE,
        NULL, 0,                              /* no AAD */
        (const sgx_aes_gcm_128bit_tag_t*)mac);
}

/* ---------- AES-128-GCM encrypt ---------- */

static inline sgx_status_t enclave_aes_gcm_encrypt(
    const uint8_t key[SGX_AES_GCM_KEY_SIZE],
    const uint8_t* plaintext, size_t plain_len,
    uint8_t iv[SGX_AES_GCM_IV_SIZE],
    uint8_t mac_out[SGX_AES_GCM_MAC_SIZE],
    uint8_t* ciphertext_out)
{
    /* Generate a random IV inside the Enclave */
    sgx_status_t ret = sgx_read_rand(iv, SGX_AES_GCM_IV_SIZE);
    if (ret != SGX_SUCCESS) return ret;

    return sgx_rijndael128GCM_encrypt(
        (const sgx_aes_gcm_128bit_key_t*)key,
        plaintext, (uint32_t)plain_len,
        ciphertext_out,
        iv, SGX_AES_GCM_IV_SIZE,
        NULL, 0,                              /* no AAD */
        (sgx_aes_gcm_128bit_tag_t*)mac_out);
}

#endif /* ENCLAVE_CRYPTO_H */

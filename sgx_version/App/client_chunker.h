#ifndef CLIENT_CHUNKER_H
#define CLIENT_CHUNKER_H

#include <vector>
#include <string>
#include <stdint.h>
#include "../include/sgx_common.h"
#include "../include/secure_queue.h"

class ClientChunker {
public:
    /* Entry point for the Client thread.
     * Reads up to num_versions files from file_list, cuts them into chunks
     * using FASTCDC, encrypts each chunk with AES-128-GCM, and pushes the
     * result onto out_queue.  Pushes a sentinel (chunk_size==0) after each
     * version, then calls out_queue.SetDone() when all versions are done. */
    static void Run(
        const std::vector<std::string>& file_list,
        int num_versions,
        const uint8_t client_key[SGX_AES_GCM_KEY_SIZE],
        ThreadSafeQueue<EncryptedChunkPacket>& out_queue);
};

#endif /* CLIENT_CHUNKER_H */

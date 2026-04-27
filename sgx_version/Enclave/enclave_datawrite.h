#ifndef ENCLAVE_DATAWRITE_H
#define ENCLAVE_DATAWRITE_H

/*
 * Trusted dataWrite — runs entirely inside the Enclave.
 *
 * It mirrors the original dataWrite but:
 *   1. Uses OCALL instead of fstream for I/O.
 *   2. Re-encrypts Container data before calling OCALL.
 *   3. Strips all iostream / cout / boost dependencies.
 */

#include "../include/sgx_common.h"
#include "enclave_crypto.h"
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <unordered_map>
#include <string>

/* ---- Trusted Chunk_t (Enclave-internal, with pointer) ---- */
struct TChunk_t {
    uint64_t chunkID;
    uint64_t chunkSize;
    uint64_t saveSize;
    uint64_t offset;
    uint64_t containerID;
    uint8_t* chunkPtr;         /* valid only inside Enclave memory */
    uint64_t name;
    uint64_t mtime;
    char     typeflag;
    std::string uname;
    std::string gname;
    std::string linkname;
    int      mode;
    int      basechunkID;
    uint8_t  deltaFlag;
    bool     loadFromDisk;
    bool     headerFlag;
    bool     nameExist;
    bool     cdcFlag;
    uint64_t parentDirName;
    uint64_t fileName;
};

/* ---- Trusted Container ---- */
struct TContainer_t {
    uint64_t size;
    uint64_t chunkNum;
    uint64_t containerID;
    uint8_t  data[SGX_CONTAINER_MAX_SIZE];
};

typedef uint64_t TRecipe_t;
typedef uint64_t TRecipe_Header_t;

/* ---- Trusted dataWrite class ---- */
class EnclaveDataWrite {
public:
    EnclaveDataWrite();
    ~EnclaveDataWrite();

    /* Insert a chunk into the current container.
       If the container is full, flush it via OCALL. */
    bool Chunk_Insert(TChunk_t& chunk);

    /* Insert a chunk whose payload has been replaced by lz4 compressed data
       stored in lz4Buffer of length chunk.saveSize. */
    bool Chunk_Insert(TChunk_t& chunk, uint8_t* lz4Buffer);

    /* Flush the last partial container. */
    void ProcessLastContainer();

    /* Get chunk metadata (no disk read — everything in Enclave memory). */
    TChunk_t Get_Chunk_Info(int id);
    TChunk_t Get_Chunk_MetaInfo(int id);

    /* Recipe management */
    void SetFilename(const std::string& name) { filename_ = name; }
    bool Recipe_Insert(uint64_t chunkID);
    bool Recipe_Header_Insert(uint64_t chunkID);

    /* Clear per-version caches (called between backup versions). */
    void ClearAllCache();

    /* Public chunk list — needed by BiSearch for base-chunk lookups. */
    std::vector<TChunk_t> chunklist;

    /* Set the AES key used to re-encrypt container data before OCALL. */
    void SetStorageKey(const uint8_t key[SGX_AES_GCM_KEY_SIZE]);

private:
    void FlushContainer();

    TContainer_t curContainer_;
    uint64_t     curOffset_;
    uint64_t     containerNum_;

    std::string  filename_;

    /* Recipe maps (version -> list of chunk IDs) */
    std::unordered_map<std::string, std::vector<TRecipe_t>>        recipeMap_;
    std::unordered_map<std::string, std::vector<TRecipe_Header_t>> recipeMapHeader_;

    /* Storage encryption key */
    uint8_t storageKey_[SGX_AES_GCM_KEY_SIZE];
};

#endif /* ENCLAVE_DATAWRITE_H */

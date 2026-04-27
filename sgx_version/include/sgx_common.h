#ifndef SGX_COMMON_H
#define SGX_COMMON_H

#include <stdint.h>
#include <stddef.h>

/*
 * Constants shared between App (Untrusted) and Enclave (Trusted).
 * Keep this file free of C++ STL or OS-specific headers so it can be
 * included from the EDL-generated code without conflicts.
 */

#define SGX_CONTAINER_MAX_SIZE   (4 * 1024 * 1024)   /* 4 MiB */
#define SGX_MAX_CHUNK_SIZE       16384
#define SGX_CHUNK_HASH_SIZE      32
#define SGX_AES_GCM_KEY_SIZE     16
#define SGX_AES_GCM_MAC_SIZE     16
#define SGX_AES_GCM_IV_SIZE      12

/* ---- Delta / compression flag (mirrors original define.h) ---- */
enum SgxDeltaType {
    SGX_NO_DELTA = 0,
    SGX_NO_LZ4,
    SGX_DELTA,
    SGX_FINESSE_TO_BASE,
    SGX_FINESSE_DELTA,
    SGX_LOCAL_DELTA
};

/* ---- Metadata struct passed via ECALL (plain C, no pointers) ---- */
typedef struct {
    uint64_t chunk_size;         /* original chunk size                    */
    uint64_t name_hash;          /* hash of the file name                  */
    uint64_t parent_dir_hash;    /* parent directory hash                  */
    uint64_t file_name_hash;     /* file name hash                         */
    uint64_t mtime;              /* modification time                      */
    int      mode;               /* file permission mode                   */
    char     typeflag;           /* tar type flag                          */
    uint8_t  header_flag;        /* 1 = header chunk                       */
    uint8_t  cdc_flag;           /* 1 = CDC big-chunk                      */
    uint8_t  name_exist;         /* 1 = name is valid                      */
    char     uname[33];          /* owner user name                        */
    char     gname[33];          /* owner group name                       */
    char     linkname[101];      /* link target                            */
} ChunkMeta_t;

/* ---- Stats returned by ecall_finish ---- */
typedef struct {
    uint64_t logical_chunk_num;
    uint64_t unique_chunk_num;
    uint64_t base_chunk_num;
    uint64_t delta_chunk_num;
    uint64_t logical_chunk_size;
    uint64_t unique_chunk_size;
    uint64_t base_chunk_size;
    uint64_t delta_chunk_size;
    uint64_t dedup_reduct;
    uint64_t delta_reduct;
    uint64_t local_reduct;
    uint64_t feature_reduct;
    uint64_t locality_reduct;
    uint64_t container_num;
} MethodStats_t;

/* ---- Method IDs (mirrors original define.h) ---- */
enum SgxMethodType {
    SGX_METHOD_DEDUP = 0,
    SGX_METHOD_NTRANSFORM,
    SGX_METHOD_FINESSE,
    SGX_METHOD_ODESS,
    SGX_METHOD_PALANTIR,
    SGX_METHOD_BISEARCH,
    SGX_METHOD_LOCALITY
};

#endif /* SGX_COMMON_H */

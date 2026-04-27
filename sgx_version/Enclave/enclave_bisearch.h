#ifndef ENCLAVE_BISEARCH_H
#define ENCLAVE_BISEARCH_H

/*
 * Enclave-side BiSearch — event-driven refactoring.
 *
 * The original BiSearch::ProcessTrace() was a blocking while-loop pulling
 * from a MessageQueue.  In the SGX model, data is pushed into the Enclave
 * via ECALL.  Each ECALL triggers one call to ProcessChunk().
 *
 * Key changes vs. the original:
 *   - OpenSSL EVP replaced by enclave_crypto.h (sgx_tcrypto)
 *   - iostream/cout replaced by ocall_print_string
 *   - No boost dependency
 *   - LZ4 and xdelta3 compiled as trusted static libraries
 */

#include "../include/sgx_common.h"
#include "enclave_datawrite.h"
#include "enclave_crypto.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include <algorithm>

/* Forward declarations for third-party libs compiled into the Enclave */
extern "C" {
    /* LZ4 */
    int LZ4_compress_fast(const char* src, char* dst,
                          int srcSize, int dstCapacity, int acceleration);
    int LZ4_decompress_safe(const char* src, char* dst,
                            int compressedSize, int dstCapacity);

    /* xdelta3 */
    int xd3_encode_memory(const uint8_t *input, size_t input_size,
                          const uint8_t *source, size_t source_size,
                          uint8_t *output, size_t *output_size,
                          size_t output_size_max, int flags);
    const char* xd3_strerror(int ret);
}

/* ---- SuperFeature types (simplified from odess_similarity_detection.h) ---- */
typedef uint64_t feature_t;
typedef unsigned long long super_feature_t;
typedef std::vector<super_feature_t> SuperFeatures;

/* Minimal FeatureGenerator ported for Enclave use */
class EnclaveFeatureGenerator {
public:
    static const size_t kFeatureNumber       = 12;
    static const size_t kSuperFeatureNumber  = 3;

    EnclaveFeatureGenerator();
    SuperFeatures GenerateSuperFeatures(const std::string& value);

private:
    void OdessResemblanceDetect(const std::string& value);
    SuperFeatures MakeSuperFeatures();

    std::vector<feature_t> features_;
    std::vector<feature_t> random_transform_args_a_;
    std::vector<feature_t> random_transform_args_b_;
    static const feature_t kSampleRatioMask = 0x0000400303410000ULL; /* 1/128 */
};

/* Minimal FeatureIndexTable ported for Enclave use */
class EnclaveFeatureIndexTable {
public:
    EnclaveFeatureIndexTable() {}

    uint64_t SF_Find(const SuperFeatures& sf);
    void     SF_Insert(const SuperFeatures& sf, uint64_t chunkid);

    EnclaveFeatureGenerator feature_generator_;

private:
    std::unordered_map<super_feature_t, std::vector<uint64_t>> SFindex_;
};

/* ---- Enclave BiSearch processor ---- */
class EnclaveBiSearch {
public:
    EnclaveBiSearch(double ratio, double accept_thr, int turn_on_name);
    ~EnclaveBiSearch();

    /* Event-driven entry: process exactly one decrypted chunk.
       Returns 0 on success. */
    int ProcessChunk(TChunk_t& chunk);

    /* Mark end of one backup version (flush, update counters). */
    void VersionDone();

    /* Fill stats struct for returning to the host. */
    void GetStats(MethodStats_t* stats);

    /* Public reference to the trusted dataWrite. */
    EnclaveDataWrite* dataWrite_;

private:
    /* --- Fingerprint index --- */
    std::unordered_map<std::string, int> FPindex_;
    int FP_Find(const std::string& fp);
    void FP_Insert(const std::string& fp, int chunkid);

    /* --- Hash helper --- */
    void GenerateHash(const uint8_t* data, size_t len, uint8_t hash[SGX_CHUNK_HASH_SIZE]);

    /* --- Delta encoding --- */
    uint8_t* xd3_encode_wrapper(
        const uint8_t* target, size_t target_size,
        const uint8_t* base, size_t base_size,
        size_t* delta_size);

    /* --- BiSearch-specific state --- */
    double beta_;                /* β — false filter ratio */
    double acceptThreshold_;
    bool   isFalseFilter_;
    bool   turnOnNameHash_;

    int    version_;
    int    dedupGap_;
    int    localError_;
    bool   localFlag_;

    uint64_t lz4LogicalSize_;
    uint64_t lz4UniqueSize_;

    EnclaveFeatureIndexTable table_;

    /* Metadata-aware candidate indices */
    std::unordered_multimap<uint64_t, uint32_t> parentDirIndex_;
    std::unordered_multimap<uint64_t, uint32_t> fileNameIndex_;

    struct CandidateResult {
        bool     found;
        uint32_t chunkId;
        double   score;
    };
    CandidateResult SelectCandidate(const TChunk_t& target);
    double ComputeCandidateScore(const TChunk_t& target, const TChunk_t& cand) const;
    bool   IsValidCandidate(const TChunk_t& target, const TChunk_t& cand) const;
    void   IndexChunkMetadata(const TChunk_t& chunk);

    bool estimateGain(uint64_t chunkSize, uint64_t deltaSize);

    /* PLChunk locality predictor */
    struct PLChunk {
        uint64_t chunkId;
        uint8_t  chunkType;
        double   compressionRatio;
    };
    PLChunk plchunk_;

    std::unordered_map<uint64_t, uint32_t> nameTable_;

    /* Scoring weights */
    static constexpr double wt_time     = 0.5;
    static constexpr double ws_size     = 0.4;
    static constexpr double wm_meta     = 0.1;
    static constexpr double lambda_time = 0.01;
    static constexpr double beta_size   = 2.0;
    static constexpr size_t candidateTopK = 8;

    /* Buffers */
    uint8_t* lz4ChunkBuffer_;
    uint8_t* hashBuf_;
    uint8_t* deltaMaxChunkBuffer_;

    /* --- Statistics --- */
    uint64_t logicalChunkNum_;
    uint64_t uniqueChunkNum_;
    uint64_t baseChunkNum_;
    uint64_t deltaChunkNum_;
    uint64_t logicalChunkSize_;
    uint64_t uniqueChunkSize_;
    uint64_t baseChunkSize_;
    uint64_t deltaChunkSize_;
    uint64_t deltaChunkOriSize_;
    uint64_t dedupReduct_;
    uint64_t deltaReduct_;
    uint64_t localReduct_;
    uint64_t featureReduct_;
    uint64_t localityReduct_;
    double   DCESum_INT_;
    uint64_t finessehit_;
    uint64_t bugCount_;

    /* helper stats methods */
    void StatsDeltaFeature(TChunk_t& chunk);
    void StatsDeltaLocality(TChunk_t& chunk);
};

#endif /* ENCLAVE_BISEARCH_H */

/*
 * enclave_bisearch.cc — event-driven BiSearch inside SGX Enclave.
 * "One ECALL = one chunk" instead of a blocking while-loop.
 */

#include "enclave_bisearch.h"
#include "Enclave_t.h"   /* ocall_print_string */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <algorithm>

/* ================================================================
 * EnclaveFeatureGenerator
 * ================================================================ */
static const uint32_t GEAR_TABLE[] = {
    0x5C95C078,0x22408989,0x2D48A214,0x12842087,0x530F8AFB,0x474536B9,
    0x2963B4F1,0x44CB738B,0x4EA7403D,0x4D606B6E,0x074EC5D3,0x3AF39D18,
    0x726003CA,0x37A62A74,0x51A2F58E,0x7506358E,0x5D4AB128,0x4D4AE17B,
    0x41E85924,0x470C36F7,0x4741CBE1,0x01BB7F30,0x617C1DE3,0x2B0C3A1F,
    0x50C48F73,0x21A82D37,0x6095ACE0,0x419167A0,0x3CAF49B0,0x40CEA62D,
    0x66BC1C66,0x545E1DAD
};

EnclaveFeatureGenerator::EnclaveFeatureGenerator()
{
    /* Seed deterministic transform args (mirrors odess original) */
    uint64_t seed1 = 922, seed2 = 314159;
    for (size_t i = 0; i < kFeatureNumber; i++) {
        seed1 = seed1 * 6364136223846793005ULL + 1442695040888963407ULL;
        seed2 = seed2 * 6364136223846793005ULL + 1442695040888963407ULL;
        random_transform_args_a_.push_back((seed1 >> 33) | 1ULL);
        random_transform_args_b_.push_back(seed2 >> 33);
    }
    features_.resize(kFeatureNumber, 0);
}

SuperFeatures EnclaveFeatureGenerator::GenerateSuperFeatures(const std::string& value)
{
    std::fill(features_.begin(), features_.end(), 0ULL);
    OdessResemblanceDetect(value);
    return MakeSuperFeatures();
}

void EnclaveFeatureGenerator::OdessResemblanceDetect(const std::string& value)
{
    const uint8_t* data = (const uint8_t*)value.data();
    size_t len = value.size();
    uint64_t hash = 0;
    for (size_t i = 0; i < len; i++) {
        hash = (hash >> 1) + GEAR_TABLE[data[i] & 0x1F];
        if (!(hash & kSampleRatioMask)) {
            for (size_t f = 0; f < kFeatureNumber; f++) {
                uint64_t h = hash * random_transform_args_a_[f] + random_transform_args_b_[f];
                if (h > features_[f]) features_[f] = h;
            }
        }
    }
}

SuperFeatures EnclaveFeatureGenerator::MakeSuperFeatures()
{
    SuperFeatures sf;
    size_t perGroup = kFeatureNumber / kSuperFeatureNumber;
    for (size_t g = 0; g < kSuperFeatureNumber; g++) {
        uint64_t combined = 0;
        for (size_t i = 0; i < perGroup; i++) {
            combined ^= features_[g * perGroup + i] * 0x9e3779b97f4a7c15ULL;
        }
        sf.push_back(combined);
    }
    return sf;
}

/* ================================================================
 * EnclaveFeatureIndexTable
 * ================================================================ */
uint64_t EnclaveFeatureIndexTable::SF_Find(const SuperFeatures& sf)
{
    for (auto& s : sf) {
        auto it = SFindex_.find(s);
        if (it != SFindex_.end() && !it->second.empty())
            return it->second.back();
    }
    return (uint64_t)-1;
}

void EnclaveFeatureIndexTable::SF_Insert(const SuperFeatures& sf, uint64_t chunkid)
{
    for (auto& s : sf) SFindex_[s].push_back(chunkid);
}

/* ================================================================
 * EnclaveBiSearch
 * ================================================================ */
EnclaveBiSearch::EnclaveBiSearch(double ratio, double accept_thr, int turn_on_name)
    : beta_(ratio), acceptThreshold_(accept_thr),
      isFalseFilter_(accept_thr == 0.0),
      turnOnNameHash_(turn_on_name != 0),
      version_(0), dedupGap_(0), localError_(0), localFlag_(true),
      lz4LogicalSize_(0), lz4UniqueSize_(0),
      logicalChunkNum_(0), uniqueChunkNum_(0), baseChunkNum_(0),
      deltaChunkNum_(0), logicalChunkSize_(0), uniqueChunkSize_(0),
      baseChunkSize_(0), deltaChunkSize_(0), deltaChunkOriSize_(0),
      dedupReduct_(0), deltaReduct_(0), localReduct_(0),
      featureReduct_(0), localityReduct_(0), DCESum_INT_(0),
      finessehit_(0), bugCount_(0), dataWrite_(nullptr)
{
    lz4ChunkBuffer_      = (uint8_t*)malloc(SGX_CONTAINER_MAX_SIZE);
    hashBuf_             = (uint8_t*)malloc(SGX_CHUNK_HASH_SIZE);
    deltaMaxChunkBuffer_ = (uint8_t*)malloc(2 * SGX_CONTAINER_MAX_SIZE);
    plchunk_ = {(uint64_t)-1, 1, 0.0};
}

EnclaveBiSearch::~EnclaveBiSearch()
{
    free(lz4ChunkBuffer_);
    free(hashBuf_);
    free(deltaMaxChunkBuffer_);
}

void EnclaveBiSearch::GenerateHash(const uint8_t* data, size_t len,
                                   uint8_t hash[SGX_CHUNK_HASH_SIZE])
{
    enclave_sha256(data, len, hash);
}

int EnclaveBiSearch::FP_Find(const std::string& fp)
{
    auto it = FPindex_.find(fp);
    return (it != FPindex_.end()) ? it->second : -1;
}

void EnclaveBiSearch::FP_Insert(const std::string& fp, int id)
{
    FPindex_[fp] = id;
}

uint8_t* EnclaveBiSearch::xd3_encode_wrapper(
    const uint8_t* target, size_t target_size,
    const uint8_t* base,   size_t base_size,
    size_t* delta_size)
{
    size_t out_size = 0;
    int ret = xd3_encode_memory(target, target_size, base, base_size,
                                deltaMaxChunkBuffer_, &out_size,
                                2 * SGX_CONTAINER_MAX_SIZE, 0);
    if (ret != 0) { *delta_size = 0; return nullptr; }
    uint8_t* buf = (uint8_t*)malloc(out_size);
    memcpy(buf, deltaMaxChunkBuffer_, out_size);
    *delta_size = out_size;
    return buf;
}

bool EnclaveBiSearch::estimateGain(uint64_t chunkSize, uint64_t deltaSize)
{
    if (isFalseFilter_) {
        double avgLz4 = (lz4UniqueSize_ > 0)
            ? (double)lz4LogicalSize_ / (double)lz4UniqueSize_ : 1.0;
        double newDeltaNum = (baseChunkNum_ > 0)
            ? (double)deltaChunkNum_ / (double)baseChunkNum_ : 0.0;
        double deltaGain = (deltaChunkNum_ > 0)
            ? DCESum_INT_ / (double)deltaChunkNum_ : 0.0;
        double futureDeltaCost = deltaGain * newDeltaNum;
        return (avgLz4 + futureDeltaCost * beta_) <= (double)chunkSize / (double)deltaSize;
    }
    return acceptThreshold_ <= (double)chunkSize / (double)deltaSize;
}

bool EnclaveBiSearch::IsValidCandidate(const TChunk_t& t, const TChunk_t& c) const
{
    if (t.typeflag != c.typeflag) return false;
    if (!t.linkname.empty() && t.linkname != c.linkname) return false;
    return true;
}

double EnclaveBiSearch::ComputeCandidateScore(const TChunk_t& t, const TChunk_t& c) const
{
    if (!IsValidCandidate(t, c)) return 0.0;
    double dtHour = (t.mtime > 0 && c.mtime > 0)
        ? std::abs((double)t.mtime - (double)c.mtime) / 3600.0 : 0.0;
    double ftime = std::exp(-lambda_time * dtHour);
    double maxSz = (double)std::max(t.chunkSize, c.chunkSize);
    double fsize = (maxSz > 0)
        ? std::max(0.0, 1.0 - std::abs((double)t.chunkSize - (double)c.chunkSize) / (maxSz * beta_size))
        : 0.0;
    double fmeta = ((t.mode == c.mode ? 1.0 : 0.0) +
                    (!t.uname.empty() && t.uname == c.uname ? 1.0 : 0.0)) / 2.0;
    return wt_time * ftime + ws_size * fsize + wm_meta * fmeta;
}

EnclaveBiSearch::CandidateResult EnclaveBiSearch::SelectCandidate(const TChunk_t& target)
{
    CandidateResult res{false, 0, 0.0};
    std::vector<std::pair<uint32_t, double>> scored;
    std::unordered_set<uint32_t> visited;

    auto collect = [&](const std::unordered_multimap<uint64_t,uint32_t>& idx, uint64_t key) {
        auto range = idx.equal_range(key);
        for (auto it = range.first; it != range.second; ++it) {
            if (!visited.insert(it->second).second) continue;
            auto meta = dataWrite_->Get_Chunk_MetaInfo(it->second);
            double score = ComputeCandidateScore(target, meta);
            if (score > 0.0) scored.emplace_back(it->second, score);
        }
    };

    if (target.parentDirName) collect(parentDirIndex_, target.parentDirName);
    if (target.fileName)      collect(fileNameIndex_,  target.fileName);
    if (scored.empty()) return res;

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    if (scored.size() > candidateTopK) scored.resize(candidateTopK);
    res.found   = true;
    res.chunkId = scored.front().first;
    res.score   = scored.front().second;
    return res;
}

void EnclaveBiSearch::IndexChunkMetadata(const TChunk_t& chunk)
{
    parentDirIndex_.emplace(chunk.parentDirName, chunk.chunkID);
    fileNameIndex_.emplace(chunk.fileName, chunk.chunkID);
}

void EnclaveBiSearch::StatsDeltaFeature(TChunk_t& c)
{
    deltaChunkOriSize_ += c.chunkSize;
    deltaChunkSize_    += c.saveSize;
    deltaChunkNum_++;
    deltaReduct_   += c.chunkSize - c.saveSize;
    featureReduct_ += c.chunkSize - c.saveSize;
    DCESum_INT_    += (double)c.chunkSize / (double)c.saveSize;
}

void EnclaveBiSearch::StatsDeltaLocality(TChunk_t& c)
{
    deltaChunkOriSize_ += c.chunkSize;
    deltaChunkSize_    += c.saveSize;
    deltaChunkNum_++;
    deltaReduct_    += c.chunkSize - c.saveSize;
    localityReduct_ += c.chunkSize - c.saveSize;
    DCESum_INT_     += (double)c.chunkSize / (double)c.saveSize;
}

/* ================================================================
 * ProcessChunk — the event-driven core
 * ================================================================ */
int EnclaveBiSearch::ProcessChunk(TChunk_t& tmpChunk)
{
    /* 1. Compute fingerprint */
    GenerateHash(tmpChunk.chunkPtr, tmpChunk.chunkSize, hashBuf_);
    std::string hashStr((char*)hashBuf_, SGX_CHUNK_HASH_SIZE);
    std::string chunkContent((char*)tmpChunk.chunkPtr, tmpChunk.chunkSize);

    int findRes = FP_Find(hashStr);

    if (findRes == -1) {
        /* ---- UNIQUE CHUNK ---- */
        tmpChunk.chunkID   = (uint64_t)uniqueChunkNum_;
        tmpChunk.deltaFlag = SGX_NO_DELTA;
        FP_Insert(hashStr, (int)tmpChunk.chunkID);
        dedupGap_++;

        bool sameName = false;
        if (version_ > 0) {
            auto it = nameTable_.find(tmpChunk.name);
            if (it != nameTable_.end()) {
                plchunk_.chunkId = it->second;
                sameName = turnOnNameHash_;
            }
            if (!sameName) {
                auto cand = SelectCandidate(tmpChunk);
                if (cand.found) { plchunk_.chunkId = cand.chunkId; sameName = true; }
            }
        }

        if (version_ > 0 && sameName) {
            /* Locality attempt */
            TChunk_t localMeta = dataWrite_->Get_Chunk_MetaInfo((int)plchunk_.chunkId);
            TChunk_t baseInfo;
            if (localMeta.deltaFlag == SGX_FINESSE_DELTA || localMeta.deltaFlag == SGX_LOCAL_DELTA)
                baseInfo = dataWrite_->Get_Chunk_Info((int)localMeta.basechunkID),
                tmpChunk.basechunkID = localMeta.basechunkID;
            else
                baseInfo = dataWrite_->Get_Chunk_Info((int)plchunk_.chunkId),
                tmpChunk.basechunkID = (int)plchunk_.chunkId;

            size_t deltaSize = 0;
            uint8_t* deltaBuf = xd3_encode_wrapper(
                tmpChunk.chunkPtr, tmpChunk.chunkSize,
                baseInfo.chunkPtr, baseInfo.chunkSize, &deltaSize);

            if (deltaSize == 0 || deltaSize > tmpChunk.chunkSize) {
                tmpChunk.saveSize  = tmpChunk.chunkSize;
                tmpChunk.deltaFlag = SGX_NO_DELTA;
                if (deltaSize > tmpChunk.chunkSize) bugCount_++;
                if (deltaBuf) { free(deltaBuf); deltaBuf = nullptr; }
            } else {
                tmpChunk.deltaFlag = SGX_LOCAL_DELTA;
            }

            if (tmpChunk.deltaFlag == SGX_LOCAL_DELTA &&
                estimateGain(tmpChunk.chunkSize, deltaSize))
            {
                tmpChunk.saveSize = deltaSize;
                memcpy(tmpChunk.chunkPtr, deltaBuf, deltaSize);
                free(deltaBuf);
                localReduct_ += tmpChunk.chunkSize - tmpChunk.saveSize;
                StatsDeltaLocality(tmpChunk);
                dataWrite_->Chunk_Insert(tmpChunk);
            } else {
                if (deltaBuf) { free(deltaBuf); deltaBuf = nullptr; }

                /* Feature-based (Odess) lookup */
                auto sf = table_.feature_generator_.GenerateSuperFeatures(chunkContent);
                uint64_t baseID = table_.SF_Find(sf);

                if (baseID == (uint64_t)-1) {
                    /* New base chunk — LZ4 */
                    int lz4sz = LZ4_compress_fast((char*)tmpChunk.chunkPtr,
                                                  (char*)lz4ChunkBuffer_,
                                                  (int)tmpChunk.chunkSize,
                                                  (int)tmpChunk.chunkSize, 3);
                    tmpChunk.deltaFlag  = (lz4sz > 0) ? SGX_NO_DELTA : SGX_NO_LZ4;
                    tmpChunk.saveSize   = (lz4sz > 0) ? (uint64_t)lz4sz : tmpChunk.chunkSize;
                    tmpChunk.basechunkID = -1;
                    table_.SF_Insert(sf, tmpChunk.chunkID);
                    baseChunkNum_++;
                    baseChunkSize_ += tmpChunk.saveSize;
                    lz4LogicalSize_ += tmpChunk.chunkSize;
                    lz4UniqueSize_  += tmpChunk.saveSize;
                    localReduct_    += tmpChunk.chunkSize - tmpChunk.saveSize;
                    nameTable_[tmpChunk.name] = (uint32_t)tmpChunk.chunkID;
                    if (tmpChunk.deltaFlag == SGX_NO_LZ4)
                        dataWrite_->Chunk_Insert(tmpChunk);
                    else
                        dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer_);
                } else {
                    /* Feature hit — delta vs base */
                    TChunk_t baseInfo2 = dataWrite_->Get_Chunk_Info((int)baseID);
                    size_t ds2 = 0;
                    uint8_t* db2 = xd3_encode_wrapper(
                        tmpChunk.chunkPtr, tmpChunk.chunkSize,
                        baseInfo2.chunkPtr, baseInfo2.chunkSize, &ds2);

                    if (ds2 > 0 && ds2 <= tmpChunk.chunkSize) {
                        memcpy(tmpChunk.chunkPtr, db2, ds2);
                        tmpChunk.saveSize    = ds2;
                        tmpChunk.deltaFlag   = SGX_FINESSE_DELTA;
                        tmpChunk.basechunkID = (int)baseID;
                        plchunk_ = {baseID, 0, (double)tmpChunk.chunkSize / (double)ds2};
                        dedupGap_ = 0;
                        finessehit_++;
                        StatsDeltaFeature(tmpChunk);
                        dataWrite_->Chunk_Insert(tmpChunk);
                    } else {
                        /* Delta worse — fallback to LZ4 base */
                        int lz4sz2 = LZ4_compress_fast((char*)tmpChunk.chunkPtr,
                                                       (char*)lz4ChunkBuffer_,
                                                       (int)tmpChunk.chunkSize,
                                                       (int)tmpChunk.chunkSize, 3);
                        tmpChunk.deltaFlag  = (lz4sz2 > 0) ? SGX_NO_DELTA : SGX_NO_LZ4;
                        tmpChunk.saveSize   = (lz4sz2 > 0) ? (uint64_t)lz4sz2 : tmpChunk.chunkSize;
                        tmpChunk.basechunkID = -1;
                        table_.SF_Insert(sf, tmpChunk.chunkID);
                        baseChunkNum_++; baseChunkSize_ += tmpChunk.saveSize;
                        lz4LogicalSize_ += tmpChunk.chunkSize;
                        lz4UniqueSize_  += tmpChunk.saveSize;
                        bugCount_++;
                        nameTable_[tmpChunk.name] = (uint32_t)tmpChunk.chunkID;
                        if (tmpChunk.deltaFlag == SGX_NO_LZ4)
                            dataWrite_->Chunk_Insert(tmpChunk);
                        else
                            dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer_);
                    }
                    if (db2) free(db2);
                    if (baseInfo2.loadFromDisk) free(baseInfo2.chunkPtr);
                }
                if (baseInfo.loadFromDisk) free(baseInfo.chunkPtr);
            }
        } else {
            /* No locality — Odess path */
            auto sf = table_.feature_generator_.GenerateSuperFeatures(chunkContent);
            uint64_t baseID = table_.SF_Find(sf);

            if (baseID == (uint64_t)-1) {
                int lz4sz = LZ4_compress_fast((char*)tmpChunk.chunkPtr,
                                              (char*)lz4ChunkBuffer_,
                                              (int)tmpChunk.chunkSize,
                                              (int)tmpChunk.chunkSize, 3);
                tmpChunk.deltaFlag  = (lz4sz > 0) ? SGX_NO_DELTA : SGX_NO_LZ4;
                tmpChunk.saveSize   = (lz4sz > 0) ? (uint64_t)lz4sz : tmpChunk.chunkSize;
                tmpChunk.basechunkID = -1;
                table_.SF_Insert(sf, tmpChunk.chunkID);
                baseChunkNum_++; baseChunkSize_ += tmpChunk.saveSize;
                lz4LogicalSize_ += tmpChunk.chunkSize;
                lz4UniqueSize_  += tmpChunk.saveSize;
                localReduct_    += tmpChunk.chunkSize - tmpChunk.saveSize;
                nameTable_[tmpChunk.name] = (uint32_t)tmpChunk.chunkID;
                if (tmpChunk.deltaFlag == SGX_NO_LZ4)
                    dataWrite_->Chunk_Insert(tmpChunk);
                else
                    dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer_);
            } else {
                TChunk_t baseInfo3 = dataWrite_->Get_Chunk_Info((int)baseID);
                size_t ds3 = 0;
                uint8_t* db3 = xd3_encode_wrapper(
                    tmpChunk.chunkPtr, tmpChunk.chunkSize,
                    baseInfo3.chunkPtr, baseInfo3.chunkSize, &ds3);

                if (ds3 > 0 && ds3 <= tmpChunk.chunkSize) {
                    memcpy(tmpChunk.chunkPtr, db3, ds3);
                    tmpChunk.saveSize    = ds3;
                    tmpChunk.deltaFlag   = SGX_FINESSE_DELTA;
                    tmpChunk.basechunkID = (int)baseID;
                    plchunk_ = {baseID, 0, (double)tmpChunk.chunkSize / (double)ds3};
                    dedupGap_ = 0; finessehit_++;
                    StatsDeltaFeature(tmpChunk);
                    dataWrite_->Chunk_Insert(tmpChunk);
                } else {
                    int lz4sz3 = LZ4_compress_fast((char*)tmpChunk.chunkPtr,
                                                   (char*)lz4ChunkBuffer_,
                                                   (int)tmpChunk.chunkSize,
                                                   (int)tmpChunk.chunkSize, 3);
                    tmpChunk.deltaFlag  = (lz4sz3 > 0) ? SGX_NO_DELTA : SGX_NO_LZ4;
                    tmpChunk.saveSize   = (lz4sz3 > 0) ? (uint64_t)lz4sz3 : tmpChunk.chunkSize;
                    tmpChunk.basechunkID = -1;
                    table_.SF_Insert(sf, tmpChunk.chunkID);
                    baseChunkNum_++; baseChunkSize_ += tmpChunk.saveSize;
                    lz4LogicalSize_ += tmpChunk.chunkSize; lz4UniqueSize_ += tmpChunk.saveSize;
                    bugCount_++;
                    nameTable_[tmpChunk.name] = (uint32_t)tmpChunk.chunkID;
                    if (tmpChunk.deltaFlag == SGX_NO_LZ4)
                        dataWrite_->Chunk_Insert(tmpChunk);
                    else
                        dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer_);
                }
                if (db3) free(db3);
                if (baseInfo3.loadFromDisk) free(baseInfo3.chunkPtr);
            }
        }

        IndexChunkMetadata(tmpChunk);
        uniqueChunkSize_ += tmpChunk.saveSize;
        uniqueChunkNum_++;
    } else {
        /* ---- DUPLICATE CHUNK ---- */
        free(tmpChunk.chunkPtr);
        tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
        plchunk_.chunkId = (uint64_t)findRes;
        dedupReduct_    += tmpChunk.chunkSize;
    }

    /* Recipe */
    if (!tmpChunk.headerFlag)
        dataWrite_->Recipe_Insert(tmpChunk.chunkID);
    else
        dataWrite_->Recipe_Header_Insert(tmpChunk.chunkID);

    logicalChunkNum_++;
    logicalChunkSize_ += tmpChunk.chunkSize;
    return 0;
}

void EnclaveBiSearch::VersionDone()
{
    version_++;
    dataWrite_->ProcessLastContainer();
}

void EnclaveBiSearch::GetStats(MethodStats_t* stats)
{
    stats->logical_chunk_num  = logicalChunkNum_;
    stats->unique_chunk_num   = uniqueChunkNum_;
    stats->base_chunk_num     = baseChunkNum_;
    stats->delta_chunk_num    = deltaChunkNum_;
    stats->logical_chunk_size = logicalChunkSize_;
    stats->unique_chunk_size  = uniqueChunkSize_;
    stats->base_chunk_size    = baseChunkSize_;
    stats->delta_chunk_size   = deltaChunkSize_;
    stats->dedup_reduct       = dedupReduct_;
    stats->delta_reduct       = deltaReduct_;
    stats->local_reduct       = localReduct_;
    stats->feature_reduct     = featureReduct_;
    stats->locality_reduct    = localityReduct_;
    stats->container_num      = dataWrite_ ? dataWrite_->chunklist.size() : 0;
}

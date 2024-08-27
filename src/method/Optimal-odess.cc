#include "../../include/optimalodess.h"

OptimalOdess::OptimalOdess()
{
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
    deltaMaxChunkBuffer = (uint8_t *)malloc(2 * CONTAINER_MAX_SIZE * sizeof(uint8_t));
    SFindex = new unordered_map<string, vector<int>>[FINESSE_SF_NUM];
}

OptimalOdess::~OptimalOdess()
{
    free(lz4ChunkBuffer);
    free(deltaMaxChunkBuffer);
    EVP_MD_CTX_free(mdCtx);
    free(hashBuf);
}

void OptimalOdess::ProcessTrace()
{
    string tmpChunkHash;
    string tmpChunkContent;
    while (true)
    {
        string hashStr;
        hashStr.assign(CHUNK_HASH_SIZE, 0);
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;

        if (recieveQueue->done_ && recieveQueue->IsEmpty())
        {
            // outputMQ_->done_ = true;
            recieveQueue->done_ = false;
            break;
        }
        Chunk_t tmpChunk;
        if (recieveQueue->Pop(tmpChunk))
        {
            // calculate feature
            // compute cutPoint
            // generate chunk
            GenerateHash(mdCtx, tmpChunk.chunkPtr, tmpChunk.chunkSize, hashBuf);
            hashStr.assign((char *)hashBuf, CHUNK_HASH_SIZE);
            int tmpChunkid;
            int findRes = FP_Find(hashStr);
            if (findRes == -1)
            {
                // Unique chunk found
                tmpChunk.chunkID = uniquechunkNum;
                tmpChunk.deltaFlag = NO_DELTA;
                FP_Insert(hashStr, tmpChunk.chunkID);
                tmpChunkContent.assign((char *)tmpChunk.chunkPtr, tmpChunk.chunkSize);
                tmpChunkHash.assign((char *)hashBuf, CHUNK_HASH_SIZE);
                // OptimalOdess get superfeature
                auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                auto basechunkid = table.SF_Find(superfeature);
                int tmpChunkLz4CompressSize = 0;
                tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                if (tmpChunkLz4CompressSize > 0)
                {
                    tmpChunk.deltaFlag = NO_DELTA;
                    tmpChunk.saveSize = tmpChunkLz4CompressSize;
                }
                else
                {
                    // cout << "lz4 compress error" << endl;
                    tmpChunk.deltaFlag = NO_LZ4;
                    tmpChunk.saveSize = tmpChunk.chunkSize;
                }

                tmpChunk.basechunkID = -1;
                tmpChunkid = tmpChunk.chunkID;
                table.SF_Insert(superfeature, tmpChunk.chunkID);
                basechunkNum++;
                basechunkSize += tmpChunk.saveSize;
                LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                if (tmpChunk.deltaFlag == NO_LZ4)
                    // base chunk & Lz4 error
                    dataWrite_->Chunk_Insert(tmpChunk);
                else
                    // base chunk &lz4 compress
                    dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                // }
                uniquechunkNum++;
                uniquechunkSize += tmpChunk.saveSize;
            }
            else
            {
                // Dedup chunk found
                free(tmpChunk.chunkPtr);
                tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
                tmpChunkid = findRes;
                PrevDedupChunkid = findRes;
                DedupReduct += tmpChunk.chunkSize;
            }
            if (tmpChunk.HeaderFlag == 0)
                dataWrite_->Recipe_Insert(tmpChunk.chunkID);
            else
                dataWrite_->Recipe_Header_Insert(tmpChunk.chunkID);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
        }
    }

    Version_log();
    recieveQueue->done_ = false;
    return;
}

std::vector<uint64_t> OptimalOdess::matrixS()
{
    std::ofstream matrixS("matrixS.txt");
    const int n = dataWrite_->Get_Chunk_Num();
    std::vector<uint64_t> S(n);
    for (int i = 0; i < n; ++i)
    {
        Chunk_t tmpChunk = dataWrite_->Get_Chunk_MetaInfo(i);
        S[i] = tmpChunk.saveSize;
        matrixS << "chunk" << i << "lz4size: " << S[i] << endl;
    }
    matrixS.close();
    return S;
};

std::vector<std::vector<std::pair<uint64_t, uint64_t>>> OptimalOdess::matrixD()
{
    std::ofstream matrixD("matrixD.txt");
    const int n = dataWrite_->Get_Chunk_Num();
    std::vector<std::vector<std::pair<uint64_t, uint64_t>>> D;
    for (int i = 0; i < n; ++i)
    {
        auto tmpChunk = dataWrite_->Get_Chunk_Info(i);
        std::string tmpChunkContent((char *)tmpChunk.chunkPtr, tmpChunk.chunkSize);
        auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
        vector<uint64_t> basechunkid = table.SF_MutiFind(superfeature);

        for (const auto &basechunkidvalue : basechunkid)
        {
            uint64_t DeltaSize = 0;
            auto basechunkInfo = dataWrite_->Get_Chunk_Info(basechunkidvalue);
            uint8_t *deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, basechunkInfo.chunkPtr, basechunkInfo.chunkSize, &DeltaSize, deltaMaxChunkBuffer);
            if (DeltaSize == 0)
            {
                cout << "delta error" << endl;
            }
            else
                D[i].push_back(std::make_pair(basechunkidvalue, DeltaSize));
            matrixD << "(" << i << ", " << basechunkidvalue << ") delta size: " << DeltaSize << endl;
            if (basechunkInfo.loadFromDisk)
                free(basechunkInfo.chunkPtr);
        }
        if (tmpChunk.loadFromDisk)
            free(tmpChunk.chunkPtr);
    }
    matrixD.close();
    return D;
};
void OptimalOdess::ILP()
{
    std::vector<uint64_t> S = matrixS();
    std::vector<std::vector<std::pair<uint64_t, uint64_t>>> D = matrixD();
};
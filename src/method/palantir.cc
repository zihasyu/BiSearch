#include "../../include/palantir.h"

Palantir::Palantir()
{
    cout << " FP size is " << sizeof(int) << " Chunk_t is " << sizeof(Chunk_t) << " <super_feature_t, unordered_set<string>> is " << sizeof(super_feature_t);
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
    deltaMaxChunkBuffer = (uint8_t *)malloc(2 * CONTAINER_MAX_SIZE * sizeof(uint8_t));
    SFindex = new unordered_map<string, vector<int>>[FINESSE_SF_NUM];
}

Palantir::~Palantir()
{
    free(lz4ChunkBuffer);
    free(deltaMaxChunkBuffer);
    EVP_MD_CTX_free(mdCtx);
    free(hashBuf);
}

void Palantir::ProcessTrace()
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
            CleanIndex();
            Version++;
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
            // Palantir get superfeature per logical chunk
            tmpChunkContent.assign((char *)tmpChunk.chunkPtr, tmpChunk.chunkSize);
            auto superfeature = table.feature_generator_.PalantirGetSF(tmpChunkContent);
            if (findRes == -1)
            {
                // Unique chunk found
                tmpChunk.chunkID = uniquechunkNum;
                tmpChunk.deltaFlag = NO_DELTA;
                FP_Insert(hashStr, tmpChunk.chunkID);

                tmpChunkHash.assign((char *)hashBuf, CHUNK_HASH_SIZE);
                auto basechunkid = SF_Find(superfeature);
                // auto ret = table.GetSimilarRecordsKeys(tmpChunkHash);

                if (basechunkid != -1)
                // unique chunk & delta chunk
                {
                    auto basechunkInfo = dataWrite_->Get_Chunk_Info(basechunkid);
                    uint8_t *deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, basechunkInfo.chunkPtr, basechunkInfo.chunkSize, &tmpChunk.saveSize, deltaMaxChunkBuffer);
                    if (tmpChunk.saveSize == 0)
                    {
                        cout << "delta error" << endl;
                        return;
                    }
                    // consider this is false fit
                    else if ((double)tmpChunk.chunkSize / (double)tmpChunk.saveSize < LZ4Ratio)
                    {

                        int tmpChunkLz4CompressSize = 0;
                        tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                        if (tmpChunkLz4CompressSize > 0)
                        {
                            tmpChunk.deltaFlag = NO_DELTA;
                            tmpChunk.saveSize = tmpChunkLz4CompressSize;
                        }
                        else
                        {
                            //cout << "lz4 compress error" << endl;
                            tmpChunk.deltaFlag = NO_LZ4;
                            tmpChunk.saveSize = tmpChunk.chunkSize;
                        }
                        tmpChunk.basechunkID = -1;
                        tmpChunkid = tmpChunk.chunkID;
                        SF_Insert(superfeature, tmpChunk.chunkID);
                        basechunkNum++;
                        basechunkOriSize += tmpChunk.chunkSize;
                        basechunkSize += tmpChunk.saveSize;
                        LZ4Ratio = (double)basechunkOriSize / (double)basechunkSize;
                        if (tmpChunk.deltaFlag == NO_LZ4)
                            // base chunk & Lz4 error
                            dataWrite_->Chunk_Insert(tmpChunk);
                        else
                            // base chunk &lz4 compress
                            dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                        LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                    }
                    else
                    {
                        tmpChunk.deltaFlag = DELTA;
                        tmpChunk.basechunkID = basechunkid;
                        memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize);
                        deltachunkNum++;
                        deltachunkSize += tmpChunk.saveSize;
                        free(deltachunk);
                        dataWrite_->Chunk_Insert(tmpChunk);
                        DeltaReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                    }
                    if (basechunkInfo.loadFromDisk)
                        free(basechunkInfo.chunkPtr);
                }
                // unique chunk & base chunk
                else
                {
                    int tmpChunkLz4CompressSize = 0;
                    tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                    if (tmpChunkLz4CompressSize > 0)
                    {
                        tmpChunk.deltaFlag = NO_DELTA;
                        tmpChunk.saveSize = tmpChunkLz4CompressSize;
                    }
                    else
                    {
                        //cout << "lz4 compress error" << endl;
                        tmpChunk.deltaFlag = NO_LZ4;
                        tmpChunk.saveSize = tmpChunk.chunkSize;
                    }
                    tmpChunk.basechunkID = -1;
                    tmpChunkid = tmpChunk.chunkID;
                    SF_Insert(superfeature, tmpChunk.chunkID);
                    basechunkNum++;
                    basechunkOriSize += tmpChunk.chunkSize;
                    basechunkSize += tmpChunk.saveSize;
                    LZ4Ratio = (double)basechunkOriSize / (double)basechunkSize;
                    if (tmpChunk.deltaFlag == NO_LZ4)
                        // base chunk & Lz4 error
                        dataWrite_->Chunk_Insert(tmpChunk);
                    else
                        // base chunk &lz4 compress
                        dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                    LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                }
                uniquechunkNum++;
                uniquechunkSize += tmpChunk.saveSize;
            }
            else
            {
                // Dedup chunk found
                free(tmpChunk.chunkPtr);
                tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
                if (tmpChunk.deltaFlag == NO_DELTA || tmpChunk.deltaFlag == NO_LZ4)
                    SF_Insert(superfeature, tmpChunk.chunkID);
                DedupReduct += tmpChunk.chunkSize;
                PrevDedupChunkid = findRes;
            }

            if (tmpChunk.HeaderFlag == 0)
                dataWrite_->Recipe_Insert(tmpChunk.chunkID);
            else
                dataWrite_->Recipe_Header_Insert(tmpChunk.chunkID);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
        }
    }
    cout << " avg Lz4Ratio is " << LZ4Ratio << endl;
    cout << "logical chunk num: " << logicalchunkNum << endl;
    cout << "unique chunk num: " << uniquechunkNum << endl;
    cout << "base chunk num: " << basechunkNum << endl;
    cout << "delta chunk num: " << deltachunkNum << endl;
    cout << "logicalchunkSize is " << logicalchunkSize << endl;
    cout << "uniquechunkSize is " << uniquechunkSize << endl;
    cout << "Overall Compression Ratio: " << (double)logicalchunkSize / (double)uniquechunkSize << endl;
    recieveQueue->done_ = false;
    return;
}

uint64_t Palantir::SF_Find(const SuperFeatures &superfeatures)
{
    for (const super_feature_t &sf : superfeatures)
    {
        if (auto it = SFindex1.find(sf) != SFindex1.end())
        {
            for (auto id : SFindex1[sf])
            {
                // cout << "SF1 hit";
                return id;
            }
        }
    }
    for (const super_feature_t &sf : superfeatures)
    {
        if (auto it = SFindex2.find(sf) != SFindex2.end())
        {
            // cout << "SF2 hit";
            return SFindex2[sf].begin()->chunkid;
        }
    }
    for (const super_feature_t &sf : superfeatures)
    {
        if (auto it = SFindex3.find(sf) != SFindex3.end())
        {
            // cout << "SF3 hit";
            return SFindex3[sf].begin()->chunkid;
        }
    }
    // return -1 if not found, uint64_t's MAX value
    return -1;
}

void Palantir::SF_Insert(const SuperFeatures &superfeatures, const uint64_t chunkid)
{

    for (int i = 0; i < 3; i++)
    {
        if (SFindex1[superfeatures[i]].empty())
            SFindex1[superfeatures[i]].push_back(chunkid);
        else
            SFindex1[superfeatures[i]][0] = chunkid;
    }
    for (int i = 3; i < 7; i++)
    {
        if (SFindex2[superfeatures[i]].empty())
        {
            SFindex2[superfeatures[i]].push_back({chunkid, Version});
        }
        else
        {
            SFindex2[superfeatures[i]][0] = {chunkid, Version};
        }
    }
    for (int i = 7; i < 13; i++)
    {
        if (SFindex3[superfeatures[i]].empty())
        {
            SFindex3[superfeatures[i]].push_back({chunkid, Version});
        }
        else
        {
            SFindex3[superfeatures[i]][0] = {chunkid, Version};
        }
    }
    // return -1 if not found, uint64_t's MAX value
    return;
}

void Palantir::CleanIndex()
{
    cout << "clear and the version is " << Version - 5 << " and " << Version - 2 << endl;
    for (auto &pair : SFindex2)
    {
        if (!pair.second.empty() && pair.second[0].version < Version - 5)
        {
            pair.second.erase(pair.second.begin());
            // cout << "clear and the version is " << pair.second[0].version << endl;
        }
    }
    for (auto &pair : SFindex3)
    {
        if (!pair.second.empty() && pair.second[0].version < Version - 2)
        {
            pair.second.erase(pair.second.begin());
            // cout << "clear and the version is " << pair.second[0].version << endl;
        }
    }
    // return -1 if not found, uint64_t's MAX value
    return;
}

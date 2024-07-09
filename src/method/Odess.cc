#include "../../include/odess.h"

Odess::Odess()
{
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
    deltaMaxChunkBuffer = (uint8_t *)malloc(2 * CONTAINER_MAX_SIZE * sizeof(uint8_t));
}

Odess::~Odess()
{
    free(lz4ChunkBuffer);
    free(deltaMaxChunkBuffer);
    EVP_MD_CTX_free(mdCtx);
}

void Odess::ProcessTrace()
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
                // /cout << tmpChunkContent << endl;
                // Odess get superfeature
                auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                auto ret = table.GetSimilarRecordKey(superfeature);
                // auto ret = table.GetSimilarRecordsKeys(tmpChunkHash);
                if (ret != "not found")
                {
                    int basechunkId = FP_Find(ret);
                    if (basechunkId != -1)
                    {
                        auto basechunkInfo = dataWrite_->Get_Chunk_Info(basechunkId);
                        uint8_t *deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, basechunkInfo.chunkPtr, basechunkInfo.chunkSize, &tmpChunk.saveSize, deltaMaxChunkBuffer);
                        if (tmpChunk.saveSize == 0)
                        {
                            cout << "delta error" << endl;
                            return;
                        }
                        else
                        {
                            tmpChunk.deltaFlag = DELTA;
                            tmpChunk.basechunkid = basechunkId;
                            chunkSet_->Chunk_Insert(tmpChunk);
                            deltachunkNum++;
                            deltachunkSize += tmpChunk.saveSize;
                            DeltaReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
                            free(deltachunk);
                        }
                        if (basechunkInfo.loadFromDisk)
                            free(basechunkInfo.chunkPtr);
                    }
                    else
                    {
                        cout << "find base chunk error" << endl;
                    }
                }
                else
                {
                    int tmpChunkLz4CompressSize = 0;
                    tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                    if (tmpChunkLz4CompressSize > 0)
                    {
                        tmpChunk.saveSize = tmpChunkLz4CompressSize;
                        // memcpy(tmpChunk.chunkPtr, lz4ChunkBuffer, tmpChunk.saveSize);
                    }
                    else
                    {
                        tmpChunk.saveSize = tmpChunk.chunkSize;
                        ;
                    }
                    // cout << "lz4 chunk size is " << tmpChunk.chunkSize << "save size is " << tmpChunk.saveSize << endl;
                    tmpChunk.deltaFlag = NO_DELTA;
                    tmpChunk.basechunkid = -1;
                    tmpChunkid = tmpChunk.chunkID;
                    table.Put(tmpChunkHash, tmpChunkContent);
                    chunkSet_->Chunk_Insert(tmpChunk);

                    basechunkNum++;
                    basechunkSize += tmpChunk.saveSize;
                    LocalReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
                }
                uniquechunkNum++;
                uniquechunkSize += tmpChunk.saveSize;
            }
            else
            {
                // Dedup chunk found
                tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
                tmpChunkid = findRes;
                PrevDedupChunkid = findRes;
                DedupGap = 0;
                DedupReductSize += tmpChunk.chunkSize;
            }
            chunkSet_->Recipe_Insert(tmpChunk);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
        }
    }

    // auto start = std::chrono::high_resolution_clock::now();
    // for (auto it : table.original_feature_key_table)
    // {
    //     for (auto id : it.second)
    //     {
    //     }
    // }
    // auto end = std::chrono::high_resolution_clock::now();
    // clustringTime += end - start;
    recieveQueue->done_ = false;
    return;
}

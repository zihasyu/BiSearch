#include "../../include/finesse.h"

Finesse::Finesse()
{
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
    deltaMaxChunkBuffer = (uint8_t *)malloc(2 * CONTAINER_MAX_SIZE * sizeof(uint8_t));
}

Finesse::~Finesse()
{
    free(lz4ChunkBuffer);
    free(deltaMaxChunkBuffer);
    EVP_MD_CTX_free(mdCtx);
}

void Finesse::ProcessTrace()
{

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
            hashStr.assign((char *)hashBuf, 32);
            int findRes = FP_Find(hashStr);
            if (findRes == -1)
            {
                // Unique chunk
                tmpChunk.chunkID = uniquechunkNum;
                tmpChunk.deltaFlag = NO_DELTA;
                FP_Insert(hashStr, tmpChunk.chunkID);
                uint8_t *tmpChunkSF;
                tmpChunkSF = (uint8_t *)malloc(FINESSE_SF_NUM * CHUNK_HASH_SIZE);
                // find basechunk
                startTime = std::chrono::high_resolution_clock::now();
                GetSF(tmpChunk.chunkPtr, mdCtx, tmpChunkSF, tmpChunk.chunkSize);
                int basechunkID = SF_Find((char *)tmpChunkSF, FINESSE_SF_NUM * CHUNK_HASH_SIZE);
                endTime = std::chrono::high_resolution_clock::now();
                getSFTime += (endTime - startTime);
                computeSFtimes++;
                if (basechunkID == -1)
                {
                    int tmpChunkLz4CompressSize = 0;
                    startTime = std::chrono::high_resolution_clock::now();
                    tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                    endTime = std::chrono::high_resolution_clock::now();
                    lz4CompressionTime += (endTime - startTime);
                    if (tmpChunkLz4CompressSize > 0)
                    {
                        tmpChunk.saveSize = tmpChunkLz4CompressSize;
                        // memcpy(tmpChunk.chunkptr, lz4ChunkBuffer, tmpChunk.savesize);
                    }
                    else
                    {
                        tmpChunk.saveSize = tmpChunk.chunkSize;
                    }
                    // cout << "lz4 chunk size is " << tmpChunk.chunksize << "save size is " << tmpChunk.savesize << endl;
                    tmpChunk.deltaFlag = NO_DELTA;
                    tmpChunk.basechunkID = -1;
                    SF_Insert((char *)tmpChunkSF, FINESSE_SF_NUM * CHUNK_HASH_SIZE, tmpChunk.chunkID);
                    basechunkNum++;
                    basechunkSize += tmpChunk.saveSize;
                }
                else
                {
                    Chunk_t basechunkinfo;
                    uint8_t *deltachunk;
                    int lz4size = 0;
                    tmpChunk.saveSize = 0;
                    basechunkinfo = dataWrite_->Get_Chunk_Info(basechunkID);
                    startTime = std::chrono::high_resolution_clock::now();
                    deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, basechunkinfo.chunkPtr, basechunkinfo.chunkSize, &tmpChunk.saveSize, deltaMaxChunkBuffer);
                    endTime = std::chrono::high_resolution_clock::now();
                    deltaCompressionTime += (endTime - startTime);
                    if (tmpChunk.saveSize > tmpChunk.chunkSize)
                    {
                        cout << "bug" << endl;
                        bugCount++;
                    }
                    // 9513209
                    // cout << "delta size: " << tmpChunk.savesize << " chunk size is " << tmpChunk.chunksize << "base id " << basechunkinfo.chunkid << endl;
                    if (tmpChunk.saveSize == 0)
                    {
                        cout << "delta error and can't to restore" << endl;
                        return;
                    }
                    else
                    {
                        memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize);
                        tmpChunk.deltaFlag = FINESSE_DELTA;
                        tmpChunk.basechunkID = basechunkID;
                        deltachunkNum++;
                        deltachunkSize += tmpChunk.saveSize;
                        free(deltachunk);
                    }
                    dataWrite_->Chunk_Insert(tmpChunk);
                    // free outside of Get_Chunk_Info
                    if (basechunkinfo.loadFromDisk)
                        free(basechunkinfo.chunkPtr);
                }
                uniquechunkNum++;
                uniquechunkSize += tmpChunk.saveSize;
                free(tmpChunkSF);
            }
            else
            {
                free(tmpChunk.chunkPtr);
                tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
            }
            dataWrite_->Recipe_Insert(tmpChunk);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
        }
    }
    recieveQueue->done_ = false;
    return;
}

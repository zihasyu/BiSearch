#include "../../include/dedup.h"

Dedup::Dedup()
{
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
}

Dedup::~Dedup()
{
    free(lz4ChunkBuffer);
    EVP_MD_CTX_free(mdCtx);
    free(hashBuf);
}

void Dedup::ProcessTrace()
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
            cout << "dedup done" << endl;
            break;
        }
        Chunk_t tmpChunk;
        if (recieveQueue->Pop(tmpChunk))
        {
            GenerateHash(mdCtx, tmpChunk.chunkPtr, tmpChunk.chunkSize, hashBuf);
            hashStr.assign((char *)hashBuf, CHUNK_HASH_SIZE);
            int tmpChunkid;
            int findRes = FP_Find(hashStr);
            if (findRes == -1)
            {
                // Unique chunk found
                tmpChunk.chunkID = uniquechunkNum;
                tmpChunk.deltaFlag = NO_DELTA;
                int lz4Size = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                if (lz4Size <= 0)
                {
                    cout << "lz4 compress error" << endl;
                    tmpChunk.deltaFlag = NO_LZ4;
                    lz4Size = tmpChunk.chunkSize;
                }
                //***compare diff ***/
                // uint8_t *lz4SafeChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
                // int decompressedSize = LZ4_decompress_safe((char *)lz4ChunkBuffer, (char *)lz4SafeChunkBuffer, lz4Size, CONTAINER_MAX_SIZE);
                // if (decompressedSize != tmpChunk.chunkSize)
                //     cout << "decompress error" << endl;

                // cout << "cmp is " << std::memcmp(tmpChunk.chunkPtr, lz4SafeChunkBuffer, tmpChunk.chunkSize) << endl;

                // free(lz4SafeChunkBuffer);
                tmpChunk.saveSize = lz4Size;
                FP_Insert(hashStr, tmpChunk.chunkID);
                // /cout << tmpChunkContent << endl;
                // Dedup get superfeature
                // cout << "unique chunk found" << endl;
                if (ContainerSize + tmpChunk.chunkSize > CONTAINER_MAX_SIZE)
                {
                    tmpChunk.containerID = ++containerNum;
                    ContainerSize = 0;
                }
                else
                {
                    tmpChunk.containerID = containerNum;
                }
                tmpChunk.offset = ContainerSize;
                ContainerSize += tmpChunk.chunkSize;
                if (tmpChunk.deltaFlag == NO_LZ4)
                    dataWrite_->Chunk_Insert(tmpChunk);
                else
                    dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                basechunkNum++;
                basechunkSize += tmpChunk.saveSize;
                uniquechunkNum++;
                uniquechunkSize += tmpChunk.saveSize;
            }
            else
            {
                // cout << "dedup chunk found findRes is" << findRes << endl; //debug
                free(tmpChunk.chunkPtr);
                tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
                tmpChunkid = findRes; // 好像没用
            }
            if (tmpChunk.HeaderFlag == 0)
                dataWrite_->Recipe_Insert(tmpChunk.chunkID);
            else
                dataWrite_->Recipe_Header_Insert(tmpChunk.chunkID);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
        }
    }
    cout << "logicalchunkSize is " << logicalchunkSize << endl;
    cout << "uniquechunkSize is " << uniquechunkSize << endl;
    cout << "Overall Compression Ratio: " << (double)logicalchunkSize / (double)uniquechunkSize << endl;
    recieveQueue->done_ = false;
    return;
}

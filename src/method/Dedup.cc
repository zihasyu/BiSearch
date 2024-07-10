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
            outputMQ_->done_ = true;
            recieveQueue->done_ = false;
            cout << "dedup done" << endl;
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
                // /cout << tmpChunkContent << endl;
                // Dedup get superfeature
                cout << "unique chunk found" << endl;
                if (!outputMQ_->Push(tmpChunk)) // chunkSet_->Chunk_Insert(tmpChunk);
                {
                    tool::Logging(myName_.c_str(), "insert chunk to output MQ error.\n");
                    exit(EXIT_FAILURE);
                }
                basechunkNum++;
                basechunkSize += tmpChunk.saveSize;
                uniquechunkNum++;
                uniquechunkSize += tmpChunk.saveSize;
            }
            else
            {
                // tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
                tmpChunkid = findRes; // 好像没用
            }
            dataWrite_->Recipe_Insert(tmpChunk);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
        }
    }
}

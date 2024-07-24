#include "../../include/odess.h"

Odess::Odess()
{
    cout << " FP size is " << sizeof(int) << " Chunk_t is " << sizeof(Chunk_t) << " <super_feature_t, unordered_set<string>> is " << sizeof(super_feature_t);
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
    deltaMaxChunkBuffer = (uint8_t *)malloc(2 * CONTAINER_MAX_SIZE * sizeof(uint8_t));
    SFindex = new unordered_map<string, vector<int>>[FINESSE_SF_NUM];
}

Odess::~Odess()
{
    free(lz4ChunkBuffer);
    free(deltaMaxChunkBuffer);
    EVP_MD_CTX_free(mdCtx);
    free(hashBuf);
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
                // Odess get superfeature
                auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                auto basechunkid = table.SF_Find(superfeature);
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
                    else
                    {
                        tmpChunk.deltaFlag = DELTA;
                        tmpChunk.basechunkID = basechunkid;
                        memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize);
                        deltachunkNum++;
                        deltachunkSize += tmpChunk.saveSize;
                        DeltaReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
                        free(deltachunk);
                    }
                    if (basechunkInfo.loadFromDisk)
                        free(basechunkInfo.chunkPtr);
                    dataWrite_->Chunk_Insert(tmpChunk);
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
                        cout << "lz4 compress error" << endl;
                        tmpChunk.deltaFlag = NO_LZ4;
                        tmpChunk.saveSize = tmpChunk.chunkSize;
                    }

                    tmpChunk.basechunkID = -1;
                    tmpChunkid = tmpChunk.chunkID;
                    table.SF_Insert(superfeature, tmpChunk.chunkID);
                    basechunkNum++;
                    basechunkSize += tmpChunk.saveSize;
                    LocalReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
                    if (tmpChunk.deltaFlag == NO_LZ4)
                        // base chunk & Lz4 error
                        dataWrite_->Chunk_Insert(tmpChunk);
                    else
                        // base chunk &lz4 compress
                        dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                }
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
                DedupGap = 0;
                DedupReductSize += tmpChunk.chunkSize;
            }
            if (tmpChunk.HeaderFlag == 0)
                dataWrite_->Recipe_Insert(tmpChunk.chunkID);
            else
                dataWrite_->Recipe_Header_Insert(tmpChunk.chunkID);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
        }
    }
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

// void Odess::ProcessTrace()
// {
//     string tmpChunkHash;
//     string tmpChunkContent;
//     while (true)
//     {
//         string hashStr;
//         hashStr.assign(CHUNK_HASH_SIZE, 0);
//         std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;

//         if (recieveQueue->done_ && recieveQueue->IsEmpty())
//         {
//             // outputMQ_->done_ = true;
//             recieveQueue->done_ = false;
//             break;
//         }
//         Chunk_t tmpChunk;
//         if (recieveQueue->Pop(tmpChunk))
//         {
//             // calculate feature
//             // compute cutPoint
//             // generate chunk
//             GenerateHash(mdCtx, tmpChunk.chunkPtr, tmpChunk.chunkSize, hashBuf);
//             hashStr.assign((char *)hashBuf, CHUNK_HASH_SIZE);
//             int tmpChunkid;
//             int findRes = FP_Find(hashStr);
//             if (findRes == -1)
//             {
//                 // Unique chunk found
//                 tmpChunk.chunkID = uniquechunkNum;
//                 tmpChunk.deltaFlag = NO_DELTA;
//                 FP_Insert(hashStr, tmpChunk.chunkID);
//                 tmpChunkContent.assign((char *)tmpChunk.chunkPtr, tmpChunk.chunkSize);
//                 tmpChunkHash.assign((char *)hashBuf, CHUNK_HASH_SIZE);
//                 // Odess get superfeature
//                 auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
//                 auto ret = table.GetSimilarRecordKey(superfeature);
//                 // auto ret = table.GetSimilarRecordsKeys(tmpChunkHash);

//                 if (ret != "not found")
//                 // unique chunk & delta chunk
//                 {
//                     int basechunkID = FP_Find(ret);
//                     if (basechunkID != -1)
//                     {
//                         auto basechunkInfo = dataWrite_->Get_Chunk_Info(basechunkID);
//                         uint8_t *deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, basechunkInfo.chunkPtr, basechunkInfo.chunkSize, &tmpChunk.saveSize, deltaMaxChunkBuffer);
//                         if (tmpChunk.saveSize == 0)
//                         {
//                             cout << "delta error" << endl;
//                             return;
//                         }
//                         else
//                         {
//                             tmpChunk.deltaFlag = DELTA;
//                             tmpChunk.basechunkID = basechunkID;
//                             memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize);
//                             deltachunkNum++;
//                             deltachunkSize += tmpChunk.saveSize;
//                             DeltaReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
//                             free(deltachunk);
//                         }
//                         if (basechunkInfo.loadFromDisk)
//                             free(basechunkInfo.chunkPtr);
//                     }
//                     else
//                     {
//                         cout << "find base chunk error" << endl;
//                     }
//                     dataWrite_->Chunk_Insert(tmpChunk);
//                 }
//                 // unique chunk & base chunk
//                 else
//                 {
//                     int tmpChunkLz4CompressSize = 0;
//                     tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
//                     if (tmpChunkLz4CompressSize > 0)
//                     {
//                         tmpChunk.deltaFlag = NO_DELTA;
//                         tmpChunk.saveSize = tmpChunkLz4CompressSize;
//                     }
//                     else
//                     {
//                         cout << "lz4 compress error" << endl;
//                         tmpChunk.deltaFlag = NO_LZ4;
//                         tmpChunk.saveSize = tmpChunk.chunkSize;
//                     }

//                     tmpChunk.basechunkID = -1;
//                     tmpChunkid = tmpChunk.chunkID;
//                     table.Put(tmpChunkHash, tmpChunkContent);
//                     basechunkNum++;
//                     basechunkSize += tmpChunk.saveSize;
//                     LocalReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
//                     if (tmpChunk.deltaFlag == NO_LZ4)
//                         // base chunk & Lz4 error
//                         dataWrite_->Chunk_Insert(tmpChunk);
//                     else
//                         // base chunk &lz4 compress
//                         dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
//                 }
//                 uniquechunkNum++;
//                 uniquechunkSize += tmpChunk.saveSize;
//             }
//             else
//             {
//                 // Dedup chunk found
//                 free(tmpChunk.chunkPtr);
//                 tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
//                 tmpChunkid = findRes;
//                 PrevDedupChunkid = findRes;
//                 DedupGap = 0;
//                 DedupReductSize += tmpChunk.chunkSize;
//             }
//             if (tmpChunk.HeaderFlag == 0)
//                 dataWrite_->Recipe_Insert(tmpChunk.chunkID);
//             else
//                 dataWrite_->Recipe_Header_Insert(tmpChunk.chunkID);
//             logicalchunkNum++;
//             logicalchunkSize += tmpChunk.chunkSize;
//         }
//     }
//     cout << "logicalchunkSize is " << logicalchunkSize << endl;
//     cout << "uniquechunkSize is " << uniquechunkSize << endl;
//     cout << "Overall Compression Ratio: " << (double)logicalchunkSize / (double)uniquechunkSize << endl;
//     recieveQueue->done_ = false;
//     return;
// }

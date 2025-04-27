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
    uint64_t cpSum = 0;
    size_t segmentIndex = 0; // 初始化段索引
    size_t AllFileSegmentIndex = 0;
    double TmpRedundantSize = 0;
    double TmpSumChunkSize = 0;
    std::ofstream FinalVersion;
    FinalVersion.open("FinalVersion.txt", std::ios::out | std::ios::app);
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
            ads_Version++;
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
                while (segmentIndex < dedupSegments[ads_Version].size() && dedupSegments[ads_Version][segmentIndex].end <= cpSum)
                {
                    ++segmentIndex; // 移动到下一个段
                } // 执行完while后，segmentIndex指向第一个文件end大于当前chunk的开始位置
                if (segmentIndex < dedupSegments[ads_Version].size() && cpSum + tmpChunk.chunkSize > dedupSegments[ads_Version][segmentIndex].start)
                {
                    // 外层if的条件是当前chunk的尾部在重复File的头部之后
                    if (cpSum > dedupSegments[ads_Version][segmentIndex].start) // 该chunk完全由重复文件组成
                    {
                        TmpRedundantSize += tmpChunk.chunkSize;
                        TmpSumChunkSize += tmpChunk.chunkSize;
                        casecount_3++;
                        if (ads_Version == FinishVersionNum - 1) // 因为ads_Version在全做完才加1
                        {
                            FinalVersion << 1 << " " << tmpChunk.chunkSize << endl;
                        }
                    }
                    if (cpSum <= dedupSegments[ads_Version][segmentIndex].start) // 该chunk含有重复文件的一部分
                    {
                        TmpRedundantSize += cpSum + tmpChunk.chunkSize - dedupSegments[ads_Version][segmentIndex].start;
                        TmpSumChunkSize += tmpChunk.chunkSize;
                        casecount_2++;
                        if (ads_Version == FinishVersionNum - 1) // 因为ads_Version在全做完才加1
                        {
                            FinalVersion << (double)(cpSum + tmpChunk.chunkSize - dedupSegments[ads_Version][segmentIndex].start) / (double)tmpChunk.chunkSize << " " << tmpChunk.chunkSize << endl;
                        }
                    }
                    casecount++;
                }
                else
                {
                    if (ads_Version == FinishVersionNum - 1) // 因为ads_Version在全做完才加1
                    {
                        FinalVersion << 0 << " " << tmpChunk.chunkSize << endl;
                    }
                }
                while (AllFileSegmentIndex < AllFileSegments[ads_Version].size() && AllFileSegments[ads_Version][AllFileSegmentIndex].end <= cpSum)
                {
                    ++AllFileSegmentIndex; // 移动到下一个段
                } // 执行完while后，AllFileSegmentIndex指向第一个文件end大于当前chunk的开始位置
                tmpChunk.chunkID = uniquechunkNum;
                tmpChunk.deltaFlag = NO_DELTA;
                int lz4Size = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                if (lz4Size <= 0)
                {
                    // cout << "lz4 compress error" << endl;
                    tmpChunk.deltaFlag = NO_LZ4;
                    lz4Size = tmpChunk.chunkSize;
                }
                tmpChunk.saveSize = lz4Size;
                FP_Insert(hashStr, tmpChunk.chunkID);
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
                LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
            }
            else
            {
                free(tmpChunk.chunkPtr);
                tmpChunk = dataWrite_->Get_Chunk_MetaInfo(findRes);
                DedupReduct += tmpChunk.chunkSize;
            }
            cpSum += tmpChunk.chunkSize;
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
    // cout << "Overall Compression Ratio: " << (double)logicalchunkSize / (double)uniquechunkSize << endl;
    cout << "casecount is " << casecount << endl;
    cout << "TmpRedundantSize / TmpSumChunkSize For This Version is " << TmpRedundantSize / TmpSumChunkSize << endl;
    cout << "OffsetChunk percentage " << (double)casecount_3 / (double)uniquechunkNum << endl;
    recieveQueue->done_ = false;
    return;
}

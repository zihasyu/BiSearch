#include "../../include/locality.h"
#define LOCAL_MAX_ERROR 2
LocalDedup::LocalDedup()
{
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
    deltaMaxChunkBuffer = (uint8_t *)malloc(2 * CONTAINER_MAX_SIZE * sizeof(uint8_t));
    plchunk.chunkId = -1;
    plchunk.chunkType = DUP;
    plchunk.compressionRatio = 0.0;
}

LocalDedup::~LocalDedup()
{
    free(lz4ChunkBuffer);
    free(deltaMaxChunkBuffer);
    free(hashBuf);
    EVP_MD_CTX_free(mdCtx);
}

void LocalDedup::ProcessTrace()
{
    string tmpChunkHash;
    string tmpChunkContent;
    while (true)
    {
        string hashStr;
        hashStr.assign(CHUNK_HASH_SIZE, 0);
        if (recieveQueue->done_ && recieveQueue->IsEmpty())
        {
            // outputMQ_->done_ = true;
            recieveQueue->done_ = false;
            Version++;
            ads_Version++;
            break;
        }
        Chunk_t tmpChunk;
        if (recieveQueue->Pop(tmpChunk))
        {
            GenerateHash(mdCtx, tmpChunk.chunkPtr, tmpChunk.chunkSize, hashBuf);
            hashStr.assign((char *)hashBuf, 32);
            int findRes = FP_Find(hashStr);

            if (findRes == -1)
            // unique chunk
            {
                // Unique chunk
                tmpChunk.chunkID = uniquechunkNum;
                uint64_t cp = tmpChunk.chunkSize;
                tmpChunk.deltaFlag = NO_DELTA;
                FP_Insert(hashStr, tmpChunk.chunkID);
                DedupGap++;
                if (plchunk.chunkId + DedupGap < tmpChunk.chunkID - 1 && Version > 0)
                {
                    uint8_t *deltachunk;
                    size_t tmpdeltachunksize = 0;
                    Chunk_t tmpbaseChunkinfo;
                    Chunk_t tmpLocalChunkInfo = dataWrite_->Get_Chunk_MetaInfo(plchunk.chunkId + DedupGap);
                    if (tmpLocalChunkInfo.deltaFlag == LOCAL_DELTA)
                        tmpbaseChunkinfo = dataWrite_->Get_Chunk_Info(tmpLocalChunkInfo.basechunkID);
                    else
                        tmpbaseChunkinfo = dataWrite_->Get_Chunk_Info(plchunk.chunkId + DedupGap);
                    deltachunk = xd3_encode(tmpChunk.chunkPtr, cp, lz4ChunkBuffer, tmpbaseChunkinfo.chunkSize, &tmpdeltachunksize, deltaMaxChunkBuffer);
                    // if delta chunk size > chunk size, then save the base chunk
                    if (tmpdeltachunksize > tmpChunk.chunkSize)
                    {
                        bugCount++;
                        tmpChunk.saveSize = tmpChunk.chunkSize;
                        tmpChunk.deltaFlag = NO_DELTA;
                    }
                    else
                    {
                        tmpChunk.deltaFlag = LOCAL_DELTA;
                    }
                    // unique chunk & locality hit &locality can be accept
                    if (tmpChunk.deltaFlag != NO_DELTA) //&&  ((plchunk.chunkType == FI && (tmpratio  >= plchunk.compressionRatio - FiOffset)) || plchunk.chunkType == DUP) )
                    {
                        tmpChunk.basechunkID = plchunk.chunkId + DedupGap;
                        tmpChunk.deltaFlag = LOCAL_DELTA;
                        tmpChunk.saveSize = tmpdeltachunksize;
                        memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize);
                        free(deltachunk);
                        StatsDelta(tmpChunk);
                        localUniqueSize += tmpChunk.saveSize;
                        localLogicalSize += tmpChunk.chunkSize;
                        localchunkSize += tmpChunk.saveSize;
                        localPrechunkSize += tmpChunk.chunkSize;
                        localError = 0;
                        // save delta
                        dataWrite_->Chunk_Insert(tmpChunk);
                    }
                    // unique chunk & locality hit &but locality can't be accept& do lz4
                    else
                    {
                        if (deltachunk != nullptr)
                        {
                            free(deltachunk);
                            deltachunk = nullptr;
                        }
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
                        localError++;
                        tmpChunk.basechunkID = -1;
                        basechunkNum++;
                        basechunkSize += tmpChunk.saveSize;
                        LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                        if (localError > LOCAL_MAX_ERROR)
                        {
                            localFlag = false;
                        }
                        // save base
                        if (tmpChunk.deltaFlag == NO_LZ4)
                            dataWrite_->Chunk_Insert(tmpChunk);
                        else
                            dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                    }
                    if (tmpbaseChunkinfo.loadFromDisk)
                    {
                        free(tmpbaseChunkinfo.chunkPtr);
                        tmpbaseChunkinfo.chunkPtr = nullptr;
                    }
                }
                else
                {
                    // do lz4compress
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
                    basechunkNum++;
                    basechunkSize += tmpChunk.saveSize;
                    LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                    // save base
                    if (tmpChunk.deltaFlag == NO_LZ4)
                        dataWrite_->Chunk_Insert(tmpChunk);
                    else
                        dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                }
                uniquechunkNum++;
                uniquechunkSize += tmpChunk.saveSize;
            }
            else
            {
                free(tmpChunk.chunkPtr);
                auto tmpInfo = dataWrite_->Get_Chunk_MetaInfo(findRes);
                tmpChunk = tmpInfo;
                localFlag = true;
                if (tmpChunk.HeaderFlag == 0)
                    plchunk.chunkId = findRes;
                plchunk.chunkType = DUP;
                DedupGap = 0;
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

    recieveQueue->done_ = false;
    return;
}

void LocalDedup::Version_log(double time)
{
    cout << "Version: " << ads_Version << endl;
    cout << "-----------------CHUNK NUM-----------------------" << endl;
    cout << "logical chunk num: " << logicalchunkNum << endl;
    cout << "unique chunk num: " << uniquechunkNum << endl;
    cout << "base chunk num: " << basechunkNum << endl;
    cout << "delta chunk num: " << deltachunkNum << endl;
    cout << "-----------------CHUNK SIZE-----------------------" << endl;
    cout << "logicalchunkSize is " << logicalchunkSize << endl;
    cout << "uniquechunkSize is " << uniquechunkSize << endl;
    cout << "base chunk size: " << basechunkSize << endl;
    cout << "delta chunk size: " << deltachunkSize << endl;
    cout << "-----------------METRICS-------------------------" << endl;
    cout << "Overall Compression Ratio: " << (double)logicalchunkSize / (double)uniquechunkSize << endl;
    cout << "DCC: " << (double)deltachunkNum / (double)uniquechunkNum << endl;
    cout << "DCR: " << (double)deltachunkOriSize / (double)deltachunkSize << endl;
    cout << "DCE: " << DCESum / (double)deltachunkNum << endl;
    cout << "-----------------Time------------------------------" << endl;
    // out << "deltaCompressionTime: " << deltaCompressionTime.count() << "s" << endl;
    cout << "Version time: " << time << "s" << endl;
    cout << "Throughput: " << (double)(logicalchunkSize - preLogicalchunkiSize) / time / 1024 / 1024 << "MiB/s" << endl;
    cout << "Reduce data speed: " << (double)(logicalchunkSize - preLogicalchunkiSize - uniquechunkSize + preuniquechunkSize) / time / 1024 / 1024 << "MiB/s" << endl;
    cout << "-----------------OverHead--------------------------" << endl;
    // out << "deltaCompressionTime: " << deltaCompressionTime.count() << "s" << endl;
    cout << "Index Overhead: " << (double)(uniquechunkNum * 112) / 1024 / 1024 << "MiB" << endl;
    cout << "FP Overhead: " << (double)(uniquechunkNum * 80 + uniquechunkNum * 32) / 1024 / 1024 << "MiB" << endl;
    cout << "Recipe Overhead: " << (double)logicalchunkNum * 8 / 1024 / 1024 << "MiB" << endl;
    cout << "-----------------END-------------------------------" << endl;

    preLogicalchunkiSize = logicalchunkSize;
}

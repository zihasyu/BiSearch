#include "../../include/bisearch.h"
#define LOCAL_MAX_ERROR 2
BiSearch::BiSearch(double ratio)
{
    lz4ChunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * sizeof(uint8_t));
    mdCtx = EVP_MD_CTX_new();
    hashBuf = (uint8_t *)malloc(CHUNK_HASH_SIZE * sizeof(uint8_t));
    deltaMaxChunkBuffer = (uint8_t *)malloc(2 * CONTAINER_MAX_SIZE * sizeof(uint8_t));
    // only biSearch has
    LZ4_RATIO = ratio;
    plchunk.chunkId = -1;
    plchunk.chunkType = DUP;
    plchunk.compressionRatio = 0.0;
}

BiSearch::~BiSearch()
{
    free(lz4ChunkBuffer);
    free(deltaMaxChunkBuffer);
    free(hashBuf);
    EVP_MD_CTX_free(mdCtx);
}

void BiSearch::ProcessTrace()
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
            Version++;
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
                tmpChunk.chunkID = uniquechunkNum;
                tmpChunk.deltaFlag = NO_DELTA;
                FP_Insert(hashStr, tmpChunk.chunkID);
                DedupGap++;
                tmpChunkContent.assign((char *)tmpChunk.chunkPtr, tmpChunk.chunkSize);
                tmpChunkHash.assign((char *)hashBuf, CHUNK_HASH_SIZE);

                // unique chunk & locality try & in locality windows
                if (plchunk.chunkId + DedupGap < tmpChunk.chunkID - 1 && Version > 0 && localFlag == true)
                {
                    startTime = std::chrono::high_resolution_clock::now();
                    uint8_t *deltachunk;
                    uint64_t tmpdeltachunksize = 0;
                    Chunk_t tmpbaseChunkinfo;
                    Chunk_t tmpLocalChunkInfo = dataWrite_->Get_Chunk_MetaInfo(plchunk.chunkId + DedupGap);
                    // the chunk who locality find is delta chunk, then we need to find its basechunk as tmpchunk's base chunk
                    if (tmpLocalChunkInfo.deltaFlag == FINESSE_DELTA || tmpLocalChunkInfo.deltaFlag == LOCAL_DELTA)
                    {
                        tmpbaseChunkinfo = dataWrite_->Get_Chunk_Info(tmpLocalChunkInfo.basechunkID);
                        tmpChunk.basechunkID = tmpLocalChunkInfo.basechunkID;
                    }
                    // the chunk who locality find is basechunk, then turn again
                    else
                    {
                        tmpbaseChunkinfo = dataWrite_->Get_Chunk_Info(plchunk.chunkId + DedupGap);
                        tmpChunk.basechunkID = plchunk.chunkId + DedupGap;
                    }
                    endTime = std::chrono::high_resolution_clock::now();
                    fetchBaseChunkTime += (endTime - startTime);

                    startTime = std::chrono::high_resolution_clock::now();
                    deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, tmpbaseChunkinfo.chunkPtr, tmpbaseChunkinfo.chunkSize, &tmpdeltachunksize, deltaMaxChunkBuffer);
                    endTime = std::chrono::high_resolution_clock::now();
                    deltaCompressionTime += (endTime - startTime);

                    if (tmpdeltachunksize > tmpChunk.chunkSize)
                    {
                        cout << "bug in unique chunk & locality try & in locality windows" << endl;
                        cout << "tmpdeltachunksize:" << tmpdeltachunksize << " tmpchunk size is " << tmpChunk.chunkSize << " tmpchunk id is " << tmpChunk.chunkID << endl;
                        bugCount++;
                        tmpChunk.saveSize = tmpChunk.chunkSize;
                        tmpChunk.deltaFlag = NO_DELTA;
                    }
                    else
                    {
                        // memcpy(tmpbaseChunkinfo.chunkPtr, deltachunk,tmpdeltachunksize);//迁移到后面那个if里写比较好
                        // tmpChunk.saveSize = tmpdeltachunksize;后面的if里已经有了
                        tmpChunk.deltaFlag = LOCAL_DELTA;
                    }

                    if (tmpbaseChunkinfo.loadFromDisk)
                    {
                        free(tmpbaseChunkinfo.chunkPtr);
                        tmpbaseChunkinfo.chunkPtr = nullptr;
                    }

                    float tmpratio = tmpChunk.chunkSize / tmpdeltachunksize;

                    // unique chunk & locality hit &locality can be accept
                    if (tmpratio > LZ4_RATIO && tmpChunk.deltaFlag != NO_DELTA) //&&  ((plchunk.chunkType == FI && (tmpratio  >= plchunk.compressionRatio - FiOffset)) || plchunk.chunkType == DUP) )
                    {
                        tmpChunk.deltaFlag = LOCAL_DELTA;
                        tmpChunk.saveSize = tmpdeltachunksize;
                        memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize);
                        free(deltachunk);

                        deltachunkNum++;
                        deltachunkSize += tmpChunk.saveSize;
                        localUniqueSize += tmpChunk.saveSize;
                        localLogicalSize += tmpChunk.chunkSize;
                        localchunkSize += tmpChunk.saveSize;
                        localPrechunkSize += tmpChunk.chunkSize;
                        localError = 0;
                        DeltaReduct += tmpChunk.chunkSize - tmpChunk.saveSize; // delta的贡献
                        // save delta
                        dataWrite_->Chunk_Insert(tmpChunk);
                    }
                    // unique chunk & locality can't be accept
                    else
                    {
                        if (deltachunk != nullptr)
                        {
                            free(deltachunk);
                            deltachunk = nullptr;
                        }
                        // unique chunk & in locality windows & running odess
                        string ret = "not found";
                        if (tmpChunk.chunkSize >= 60)
                        {
                            auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                            ret = table.GetSimilarRecordKey(superfeature);
                        }
                        // unique chunk & in locality windows & odess considered this is a base chunk
                        if (ret == "not found")
                        {
                            int tmpChunkLz4CompressSize = 0;
                            startTime = std::chrono::high_resolution_clock::now();
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

                            localError++;
                            tmpChunk.basechunkID = -1;

                            if (tmpChunk.chunkSize >= 60)
                                table.Put(tmpChunkHash, tmpChunkContent);
                            basechunkNum++;
                            basechunkSize += tmpChunk.saveSize;
                            lz4LogicalSize += tmpChunk.chunkSize;
                            lz4UniqueSize += tmpChunk.saveSize;
                            LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;

                            if (localError > LOCAL_MAX_ERROR)
                            {
                                localFlag = false;
                            }
                            endTime = std::chrono::high_resolution_clock::now();
                            lz4CompressionTime += (endTime - startTime);
                            // save base
                            if (tmpChunk.deltaFlag == NO_LZ4)
                                dataWrite_->Chunk_Insert(tmpChunk);
                            else
                                dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                        }
                        // unique chunk & in locality windows & odess hits
                        else
                        {
                            int basechunkID = FP_Find(ret);
                            Chunk_t basechunkinfo;
                            uint8_t *deltachunk;
                            tmpChunk.saveSize = 0;
                            startTime = std::chrono::high_resolution_clock::now();
                            basechunkinfo = dataWrite_->Get_Chunk_Info(basechunkID);
                            endTime = std::chrono::high_resolution_clock::now();
                            fetchBaseChunkTime += (endTime - startTime);
                            startTime = std::chrono::high_resolution_clock::now();
                            deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, basechunkinfo.chunkPtr, basechunkinfo.chunkSize, &tmpChunk.saveSize, deltaMaxChunkBuffer);
                            if (tmpChunk.saveSize == 0)
                            // odess hit but odess bug
                            {
                                cout << "delta error" << endl;
                                return;
                            }
                            else
                            {
                                // odess hit but odess bug
                                if (tmpChunk.saveSize > tmpChunk.chunkSize)
                                {
                                    cout << "bug in odess hit but odess bug" << endl;
                                    bugCount++;
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
                                    localError++;
                                    tmpChunk.basechunkID = -1;

                                    if (tmpChunk.chunkSize >= 60)
                                        table.Put(tmpChunkHash, tmpChunkContent);
                                    basechunkNum++;
                                    basechunkSize += tmpChunk.saveSize;
                                    lz4LogicalSize += tmpChunk.chunkSize;
                                    lz4UniqueSize += tmpChunk.saveSize;
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
                                else
                                {
                                    // unique chunk & in locality windows & odess hits &odess delta normally
                                    memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize);
                                    plchunk.chunkId = basechunkID;
                                    plchunk.chunkType = FI;
                                    plchunk.compressionRatio = (double)tmpChunk.chunkSize / (double)tmpChunk.saveSize;
                                    DedupGap = 0;
                                    tmpChunk.deltaFlag = FINESSE_DELTA;
                                    tmpChunk.basechunkID = basechunkID;
                                    localError = 0;
                                    finessehit++;
                                    deltachunkNum++;
                                    deltachunkSize += tmpChunk.saveSize;
                                    finessechunkSize += tmpChunk.saveSize;
                                    finessePrechunkSize += tmpChunk.chunkSize;
                                    DeltaReduct += tmpChunk.chunkSize - tmpChunk.saveSize; // delta的贡献
                                    localFlag = true;
                                    // save delta
                                    dataWrite_->Chunk_Insert(tmpChunk);
                                }
                                free(deltachunk);
                                if (basechunkinfo.loadFromDisk)
                                    free(basechunkinfo.chunkPtr);
                            }
                            endTime = std::chrono::high_resolution_clock::now();
                            deltaCompressionTime += (endTime - startTime);
                        }
                        // free(tmpChunkSF);
                    }

                    endTime = std::chrono::high_resolution_clock::now();
                    sumTime5 += (endTime - startTime);
                }
                // odess try & not in locality windows
                else
                {
                    string ret = "not found";
                    if (tmpChunk.chunkSize >= 60)
                    {
                        auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                        ret = table.GetSimilarRecordKey(superfeature);
                    }
                    computeSFtimes++;
                    if (ret == "not found")
                    // odess try & not in locality windows &odess considered this is a base chunk
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

                        if (tmpChunk.chunkSize >= 60)
                            table.Put(tmpChunkHash, tmpChunkContent);
                        basechunkNum++;
                        basechunkSize += tmpChunk.saveSize;
                        lz4LogicalSize += tmpChunk.chunkSize;
                        lz4UniqueSize += tmpChunk.saveSize;
                        LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                        // save base
                        if (tmpChunk.deltaFlag == NO_LZ4)
                            dataWrite_->Chunk_Insert(tmpChunk);
                        else
                            dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                    }
                    else
                    // odess try & not in locality windows &odess hits
                    {
                        Chunk_t basechunkinfo;
                        tmpChunk.saveSize = 0;
                        uint8_t *deltachunk;
                        int basechunkID = FP_Find(ret);
                        basechunkinfo = dataWrite_->Get_Chunk_Info(basechunkID);
                        deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, basechunkinfo.chunkPtr, basechunkinfo.chunkSize, &tmpChunk.saveSize, deltaMaxChunkBuffer);
                        if (tmpChunk.saveSize == 0)
                        {
                            cout << "delta error" << endl;
                            return;
                        }
                        else
                        {
                            if (tmpChunk.saveSize > tmpChunk.chunkSize)
                            {
                                cout << "bug in odess try & not in locality windows &odess hits" << endl;
                                bugCount++;
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

                                if (tmpChunk.chunkSize >= 60)
                                    table.Put(tmpChunkHash, tmpChunkContent);
                                basechunkNum++;
                                basechunkSize += tmpChunk.saveSize;
                                lz4LogicalSize += tmpChunk.chunkSize;
                                lz4UniqueSize += tmpChunk.saveSize;
                                LocalReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                                // save base
                                if (tmpChunk.deltaFlag == NO_LZ4)
                                    dataWrite_->Chunk_Insert(tmpChunk);
                                else
                                    dataWrite_->Chunk_Insert(tmpChunk, lz4ChunkBuffer);
                            }
                            else
                            {
                                plchunk.chunkId = basechunkID;
                                plchunk.chunkType = FI;
                                plchunk.compressionRatio = (double)tmpChunk.chunkSize / (double)tmpChunk.saveSize;

                                memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize); // new

                                DedupGap = 0;
                                tmpChunk.deltaFlag = FINESSE_DELTA;
                                tmpChunk.basechunkID = basechunkID;
                                finessehit++;
                                deltachunkNum++;
                                deltachunkSize += tmpChunk.saveSize;
                                DeltaReduct += tmpChunk.chunkSize - tmpChunk.saveSize;
                                localFlag = true;
                                // save delta
                                dataWrite_->Chunk_Insert(tmpChunk);
                            }
                            free(deltachunk);
                            if (basechunkinfo.loadFromDisk)
                                free(basechunkinfo.chunkPtr);
                        }
                    }
                }
                // dataWrite_->Chunk_Insert(tmpChunk);
                uniquechunkSize += tmpChunk.saveSize;
                uniquechunkNum++;
            }
            else
            {
                free(tmpChunk.chunkPtr);
                auto tmpInfo = dataWrite_->Get_Chunk_MetaInfo(findRes);
                tmpChunk = tmpInfo;
                localFlag = true;
                plchunk.chunkId = findRes;
                plchunk.chunkType = DUP;
                DedupGap = 0;
                lz4LogicalSize += tmpChunk.chunkSize;
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
    cout << "logicalchunkSize is " << logicalchunkSize << endl;
    cout << "uniquechunkSize is " << uniquechunkSize << endl;
    cout << "Overall Compression Ratio: " << (double)logicalchunkSize / (double)uniquechunkSize << endl;
    recieveQueue->done_ = false;
    return;
}

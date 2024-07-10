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
    EVP_MD_CTX_free(mdCtx);
}

void BiSearch::ProcessTrace()
{
    string tmpChunkHash;
    string tmpChunkContent;
    uint64_t LocalityFoundId = 0;
    while (true)
    {
        string hashStr;
        hashStr.assign(CHUNK_HASH_SIZE, 0);
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;

        if (recieveQueue->done_ && recieveQueue->IsEmpty())
        {
            outputMQ_->done_ = true;
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
            int tmpChunkid;
            int findRes = FP_Find(hashStr);
            // tool::PrintBinaryArray((uint8_t*)hashStr.c_str(), 10);
            // cout << "chunking end" << endl;
            if (findRes == -1)
            {
                // Unique chunk
                // cout << "unique chunk" << endl;
                startTime = std::chrono::high_resolution_clock::now();
                tmpChunk.chunkID = uniquechunkNum;
                tmpChunk.deltaFlag = NO_DELTA;
                FP_Insert(hashStr, tmpChunk.chunkID);
                // do lz4compress
                DedupGap++;
                endTime = std::chrono::high_resolution_clock::now();
                sumTime4 += (endTime - startTime);
                // int physicalId = logical2Phyical[plchunk.chunkId + DedupGap];
                int physicalId = 0;
                int prePhysicalId = -1;
                tmpChunkContent.assign((char *)tmpChunk.chunkPtr, tmpChunk.chunkSize);
                tmpChunkHash.assign((char *)hashBuf, CHUNK_HASH_SIZE);
                if (plchunk.chunkId + DedupGap < tmpChunk.chunkID - 1 && Version > 0 && localFlag == true)
                {
                    // cout << "local start" << endl;
                    startTime = std::chrono::high_resolution_clock::now();
                    uint8_t *deltachunk;
                    uint64_t tmpdeltachunksize = 0;
                    Chunk_t tmpbaseChunkinfo;
                    Chunk_t tmpLocalChunkInfo = dataWrite_->Get_Chunk_MetaInfo(plchunk.chunkId + DedupGap);

                    int recursiveRestoreFlag = 0;
                    if (tmpLocalChunkInfo.deltaFlag == FINESSE_DELTA || tmpLocalChunkInfo.deltaFlag == LOCAL_DELTA)
                    {
                        tmpbaseChunkinfo = dataWrite_->Get_Chunk_Info(tmpLocalChunkInfo.basechunkID);
                        tmpChunk.basechunkID = tmpLocalChunkInfo.basechunkID;
                    }
                    else
                    {
                        tmpbaseChunkinfo = dataWrite_->Get_Chunk_Info(plchunk.chunkId + DedupGap);
                        tmpChunk.basechunkID = plchunk.chunkId + DedupGap;
                    }
                    endTime = std::chrono::high_resolution_clock::now();
                    fetchBaseChunkTime += (endTime - startTime);
                    startTime = std::chrono::high_resolution_clock::now();
                    deltachunk = xd3_encode(tmpChunk.chunkPtr, tmpChunk.chunkSize, tmpbaseChunkinfo.chunkPtr, tmpbaseChunkinfo.chunkSize, &tmpdeltachunksize, deltaMaxChunkBuffer);
                    LocalityFoundId = tmpbaseChunkinfo.chunkID;
                    if (tmpbaseChunkinfo.chunkSize < tmpChunk.chunkSize)
                    {
                        // cout << "basechunk size is " << tmpbaseChunkinfo.chunkSize << " current size is " << tmpChunk.chunkSize << endl;
                        // cout << "delta compression ratio is " << (float)tmpChunk.chunkSize / (float)tmpdeltachunksize << endl;
                        smallCounter++;
                    }
                    if (tmpLocalChunkInfo.deltaFlag == FINESSE_DELTA || tmpLocalChunkInfo.deltaFlag == LOCAL_DELTA)
                    {
                        // cout << "after delta size is " << tmpdeltachunksize << endl;
                    }
                    if (tmpdeltachunksize > tmpChunk.chunkSize)
                    {
                        cout << "bug" << endl;
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

                    // free(deltachunk);
                    if (tmpbaseChunkinfo.loadFromDisk)
                    {
                        free(tmpbaseChunkinfo.chunkPtr);
                        tmpbaseChunkinfo.chunkPtr = nullptr;
                    }

                    if (recursiveRestoreFlag == 1 && tmpbaseChunkinfo.chunkPtr != nullptr)
                    {
                        free(tmpbaseChunkinfo.chunkPtr);
                        tmpbaseChunkinfo.chunkPtr = nullptr;
                    }

                    endTime = std::chrono::high_resolution_clock::now();
                    deltaCompressionTime += (endTime - startTime);

                    float tmpratio = tmpChunk.chunkSize / tmpdeltachunksize;
                    // cout << "tmpratio is " << tmpratio << endl;
                    if (tmpratio > LZ4_RATIO && tmpChunk.deltaFlag != NO_DELTA) //&&  ((plchunk.chunkType == FI && (tmpratio  >= plchunk.compressionRatio - FiOffset)) || plchunk.chunkType == DUP) )
                    {
                        // do delta compression
                        tmpChunkid = tmpChunk.chunkID;
                        tmpChunk.deltaFlag = LOCAL_DELTA;
                        // printf("mem1 start \n");
                        tmpChunk.saveSize = tmpdeltachunksize;
                        memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize); // 这行导致的变化
                        free(deltachunk);

                        deltachunkNum++;
                        deltachunkSize += tmpChunk.saveSize;
                        localUniqueSize += tmpChunk.saveSize;
                        localLogicalSize += tmpChunk.chunkSize;
                        localchunkSize += tmpChunk.saveSize;
                        localPrechunkSize += tmpChunk.chunkSize;
                        localError = 0;
                        DeltaReductSize += tmpChunk.chunkSize - tmpChunk.saveSize; // delta的贡献
                        // cout << "local end" << endl;
                    }
                    else
                    {
                        if (deltachunk != nullptr)
                        {
                            free(deltachunk);
                            deltachunk = nullptr;
                        }
                        // get superfeature
                        // cout << "cal sf 1 start" << endl;
                        // uint8_t *tmpChunkSF;
                        // tmpChunkSF = (uint8_t *)malloc(FINESSE_SF_NUM * CHUNK_HASH_SIZE);
                        // startTime = std::chrono::high_resolution_clock::now();
                        // GetSF(tmpChunk.chunkPtr, mdCtx, tmpChunkSF, tmpChunk.chunkSize);

                        // odess
                        string ret = "not found";
                        if (tmpChunk.chunkSize >= 60)
                        {
                            auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                            ret = table.GetSimilarRecordKey(superfeature);
                        }
                        // auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                        // auto ret = table.GetSimilarRecordKey(superfeature);

                        // int basechunkID = chunkSet_->SF_Find((char *)tmpChunkSF, FINESSE_SF_NUM * CHUNK_HASH_SIZE);
                        // computeSFtimes++;
                        // endTime = std::chrono::high_resolution_clock::now();
                        // getSFTime += (endTime - startTime);
                        // cout << "cal sf 1 end" << endl;
                        if (ret == "not found")
                        {

                            // connot find basechunk, do lz4
                            // cout << "lz4 1 start" << endl;
                            int tmpChunkLz4CompressSize = 0;
                            startTime = std::chrono::high_resolution_clock::now();
                            tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                            if (tmpChunkLz4CompressSize <= 0)
                            {
                                tmpChunkLz4CompressSize = tmpChunk.chunkSize;
                                ;
                            }

                            localError++;
                            tmpChunk.basechunkID = -1;
                            tmpChunk.deltaFlag = NO_DELTA;
                            tmpChunk.saveSize = tmpChunkLz4CompressSize;
                            tmpChunkid = tmpChunk.chunkID;
                            if (tmpChunk.chunkSize >= 60)
                                table.Put(tmpChunkHash, tmpChunkContent);
                            basechunkNum++;
                            basechunkSize += tmpChunk.saveSize;
                            lz4LogicalSize += tmpChunk.chunkSize;
                            lz4UniqueSize += tmpChunk.saveSize;
                            LocalReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;

                            if (localError > LOCAL_MAX_ERROR)
                            {
                                localFlag = false;
                            }
                            endTime = std::chrono::high_resolution_clock::now();
                            lz4CompressionTime += (endTime - startTime);
                            // cout << "lz4 1 end" << endl;
                        }
                        else
                        {
                            // cout << "finesse 1 start" << endl;
                            int basechunkID = FP_Find(ret);
                            Chunk_t basechunkinfo;
                            uint8_t *deltachunk;
                            tmpChunk.saveSize = 0;
                            startTime = std::chrono::high_resolution_clock::now();
                            basechunkinfo = dataWrite_->Get_Chunk_Info(basechunkID);
                            endTime = std::chrono::high_resolution_clock::now();
                            fetchBaseChunkTime += (endTime - startTime);
                            if (basechunkinfo.chunkID == LocalityFoundId)
                                sameCount++;
                            startTime = std::chrono::high_resolution_clock::now();
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
                                    cout << "bug" << endl;
                                    bugCount++;
                                    int tmpChunkLz4CompressSize = 0;

                                    tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);

                                    if (tmpChunkLz4CompressSize <= 0)
                                    {
                                        tmpChunkLz4CompressSize = tmpChunk.chunkSize;
                                        ;
                                    }
                                    localError++;
                                    tmpChunk.basechunkID = -1;
                                    tmpChunk.deltaFlag = NO_DELTA;
                                    tmpChunk.saveSize = tmpChunkLz4CompressSize;
                                    tmpChunkid = tmpChunk.chunkID;
                                    if (tmpChunk.chunkSize >= 60)
                                        table.Put(tmpChunkHash, tmpChunkContent);
                                    basechunkNum++;
                                    basechunkSize += tmpChunk.saveSize;
                                    lz4LogicalSize += tmpChunk.chunkSize;
                                    lz4UniqueSize += tmpChunk.saveSize;
                                    LocalReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
                                    if (localError > LOCAL_MAX_ERROR)
                                    {
                                        localFlag = false;
                                    }
                                }
                                else
                                {
                                    // printf("mem2 ");
                                    memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize); // new
                                    // 这个地方不用维护tmpChunkInfo.chunkptr的savesize，因为这里的xd3直接更新了它

                                    plchunk.chunkId = basechunkID;
                                    // localError = 0;
                                    // finessehit++;
                                    // deltachunkNum++;
                                    // deltachunkSize += tmpChunk.saveSize;
                                    // finessechunkSize += tmpChunk.saveSize;
                                    // finessePrechunkSize += tmpChunk.chunkSize;
                                    // localFlag = true;
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
                                    DeltaReductSize += tmpChunk.chunkSize - tmpChunk.saveSize; // delta的贡献
                                    localFlag = true;
                                    // 这个地方为什么要把统计操作做两次？
                                }

                                // int size = chunkSet_->chunklist[basechunkID].dedupChunks.size();
                                // if(size != 0)
                                // {
                                //     plchunk.chunkId = chunkSet_->chunklist[basechunkID].dedupChunks[size - 1];
                                // }else
                                // {
                                //     plchunk.chunkId = basechunkID;
                                // }
                                free(deltachunk);
                                if (basechunkinfo.loadFromDisk)
                                    free(basechunkinfo.chunkPtr);
                            }
                            endTime = std::chrono::high_resolution_clock::now();
                            deltaCompressionTime += (endTime - startTime);
                            // cout << "finesse 1 end" << endl;
                        }
                        // free(tmpChunkSF);

                        // ture to finesse
                    }

                    endTime = std::chrono::high_resolution_clock::now();
                    sumTime5 += (endTime - startTime);
                }
                else
                {
                    // cout << "cal sf 2 start" << endl;
                    // uint8_t *tmpChunkSF;
                    // tmpChunkSF = (uint8_t *)malloc(FINESSE_SF_NUM * CHUNK_HASH_SIZE);
                    // GetSF(tmpChunk.chunkPtr, mdCtx, tmpChunkSF, tmpChunk.chunkSize);
                    // int basechunkID = chunkSet_->SF_Find((char *)tmpChunkSF, FINESSE_SF_NUM * CHUNK_HASH_SIZE);
                    string ret = "not found";
                    if (tmpChunk.chunkSize >= 60)
                    {
                        auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                        ret = table.GetSimilarRecordKey(superfeature);
                    }
                    // auto superfeature = table.feature_generator_.GenerateSuperFeatures(tmpChunkContent);
                    // auto ret = table.GetSimilarRecordKey(superfeature);

                    computeSFtimes++;
                    // cout << "cal sf 2 end" << endl;
                    if (ret == "not found")
                    {

                        // connot find basechunk, do lz4
                        // cout << "lz4 2 start" << endl;
                        int tmpChunkLz4CompressSize = 0;

                        tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);

                        if (tmpChunkLz4CompressSize <= 0)
                        {
                            tmpChunkLz4CompressSize = tmpChunk.chunkSize;
                            ;
                        }

                        tmpChunk.basechunkID = -1;
                        tmpChunk.deltaFlag = NO_DELTA;
                        tmpChunk.saveSize = tmpChunkLz4CompressSize;
                        tmpChunkid = tmpChunk.chunkID;
                        if (tmpChunk.chunkSize >= 60)
                            table.Put(tmpChunkHash, tmpChunkContent);
                        basechunkNum++;
                        basechunkSize += tmpChunk.saveSize;
                        lz4LogicalSize += tmpChunk.chunkSize;
                        lz4UniqueSize += tmpChunk.saveSize;
                        LocalReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
                        // cout << "lz4 2 end" << endl;
                    }
                    else
                    {
                        // cout << "finesse 2 start" << endl;
                        Chunk_t basechunkinfo;
                        tmpChunk.saveSize = 0;
                        uint8_t *deltachunk;
                        int basechunkID = FP_Find(ret);
                        basechunkinfo = dataWrite_->Get_Chunk_Info(basechunkID);

                        if (basechunkinfo.chunkID == LocalityFoundId)
                            sameCount++;
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
                                cout << "bug" << endl;
                                bugCount++;
                                int tmpChunkLz4CompressSize = 0;
                                tmpChunkLz4CompressSize = LZ4_compress_fast((char *)tmpChunk.chunkPtr, (char *)lz4ChunkBuffer, tmpChunk.chunkSize, tmpChunk.chunkSize, 3);
                                if (tmpChunkLz4CompressSize <= 0)
                                {
                                    tmpChunkLz4CompressSize = tmpChunk.chunkSize;
                                    ;
                                }
                                tmpChunk.basechunkID = -1;
                                tmpChunk.deltaFlag = NO_DELTA;
                                tmpChunk.saveSize = tmpChunkLz4CompressSize;
                                tmpChunkid = tmpChunk.chunkID;
                                if (tmpChunk.chunkSize >= 60)
                                    table.Put(tmpChunkHash, tmpChunkContent);
                                basechunkNum++;
                                basechunkSize += tmpChunk.saveSize;
                                lz4LogicalSize += tmpChunk.chunkSize;
                                lz4UniqueSize += tmpChunk.saveSize;
                                LocalReductSize += tmpChunk.chunkSize - tmpChunk.saveSize;
                            }
                            else
                            {
                                plchunk.chunkId = basechunkID;
                                plchunk.chunkType = FI;
                                plchunk.compressionRatio = (double)tmpChunk.chunkSize / (double)tmpChunk.saveSize;
                                // printf("mem3 ");
                                memcpy(tmpChunk.chunkPtr, deltachunk, tmpChunk.saveSize); // new

                                DedupGap = 0;
                                tmpChunk.deltaFlag = FINESSE_DELTA;
                                tmpChunk.basechunkID = basechunkID;
                                finessehit++;
                                deltachunkNum++;
                                deltachunkSize += tmpChunk.saveSize;
                                DeltaReductSize += tmpChunk.chunkSize - tmpChunk.saveSize; // delta的贡献
                                localFlag = true;
                            }
                            free(deltachunk);
                            if (basechunkinfo.loadFromDisk)
                                free(basechunkinfo.chunkPtr);
                        }
                        // cout << "finesse 2 end" << endl;
                    }
                    // free(tmpChunkSF);
                }
                if (!outputMQ_->Push(tmpChunk)) // chunkSet_->Chunk_Insert(tmpChunk);
                {
                    tool::Logging(myName_.c_str(), "insert chunk to output MQ error.\n");
                    exit(EXIT_FAILURE);
                }
                tmpChunkid = tmpChunk.chunkID;
                uniquechunkSize += tmpChunk.saveSize;
                uniquechunkNum++;
            }
            else
            {
                auto tmpInfo = dataWrite_->Get_Chunk_MetaInfo(findRes);
                tmpChunk = tmpInfo;
                localFlag = true;
                tmpChunkid = findRes;
                plchunk.chunkId = findRes;
                plchunk.chunkType = DUP;
                DedupGap = 0;
                lz4LogicalSize += tmpChunk.chunkSize;
                DedupReductSize += tmpChunk.chunkSize; // 单独计算去重的贡献
            }

            dataWrite_->Recipe_Insert(tmpChunk);
            logicalchunkNum++;
            logicalchunkSize += tmpChunk.chunkSize;
            Recipe_t recipe;
            recipe.chunkId = tmpChunk.chunkID;
        }
    }
    recieveQueue->done_ = false;
    return;
}

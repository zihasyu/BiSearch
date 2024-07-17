#include "../../include/datawrite.h"

dataWrite::dataWrite()
{
    // MQ = new MessageQueue<Container_t>(32);
    curContainer.size = 0;
    curContainer.containerID = 0;
    curContainer.chunkNum = 0;
    containerCache = new ReadCache();
}
dataWrite::~dataWrite()
{
    // for (int i = 0; i < chunkNum; i++)
    // {
    //     free(chunklist[i].chunkPtr);
    // }
    delete containerCache;
}
void dataWrite::PrintBinaryArray(const uint8_t *buffer, size_t buffer_size)
{
    for (size_t i = 0; i < buffer_size; i++)
    {
        fprintf(stdout, "%02x", buffer[i]);
    }
    fprintf(stdout, "\n");
    return;
}
void dataWrite::writing()
{
    while (true)
    {
        if (recieveQueue->done_ && recieveQueue->IsEmpty())
        {
            cout << "writing end" << endl;
            recieveQueue->done_ = false;
            break;
        }
        Chunk_t chunk;
        if (recieveQueue->Pop(chunk))
        {
            // cout << "writing if start" << endl;
            int tmpSize = 0;
            if (chunk.deltaFlag == NO_DELTA)
                tmpSize = chunk.chunkSize;
            else
                tmpSize = chunk.saveSize;
            // cout << "flag is " << static_cast<int>(chunk.deltaFlag) << endl;
            chunkNum++;
            containerSize += chunk.saveSize;
            curContainer.chunkNum++;

            if (curContainer.size + tmpSize > CONTAINER_MAX_SIZE)
            {
                // TODO put into MQ
                // cout << " curContainer.chunkNum is" << curContainer.chunkNum << " curContainer.containerID is " << curContainer.containerID << endl;
                startTime = std::chrono::high_resolution_clock::now();
                // cout << "push container " << containerNum << " into MQ" << endl;
                // cout << "cur container size is " << curContainer.size << endl;
                // MQ->Push(curContainer);
                string fileName = "./Containers/" + to_string(curContainer.containerID);
                ofstream outfile(fileName);
                if (outfile.is_open())
                {
                    // cout << "write id is " << tmpContainer.containerID << " size is " << tmpContainer.size << endl;
                    outfile.write(reinterpret_cast<const char *>(&curContainer.size), sizeof(curContainer.size));

                    outfile.write(reinterpret_cast<const char *>(curContainer.data), curContainer.size);
                    // outfile.write(reinterpret_cast<const char *>(&tmpContainer.size), sizeof(tmpContainer.size));
                    // outfile << tmpContainer.data;
                    outfile.close();
                    // cout << "write done" << endl;
                }
                else
                {
                    cout << "open file failed" << endl;
                }

                // sleep(1);
                containerNum++;
                containerSize = 0;
                curOffset = 0;
                curContainer.size = 0;
                curContainer.containerID = containerNum;
                curContainer.chunkNum = 0;
                endTime = std::chrono::high_resolution_clock::now();
                writeIOTime += (endTime - startTime);
            }
            // TODO: put chunk into container
            chunk.containerID = containerNum;
            chunk.offset = curOffset;
            // cout << " curContainer.size is " << curContainer.size << " tmpSize is " << tmpSize << " offset is " << curOffset << endl;
            curContainer.size += tmpSize;
            // cout<< "tmp size is " << tmpSize << " curoffset is " << curOffset<<endl;
            memcpy(curContainer.data + curOffset, chunk.chunkPtr, tmpSize);
            curOffset += tmpSize;
            // cout << "free chunk " << endl;
            free(chunk.chunkPtr);
            // cout << "free chunk done" << endl;
            chunk.chunkPtr = nullptr;
            // chunkprint(chunk); //debug
            // std::lock_guard<std::mutex> lock(mtx);
            // chunklist.push_back(chunk);
            // cout << "dataWrite entry id is  " << chunklist[chunk.chunkID].chunkID << endl;
            // cout << "writing start if end" << endl;
        }
    }
}
bool dataWrite::Chunk_Insert(Chunk_t chunk)
{
    int tmpSize = 0;
    if (chunk.deltaFlag == NO_DELTA)
        tmpSize = chunk.chunkSize;
    else
        tmpSize = chunk.saveSize;
    // cout << "flag is " << static_cast<int>(chunk.deltaFlag) << endl;
    chunkNum++;
    containerSize += chunk.saveSize;
    curContainer.chunkNum++;

    if (curContainer.size + tmpSize > CONTAINER_MAX_SIZE)
    {
        // TODO put into MQ
        // cout << " curContainer.chunkNum is" << curContainer.chunkNum << " curContainer.containerId is " << curContainer.containerID << endl;
        startTime = std::chrono::high_resolution_clock::now();
        // cout << "push container " << containerNum << " into MQ" << endl;
        // cout << "cur container size is " << curContainer.size << endl;
        // MQ->Push(curContainer);
        string fileName = "./Containers/" + to_string(curContainer.containerID);
        ofstream outfile(fileName);
        if (outfile.is_open())
        {
            // cout << "write id is " << tmpContainer.containerId << " size is " << tmpContainer.size << endl;
            outfile.write(reinterpret_cast<const char *>(&curContainer.size), sizeof(curContainer.size));

            outfile.write(reinterpret_cast<const char *>(curContainer.data), curContainer.size);
            // outfile.write(reinterpret_cast<const char *>(&tmpContainer.size), sizeof(tmpContainer.size));
            // outfile << tmpContainer.data;
            outfile.close();
            // cout << "write done" << endl;
        }
        else
        {
            cout << "open file failed" << endl;
        }

        // sleep(1);
        containerNum++;
        containerSize = 0;
        curOffset = 0;
        curContainer.size = 0;
        curContainer.containerID = containerNum;
        curContainer.chunkNum = 0;
        endTime = std::chrono::high_resolution_clock::now();
        writeIOTime += (endTime - startTime);
    }
    // TODO: put chunk into container
    chunk.containerID = containerNum;
    chunk.offset = curOffset;
    // cout << " curContainer.size is " << curContainer.size << " tmpSize is " << tmpSize << " offset is " << curOffset << endl;
    curContainer.size += tmpSize;
    // cout<< "tmp size is " << tmpSize << " curoffset is " << curOffset<<endl;
    memcpy(curContainer.data + curOffset, chunk.chunkPtr, tmpSize);
    curOffset += tmpSize;
    // cout << "free chunk " << endl;
    free(chunk.chunkPtr);
    // cout << "free chunk done" << endl;
    chunk.chunkPtr = nullptr;
    chunklist.push_back(chunk);
    // cout << "chunkset entry id is  " << chunklist[chunk.chunkid].chunkid << endl;
    return true;
}

void dataWrite::restoreFile(string fileName)
{
    string name;
    size_t pos = fileName.find_last_of('/');
    if (pos != std::string::npos)
    {
        name = fileName.substr(pos + 1);
    }
    else
    {
        name = fileName;
    }
    string writePath = "./restoreFile/" + name;
    // cout << chunkSet_.size() << endl;
    cout << "write path is " << writePath << endl;
    ofstream outFile(writePath);

    auto tmpRecipe = RecipeMap[fileName];
    for (auto recipe : tmpRecipe)
    {
        Chunk_t tmpChunkInfo = Get_Chunk_Info(recipe);
        if (tmpChunkInfo.deltaFlag == NO_DELTA)
        {
            outFile.write((char *)tmpChunkInfo.chunkPtr, tmpChunkInfo.chunkSize);
        }
        else
        {
            // auto tmpLocalChunkInfo = xd3_recursive_restore(tmpChunkInfo);
            auto baseChunkInfo = Get_Chunk_Info(tmpChunkInfo.basechunkID);
            uint64_t recSize = 0;
            auto chunk_ptr = xd3_decode(tmpChunkInfo.chunkPtr, tmpChunkInfo.saveSize, baseChunkInfo.chunkPtr, baseChunkInfo.chunkSize, &recSize);
            // cout << "rec size is " << recSize << endl;
            //  memcpy(tmpChunkInfo.chunkptr, chunk_ptr, recSize);
            outFile.write((char *)chunk_ptr, tmpChunkInfo.chunkSize);

            if (baseChunkInfo.loadFromDisk)
                free(baseChunkInfo.chunkPtr);
            if (chunk_ptr != nullptr)
            {
                free(chunk_ptr);
                chunk_ptr = nullptr;
            }
        }
        if (tmpChunkInfo.loadFromDisk)
            free(tmpChunkInfo.chunkPtr);
    }

    outFile.close();
    return;
}

int dataWrite::Get_Chunk_Num()
{
    return chunkNum;
}

int dataWrite::Get_Container_Num(Chunk_t chunk)
{
    if (containerSize + chunk.saveSize > CONTAINER_MAX_SIZE)
    {
        return containerNum + 1;
    }
    else
    {
        return containerNum;
    }
}

Chunk_t dataWrite::Get_Chunk_Info(int id)
{
    // TODO: cache read container
    // cout << "chunk list size is " << chunklist.size() << endl;
    int tmpSize = 0;
    if (chunklist[id].deltaFlag == NO_DELTA)
        tmpSize = chunklist[id].chunkSize;
    else
        tmpSize = chunklist[id].saveSize;

    string tmpContainerIDcontainerID = to_string(chunklist[id].containerID);
    bool cacheHitResult = containerCache->ExistsInCache(tmpContainerIDcontainerID);
    // TODO: if cache hit read from cache

    if (cacheHitResult)
    {

        startTime = std::chrono::high_resolution_clock::now();
        string tmpContainer;
        tmpContainer.assign(CONTAINER_MAX_SIZE, 0);
        uint8_t *tmpContainerData = containerCache->ReadFromCache(tmpContainerIDcontainerID);
        // memcpy((uint8_t *)tmpContainer.c_str(), tmpContainerData, CONTAINER_MAX_SIZE);
        // memcpy(chunklist[id].chunkPtr, tmpContainer.c_str() + chunklist[id].offset, tmpSize);
        chunklist[id].chunkPtr = tmpContainerData + chunklist[id].offset;
        // cout << "read from cache and size is " << chunklist[id].chunkSize << endl;
        //  memcpy(chunklist[id].chunkPtr, tmpContainerData + chunklist[id].offset, tmpSize);
        cacheHitTimes++;
        endTime = std::chrono::high_resolution_clock::now();
        readCacheTime += (endTime - startTime);
        chunklist[id].loadFromDisk = false;
    }
    // TODO: if cache miss, read from file
    else if (chunklist[id].containerID != containerNum)
    {
        chunklist[id].chunkPtr = (uint8_t *)malloc(tmpSize);
        startTime = std::chrono::high_resolution_clock::now();
        string fileName = "./Containers/" + tmpContainerIDcontainerID;
        // cout << fileName << endl;
        ifstream infile(fileName, ios::binary);
        if (infile.is_open())
        {
            uint64_t size;
            infile.read((char *)&size, sizeof(uint64_t));

            //     Allocate memory for the container
            string container;
            container.assign(size, 0);

            // Read the entire container
            infile.read((char *)container.c_str(), size);
            // Copy the required data to chunkPtr
            // cout << "offset is " << chunklist[id].offset << "saveSize is " << chunklist[id].saveSize << endl;
            memcpy(chunklist[id].chunkPtr, (uint8_t *)(container.c_str() + chunklist[id].offset), tmpSize);

            // Add the container to the cache
            startTime2 = std::chrono::high_resolution_clock::now();
            containerCache->InsertToCache(tmpContainerIDcontainerID, (uint8_t *)container.c_str(), size);
            endTime2 = std::chrono::high_resolution_clock::now();
            UpdateCacheTime += (endTime2 - startTime2);
            infile.close();
            // sleep(0.5);
        }
        endTime = std::chrono::high_resolution_clock::now();
        readIOTime += (endTime - startTime);
        loadContainerTimes++;
        chunklist[id].loadFromDisk = true;
        // cout << "read from disk and size is " << chunklist[id].chunkSize << endl;
    }
    else
    {
        if (chunklist[id].containerID == containerNum)
        {
            // free(chunklist[id].chunkPtr);
            //  memcpy(chunklist[id].chunkPtr, curContainer.data + chunklist[id].offset, tmpSize);
            chunklist[id].chunkPtr = curContainer.data + chunklist[id].offset;
            chunklist[id].loadFromDisk = false;
        }
        else
        {
            cout << "open file failed" << endl;
        }
    }

    return chunklist[id];
}

// bool dataWrite::Recipe_Insert(Chunk_t &info)
// {
//     recipelist.push_back(info);
//     return true;
// }

bool dataWrite::Recipe_Insert(Chunk_t &info)
{
    // if (info.HeaderFlag == 0)
    //     RecipeMap[filename].push_back(info.chunkID);
    // else
    // {
    //     Recipe_Header_t tmpHeader;
    //     tmpHeader.chunkId = info.chunkID;
    //     tmpHeader.mask = info.saveSize;
    //     RecipeMap_header[filename].push_back(tmpHeader);
    // }
    RecipeMap[filename].push_back(info.chunkID);
    return true;
}

bool dataWrite::Recipe_Header_Insert(uint64_t chunkID, uint64_t mask)
{
    Recipe_Header_t tmpHeader;
    tmpHeader.chunkId = chunkID;
    tmpHeader.mask = mask;
    RecipeMap_header[filename].push_back(tmpHeader);
    return true;
}

void dataWrite::Save_to_File_Chunking(string methodname)
{
    ofstream outfile;
    // uint64_t traceid = 0;
    string filename = "./" + methodname + "_recipe.txt";
    if (!tool::FileExist(filename))
    {
        outfile.open(filename, ios::out);
        outfile << "Traceid,"
                << "ChunkID,"
                << "BasechunkID,"
                << "ChunkSize,"
                << "SaveSize,"
                << "DeltaFlag,"
                //<< "tmpFinesseSize"
                //<< "tmpLocalSize"
                << endl;
    }
    else
    {
        outfile.open(filename, ios::out | ios::binary);
    }

    if (!outfile.is_open())
    {
        cout << "open file failed" << endl;
        return;
    }
    for (int i = 0; i < recipelist.size(); i++)
    {
        uint64_t traceid = i;
        auto tmpChunkrecipe = recipelist[i];
        // Chunk_t tmpChunkrecipe = this->Get_Chunk_Info(chunkID);
        int basechunkID = tmpChunkrecipe.basechunkID;
        uint64_t chunkSize = tmpChunkrecipe.chunkSize;
        uint64_t saveSize = tmpChunkrecipe.saveSize;
        uint64_t chunkID = tmpChunkrecipe.chunkID;

        int DeltaFlag = tmpChunkrecipe.deltaFlag;
        string ChunkFlag;
        // BUG flag

        // cutpoint
        stringstream ss;
        ss << hex << tmpChunkrecipe.chunkSize;
        string cutPoint = ss.str();

        if (DeltaFlag == NO_DELTA)
        {
            ChunkFlag = "Base";
        }
        else if (DeltaFlag == FINESSE_TO_BASE)
        {
            ChunkFlag = "Finesse_To_Base";
        }
        else if (DeltaFlag == FINESSE_DELTA)
        {
            ChunkFlag = "Finesse";
        }
        else if (DeltaFlag == LOCAL_DELTA)
        {
            // cout << "Local" << endl;
            ChunkFlag = "Local";
        }

        outfile << traceid << "," << chunkID << "," << basechunkID << "," << chunkSize << "," << saveSize
                << "," << ChunkFlag << "," << cutPoint << endl;
    }
    outfile.close();
    return;
}
void dataWrite::Save_to_File(string methodname)
{
    ofstream outfile;
    // uint64_t traceid = 0;
    string filename = "./" + methodname + "_recipe.txt";
    if (!tool::FileExist(filename))
    {
        outfile.open(filename, ios::out);
        outfile << "Traceid,"
                << "ChunkID,"
                << "BasechunkID,"
                << "ChunkSize,"
                << "SaveSize,"
                << "DeltaFlag,"
                << "ChunkFlag"
                //<< "tmpFinesseSize"
                //<< "tmpLocalSize"
                << endl;
    }
    else
    {
        outfile.open(filename, ios::out | ios::binary);
    }

    if (!outfile.is_open())
    {
        cout << "open file failed" << endl;
        return;
    }
    for (int i = 0; i < recipelist.size(); i++)
    {
        uint64_t traceid = i;
        auto tmpChunkrecipe = recipelist[i];
        // Chunk_t tmpChunkrecipe = this->Get_Chunk_Info(chunkID);
        int basechunkID = tmpChunkrecipe.basechunkID;
        uint64_t chunkSize = tmpChunkrecipe.chunkSize;
        uint64_t saveSize = tmpChunkrecipe.saveSize;

        uint64_t chunkID = tmpChunkrecipe.chunkID;

        int DeltaFlag = tmpChunkrecipe.deltaFlag;
        string ChunkFlag;
        // BUG flag

        // cutpoint
        stringstream ss;
        ss << hex << tmpChunkrecipe.chunkSize;
        string cutPoint = ss.str();

        if (DeltaFlag == NO_DELTA)
        {
            ChunkFlag = "Base";
        }
        else if (DeltaFlag == FINESSE_TO_BASE)
        {
            ChunkFlag = "Finesse_To_Base";
        }
        else if (DeltaFlag == FINESSE_DELTA)
        {
            ChunkFlag = "Finesse";
        }
        else if (DeltaFlag == LOCAL_DELTA)
        {
            // cout << "Local" << endl;
            ChunkFlag = "Local";
        }

        outfile << traceid << "," << chunkID << "," << basechunkID << "," << chunkSize << "," << saveSize
                << "," << ChunkFlag << endl;
    }
    outfile.close();
    return;
}

void dataWrite::Save_to_File_unique(string methodname)
{
    ofstream outfile;
    // uint64_t traceid = 0;
    string filename = "./" + methodname + "_recipe_unique.txt";
    if (!tool::FileExist(filename))
    {
        outfile.open(filename, ios::out);
        outfile << "ChunkID,"
                << "BasechunkID,"
                << "ChunkSize,"
                << "SaveSize,"
                << "DeltaFlag,"
                << "ChunkFlag"
                //<< "tmpFinesseSize"
                //<< "tmpLocalSize"
                << endl;
    }
    else
    {
        outfile.open(filename, ios::out | ios::binary);
    }

    if (!outfile.is_open())
    {
        cout << "open file failed" << endl;
        return;
    }
    for (int i = 0; i < chunklist.size(); i++)
    {
        Chunk_t tmpChunkrecipe = this->Get_Chunk_Info(i);
        int basechunkID = tmpChunkrecipe.basechunkID;
        uint64_t chunkSize = tmpChunkrecipe.chunkSize;
        uint64_t saveSize = tmpChunkrecipe.saveSize;
        int DeltaFlag = tmpChunkrecipe.deltaFlag;
        string ChunkFlag;
        if (DeltaFlag == NO_DELTA)
        {
            ChunkFlag = "Base";
        }
        else if (DeltaFlag == FINESSE_TO_BASE)
        {
            ChunkFlag = "Finesse_To_Base";
        }
        else if (DeltaFlag == FINESSE_DELTA)
        {
            ChunkFlag = "Odess";
        }
        else if (DeltaFlag == LOCAL_DELTA)
        {
            // cout << "Local" << endl;
            ChunkFlag = "Local";
        }

        outfile << i << "," << basechunkID << "," << chunkSize << "," << saveSize
                << "," << DeltaFlag << "," << ChunkFlag << endl;
    }
    outfile.close();
    return;
}

void dataWrite::writeContainers()
{
    Container_t tmpContainer;
    bool jobDoneFlag = false;

    while (true)
    {
        if (MQ->done_ && MQ->IsEmpty())
        {
            jobDoneFlag = true;
        }
        // consume a message
        if (MQ->Pop(tmpContainer))
        {
            //  write a container
            // cout << "write a container " << tmpContainer.containerID << endl;
            string fileName = "./Containers/" + to_string(tmpContainer.containerID);
            ofstream outfile(fileName);
            if (outfile.is_open())
            {
                // cout << "write id is " << tmpContainer.containerID << " size is " << tmpContainer.size << endl;
                outfile.write(reinterpret_cast<const char *>(&tmpContainer.size), sizeof(tmpContainer.size));

                outfile.write(reinterpret_cast<const char *>(tmpContainer.data), tmpContainer.size);
                // outfile.write(reinterpret_cast<const char *>(&tmpContainer.size), sizeof(tmpContainer.size));
                // outfile << tmpContainer.data;
                outfile.close();
                // cout << "write done" << endl;
            }
            else
            {
                cout << "open file failed" << endl;
            }
        }

        if (jobDoneFlag)
        {
            break;
        }
    }
    // cout << "write done" << endl;

    return;
}

void dataWrite::ProcessLastContainer()
{
    // if (curContainer.size != 0)
    // {
    //     MQ->Push(curContainer);
    // }
    // MQ->done_ = true;
    if (curContainer.size != 0)
    {
        string fileName = "./Containers/" + to_string(curContainer.containerID);
        ofstream outfile(fileName);
        if (outfile.is_open())
        {
            outfile.write(reinterpret_cast<const char *>(&curContainer.size), sizeof(curContainer.size));
            outfile.write(reinterpret_cast<const char *>(curContainer.data), curContainer.size);

            outfile.close();
        }
        else
        {
            cout << "open file failed" << endl;
        }
    }

    return;
}

bool dataWrite::isLz4(int id)
{
    if (chunklist[id].deltaFlag == NO_DELTA)
    {
        return true;
    }
    else
    {
        return false;
    }
}

Chunk_t dataWrite::Get_Chunk_MetaInfo(int id)
{
    // std::lock_guard<std::mutex> lock(mtx);
    if (chunklist.size() < id)
    {
        cout << "errorrrr!" << endl;
        cout << "size is " << chunklist.size() << " id is " << id << endl;
    }
    auto ret = chunklist[id];
    return ret;
}

void dataWrite::PrintMetrics()
{
    cout << "load container times: " << loadContainerTimes << endl;
    cout << "cache hit times: " << cacheHitTimes << endl;

    auto readIOTimeMin = std::chrono::duration_cast<std::chrono::seconds>(readIOTime);
    cout << "Read IO time: " << readIOTimeMin.count() << " seconds" << endl;
    auto writeIOTimeMin = std::chrono::duration_cast<std::chrono::seconds>(writeIOTime);
    cout << "Write IO time: " << writeIOTimeMin.count() << " seconds" << endl;
    auto UpdateCacheTimeMin = std::chrono::duration_cast<std::chrono::seconds>(UpdateCacheTime);
    cout << "Update Cache time: " << UpdateCacheTimeMin.count() << "seconds" << endl;
    auto readCacheTimeMin = std::chrono::duration_cast<std::chrono::seconds>(readCacheTime);
    cout << "Read Cache time: " << readCacheTimeMin.count() << "seconds" << endl;
    return;
}

// bool dataWrite::isDuplicate(int id)
// {
//     // cout << 777 <<endl;
//     if (chunklist[id].dedupChunks.size() != 0)
//     {
//         // cout << "is duplicate" << endl;
//         return true;
//     }
//     else
//     {
//         // cout << "not duplicate" << endl;
//         return false;
//     }
// }
uint8_t *dataWrite::xd3_decode(const uint8_t *in, size_t in_size, const uint8_t *ref, size_t ref_size, size_t *res_size) // 更改函数
{
    const auto max_buffer_size = CONTAINER_MAX_SIZE * 2;
    uint8_t *buffer;
    buffer = (uint8_t *)malloc(max_buffer_size);
    size_t sz;
    // cout << sz << endl;
    // cout << "max_buffer_size:" << max_buffer_size << endl;
    // auto ret = xd3_decode_memory(in, in_size, ref, ref_size, buffer, &sz, max_buffer_size, 0);
    auto ret = xd3_decode_memory(in, in_size, ref, ref_size, buffer, &sz, max_buffer_size, 0);
    if (ret != 0)
    {
        cout << "decode error" << endl;
        cout << "ret code is " << ret << endl;
        const char *errMsg = xd3_strerror(ret);
        if (errMsg != nullptr)
        {
            printf("%s\n", errMsg);
        }
        else
        {
            printf("Unknown error\n");
        }
    }
    uint8_t *res;
    res = (uint8_t *)malloc(sz);
    *res_size = sz;

    // printf("xdxd3的mem前\n");

    // cout << sz << endl;

    memcpy(res, buffer, sz);
    // printf("xdxd3的mem后\n");
    free(buffer);
    // printf("buffer后\n");
    return res;
}
void dataWrite::SetFilename(string name)
{
    filename.assign(name);
    return;
}
void dataWrite::chunkprint(const Chunk_t chunk)
{
    cout << " chunkID: " << chunk.chunkID << endl;
    cout << " chunkSize: " << chunk.chunkSize << endl;
    cout << " saveSize: " << chunk.saveSize << endl;
    cout << " deltaFlag: " << chunk.deltaFlag << endl;
    // cout << " chunkPtr: " << chunk.chunkPtr << endl;
    cout << " loadFromDisk: " << chunk.loadFromDisk << endl;
    cout << " Offset: " << chunk.offset << endl;
    cout << " containerID: " << chunk.containerID << endl;
    cout << endl;
}
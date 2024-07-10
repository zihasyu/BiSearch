#include "../../include/datawrite.h"

dataWrite::dataWrite()
{
    MQ = new MessageQueue<Container_t>(32);
    curContainer.size = 0;
    curContainer.containerID = 0;
    curContainer.chunkNum = 0;
    containerCache = new ReadCache();
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
            break;
        }
        Chunk_t chunk;
        if (recieveQueue->Pop(chunk))
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
                cout << " curContainer.chunkNum is" << curContainer.chunkNum << " curContainer.containerID is " << curContainer.containerID << endl;
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
            chunklist.push_back(chunk);
            // cout << "dataWrite entry id is  " << chunklist[chunk.chunkID].chunkID << endl;
            return;
        }
    }
}
dataWrite::~dataWrite()
{
    for (int i = 0; i < chunkNum; i++)
    {
        free(chunklist[i].chunkPtr);
    }

    delete containerCache;
    delete MQ;
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
        cout << " curContainer.chunkNum is" << curContainer.chunkNum << " curContainer.containerID is " << curContainer.containerID << endl;
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
    chunklist.push_back(chunk);
    // cout << "dataWrite entry id is  " << chunklist[chunk.chunkID].chunkID << endl;
    return true;
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

bool dataWrite::Recipe_Insert(Chunk_t &info)
{
    recipelist.push_back(info);
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
                << "ChunkFlag,"
                << "bugFlag,"
                << "dedupFlag,"
                << "cp"
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
    return chunklist[id];
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
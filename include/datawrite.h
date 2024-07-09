/*
 * @Author: Helix0503 834991203@qq.com
 * @Date: 2024-01-08 16:42:12
 * @LastEditors: Helix0503 834991203@qq.com
 * @LastEditTime: 2024-01-30 15:58:16
 * @FilePath: /Maplemeo/LocalDedupSim/include/dataWrite.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef DATA_WRITE_H
#define DATA_WRITE_H
#include "struct.h"
#include "define.h"
#include "messageQueue.h"
#include "readCache.h"
#include <vector>
#include <sstream>
using namespace std;

// static int curContainerIdGlobal = 0;
static int containerNum = 0;
static Container_t curContainer;
static uint64_t curOffset = 0;
class dataWrite
{
private:
    int chunkNum = 0;
    int containerSize = 0;

    uint64_t loadContainerTimes = 0;
    uint64_t cacheHitTimes = 0;

    // unordered_map<string, int> FPindex; //(fp,chunkid)
    //  unordered_map<uint32_t, vector<int>> ObjectIndex;
    // unordered_map<string, vector<int>> ObjectIndex;
    vector<Chunk_t> recipelist;
    // unordered_map<string, vector<int>> *SFindex;
    //  static Container_t curContainer;
    ReadCache *containerCache;
    std::chrono::duration<double> readIOTime;
    std::chrono::duration<double> writeIOTime;
    std::chrono::duration<double> UpdateCacheTime;
    std::chrono::duration<double> readCacheTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime2, endTime2;

public:
    vector<Chunk_t> chunklist;

    MessageQueue<Container_t> *MQ;
    bool Chunk_Insert(Chunk_t chunk);
    int Get_Chunk_Num();
    int Get_Container_Num(Chunk_t chunk);
    Chunk_t Get_Chunk_Info(int id);
    bool FP_Insert(string fp, int chunkid);
    int FP_Find(string fp);
    // int Obj_Find(uint32_t objid);
    // bool Obj_Insert(uint32_t objid, int chunkid);
    int Obj_Find(string objid);
    bool Obj_Insert(string objid, int chunkid);
    bool Recipe_Insert(Chunk_t &info);
    bool SF_Insert(const char *key, size_t keySize, int chunkid);
    bool SF_Insert_Adjacency(const char *key, size_t keySize, int chunkid);
    int SF_Find(const char *key, size_t keySize);
    int SF_Find_random(const char *key, size_t keySize);
    int SF_Find_Debug(const char *key, size_t keySize);
    int SF_Find_Adjacency(const char *key, size_t keySize);
    //_Adjacency
    void Save_to_File(string methodname);
    void Save_to_File_unique(string methodname);
    void Save_to_File_Chunking(string methodname);
    void writeContainers();
    void ProcessLastContainer();
    void PrintBinaryArray(const uint8_t *buffer, size_t buffer_size);
    bool isLz4(int id);
    bool isDuplicate(int id);
    Chunk_t Get_Chunk_MetaInfo(int id);
    void PrintMetrics();
    dataWrite();
    ~dataWrite();
};

#endif
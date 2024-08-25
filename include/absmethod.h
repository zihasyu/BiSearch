#ifndef ABS_METHOD_H
#define ABS_METHOD_H

#include <string>
#include <chrono>
#include "define.h"
#include "chunker.h"
#include "lz4.h"
#include "datawrite.h"

extern "C"
{
#include "./config.h"
#include "./xdelta3.h"
}

using namespace std;

class AbsMethod
{
protected:
public:
    // util
    string filename;
    dataWrite *dataWrite_;
    uint8_t *lz4ChunkBuffer;
    uint8_t *hashBuf;
    uint8_t *deltaMaxChunkBuffer;
    EVP_MD_CTX *mdCtx;
    // statics
    uint64_t totalLogicalSize = 0;
    uint64_t totalCompressedSize = 0;
    uint64_t logicalchunkNum = 0;
    uint64_t uniquechunkNum = 0;
    uint64_t basechunkNum = 0;
    uint64_t deltachunkNum = 0;
    uint64_t bugCount = 0;
    uint64_t finessehit = 0;

    unordered_map<string, int> FPindex; //(fp,chunkid)
    // 消息队列
    MessageQueue<Chunk_t> *recieveQueue;
    // MessageQueue<uint64_t> *MaskRecieveQueue;
    //  MessageQueue<Chunk_t> *outputMQ_; // to datawrite but not used
    unordered_map<string, vector<int>> *SFindex;
    std::chrono::duration<double> getSFTime;
    uint64_t computeSFtimes = 0;
    // total
    std::chrono::duration<double> deltaCompressionTime;
    std::chrono::duration<double> lz4CompressionTime;
    uint64_t logicalchunkSize = 0;
    uint64_t uniquechunkSize = 0;
    uint64_t dedupchunkSize = 0;
    uint64_t basechunkSize = 0;
    uint64_t basechunkOriSize = 0;
    uint64_t deltachunkSize = 0;
    uint64_t deltachunkOriSize = 0;
    uint64_t finessechunkSize = 0;
    uint64_t finessePrechunkSize = 0;
    uint64_t localchunkSize = 0;
    uint64_t localPrechunkSize = 0;

    uint64_t ContainerNum = 0;
    uint64_t ContainerSize = 0;

    uint64_t DedupReduct = 0;
    // DedupReduct+=tmpChunk.chunkSize;
    uint64_t DeltaReduct = 0;
    // DeltaReduct+=tmpChunk.chunkSize-tmpChunk.saveSize;
    uint64_t LocalReduct = 0;
    // LocalReduct+=tmpChunk.chunkSize-tmpChunk.saveSize;
    // time total
    std::chrono::duration<double> sumTime1;
    std::chrono::duration<double> sumTime2;
    std::chrono::duration<double> sumTime3;
    std::chrono::duration<double> sumTime4;
    std::chrono::duration<double> sumTime5;
    std::chrono::duration<double> sumTime6;
    std::chrono::duration<double> sumTime7;
    std::chrono::duration<double> sumTime8;
    std::chrono::duration<double> sumTime9;

    std::chrono::duration<double> fetchBaseChunkTime;

    AbsMethod();
    ~AbsMethod();
    void SetFilename(string name);
    virtual void ProcessTrace() = 0;
    void SetInputMQ(MessageQueue<Chunk_t> *mq) { recieveQueue = mq; }
    // void SetInputMaskMQ(MessageQueue<uint64_t> *mq) { MaskRecieveQueue = mq; }
    //  void SetOutputMQ(MessageQueue<Chunk_t> *outputMQ)
    //  {
    //      outputMQ_ = outputMQ;
    //      return;
    //  }
    static bool compareNat(const std::string &a, const std::string &b);
    void GenerateHash(EVP_MD_CTX *mdCtx, uint8_t *dataBuffer, const int dataSize, uint8_t *hash);
    int FP_Find(string fp);
    bool FP_Insert(string fp, int chunkid);
    void GetSF(unsigned char *ptr, EVP_MD_CTX *mdCtx, uint8_t *SF, int dataSize);
    int SF_Find(const char *key, size_t keySize);
    bool SF_Insert(const char *key, size_t keySize, int chunkid);
    uint8_t *xd3_encode(const uint8_t *targetChunkbuffer, size_t targetChunkbuffer_size, const uint8_t *baseChunkBuffer, size_t baseChunkBuffer_size, size_t *deltaChunkBuffer_size, uint8_t *tmpbuffer);
    void PrintChunkInfo(string inputDirpath, int chunkingMethod, int method, int fileNum, int64_t time);
    void StatsDelta(Chunk_t &tmpChunk);
    void Version_log();
};
#endif
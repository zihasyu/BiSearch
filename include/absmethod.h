#ifndef ABS_METHOD_H
#define ABS_METHOD_H

#include <string>
#include <chrono>
#include "define.h"
#include "chunker.h"
#include "lz4.h"

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
    uint8_t *readFileBuffer;
    uint8_t *lz4ChunkBuffer;
    uint8_t *hashBuf;
    EVP_MD_CTX *mdCtx;
    // statics
    uint64_t totalLogicalSize = 0;
    uint64_t totalCompressedSize = 0;
    uint64_t ChunkNum = 0;
    uint64_t clusterNum = 0;

    // 消息队列
    MessageQueue<Chunk_t> *recieveQueue;

    AbsMethod();
    ~AbsMethod();
    virtual void ProcessOneTrace() = 0;
    void SetInputMQ(MessageQueue<Chunk_t> *mq) { recieveQueue = mq; }
    static bool compareNat(const std::string &a, const std::string &b);
};
#endif
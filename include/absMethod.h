#ifndef ABS_METHOD_H
#define ABS_METHOD_H

#include <string>
#include <chrono>
#include "define.h"
#include "chunker.h"
#include "lz4.h"

using namespace std;

class absMethod
{
protected:
public:
    // util
    uint8_t *readFileBuffer;
    uint8_t *lz4ChunkBuffer;

    // statics
    uint64_t totalLogicalSize = 0;
    uint64_t totalCompressedSize = 0;
    uint64_t ChunkNum = 0;
    uint64_t clusterNum = 0;

    // 消息队列
    MessageQueue<Chunk_t> *recieveQueue;

    absMethod();
    ~absMethod();
    virtual void ProcessOneTrace() = 0;
    void SetInputMQ(MessageQueue<Chunk_t> *mq) { recieveQueue = mq; }
};
#endif
#ifndef CHUNKER_H
#define CHUNKER_H

#include "define.h"
#include "struct.h"
#include "messageQueue.h"

using namespace std;

enum ChunkTypeNum
{
    FIXED_SIZE_CHUNKING = 0,
    FASTCDC_CHUNKING = 1,
};

class Chunker
{
private:
    /* data */
    string myName_ = "Chunker";

    int chunkType;

    // chunk size settings for FastCDC
    uint64_t avgChunkSize_;
    uint64_t minChunkSize_;
    uint64_t maxChunkSize_;

    // fixed Size Chunking
    uint64_t FixedChunkSize;

    // IO stream
    ifstream chunkingFile_;

    // buffer
    uint8_t *readFileBuffer;
    uint8_t *chunkBuffer;

    // Chunker ID
    uint64_t chunkID = 0;

    // messageQueue
    MessageQueue<Chunk_t> *outputMQ_;

public:
    Chunker(int chunkType_);
    ~Chunker();
    // util method
    void LoadChunkFile(string path);
    void ChunkerInit();
    void Chunking();

    void SetOutputMQ(MessageQueue<Chunk_t> *outputMQ)
    {
        outputMQ_ = outputMQ;
        return;
    }
    // Chunking Methods
    void FixSizedChunking();

    void FastCDCChunking();
};
#endif
#include "../../include/chunker.h"

Chunker::Chunker(int chunkType_)
{

    FixedChunkSize = 8192;
    // specifiy chunk type
    chunkType = chunkType_;
    // init chunker
    ChunkerInit();
}

Chunker::~Chunker()
{
    free(readFileBuffer);
    free(chunkBuffer);
}

void Chunker::LoadChunkFile(string path)
{
    if (chunkingFile_.is_open())
    {
        chunkingFile_.close();
    }

    chunkingFile_.open(path, ios_base::in | ios::binary);
    if (!chunkingFile_.is_open())
    {
        tool::Logging(myName_.c_str(), "open file: %s error.\n",
                      path.c_str());
        exit(EXIT_FAILURE);
    }
    return;
}

void Chunker::Chunking()
{
    switch (chunkType)
    {
    case FIXED_SIZE_CHUNKING:
    {
        FixSizedChunking();
        break;
    }
    case FASTCDC_CHUNKING:
    {
        // FastCDC();
        // break;
    }
    default:
    {
        tool::Logging(myName_.c_str(), "chunking type error.\n");
        exit(EXIT_FAILURE);
    }
    }
    tool::Logging(myName_.c_str(), "thread exit.\n");
    return;
}

void Chunker::ChunkerInit()
{
    switch (chunkType)
    {
    case 0:
        // fixed size chunking]
        readFileBuffer = (uint8_t *)malloc(READ_FILE_SIZE);
        chunkBuffer = (uint8_t *)malloc(FixedChunkSize);
        break;
    case 1:
        // FastCDC chunking
        break;
    default:
        tool::Logging(myName_.c_str(), "chunk type error.\n");
        exit(EXIT_FAILURE);
        break;
    }
    return;
}

void Chunker::FixSizedChunking()
{
    uint64_t fileSize = 0;
    bool end = false;

    while (!end)
    {
        // read file
        memset((char *)readFileBuffer, 0, sizeof(uint8_t) * READ_FILE_SIZE);
        chunkingFile_.read((char *)readFileBuffer, READ_FILE_SIZE);
        //
        end = chunkingFile_.eof();
        size_t len = chunkingFile_.gcount();
        size_t chunkedSize = 0;
        if (len == 0)
        {
            break;
        }
        fileSize += len;

        size_t remainSize = len;
        while (chunkedSize < len)
        {
            Chunk_t chunk;

            if (remainSize > FixedChunkSize)
            {
                memcpy(chunk.chunkContent, readFileBuffer + chunkedSize, FixedChunkSize);
                chunk.chunkSize = FixedChunkSize;

                chunkedSize += FixedChunkSize;
                remainSize -= FixedChunkSize;
            }
            else
            {
                memcpy(chunk.chunkContent, readFileBuffer + chunkedSize, remainSize);
                chunk.chunkSize = remainSize;

                chunkedSize += remainSize;
                remainSize = 0;
            }

            chunk.chunkID = chunkID++;

            if (!outputMQ_->Push(chunk))
            {
                tool::Logging(myName_.c_str(), "insert chunk to output MQ error.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    outputMQ_->done_ = true;
    tool::Logging(myName_.c_str(), "chunking done.\n");

    return;
}
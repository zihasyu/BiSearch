#include "../../include/chunker.h"

Chunker::Chunker(int chunkType_)
{

    FixedChunkSize = 8192;
    // specifiy chunk type
    chunkType = chunkType_;
    // init chunker
    ChunkerInit();
    // in different chunking method, the chunkBuffer is different
}
Chunker::~Chunker()
{
    free(readFileBuffer);
    free(chunkBuffer);
}

void Chunker::LoadChunkFile(string path)
{
    if (inputFile.is_open())
    {
        inputFile.close();
    }

    inputFile.open(path, ios_base::in | ios::binary);
    if (!inputFile.is_open())
    {
        tool::Logging(myName_.c_str(), "open file: %s error.\n",
                      path.c_str());
        exit(EXIT_FAILURE);
    }
    return;
}

void Chunker::ChunkerInit()
{
    switch (chunkType)
    {
    case FIXED_SIZE:
    {
        // fixed size chunking]
        readFileBuffer = (uint8_t *)malloc(READ_FILE_SIZE);
        chunkBuffer = (uint8_t *)malloc(FixedChunkSize);
        break;
    }
    case FASTCDC: // FastCDC chunking
    {
        readFileBuffer = (uint8_t *)malloc(READ_FILE_SIZE);
        chunkBuffer = (uint8_t *)malloc(MAX_CHUNK_SIZE);
        normalSize = CalNormalSize(minChunkSize, avgChunkSize, maxChunkSize);
        bits = (uint32_t)round(log2(static_cast<double>(avgChunkSize)));
        maskS = GenerateFastCDCMask(bits + 1);
        maskL = GenerateFastCDCMask(bits - 1);
        break;
    }
    case GEARCDC: // Gear chunking
    {
        readFileBuffer = (uint8_t *)malloc(READ_FILE_SIZE);
        chunkBuffer = (uint8_t *)malloc(MAX_CHUNK_SIZE);
        break;
    }
    case TAR:
    {
        readFileBuffer = (uint8_t *)malloc(READ_FILE_SIZE);
        chunkBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE); // 4MB
        normalSize = CalNormalSize(minChunkSize, avgChunkSize, maxChunkSize);
        bits = (uint32_t)round(log2(static_cast<double>(avgChunkSize)));
        maskS = GenerateFastCDCMask(bits + 1);
        maskL = GenerateFastCDCMask(bits - 1);
        break;
    }
    case TAR_SEGMENT:
    {
        headerBuffer = (uint8_t *)malloc(512 * 32);
        dataBuffer = (uint8_t *)malloc(CONTAINER_MAX_SIZE * 16); // 64MB
        normalSize = CalNormalSize(minChunkSize, avgChunkSize, maxChunkSize);
        bits = (uint32_t)round(log2(static_cast<double>(avgChunkSize)));
        maskS = GenerateFastCDCMask(bits + 1);
        maskL = GenerateFastCDCMask(bits - 1);
        break;
    }

    default:
        tool::Logging(myName_.c_str(), "chunk type error.\n");
        exit(EXIT_FAILURE);
        break;
    }
    return;
}

void Chunker::Chunking()
{
    bool end = false;
    uint32_t totalOffset = 0;
    while (!end)
    {
        memset((char *)readFileBuffer, 0, sizeof(uint8_t) * READ_FILE_SIZE);
        inputFile.read((char *)readFileBuffer, sizeof(uint8_t) * READ_FILE_SIZE);
        end = inputFile.eof();
        size_t len = inputFile.gcount();
        if (len == 0)
        {
            break;
        }
        size_t localOffset = 0;
        while (((len - localOffset) >= CONTAINER_MAX_SIZE) || (end && (localOffset < len)))
        {
            cout << " len is " << len << " localOffset is " << localOffset << endl;
            Chunk_t chunk;
            // compute cutPoint
            uint32_t cp = 0;
            switch (chunkType)
            {
            case FIXED_SIZE:
            {
                cp = avgChunkSize; // 8KB
                break;
            }
            case FASTCDC:
            {
                cp = CutPointFastCDC(readFileBuffer + localOffset, len - localOffset);
                break;
            }
            case GEARCDC:
            {
                cp = CutPointGear(readFileBuffer + localOffset, len - localOffset);
                break;
            }
            case TAR:
            {
                cp = CutPointTarFast(readFileBuffer + localOffset, len - localOffset);
                break;
            }
            case TAR_SEGMENT:
            {
                // 一次需要两个块
            }
            }
            chunk.chunkPtr = (uint8_t *)malloc(cp);
            memcpy(chunk.chunkPtr, readFileBuffer + localOffset, cp);
            chunk.chunkSize = cp;
            // chunk.chunkID = chunkID++;太早了
            if (cp == 0)
            {
                cout << "cp is 0" << endl; // debug
                continue;
            }
            localOffset += cp;
            if (!outputMQ_->Push(chunk))
            {
                tool::Logging(myName_.c_str(), "insert chunk to output MQ error.\n");
                exit(EXIT_FAILURE);
            }
        }
        totalOffset += localOffset;
        inputFile.seekg(totalOffset, ios_base::beg);
    }
    cout << "chunking done." << endl;
    outputMQ_->done_ = true;
    tool::Logging(myName_.c_str(), "chunking done.\n");
    return;
}

uint32_t Chunker::CutPointFastCDC(const uint8_t *src, const uint32_t len)
{
    uint32_t n;
    uint32_t fp = 0;
    uint32_t i;
    i = min(len, static_cast<uint32_t>(minChunkSize));
    n = min(normalSize, len);
    for (; i < n; i++)
    {
        fp = (fp >> 1) + GEAR[src[i]];
        if (!(fp & maskS))
        {
            return (i + 1);
        }
    }

    n = min(static_cast<uint32_t>(maxChunkSize), len);
    for (; i < n; i++)
    {
        fp = (fp >> 1) + GEAR[src[i]];
        if (!(fp & maskL))
        {
            return (i + 1);
        }
    }
    return i;
};
uint32_t Chunker::CutPointGear(const uint8_t *src, const uint32_t len)
{
    uint32_t fp = 0;
    uint32_t i = 0;
    for (; i < len; i++)
    {
        fp = (fp >> 1) + GEAR[src[i]];
        if (!(fp & MASK_GEAR))
        {
            return (i + 1);
        }
    }
    return i;
};
uint32_t Chunker::CutPointTarFast(const uint8_t *src, const uint32_t len)
{
    switch (Next_Chunk_Type)
    {
    case FILE_HEADER:
    {
        uint8_t data[12];
        std::memcpy(data, src + 124, 12);

        Next_Chunk_Size = 0;
        for (int i = 0; i < 11; i++)
        {
            Next_Chunk_Size = Next_Chunk_Size * 8 + data[i] - 48;
        }
        if (*(src + 156) == REGTYPE)
        {
            if (Next_Chunk_Size <= CONTAINER_MAX_SIZE)
                Next_Chunk_Type = FILE_CHUNK;
            else
            {
                Next_Chunk_Type = BIG_CHUNK;
                Big_Chunk_Size = (Next_Chunk_Size + 511) / 512 * 512;
                Big_Chunk_Offset = 0;
                // Big_Chunk_Allowance = Next_Chunk_Size / CONTAINER_MAX_SIZE;
                // Big_Chunk_Last_Size = Next_Chunk_Size % CONTAINER_MAX_SIZE;
                // if (Big_Chunk_Last_Size == 0)
                // {
                //     Big_Chunk_Allowance--;
                //     Big_Chunk_Last_Size = CONTAINER_MAX_SIZE;
                // }
            }
        }
        if (*(src + 156) == AREGTYPE)
        {
            Next_Chunk_Size = 0;
            for (int i = 0; i < 11; i++)
            {
                Next_Chunk_Size = Next_Chunk_Size * 8 + data[i];
            }
            if (Next_Chunk_Size <= CONTAINER_MAX_SIZE)
                Next_Chunk_Type = FILE_CHUNK;
            else
            {
                Next_Chunk_Type = BIG_CHUNK;
                Big_Chunk_Size = (Next_Chunk_Size + 511) / 512 * 512;
                Big_Chunk_Offset = 0;
                // Big_Chunk_Allowance = Next_Chunk_Size / CONTAINER_MAX_SIZE;
                // Big_Chunk_Last_Size = Next_Chunk_Size % CONTAINER_MAX_SIZE;
                // if (Big_Chunk_Last_Size == 0)
                // {
                //     Big_Chunk_Allowance--;
                //     Big_Chunk_Last_Size = CONTAINER_MAX_SIZE;
                // }
            }
        }
        if (*(src + 156) == 'x' || *(src + 156) == GNUTYPE_LONGNAME)
            Next_Chunk_Type = FILE_CHUNK;
        /*use to debug*/
        // cout<<"Next_Chunk_Flag: " <<int(*(src + 156));
        // cout<<"Next_Chunk_Type: " <<Next_Chunk_Type;
        // cout<<"Next_Chunk_Size: "<<Next_Chunk_Size<<endl;
        // if(int(*(src + 156)) == 32||int(*(src + 156)) == 0){
        //     for(int i=0;i<512;i++)
        //     cout<<src[i]<<" ";
        //     cout<<endl;
        // }

        if (len >= 512)
            return 512;
        else
        {
            // printf("emmmm");
            return len;
        }
        break;
    }
    case FILE_CHUNK:
    {
        uint32_t roundedUp = (Next_Chunk_Size + 511) / 512 * 512;
        Next_Chunk_Type = FILE_HEADER;
        Next_Chunk_Size = 512;
        if (roundedUp < len)
            return roundedUp;
        else
            return len;
        break;
    }
    case BIG_CHUNK:
    {
        if (Big_Chunk_Size - Big_Chunk_Offset > maxChunkSize)
        {
            // Big_Chunk_Allowance--;
            uint32_t cp = CutPointFastCDC(src,
                                          Big_Chunk_Size - Big_Chunk_Offset);
            Big_Chunk_Offset += cp;
            // cout << "offset is " << Big_Chunk_Offset << " cp is " << cp << endl;
            return cp;
            // return CONTAINER_MAX_SIZE;
        }
        else
        {
            Next_Chunk_Type = FILE_HEADER;
            return Big_Chunk_Size - Big_Chunk_Offset;
        }
        break;
    }
    }
};
uint32_t Chunker::GenerateFastCDCMask(uint32_t bits)
{
    uint32_t tmp;
    tmp = (1 << CompareLimit(bits, 1, 31)) - 1;
    return tmp;
}
inline uint32_t Chunker::CompareLimit(uint32_t input, uint32_t lower, uint32_t upper)
{
    if (input <= lower)
    {
        return lower;
    }
    else if (input >= upper)
    {
        return upper;
    }
    else
    {
        return input;
    }
}
uint32_t Chunker::CalNormalSize(const uint32_t min, const uint32_t av, const uint32_t max)
{
    uint32_t off = min + DivCeil(min, 2);
    if (off > av)
    {
        off = av;
    }
    uint32_t diff = av - off;
    if (diff > max)
    {
        return max;
    }
    return diff;
}
inline uint32_t Chunker::DivCeil(uint32_t a, uint32_t b)
{
    uint32_t tmp = a / b;
    if (a % b == 0)
    {
        return tmp;
    }
    else
    {
        return (tmp + 1);
    }
}

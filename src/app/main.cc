#include <iostream>
#include <string>
#include <csignal>
#include <sstream>
#include <chrono>

#include "../../include/allmethod.h"

using namespace std;

void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n";
    exit(signum);
}

int main(int argc, char **argv)
{
    signal(SIGINT, signalHandler);

    uint32_t chunkingType;
    uint32_t compressionMethod;
    uint32_t backupNum;

    string dirName;
    string myName = "BiSearchSystem";

    vector<string> readfileList;

    const char optString[] = "i:m:c:n:";
    if (argc < sizeof(optString))
    {
        cout << "Usage: " << argv[0] << " -i <input file> -m <chunking method> -c <compression method> -n <process number>" << endl;
        return 0;
    }

    // Grab command-line instructions
    int option = 0;
    while ((option = getopt(argc, argv, optString)) != -1)
    {
        switch (option)
        {
        case 'i':
            dirName.assign(optarg);
            break;
        case 'c':
            chunkingType = atoi(optarg);
            break;
        case 'm':
            compressionMethod = atoi(optarg);
            break;
        case 'n':
            backupNum = atoi(optarg);
            break;
        default:
            break;
        }
    }

    AbsMethod *absMethodObj;
    Chunker *chunkerObj = new Chunker(chunkingType);

    MessageQueue<Chunk_t> *chunkerMQ = new MessageQueue<Chunk_t>(CHUNK_QUEUE_SIZE);

    switch (compressionMethod)
    {
    case DEDUP:
    {
        absMethodObj = new Dedup();
        break;
    }
    case NTRANSFORM:
    {
        absMethodObj = new NTransForm();
        break;
    }
    case FINESSE:
    {
        absMethodObj = new Finesse();
        break;
    }
    case ODESS:
    {
        absMethodObj = new Odess();
        break;
    }
    case PALANTIR:
    {
        absMethodObj = new Palantir();
        break;
    }
    case BiSEARCH:
    {
        absMethodObj = new BiSearch(8.0);
        break;
    }
    case LOCALITY:
    {
        absMethodObj = new LocalDedup();
        break;
    }
    default:
        break;
    }

    tool::traverse_dir(dirName, readfileList, nofilter);
    sort(readfileList.begin(), readfileList.end(), AbsMethod::compareNat);

    boost::thread *thTmp[2] = {nullptr};
    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    chunkerObj->SetOutputMQ(chunkerMQ);
    absMethodObj->SetInputMQ(chunkerMQ);
    absMethodObj->dataWrite_ = new dataWrite();

    // new design
    // if (chunkingType == TAR_MultiHeader)
    // {
    //     MessageQueue<uint64_t> *MaskMQ = new MessageQueue<uint64_t>(CHUNK_QUEUE_SIZE);
    //     chunkerObj->SetOutputMaskMQ(MaskMQ);
    //     absMethodObj->SetInputMaskMQ(MaskMQ);
    // }

    if (chunkingType == MTAR)
    {

        chunkerObj->MTar(readfileList, backupNum);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (auto i = 0; i < backupNum; i++)
    {
        // set backup name
        chunkerObj->LoadChunkFile(readfileList[i]);
        absMethodObj->SetFilename(readfileList[i]);
        absMethodObj->dataWrite_->SetFilename(readfileList[i]);
        // thread running
        thTmp[0] = new boost::thread(attrs, boost::bind(&Chunker::Chunking, chunkerObj));
        thTmp[1] = new boost::thread(attrs, boost::bind(&AbsMethod::ProcessTrace, absMethodObj));
        for (auto it : thTmp)
        {
            it->join();
        }
        for (auto it : thTmp)
        {
            delete it;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto sumTime = (end - start);
    auto sumTimeInSeconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    std::cout << "Time taken by for loop: " << sumTimeInSeconds << " s " << std::endl;
    tool::Logging(myName.c_str(), "logical Chunk Num is %d\n", absMethodObj->logicalchunkNum);
    tool::Logging(myName.c_str(), "unique Chunk Num is %d\n", absMethodObj->uniquechunkNum);
    tool::Logging(myName.c_str(), "Total logical size is %lu\n", absMethodObj->logicalchunkSize);
    tool::Logging(myName.c_str(), "Total compressed size is %lu\n", absMethodObj->uniquechunkSize);
    tool::Logging(myName.c_str(), "Compression ratio is %.4f\n", (double)absMethodObj->logicalchunkSize / (double)absMethodObj->uniquechunkSize);
    absMethodObj->PrintChunkInfo(dirName, chunkingType, compressionMethod, backupNum, sumTimeInSeconds);

    //  restore backup if you need, but it's not necessary
    // if (chunkingType != TAR_MultiHeader)
    //     for (auto i = 0; i < backupNum; i++)
    //     {
    //         absMethodObj->dataWrite_->SetFilename(readfileList[i]);
    //         absMethodObj->dataWrite_->restoreFile(readfileList[i]);
    //     }
    // else
    //     for (auto i = 0; i < backupNum; i++)
    //     {
    //         absMethodObj->dataWrite_->SetFilename(readfileList[i]);
    //         absMethodObj->dataWrite_->restoreHeaderFile(readfileList[i]);
    //     }
    string fileName = "C" + to_string(chunkingType) + "_M" + to_string(compressionMethod);
    absMethodObj->dataWrite_->Save_to_File_unique(fileName);
    delete absMethodObj->dataWrite_;
    delete chunkerObj;
    delete absMethodObj;
    return 0;
}
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
        cout << "Chunking Methods: " << "0 for fixed size chunking, 1 for FastCDC chunking" << endl;
        cout << "Compression Methods: " << "0 for lz4, 1 lz4-cluster-basline" << endl;
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
    MessageQueue<Chunk_t> *chunkReWriteMQ = new MessageQueue<Chunk_t>(CHUNK_QUEUE_SIZE);

    chunkerObj->SetOutputMQ(chunkerMQ);

    switch (compressionMethod)
    {
    case DEDUP:
    {
        absMethodObj = new Dedup();
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
        // absMethodObj = new lz4ClusterBaseline(fileName);
        break;
    }
    case BiSEARCH:
    {
        absMethodObj = new BiSearch(8.0);
        break;
    }
    default:
        break;
    }

    tool::traverse_dir(dirName, readfileList, nofilter);
    sort(readfileList.begin(), readfileList.end(), AbsMethod::compareNat);

    boost::thread *thTmp[3] = {nullptr};
    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);

    absMethodObj->SetInputMQ(chunkerMQ);
    absMethodObj->SetOutputMQ(chunkReWriteMQ);

    absMethodObj->dataWrite_ = new dataWrite();
    absMethodObj->dataWrite_->SetInputMQ(chunkReWriteMQ);
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
        thTmp[2] = new boost::thread(attrs, boost::bind(&dataWrite::writing, absMethodObj->dataWrite_));
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
    // restore backup if you need, but it's not necessary
    // for (auto i = 0; i < backupNum; i++)
    // {
    //     absMethodObj->dataWrite_->SetFilename(readfileList[i]);
    //     absMethodObj->dataWrite_->restoreFile(readfileList[i]);
    // }
    delete absMethodObj->dataWrite_;
    delete chunkerObj;
    delete absMethodObj;
    return 0;
}
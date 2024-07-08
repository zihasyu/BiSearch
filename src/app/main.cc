#include <iostream>
#include <string>
#include <csignal>
#include <sstream>

#include "../../include/absMethod.h"

using namespace std;

void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n";
    exit(signum);
}

bool compareNat(const std::string &a, const std::string &b)
{
    if (a.empty())
        return true;
    if (b.empty())
        return false;
    if (std::isdigit(a[0]) && !std::isdigit(b[0]))
        return true;
    if (!std::isdigit(a[0]) && std::isdigit(b[0]))
        return false;
    if (!std::isdigit(a[0]) && !std::isdigit(b[0]))
    {
        if (std::toupper(a[0]) == std::toupper(b[0]))
            return compareNat(a.substr(1), b.substr(1));
        return (std::toupper(a[0]) < std::toupper(b[0]));
    }

    // Both strings begin with digit --> parse both numbers
    std::istringstream issa(a);
    std::istringstream issb(b);
    int ia, ib;
    issa >> ia;
    issb >> ib;
    if (ia != ib)
        return ia < ib;

    // Numbers are the same --> remove numbers and recurse
    std::string anew, bnew;
    std::getline(issa, anew);
    std::getline(issb, bnew);
    return (compareNat(anew, bnew));
}

int main(int argc, char **argv)
{
    signal(SIGINT, signalHandler);

    uint32_t chunkingType;
    uint32_t compressionMethod;
    uint32_t processNum;

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
            processNum = atoi(optarg);
            break;
        default:
            break;
        }
    }

    absMethod *absMethodObj;
    Chunker *chunkerObj = new Chunker(chunkingType);

    MessageQueue<Chunk_t> *chunker2lz4 = new MessageQueue<Chunk_t>(CHUNK_QUEUE_SIZE);
    chunkerObj->SetOutputMQ(chunker2lz4);

    switch (compressionMethod)
    {
    case 0:
        // lz4 comapre
        // absMethodObj = new lz4Compare();
        break;
    case 1:
        // absMethodObj = new lz4Baseline();
        break;
    case 2:
        // absMethodObj = new lz4ClusterBaseline(fileName);
        break;
    default:
        break;
    }

    tool::traverse_dir(dirName, readfileList, nofilter);
    sort(readfileList.begin(), readfileList.end(), compareNat);

    boost::thread *thTmp;
    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);

    absMethodObj->SetInputMQ(chunker2lz4);
    for (auto i = 0; i < processNum; i++)
    {

        chunkerObj->LoadChunkFile(readfileList[i]);
        thTmp = new boost::thread(attrs, boost::bind(&Chunker::Chunking, chunkerObj));

        absMethodObj->ProcessOneTrace();

        thTmp->join();
        delete thTmp;
    }

    tool::Logging(myName.c_str(), "Chunk Num is %d\n", absMethodObj->ChunkNum);
    tool::Logging(myName.c_str(), "Cluster Num is %d\n", absMethodObj->clusterNum);
    tool::Logging(myName.c_str(), "Total logical size is %lu\n", absMethodObj->totalLogicalSize);
    tool::Logging(myName.c_str(), "Total compressed size is %lu\n", absMethodObj->totalCompressedSize);
    tool::Logging(myName.c_str(), "Compression ratio is %.4f\n", (double)absMethodObj->totalLogicalSize / (double)absMethodObj->totalCompressedSize);

    delete chunkerObj;
    delete absMethodObj;
    return 0;
}
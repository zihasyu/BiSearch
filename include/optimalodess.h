#ifndef OPTIMAL_ODESS_H
#define OPTIMAL_ODESS_H

#include "absmethod.h"
#include "odess_similarity_detection.h"

using namespace std;

class OptimalOdess : public AbsMethod
{
private:
    string myName_ = "OptimalOdess";
    int PrevDedupChunkid = -1;
    int Version = 0;
    FeatureIndexTable table;

public:
    OptimalOdess();
    ~OptimalOdess();
    void ProcessTrace();
    std::vector<uint64_t> matrixS();
    std::vector<std::vector<std::pair<uint64_t, uint64_t>>> matrixD();
    void ILP();
};
#endif
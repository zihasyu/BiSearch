#ifndef OPTIMAL_ODESS_H
#define OPTIMAL_ODESS_H

#include "absmethod.h"
#include "odess_similarity_detection.h"
#include "../../../gurobi1102/linux64/include/gurobi_c++.h"
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
    std::vector<std::unordered_map<uint64_t, uint64_t>> matrixD();
    void ILP();
};
#endif
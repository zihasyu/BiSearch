#ifndef PALANTIR_H
#define PALANTIR_H

#include "absmethod.h"
#include "odess_similarity_detection.h"

typedef uint64_t feature_t;
typedef unsigned long long super_feature_t;
typedef vector<super_feature_t> SuperFeatures;

using namespace std;

typedef struct
{
    uint64_t chunkid;
    int version;
} chunkid_version;

class Palantir : public AbsMethod
{
private:
    string myName_ = "Palantir";
    int PrevDedupChunkid = -1;
    FeatureIndexTable table;

public:
    Palantir();
    ~Palantir();
    void ProcessTrace();
    uint64_t DedupReductSize = 0;
    uint64_t DeltaReductSize = 0;
    uint64_t LocalReductSize = 0;

    int Version = 0;
    double LZ4Ratio = 2;

    unordered_map<super_feature_t, vector<uint64_t>> SFindex1;
    unordered_map<super_feature_t, vector<chunkid_version>> SFindex2;
    unordered_map<super_feature_t, vector<chunkid_version>> SFindex3;
    uint64_t SF_Find(const SuperFeatures &superfeatures);
    void SF_Insert(const SuperFeatures &superfeatures, const uint64_t chunkid);
    void CleanIndex();
};
#endif
#ifndef LZ4_BASELINE_H
#define LZ4_BASELINE_H

#include "absMethod.h"
#include "odess_similarity_detection.h"

class lz4Baseline : public absMethod
{
private:
    /* data */
    string myName_ = "lz4Baseline";
    string fileName;
    // Feature Table
    FeatureIndexTable table;
    // cluster
    uint8_t *clusterBuffer;
    int clusterCnt = 0;
    uint64_t clusterSize = 0;
    uint64_t singeFeature = 0;

    // chunk set
    vector<Chunk_t> chunkSet;

    // time static
    std::chrono::duration<double> featureExtractTime;
    std::chrono::duration<double> clustringTime;

public:
    lz4Baseline();
    ~lz4Baseline();

    void ProcessOneTrace();

    void SetInputMQ(MessageQueue<Chunk_t> *mq);

    void setFileName(string &fileName_) { fileName = fileName_; }
};
#endif
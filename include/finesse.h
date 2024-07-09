#ifndef FINESSE_METHOD_H
#define FINESSE_METHOD_H

#include "absmethod.h"

using namespace std;

class Finesse : public AbsMethod
{
private:
    string myName_ = "Finesse";
    uint64_t bugCount = 0;

public:
    Finesse();
    ~Finesse();
    bool ProcessTrace(string inputFileName);
};
#endif
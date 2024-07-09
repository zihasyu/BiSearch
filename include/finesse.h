#ifndef FINESSE_METHOD_H
#define FINESSE_METHOD_H

#include "absmethod.h"

using namespace std;

class Finesse : public AbsMethod
{
private:
    string myName_ = "Finesse";

public:
    Finesse();
    ~Finesse();
    void ProcessTrace();
};
#endif
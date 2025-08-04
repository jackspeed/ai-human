#pragma once
#include "dh_data.h"
#include "wenetai.h"
#include <mutex> 

class DhWenet{
    public:
        static int calcmfcc(jmat_t* mwav,jmat_t* mmel);
        static int calcmfcc(float* fwav,float* mel2);
        static int calcmfcc(float* fwav,int fsample,float* mel2,int melcnt);
        static int cntmel(int pcmblock);
        static int cntbnf(int melblock);

};

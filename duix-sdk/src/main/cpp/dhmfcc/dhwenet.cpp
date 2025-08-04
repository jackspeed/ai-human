#include "dhwenet.h"
#include <stdio.h>
#include <vector>
#include <string>
#include "aicommon.h"
#include "mfcc/mfcc.hpp"


int DhWenet::cntmel(int pcmblock){
  int allcnt = pcmblock + 2*STREAM_MFCC_FILL;
  int pcm_allsamp = allcnt*STREAM_BASE_SAMP;
  int mel_allcnt = pcm_allsamp/160+1;
  return mel_allcnt;
}

int DhWenet::cntbnf(int melblock){
  int bnf_allcnt = melblock*0.25f - 0.75f;
  return bnf_allcnt;
}

int DhWenet::calcmfcc(float* fwav,float* mel2){
    int rst = 0;
    int melcnt = MFCC_WAVCHUNK/160+1;
    rst = log_mel(fwav,MFCC_WAVCHUNK, 16000,mel2);
    return rst;
}

int DhWenet::calcmfcc(float* fwav,int fsample,float* mel2,int melcnt){
    int rst = 0;
    rst = log_mel(fwav,fsample, 16000,mel2);
    return rst;
}

int DhWenet::calcmfcc(jmat_t* mwav,jmat_t* mmel){
    int rst = 0;
    int melcnt = MFCC_WAVCHUNK/160+1;
    for(size_t k=0;k<mwav->height;k++){
        float* fwav = (float*)jmat_row(mwav,k);
        float* mel2 = (float*)jmat_row(mmel,k);
        rst = log_mel(fwav,MFCC_WAVCHUNK, 16000,mel2);
    }
    return rst;
}


#ifndef GJDUIX_
#define GJDUIX_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dhmfcc_s dhmfcc_t;

int dhmfcc_alloc(dhmfcc_t** pdg,int mincalc);
int dhmfcc_initPcmex(dhmfcc_t* dg,int maxsize,int minoff ,int minblock ,int maxblock);
int dhmfcc_initWenet(dhmfcc_t* dg,char* fnwenet); 

uint64_t dhmfcc_newsession(dhmfcc_t* dg);
int dhmfcc_pushpcm(dhmfcc_t* dg,uint64_t sessid,char* buf,int size,int kind);
int dhmfcc_readpcm(dhmfcc_t* dg,uint64_t sessid,char* pcmbuf,int pcmlen,char* bnfbuf,int bnflen);
int dhmfcc_finsession(dhmfcc_t* dg,uint64_t sessid);
int dhmfcc_consession(dhmfcc_t* dg,uint64_t sessid);

int dhmfcc_free(dhmfcc_t* dg);


typedef struct dhunet_s dhunet_t;;
int dhunet_alloc(dhunet_t** pdg,int minrender);
int dhunet_initMunet(dhunet_t* dg,char* fnparam,char* fnbin,char* fnmsk);
int dhunet_simprst(dhunet_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,int* box,uint8_t* bmsk,uint8_t* bfg,uint8_t* bnfbuf,int bnflen);
int dhunet_free(dhunet_t* pdg);




#ifdef __cplusplus
}
#endif

#endif

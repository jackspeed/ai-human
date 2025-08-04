#ifndef GJSIMP
#define GJSIMP

#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif


typedef struct dhduix_s dhduix_t;

int dhduix_alloc(dhduix_t** pdg,int mincalc,int width,int height);
int dhduix_initPcmex(dhduix_t* dg,int maxsize,int minoff ,int minblock ,int maxblock,int rgb);
int dhduix_initWenet(dhduix_t* dg,char* fnwenet); 
int dhduix_initMunet(dhduix_t* dg,char* fnparam,char* fnbin,char* fnmsk);

uint64_t dhduix_newsession(dhduix_t* dg);

int dhduix_pushpcm(dhduix_t* dg,uint64_t sessid,char* buf,int size,int kind);
int dhduix_readpcm(dhduix_t* dg,uint64_t sessid,char* pcmbuf,int pcmlen,char* bnfbuf,int bnflen);
int dhduix_simprst(dhduix_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,int* box,uint8_t* bmsk,uint8_t* bfg,uint8_t* bnfbuf,int bnflen);

int dhduix_allcnt(dhduix_t* dg,uint64_t sessid);
int dhduix_readycnt(dhduix_t* dg,uint64_t sessid);
int dhduix_simpinx(dhduix_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,int* box,uint8_t* bmsk,uint8_t* bfg,int bnfinx);
int dhduix_fileinx(dhduix_t* dg,uint64_t sessid,char* fnpic,int* box,char* fnmsk,char* fnfg,int bnfinx,char* bimg,char* mskbuf,int imgsize);
int dhduix_simpblend(dhduix_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,uint8_t* bmsk,uint8_t* bfg);

int dhduix_simppcm(dhduix_t* dg,char* buf,int size,char* pre,int presize,char* bnf,int bnfsize);


int dhduix_finsession(dhduix_t* dg,uint64_t sessid);
int dhduix_consession(dhduix_t* dg,uint64_t sessid);



int dhduix_free(dhduix_t* dg);








#ifdef __cplusplus
}
#endif


#endif

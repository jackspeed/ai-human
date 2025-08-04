#include <stdlib.h>
#include <pthread.h>
#include "gjduix.h"
#include "dhwenet.h"
#include "wenetai.h"
#include "dhpcm.h"
#include "munet.h"
#include "malpha.h"
#include "dhwenet.h"

struct dhmfcc_s{
  int mincalc;
  int minoff;  
  int minblock;  
  int maxblock;  
  int inited;
  char* wenetfn;

  //DhWenet* wenet;
  WeAI*   weai_first;
  WeAI*   weai_common;
  PcmSession* cursess;
  PcmSession* presess;
  volatile uint64_t  sessid;

  volatile int running;
  pthread_t *calcthread;
  pthread_mutex_t pushmutex;
  pthread_mutex_t readmutex;
};


static void *calcworker(void *arg){
  dhmfcc_t* mfcc = (dhmfcc_t*)arg;
  uint64_t sessid = 0;
  while(mfcc->running){
    int rst = 0;
    PcmSession* sess = mfcc->cursess;
    if(sess &&(sess->sessid()==mfcc->sessid)){
      rst = sess->runcalc(mfcc->sessid,mfcc->weai_common,mfcc->mincalc);
    }
    if(rst!=1){
      jtimer_mssleep(20);
    }else{
      jtimer_mssleep(10);
    }
  }
  return NULL;
}

int dhmfcc_alloc(dhmfcc_t** pdg,int mincalc){
  dhmfcc_t* mfcc = (dhmfcc_t*)malloc(sizeof(dhmfcc_t));
  memset(mfcc,0,sizeof(dhmfcc_t));
  mfcc->mincalc = mincalc?mincalc:1;
  mfcc->minoff = STREAM_BASE_MINOFF;
  mfcc->minblock = STREAM_BASE_MINBLOCK;
  mfcc->maxblock = STREAM_BASE_MAXBLOCK;
  pthread_mutex_init(&mfcc->pushmutex,NULL);
  pthread_mutex_init(&mfcc->readmutex,NULL);
  mfcc->calcthread = (pthread_t *)malloc(sizeof(pthread_t) );
  mfcc->running = 1;
  pthread_create(mfcc->calcthread, NULL, calcworker, (void*)mfcc);
  *pdg = mfcc;
  return 0;
}

int dhmfcc_initPcmex(dhmfcc_t* dg,int maxsize,int minoff ,int minblock ,int maxblock){
  dg->minoff = minoff;
  dg->minblock = minblock;
  dg->maxblock = maxblock;
  dg->inited = 1;
#ifdef WENETOPENV
  if(dg->wenetfn){
    //
    std::string fnonnx(dg->wenetfn);
    std::string fnovbin = fnonnx+"_ov.bin";
    std::string fnovxml = fnonnx+"_ov.xml";
    int melcnt = DhWenet::cntmel(dg->minblock);
    int bnfcnt = DhWenet::cntbnf(melcnt);
    WeAI*  awenet ;
    awenet = new WeOpvn(fnovbin,fnovxml,melcnt,bnfcnt,4);
    if(dg->weai_first){
      WeAI* oldw = dg->weai_first;
      dg->weai_first = awenet;
      delete oldw;
    }else{
      dg->weai_first = awenet;
    }
    awenet->test();
  }
#endif
  return 0;
}

int dhmfcc_initWenet(dhmfcc_t* dg,char* fnwenet){
  dg->wenetfn = strdup(fnwenet);

  std::string fnonnx(fnwenet);
  WeAI*  awenet ;
    int melcnt = DhWenet::cntmel(dg->minblock);
    int bnfcnt = DhWenet::cntbnf(melcnt);
#ifdef WENETOPENV
  if(dg->inited){
    std::string fnovbin = fnonnx+"_ov.bin";
    std::string fnovxml = fnonnx+"_ov.xml";
    awenet = new WeOpvn(fnovbin,fnovxml,melcnt,bnfcnt,4);
  }else{
    awenet = new WeOnnx(fnwenet,melcnt,bnfcnt,4);
  }
#else
    awenet = new WeOnnx(fnwenet,melcnt,bnfcnt,4);
#endif
  WeAI* bwenet = new WeOnnx(fnwenet,321,79,4);
  if(dg->weai_first){
    WeAI* oldw = dg->weai_first;
    dg->weai_first = awenet;
    delete oldw;
  }else{
    dg->weai_first = awenet;
  }
  if(dg->weai_common){
    WeAI* oldw = dg->weai_common;
    dg->weai_common = bwenet;
    delete oldw;
  }else{
    dg->weai_common = bwenet;
  }
  awenet->test();
  bwenet->test();
  return awenet?0:-1;
}

uint64_t dhmfcc_newsession(dhmfcc_t* dg){
  uint64_t sessid = ++dg->sessid;
  PcmSession* sess = new PcmSession(sessid,dg->minoff,dg->minblock,dg->maxblock);
  PcmSession* olds = dg->presess;
  dg->presess = dg->cursess;
  dg->cursess = sess;
  if(olds)delete olds;
  return sessid;
}

int dhmfcc_pushpcm(dhmfcc_t* dg,uint64_t sessid,char* buf,int size,int kind){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  int rst =  0;
  pthread_mutex_lock(&dg->pushmutex);
  rst = sess->pushpcm(sessid,(uint8_t*)buf,size);
  pthread_mutex_unlock(&dg->pushmutex);
  if(rst>0){
    if(sess->first()){
      sess->runfirst(sessid,dg->weai_first);
      uint64_t tick = jtimer_msstamp();
      printf("====runfirst  %ld %ld \n",sessid,tick);
    }
    return 0;
  }else{
    return rst;
  }
}

int dhmfcc_readpcm(dhmfcc_t* dg,uint64_t sessid,char* pcmbuf,int pcmlen,char* bnfbuf,int bnflen){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  int rst = 0;
  pthread_mutex_lock(&dg->readmutex);
  rst =  sess->readnext(sessid,(uint8_t*)pcmbuf,pcmlen,(uint8_t*)bnfbuf,bnflen);
  pthread_mutex_unlock(&dg->readmutex);
  return rst;
}

int dhmfcc_consession(dhmfcc_t* dg,uint64_t sessid){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  return sess->conpcm(sessid);
}

int dhmfcc_finsession(dhmfcc_t* dg,uint64_t sessid){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  return sess->finpcm(sessid);
}

int dhmfcc_free(dhmfcc_t* dg){
  dg->running = 0;
  pthread_join(*dg->calcthread, NULL);
  if(dg->weai_first){
    delete dg->weai_first;
    dg->weai_first = NULL;
  }
  if(dg->weai_common){
    delete dg->weai_common;
    dg->weai_common = NULL;
  }
  if(dg->cursess){
    delete dg->cursess;
    dg->cursess = NULL;
  }
  if(dg->presess){
    delete dg->presess;
    dg->presess = NULL;
  }
  pthread_mutex_destroy(&dg->pushmutex);
  pthread_mutex_destroy(&dg->readmutex);
  free(dg->calcthread);
  free(dg);
  //
  return 0;
}

struct dhunet_s{
  int inited;
  int rgb;
  Mobunet     *munet; 
};

int dhunet_alloc(dhunet_t** pdg,int rgb){
  dhunet_t* unet = (dhunet_t*)malloc(sizeof(dhunet_t));
  memset(unet,0,sizeof(dhunet_t));
  unet->rgb = 1;
  *pdg = unet;
  return 0;
}

int dhunet_initMunet(dhunet_t* dg,char* fnparam,char* fnbin,char* fnmsk){
  dg->munet = new Mobunet(fnbin,fnparam,fnmsk,20,dg->rgb);
  dg->inited = 1;
  printf("===init munet \n");
  return 0;
}

#define AIRUN_FLAG 1
int dhunet_simprst(dhunet_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,int* box,uint8_t* bmsk,uint8_t* bfg,uint8_t* bnfbuf,int bnflen){
  //printf("simprst gogogo %d \n",dg->inited);
  if(!dg->inited)return -1;
  if(bnflen!=STREAM_ALL_BNF)return -2;
  if(!dg->munet)return -3;
  int rst = 0;
  JMat* mat_pic = new JMat(width,height,bpic);
  JMat* mat_msk = bmsk?new JMat(width,height,bmsk):NULL;
  JMat* mat_fg = bfg?new JMat(width,height,bfg):NULL;
  JMat* feat = new JMat(STREAM_CNT_BNF,STREAM_BASE_BNF,(float*)bnfbuf,1);

  MWorkMat wmat(mat_pic,mat_msk,box);
  wmat.premunet();
  JMat* mpic;
  JMat* mmsk;
  wmat.munet(&mpic,&mmsk);
  //tooken
#ifdef AIRUN_FLAG
  uint64_t ticka = jtimer_msstamp();
  rst = dg->munet->domodel(mpic, mmsk, feat);
  uint64_t tickb = jtimer_msstamp();
  uint64_t dist = tickb-ticka;
  if(dist>40){
    printf("===domodel %d dist %ld\n",rst,dist);
  }
#endif
  if(mat_fg){
    wmat.finmunet(mat_fg);
  }else{
    wmat.finmunet(mat_pic);
  }
  if(feat)delete feat;
  delete mat_pic;
  if(mat_fg)delete mat_fg;
  if(mat_msk)delete mat_msk;
  return 0;
}

int dhunet_free(dhunet_t* dg){
  dg->inited = 0;
  if(dg->munet){
    delete dg->munet;
    dg->munet = NULL;
  }
  free(dg);
  return 0;

}



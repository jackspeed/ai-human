#include "gjsimp.h"
#include <stdlib.h>
#include <pthread.h>
#include "dhwenet.h"
#include "wenetai.h"
#include "dhpcm.h"
#include "munet.h"
#include "malpha.h"
#include "dhwenet.h"
#include "Log.h"


struct dhduix_s{

  int width;
  int height;
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

  jmat_t    *mat_feat;
  volatile int running;
  pthread_t *calcthread;
  pthread_mutex_t pushmutex;
  pthread_mutex_t readmutex;

  int rgb;
  Mobunet     *munet; 
  JMat        *mat_pic;
  JMat        *mat_fg;
  JMat        *mat_msk;
};

static void *calcworker(void *arg){
  dhduix_t* mfcc = (dhduix_t*)arg;
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

int dhduix_alloc(dhduix_t** pdg,int mincalc,int width,int height){
  dhduix_t* duix = (dhduix_t*)malloc(sizeof(dhduix_t));
  memset(duix,0,sizeof(dhduix_t));
  duix->mincalc = mincalc?mincalc:1;
  duix->minoff = STREAM_BASE_MINOFF;
  duix->minblock = STREAM_BASE_MINBLOCK;
  duix->maxblock = STREAM_BASE_MAXBLOCK;
  pthread_mutex_init(&duix->pushmutex,NULL);
  pthread_mutex_init(&duix->readmutex,NULL);
  duix->calcthread = (pthread_t *)malloc(sizeof(pthread_t) );
  duix->running = 1;
  pthread_create(duix->calcthread, NULL, calcworker, (void*)duix);
  duix->rgb = 1;
  duix->width = width;
  duix->height = height;
  duix->mat_msk = new JMat(width,height);
  duix->mat_fg = new JMat(width,height);
  duix->mat_pic = new JMat(width,height);
  //duix->mat_feat = jmat_alloc(20,STREAM_BASE_BNF,1,0,4,NULL);
  duix->mat_feat = jmat_alloc(STREAM_BASE_BNF,20,1,0,4,NULL);
  *pdg = duix;
  return 0;
}

int dhduix_initPcmex(dhduix_t* dg,int maxsize,int minoff ,int minblock ,int maxblock,int rgb){
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
  dg->rgb = rgb;
  return 0;
}

int dhduix_initWenet(dhduix_t* dg,char* fnwenet){
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

uint64_t dhduix_newsession(dhduix_t* dg){
  uint64_t sessid = ++dg->sessid;
  PcmSession* sess = new PcmSession(sessid,dg->minoff,dg->minblock,dg->maxblock);
  PcmSession* olds = dg->presess;
  dg->presess = dg->cursess;
  dg->cursess = sess;
  if(olds)delete olds;
  return sessid;
}

int dhduix_pushpcm(dhduix_t* dg,uint64_t sessid,char* buf,int size,int kind){
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

int dhduix_readpcm(dhduix_t* dg,uint64_t sessid,char* pcmbuf,int pcmlen,char* bnfbuf,int bnflen){
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

int dhduix_consession(dhduix_t* dg,uint64_t sessid){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  return sess->conpcm(sessid);
}

int dhduix_finsession(dhduix_t* dg,uint64_t sessid){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  return sess->finpcm(sessid);
}

int dhduix_free(dhduix_t* dg){
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
  if(dg->munet){
    delete dg->munet;
    dg->munet = NULL;
  }
  if(dg->mat_fg){
    delete dg->mat_fg;
    dg->mat_fg = NULL;
  }
  if(dg->mat_pic){
    delete dg->mat_pic;
    dg->mat_pic = NULL;
  }
  if(dg->mat_msk){
    delete dg->mat_msk;
    dg->mat_msk = NULL;
  }
  pthread_mutex_destroy(&dg->pushmutex);
  pthread_mutex_destroy(&dg->readmutex);
  free(dg->calcthread);
  jmat_free(dg->mat_feat);
  free(dg);
  //
  return 0;
}


int dhduix_initMunet(dhduix_t* dg,char* fnparam,char* fnbin,char* fnmsk){
  dg->munet = new Mobunet(fnbin,fnparam,fnmsk,20,dg->rgb);
  dg->inited = 1;
  printf("===init munet \n");
  return 0;
}

int dhduix_simppcm(dhduix_t* dg,char* buf,int size,char* pre,int presize,char* bnf,int bnfsize){
  if(!dg->running)return -2;
  PcmFile* mfcc = new PcmFile(25,10,STREAM_BASE_MAXBLOCK,STREAM_BASE_MAXBLOCK*20);
  mfcc->prepare(buf,size,pre,presize);
  mfcc->process(-1,dg->weai_first);
  int rst = mfcc->readbnf(buf,size);

  return rst;
}

int dhduix_allcnt(dhduix_t* dg,uint64_t sessid){
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  if(sess->sessid()!=sessid)return 0;
  return sess->fileBlock();
}

int dhduix_readycnt(dhduix_t* dg,uint64_t sessid){
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  if(sess->sessid()!=sessid)return 0;
  return sess->calcBlock();
}


#define AIRUN_FLAG 1
int dhduix_fileinx(dhduix_t* dg,uint64_t sessid,char* fnpic,int* box,char* fnmsk,char* fnfg,int bnfinx,char* bimg,char* mskbuf,int imgsize){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;

  uint64_t ticka = jtimer_msstamp();
  std::string sfnpic(fnpic);
  std::string sfnmsk(fnmsk);
  std::string sfnfg(fnfg);
  JMat* mat_pic = dg->mat_pic;
  mat_pic->loadjpg(sfnpic,1);
  uint8_t* bpic = (uint8_t*)mat_pic->data();
  uint8_t* bmsk = NULL;
  uint8_t* bfg = NULL;
  JMat* mat_msk = NULL;
  if(sfnmsk.length()){
    mat_msk = dg->mat_msk;
    mat_msk->loadjpg(sfnmsk,1);
    bmsk = (uint8_t*)mat_msk->data();
    memcpy(mskbuf,bmsk,dg->width*dg->height*3);
  }
  JMat* mat_fg = NULL;
  if(sfnfg.length()){
    mat_fg = dg->mat_fg;
    mat_fg->loadjpg(sfnfg,1);
    bfg = (uint8_t*)mat_fg->data();
  }
  uint64_t tickb = jtimer_msstamp();
  uint64_t dist = tickb-ticka;
  LOGD("tooken","===loadjpg %ld\n",dist);
  int rst = 0;
  if(box){
    rst = dhduix_simpinx(dg,sessid, bpic,dg->width,dg->height, box, bmsk, bfg,bnfinx);
  }else{
    rst = dhduix_simpblend(dg,sessid, bpic,dg->width,dg->height,  bmsk, bfg);
  }
  int size = dg->width*dg->height*3;
  if(bfg){
    memcpy(bimg,bfg,size);
  }else{
    memcpy(bimg,bpic,size);
  }
  if(bmsk) memcpy(mskbuf,bmsk,size);
  return rst;
}

int dhduix_simpinx(dhduix_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,int* box,uint8_t* bmsk,uint8_t* bfg,int inx){
  if(sessid!=dg->sessid)return -1;
  if(!dg->running)return -2;
  PcmSession* sess = dg->cursess;
  if(!sess)return -3;
  int rst = 0;
  int w = width?width:dg->width;
  int h = height?height:dg->height;
  pthread_mutex_lock(&dg->readmutex);
  rst =  sess->readblock(sessid,dg->mat_feat,inx);
  pthread_mutex_unlock(&dg->readmutex);
  //printf("===readblock %d\n",rst);
  if(rst>0){
    rst = dhduix_simprst(dg,sessid, bpic,w,h, box, bmsk, bfg,(uint8_t*)dg->mat_feat->data,STREAM_ALL_BNF);
    return 1;
  }
  return rst;
}

int dhduix_simpblend(dhduix_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,uint8_t* bmsk,uint8_t* bfg){
  //
  return 0;
}

int dhduix_simprst(dhduix_t* dg,uint64_t sessid,uint8_t* bpic,int width,int height,int* box,uint8_t* bmsk,uint8_t* bfg,uint8_t* bnfbuf,int bnflen){
  //printf("simprst gogogo %d \n",dg->inited);
  if(!dg->inited)return -1;
  if(!dg->munet)return -3;
  int rst = 0;
  JMat* mat_pic = new JMat(width,height,bpic);
  JMat* mat_msk = bmsk?new JMat(width,height,bmsk):NULL;
  JMat* mat_fg = bfg?new JMat(width,height,bfg):NULL;
  //read pcm
  JMat* feat = new JMat(STREAM_CNT_BNF,STREAM_BASE_BNF,(float*)bnfbuf,1);

//    MWorkMat wmat(mat_pic,mat_msk,box);
  MWorkMat wmat(mat_pic, NULL,box);
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
  LOGD("tooken","===domodel %ld\n",dist);
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



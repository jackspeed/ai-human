#include <stdlib.h>
#include <string>
#include <stdio.h>
#include "gjsimp.h"
#include "jmat.h"
#include <pthread.h>
#include "dh_data.h"


static volatile int g_running = 0;
static volatile uint64_t g_sessid = 0;
static void* mfccworker(void* arg){

  static  uint64_t sessid ;
  for(int k=0;k<1;k++){
    dhduix_t* mfcc = (dhduix_t*)arg;
    FILE* g_wavfile = fopen("data/b10.wav","rb");
    fseek(g_wavfile,44,0);
    sessid = dhduix_newsession(mfcc);
    g_sessid = sessid;
    int psize = 1000;
    int tickcnt = 0;
    int kkk = 0;
    char* pcm = (char*)malloc(psize);
    while(sessid == g_sessid){
      int readpcm = fread(pcm,1,psize,g_wavfile);
      if(readpcm<1)break;
      dhduix_pushpcm(mfcc,sessid,pcm,readpcm,0);
      tickcnt += readpcm;
      uint64_t tick = jtimer_msstamp();
      //printf("====push %d %ld \n",tickcnt,tick);
      kkk++;
      /*
         if(kkk%100==99){
         sessid = dhduix_newsession(mfcc);
         g_sessid = sessid;
         }
         */
      jtimer_mssleep(5);
    }
    jtimer_mssleep(10000);
    dhduix_finsession(mfcc,sessid);
    free(pcm);
    fclose(g_wavfile);
    printf("===finish\n");
  }
  return NULL;
}


int mainmemcheck(int argc,char** argv){
  dhduix_t* dg = NULL;
  int rst = 0;
  int width = 540;
  int height = 720;
  rst = dhduix_alloc(&dg,20,width,height);
  //char* fnwenet = "model/wenet.onnx";
  rst = dhduix_initPcmex(dg,0,10,20,50,0);
  rst = dhduix_initMunet(dg,"model/xinyan_opt.param","model/xinyan_opt.bin","model/weight_168u.bin");
  //char* fnwenet = "model/wenet.onnx";
  char* fnwenet = "model/wenet.onnx";
  rst = dhduix_initWenet(dg,fnwenet);
  char* pcm = (char*)malloc(102400);

  std::string fnpic = "data/xinyan.jpg";
  std::string fnmsk = "data/m1.jpg";
  std::string fnfg = "data/xinyan.jpg";
  JMat* mat_msk = new JMat();
  mat_msk->loadjpg(fnmsk,1);
  JMat* mat_pic = new JMat();
  mat_pic->loadjpg(fnpic,1);
  JMat* mat_fg = new JMat();
  mat_fg->loadjpg(fnfg,1);
  int m_boxs[4];
  m_boxs[0]=170;m_boxs[2]=382;m_boxs[1]=382;m_boxs[3]=592;
  uint8_t* bpic = (uint8_t*)mat_pic->data();
  uint8_t* bmsk = (uint8_t*)mat_msk->data();
  uint8_t* bfg = (uint8_t*)mat_fg->data();
  int* box = m_boxs;
  for(int m=0;m<10;m++){
    g_sessid = dhduix_newsession(dg);
    for(int k=0;k<100;k++){
      dhduix_pushpcm(dg,g_sessid,pcm,102400,0);
      int allcnt = dhduix_allcnt(dg,g_sessid);
      printf("===allcnt %d\n",allcnt);
    }
    int readycnt = dhduix_readycnt(dg,g_sessid);
    while(readycnt<1){
      jtimer_mssleep(10);
    }
    for(int i=0;i<100;i++){
      readycnt = dhduix_readycnt(dg,g_sessid);
      //printf("===readycnt %d\n",readycnt);
      rst = dhduix_simpinx(dg,g_sessid, bpic,width,height, box, bmsk, bfg, i);
      printf("==simp %d\n",rst);
      jtimer_mssleep(10);
      if(rst<0)break;
    }
    dhduix_finsession(dg,g_sessid);
  }
  free(pcm);
  delete mat_pic;
  delete mat_msk;
  delete mat_fg;
  dhduix_free(dg);
  return 0;
}

int main(int argc,char** argv){
  dhduix_t* dg = NULL;
  int rst = 0;
  int width = 540;
  int height = 720;
  rst = dhduix_alloc(&dg,20,width,height);
  //char* fnwenet = "model/wenet.onnx";
  char* fnwenet = "model/wenet.onnx";
  rst = dhduix_initWenet(dg,fnwenet);
  rst = dhduix_initPcmex(dg,0,10,20,50,0);
  rst = dhduix_initMunet(dg,"model/xinyan_opt.param","model/xinyan_opt.bin","model/weight_168u.bin");

  std::string fnpic = "data/xinyan.jpg";
  std::string fnmsk = "data/m1.jpg";
  std::string fnfg = "data/xinyan.jpg";
  JMat* mat_msk = new JMat();
  mat_msk->loadjpg(fnmsk,1);
  JMat* mat_pic = new JMat();
  mat_pic->loadjpg(fnpic,1);
  JMat* mat_fg = new JMat();
  mat_fg->loadjpg(fnfg,1);
  int m_boxs[4];
  m_boxs[0]=170;m_boxs[2]=382;m_boxs[1]=382;m_boxs[3]=592;
  uint8_t* bpic = (uint8_t*)mat_pic->data();
  uint8_t* bmsk = (uint8_t*)mat_msk->data();
  uint8_t* bfg = (uint8_t*)mat_fg->data();
  int* box = m_boxs;
  int pcmsize = 1280;
  char* pcm = (char*)malloc(1280);
  int bnfsize = 1024*20;
  char* bnf = (char*)malloc(1024*20);
  pthread_t audtrd;
  pthread_create(&audtrd, NULL, mfccworker, (void*)dg);
  //mfccworker(mfcc);

  printf("====render\n");
  //getchar();
  int bnfinx = 0;
  while(1){
    if(!g_sessid){
      printf("+");
      //cv::waitKey(40);
      jtimer_mssleep(40);
      continue;
    }
    int readycnt = dhduix_readycnt(dg,g_sessid);
    printf("====readycnt %d\n",readycnt);
    if(!readycnt){
      jtimer_mssleep(40);
      continue;
    }
    rst = 1;//dhduix_readpcm(dg,g_sessid,pcm,pcmsize,bnf,bnfsize);
    printf("===readpcm %ld %d\n",g_sessid,rst);
    if(rst>0){
      uint64_t tick = jtimer_msstamp();
      //printf("====read  %ld \n",tick);
      //rst = dhduix_simprst(dg,g_sessid, bpic,width,height, box, bmsk, bfg, (uint8_t*)bnf,bnfsize);
      rst = dhduix_simpinx(dg,g_sessid, bpic,width,height, box, bmsk, bfg, bnfinx);
      if(rst>0)bnfinx ++;
      printf("===simprst %d\n",rst);
      mat_fg->show("aaa");
      cv::waitKey(20);
      //jtimer_mssleep(40);
    }else if(rst < 0){
      break;
    }else{
      //cv::waitKey(40);
      jtimer_mssleep(40);
    }
  }
  g_sessid = 0;
  pthread_join(audtrd,NULL);
  printf("====exit\n");
  //
  rst = dhduix_free(dg);
  printf("====exitmfcc\n");
  /*
     rst = dhduix_free(unet);
     delete mat_pic;
     delete mat_msk;
     delete mat_fg;
     */
  free(pcm);
  free(bnf);
  return 0;
}

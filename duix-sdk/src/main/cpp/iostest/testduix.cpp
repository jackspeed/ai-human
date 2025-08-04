#include <stdlib.h>
#include <string>
#include <stdio.h>
#include "gjduix.h"
#include "jmat.h"
#include <pthread.h>
#include "dh_data.h"


static volatile int g_running = 0;
static volatile uint64_t g_sessid = 0;
static void* mfccworker(void* arg){

  static  uint64_t sessid ;
  for(int k=0;k<1;k++){
    dhmfcc_t* mfcc = (dhmfcc_t*)arg;
    FILE* g_wavfile = fopen("data/b10.wav","rb");
    fseek(g_wavfile,44,0);
    sessid = dhmfcc_newsession(mfcc);
    g_sessid = sessid;
    int psize = 1000;
    int tickcnt = 0;
    int kkk = 0;
    char* pcm = (char*)malloc(psize);
    while(sessid == g_sessid){
      int readpcm = fread(pcm,1,psize,g_wavfile);
      if(readpcm<1)break;
      dhmfcc_pushpcm(mfcc,sessid,pcm,readpcm,0);
      tickcnt += readpcm;
      uint64_t tick = jtimer_msstamp();
      //printf("====push %d %ld \n",tickcnt,tick);
      kkk++;
      /*
      if(kkk%100==99){
        sessid = dhmfcc_newsession(mfcc);
        g_sessid = sessid;
      }
      */
      jtimer_mssleep(5);
    }
    jtimer_mssleep(10000);
    dhmfcc_finsession(mfcc,sessid);
    free(pcm);
    fclose(g_wavfile);
    printf("===finish\n");
  }
  return NULL;
}


int main(int argc,char** argv){
  dhmfcc_t* mfcc = NULL;
  int rst = 0;
  rst = dhmfcc_alloc(&mfcc,2);
  //char* fnwenet = "model/wenet.onnx";
  char* fnwenet = "model/wenet.onnx";
  rst = dhmfcc_initWenet(mfcc,fnwenet);
  rst = dhmfcc_initPcmex(mfcc,0,10,20,50);
  dhunet_t* unet = NULL;
  rst = dhunet_alloc(&unet,20);
  rst = dhunet_initMunet(unet,"model/xinyan_opt.param","model/xinyan_opt.bin","model/weight_168u.bin");

  std::string fnpic = "data/xinyan.jpg";
  std::string fnmsk = "data/m1.jpg";
  std::string fnfg = "data/xinyan.jpg";
  JMat* mat_msk = new JMat();
  mat_msk->loadjpg(fnmsk,1);
  JMat* mat_pic = new JMat();
  mat_pic->loadjpg(fnpic,1);
  JMat* mat_fg = new JMat();
  mat_fg->loadjpg(fnfg,1);
  int width = mat_pic->width();
  int height = mat_pic->height();
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
  pthread_create(&audtrd, NULL, mfccworker, (void*)mfcc);
  //mfccworker(mfcc);

  printf("====render\n");
  //getchar();
  while(1){
    if(!g_sessid){
      printf("+");
      //cv::waitKey(40);
      jtimer_mssleep(40);
      continue;
    }
    rst = dhmfcc_readpcm(mfcc,g_sessid,pcm,pcmsize,bnf,bnfsize);
    printf("===readpcm %ld %d\n",g_sessid,rst);
    if(rst>0){
      uint64_t tick = jtimer_msstamp();
      printf("====read  %ld \n",tick);
      rst = dhunet_simprst(unet,g_sessid, bpic,width,height, box, bmsk, bfg, (uint8_t*)bnf,bnfsize);
      printf("===simprst %d\n",rst);
      mat_fg->show("aaa");
      cv::waitKey(30);
      jtimer_mssleep(40);
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
  rst = dhmfcc_free(mfcc);
  printf("====exitmfcc\n");
  /*
  rst = dhunet_free(unet);
  delete mat_pic;
  delete mat_msk;
  delete mat_fg;
  */
  free(pcm);
  free(bnf);
  return 0;
}

#include "dhpcm.h"
#include "mfcc/mfcc.hpp"
#include <stdio.h>
#include "aicommon.h"
#include <vector>
#include <string>
#include "opencv2/core.hpp"
#ifdef USE_HELPER    
#include "dhdatahelper.h"
#endif



PcmItem::PcmItem(int sentid,int minoff,int maxblock,int flip,int inx){
  m_flip = flip;
  m_inx = inx;
  m_sentid = sentid;
  m_maxblock = maxblock;
  //int dist = minoff - STREAM_MFCC_FILL;
  //if(dist>0) m_minoff = dist;
  m_minoff = minoff;
  int allcnt = m_minoff + maxblock + 2*STREAM_MFCC_FILL;
  pcm_allsamp = allcnt*STREAM_BASE_SAMP;
  mel_allcnt = pcm_allsamp/160+1;
  bnf_allcnt = mel_allcnt*0.25f - 0.75f;
  //printf("==minoff %d max %d allcnt %d melcnt %d bnfcnt %d\n", minoff,maxblock,allcnt,mel_allcnt,bnf_allcnt );
  m_pcm = jmat_alloc(STREAM_BASE_SAMP,allcnt,1,0,4,NULL);
  //m_pcm = new jmat_t(STREAM_BASE_SAMP,allcnt,1);
  m_mel = jmat_alloc(STREAM_BASE_MEL,mel_allcnt,1,0,4,NULL);
  //m_mel = new jmat_t(STREAM_BASE_MEL,mel_allcnt,1);
  m_bnf = jmat_alloc(STREAM_BASE_BNF,bnf_allcnt,1,0,4,NULL);
  m_bnfflip = jmat_alloc(STREAM_BASE_BNF,bnf_allcnt,1,0,4,NULL);
  //m_bnf = new jmat_t(STREAM_BASE_BNF,bnf_allcnt,1);
  //gjvad_alloc(&m_vad,STREAM_BASE_SAMP/2);
  mat_flip = jmat_null();
}

PcmItem::~PcmItem(){
  if(m_pcm) jmat_free(m_pcm);
  if(m_mel) jmat_free(m_mel);
  if(m_bnf)jmat_free(m_bnf);
  if(m_wav)jmat_free(m_wav);
  if(m_mfcc)jmat_free(m_mfcc);
  if(m_bnfflip)jmat_free(m_bnfflip);
  jmat_deref(mat_flip);
  //gjvad_free(&m_vad);
}

int PcmItem::reset(){
  jbuf_zeros((jbuf_t*)m_pcm);
  jbuf_zeros((jbuf_t*)m_mel);
  jbuf_zeros((jbuf_t*)m_bnf);
  return 0;
}

int PcmItem::fillPcm(uint64_t sessid,uint64_t tickinx,jmat_t* premat,jmat_t* mat){
  m_wav = mat;
  pcm_block = mat->height;
  if(pcm_block>(m_maxblock+STREAM_MFCC_FILL))return -1;
  pre_block = premat?premat->height:0;
  int pcmcnt = pcm_block ;

  int allcnt = m_minoff + pcmcnt + 2*STREAM_MFCC_FILL;
  //printf("===off %d pcm %d pad %d\n",m_minoff,pcmcnt,2*STREAM_MFCC_FILL);
  pcm_allsamp = allcnt*STREAM_BASE_SAMP;
  mel_allcnt = pcm_allsamp/160+1;
  bnf_allcnt = mel_allcnt*0.25f - 0.75f;

  m_sessid = sessid;
  m_pcminx = tickinx;
  {
    //fill pre
    int dlen = m_minoff + STREAM_MFCC_FILL;
    int blank = dlen - pre_block;
    if(blank){
      float* pbuf = (float*)m_pcm->data;
      int samp = blank*STREAM_BASE_SAMP;
      memset(pbuf,0,samp*sizeof(float));
    }
    if(pre_block){
      short* ps = (short*)premat->data;
      for(int k=blank;k<dlen;k++){
        float* pbuf = (float*)jmat_row(m_pcm,k);
        for(int m=0;m<STREAM_BASE_SAMP;m++){
          *pbuf++ = *ps++/32768.f;
        }
      }
    }
  }
  {
    //fill pcm
    int dlen = pcmcnt + STREAM_MFCC_FILL;
    int blank = dlen - pcm_block;
    int offset = m_minoff + STREAM_MFCC_FILL;
    short* ps = (short*)mat->data;
    for(int k=0;k<pcm_block;k++){
      float* pbuf = (float*)jmat_row(m_pcm,k+offset);
      for(int m=0;m<STREAM_BASE_SAMP;m++){
        *pbuf++ = *ps++/32768.f;
      }
    }
    if(blank){
      float* pbuf = (float*)jmat_row(m_pcm,offset+pcm_block);
      float* abuf = (float*)m_pcm->data;
      int samp = blank*STREAM_BASE_SAMP;
      memset(pbuf,0,samp*sizeof(float));
    }
  }
  return 0;
}

int PcmItem::checkValid(uint64_t tickinx){
  if(!tickinx)return 1;
  return tickinx<=(m_pcminx+pcm_block);//&&(tickinx<=(m_pcminx+pcm_block));
}

jmat_t* PcmItem::readlast(int minoff){
  if(minoff>pcm_block)return NULL;
  int start = pcm_block - minoff;
  jmat_t* mpre = jmat_alloc(STREAM_BASE_PCM,minoff,1,0,1, m_wav->data + start);
  return mpre;
}

int PcmItem::readblock(){
  return  pcm_read;
}

int PcmItem::numblock(){
  return pcm_block;
}

int PcmItem::readbnf(char* buf){
  if(!m_ready)return 0;
  char* mdata = jmat_row(m_mfcc,0);
  int cnt = pcm_block ;
  memcpy(buf,mdata,STREAM_ALL_BNF*cnt);
  return 0;
}

int PcmItem::readblock(jmat_t* pcm,jmat_t* mfcc){
  if(!m_ready)return 0;
  if(pcm_read>=pcm_block)return 0;
  if(pcm){
    char* rdata = jmat_row(m_wav,pcm_read);
    memcpy(pcm->data,rdata,STREAM_BASE_PCM);
  }
  int inx =  pcm_read?pcm_read-1:0;
  char* mdata = jmat_row(m_mfcc,inx);
  //printf("===inx %d mfcc %d\n",inx,m_mfcc->height);
  memcpy(mfcc->data,mdata,STREAM_ALL_BNF);
  pcm_read++;
  return 1;
}

int PcmItem::readblock(int inx,jmat_t* pcm,jmat_t* mfcc){
  if(!m_ready)return 0;
  if(inx>=pcm_block)return 0;
  if(pcm){
    char* rdata = jmat_row(m_wav,inx);
    memcpy(pcm->data,rdata,STREAM_BASE_PCM);
  }
  int newinx =  inx?inx-1:0;
  if(m_flip){
    jmat_reroi(mat_flip,m_mfcc,STREAM_BASE_BNF,20,0,newinx);
#ifdef USE_HELPER    //jmat_dump(mat_flip);
    cv::Mat sm = dh2cvmat(mat_flip);
    jmat_reshape(mfcc,20,STREAM_BASE_BNF);
    cv::Mat dm = dh2cvmat(mfcc);
    cv::transpose(sm,dm);
#endif
    //jmat_dump(mfcc);
  }else{
    char* mdata = jmat_row(m_mfcc,newinx);
  //printf("===inx %d mfcc %d\n",inx,m_mfcc->height);
    memcpy(mfcc->data,mdata,STREAM_ALL_BNF);
  }
  return 1;
}

void PcmItem::dump(FILE* dumpfile){
  printf("===dumpone %d\n",pcm_block);
  for(int k=0;k<pcm_block;k++){
    char* rdata = jmat_row(m_wav,k);
    fwrite(rdata,1,STREAM_BASE_PCM,dumpfile);
  }
}

int PcmItem::runWenet(WeAI* weai){
  int rst = 0;
  float* fwav = (float*)m_pcm->data;
  float* mel = (float*)m_mel->data;
  rst = DhWenet::calcmfcc(fwav,pcm_allsamp,mel,mel_allcnt);

  //float* bnf = m_flip? (float*)m_bnfflip->data:(float*)m_bnf->data;
  float* bnf = (float*)m_bnf->data;
  //tooken
  uint64_t tick = jtimer_msstamp();
#ifdef AIRUN_FLAG
  rst =  weai->run(mel,mel_allcnt,bnf,bnf_allcnt);
#endif
  int dist = jtimer_msstamp()-tick;
  if(0){
    float* pf = (float*)bnf;
    for(int k=0;k<256;k++){
      printf("=%d==%f\n",k,*pf++);
    }
  }

  printf("===pcm %ld %d  mel %d bnf %d dist %d \n",tick,m_pcm->height,mel_allcnt,bnf_allcnt,dist);
  /*
  if(m_flip){
    printf("==flip\n");
    cv::Mat matbnf = dh2cvmat(m_bnf) ;
    cv::Mat matflip =dh2cvmat(m_bnfflip);
    cv::transpose(matflip,matbnf);
    //jmat_reshape(m_bnf,256,bnf_allcnt);
  }
  */

  //printf("===bbb \n");
  int inxstart = m_minoff;
  uint64_t tickinx = m_pcminx;
  float* rbnf = (float*)jmat_row(m_bnf,inxstart);
  int rcnt = pcm_block;
  jmat_t* matbnf = jmat_alloc(STREAM_BASE_BNF,rcnt+19,1,0,4,NULL);
  memcpy(matbnf->data,rbnf,matbnf->buf.size);
  m_mfcc = matbnf;
  /*
  jmat_t* dmat = jmat_alloc(20,256,1,0,4,NULL);
  cv::Mat bm =dh2cvmat(dmat);
  for(int k=0;k<10;k++){
    printf("====k%d\n",k);
    jmat_t* mat = jmat_roi(m_mfcc,256,20,0,k);
    cv::Mat am = dh2cvmat(mat) ;
    cv::transpose(am,bm);
    jmat_deref(mat);
    break;
  }
  */
  m_ready = 1;
  return 0;
}

PcmFile::PcmFile(int fps,int minoff,int mincnt,int maxcnt){
  m_fps = fps;
  m_scale = fps*1.0f/25.0f;
  m_adj = fps!=25;
  m_minoff = minoff;
  m_mincnt = mincnt;
  m_maxcnt = maxcnt;
  m_maxsize = maxcnt* STREAM_BASE_PCM;
  m_minsize = mincnt* STREAM_BASE_PCM;
  m_arrmax = (int*)malloc(sizeof(int)*1024);
  memset(m_arrmax,0,sizeof(int)*1024);
  m_arrmin = (int*)malloc(sizeof(int)*1024);
  memset(m_arrmin,0,sizeof(int)*1024);
}

PcmFile::~PcmFile(){
  for(int k=0;k<vec_pcm.size();k++){
    PcmItem* item = vec_pcm[k];
    delete item;
  }
  if(m_preitem){
    delete m_preitem;
    m_preitem = NULL;
  }
  free(m_arrmax);
  free(m_arrmin);
}

int PcmFile::itemSize(){
  return vec_pcm.size();
}

int PcmFile::process(int inx,WeAI* weai){
  if(inx<0){
    for(int k=0;k<vec_pcm.size();k++){
      PcmItem* item = vec_pcm[k];
      int rst = item->runWenet(weai);
      m_calcblock = m_calcblock + item->numblock();
      m_calccnt += 1;
    }
    return 0;
  }else{
    if(inx>=vec_pcm.size())return -1;
    PcmItem* item = vec_pcm[inx];
    int rst = item->runWenet(weai);
    m_calcblock = m_calcblock + item->numblock();
    m_calccnt += 1;
    return rst;
  }
}

int PcmFile::appenditem(jmat_t* mat,int noone){
  int chkblock = mat->height;
  int chkmin = m_lastitem?m_minoff:0;
  //printf("===chk min %d chkblock %d\n",chkmin,chkblock);
  int inx = m_fileblock ;
  PcmItem* item = new PcmItem(0,chkmin,chkblock,m_flip,inx);
  jmat_t* mpre = NULL;
  if(m_lastitem){
    mpre = m_lastitem->readlast(chkmin);
  }
  int rst = item->fillPcm(0,0,mpre,mat);
  vec_pcm.push_back(item);
  m_lastitem = item;
  m_arrmin[vec_pcm.size()-1] = m_fileblock;
  m_fileblock += item->numblock();
  //printf("===m_fileblock %d to %d \n",m_fileblock,fileBlock());
  m_arrmax[vec_pcm.size()-1] = m_fileblock;

  if(mpre)jmat_free(mpre);
  return 0;
}

int PcmFile::prepare(char* buf,int size,char* prebuf,int presize){
  int rst = 0;
  m_presize = presize;
  m_preblock = presize/STREAM_BASE_PCM;
  int cursize = size;
  char* curhead = buf;
  if(m_preblock){
    jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,m_preblock,1,0,1,prebuf);
    int chkblock = mat->height;
    int chkmin = m_lastitem?m_minoff:0;
    int inx = 0;
    PcmItem* item = new PcmItem(0,chkmin,chkblock,m_flip,inx);
    m_preitem = item;
    m_lastitem = item;
    rst = item->fillPcm(0,0,NULL,mat);
  }
  while(cursize >= m_maxsize){
    jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,m_maxcnt,1,0,1,NULL);
    memcpy(mat->data ,curhead,m_maxsize);
    rst += appenditem(mat);
    cursize -= m_maxsize;
    curhead += m_maxsize;
    //printf("====cursize %d\n",cursize);
  }
  if(cursize>0){
    int block = cursize / STREAM_BASE_PCM;
    if(block<m_mincnt)block = m_mincnt;
    //printf("===lastblock %d cursize %d \n",block,cursize);
    jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,block,1,0,1,NULL);
    memcpy(mat->data ,curhead,block*STREAM_BASE_PCM);
    rst += appenditem(mat);
  }
  return 0;
}

int PcmFile::setflip(int flip){
  m_flip = flip;
  return 0;
}

int PcmFile::prepare(std::string& pcmfn){
  /*
  void* fhnd = wav_read_open(pcmfn.c_str());
  if(!fhnd)return -1;
  int format, channels, sr, bits_per_sample;
  unsigned int data_length;
  int res = wav_get_header(fhnd, &format, &channels, &sr, &bits_per_sample, &data_length);
  if(data_length<1) return -2;
  int sample = data_length/2;
  jbuf_t* pcmbuf = jbuf_alloc(data_length);
  int rst = wav_read_data(fhnd,(unsigned char*)pcmbuf->data,data_length);
  wav_read_close(fhnd);
  int cursize = data_length;
  char* curhead = pcmbuf->data;

  rst =  prepare(curhead,cursize);
  dhmem_deref(pcmbuf);
  return rst;
  */
  return 0;
}

jmat_t* PcmFile::readbnf(int sinx){
  jmat_t* bnf = jmat_alloc(STREAM_BASE_BNF,m_fileblock,1,0,4,NULL);

  return bnf;
}

int PcmFile::readbnf(char* bnf,int bnfsize){
  int block = fileBlock();
  int allsize = block*STREAM_BASE_BNF*sizeof(float);
  if(bnfsize<allsize)return -1;
  jmat_t* mbnf = jmat_alloc(STREAM_BASE_BNF,block,1,0,4,bnf);
  int inx = 0;
  for(int k=0;k<vec_pcm.size();k++){
    PcmItem* item = vec_pcm[k];
    char* buf = jmat_row(mbnf,inx);
    item->readbnf(buf);
    inx += item->numblock();
  }
  return block;
}

int PcmFile::readblock(int sinx,jmat_t* pcm,jmat_t* feat){
  //if(pcm->width!=STREAM_BASE_PCM)return -2001; 
  //if(feat->width!=STREAM_BASE_BNF)return -2002; 
  int inx = sinx/m_scale;
  if(inx>=m_fileblock)return -1;
  printf("===inx %d calc %d\n",inx,m_calccnt);
  if(inx>=m_calcblock)return 0;
  int rst = 0;
  PcmItem* curitem = NULL;
  int newinx = 0;
  for(int k=0;k<m_calccnt;k++){
    if((inx<m_arrmax[k])&&(inx>=m_arrmin[k])){
      curitem = vec_pcm[k];
      newinx = inx - m_arrmin[k];
      break;
    }
  }
  if(curitem){
    rst = curitem->readblock(newinx,pcm,feat);
    if(rst){
      if(pcm)pcm->buf.sessid = inx;
      feat->buf.sessid = inx;
    }
    return rst;
  }
  return 0;
}

PcmSession::PcmSession(uint64_t sessid,int minoff,int mincnt,int maxcnt){
  m_sessid = sessid;
  m_minoff = minoff;
  m_mincnt = mincnt;
  m_maxcnt = maxcnt;
  m_checkcnt = (mincnt+maxcnt)/2;
  m_maxsize = maxcnt* STREAM_BASE_PCM;
  m_minsize = mincnt* STREAM_BASE_PCM;
  int csize = STREAM_BASE_PCM;
  m_pcmcache = (uint8_t*)malloc(STREAM_BASE_PCM*maxcnt*10);
  m_cachepos = 0;
  m_cachemax = STREAM_BASE_PCM*maxcnt*10;
  m_lastitem = NULL;
  m_arrflag = (int*)malloc(1024*sizeof(int));
  memset(m_arrflag,0,1024*sizeof(int));
  m_arrmax = (int*)malloc(sizeof(int)*1024000);
  memset(m_arrmax,0,sizeof(int)*1024000);
  m_arrmin = (int*)malloc(sizeof(int)*1024000);
  memset(m_arrmin,0,sizeof(int)*1024000);
}

PcmSession::~PcmSession(){
  //std::unique_lock lock(m_lock);
  for(int k=0;k<vec_pcm.size();k++){
    PcmItem* item = vec_pcm[k];
    if(item) delete item;
    vec_pcm[k] = NULL;
  }
  free(m_pcmcache);
  free(m_arrflag);
  free(m_arrmin);
  free(m_arrmax);
}

int PcmSession::setflip(int flip){
  m_flip = flip;
  return 0;
}

int PcmSession::appenditem(jmat_t* mat,int noone){
  //std::unique_lock lock(m_lock);
  //printf("===append %d\n",mat->height*STREAM_BASE_PCM);
  //printf("===cur %d min %d max %d\n",m_curflag,m_minoff,m_maxcnt);
  int chkblock = mat->height;
  //printf("===chkblock %d\n",chkblock);
  int chkmin = m_lastitem?m_minoff:0;
  int inx = m_fileblock ;
  PcmItem* item = new PcmItem(m_curflag,chkmin,chkblock,m_flip,inx);
  //printf("===check cur %d off %d block %d\n",m_curflag,chkmin,chkblock);
  jmat_t* mpre = NULL;
  if(m_lastitem){
    mpre = m_lastitem->readlast(chkmin);
  }
  int rst = item->fillPcm(m_sessid,0,mpre,mat);
  //printf("===fill %d\n",rst);
  vec_pcm.push_back(item);
  m_lastitem = item;
  m_arrmin[vec_pcm.size()-1] = m_fileblock;
  m_fileblock += item->numblock();
  m_arrmax[vec_pcm.size()-1] = m_fileblock;

  m_numpush += chkblock;
  m_lastitem = item;
  m_workcnt ++;
  if(mpre)jmat_free(mpre);
  return 1;
}

int PcmSession::checkpcmcache(int flush){
  if(m_cachepos<m_minsize)return 0;
  //printf("===checkcache %d\n",m_cachepos);
  uint8_t* curhead = m_pcmcache;
  int cursize =  m_cachepos;
  int rst = 0;
  if(!m_lastitem){
    jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,m_mincnt,1,0,1,NULL);
    memcpy(mat->data ,curhead,m_minsize);
    rst += appenditem(mat);
    cursize -= m_minsize;
    curhead += m_minsize;
  }
  while(cursize >= m_maxsize){
    jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,m_maxcnt,1,0,1,NULL);
    memcpy(mat->data ,curhead,m_maxsize);
    rst += appenditem(mat);
    cursize -= m_maxsize;
    curhead += m_maxsize;
  }
  int dist =  m_calccnt - m_readcnt;
  int force = dist<2;//distwait()<m_checkcnt;
                     //printf("===dist %d cal %d read %d\n",dist,m_calccnt,m_readcnt);
                     //printf("===force cnt %d\n",force);
  if(force){
    if(cursize >=m_minsize){
      int chkblock = cursize / STREAM_BASE_PCM;
      int chksize = chkblock * STREAM_BASE_PCM;
      jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,chkblock,1,0,1,NULL);
      memcpy(mat->data ,curhead,chksize);
      curhead += chksize;
      cursize -= chksize;
      rst += appenditem(mat);
    }
  }
  if(curhead!=m_pcmcache){
    m_cachepos = cursize;
    memmove(m_pcmcache,curhead,cursize);
  }
  return rst;
}

int PcmSession::pushpcm(uint64_t sessid,uint8_t* buf,int len){
  if(m_finished)return -1;
  if(m_sessid!=sessid)return -2;

  int rst = 0;
  uint8_t* curhead = buf;
  int cursize = len;
  m_totalpush += len;
  int allcnt = m_cachepos + cursize;

  while(allcnt >= m_cachemax){
    int cpsize = m_cachemax - m_cachepos;
    memcpy(m_pcmcache + m_cachepos,curhead,cpsize);
    m_cachepos = m_cachemax;
    cursize -= cpsize;
    curhead += cpsize;
    allcnt -= m_cachemax;
    rst += checkpcmcache();
  }
  if(cursize){
    memcpy(m_pcmcache + m_cachepos,curhead,cursize);
    m_cachepos += cursize;
    rst += checkpcmcache();
  }
  return rst;
}


int PcmSession::simppcm(uint64_t sessid,uint8_t* buf,int len){
  if(m_finished)return -1;
  if(m_sessid!=sessid)return -2;
  int rst = 0;
  //printf("==curpos %d len %d\n",m_cachepos,len);
  uint8_t* curhead = buf;
  int cursize = len;
  m_totalpush += len;
  //int chkblock = m_first&&!m_lastitem?m_mincnt:m_maxcnt;
  //int chksize = m_first&&!m_lastitem?m_firstsize:m_basesize;
  //int chkfirst = m_first&&!m_lastitem;
  if(m_cachepos){
    int cnt = m_cachepos + len;
    if(cnt>=m_minsize){
      int chkblock = cnt / STREAM_BASE_PCM;
      if(chkblock>m_maxcnt)chkblock = m_maxcnt;
      int chksize = chkblock * STREAM_BASE_PCM;
      jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,chkblock,1,0,1,NULL);
      int cpsize = (m_cachepos > chksize)?chksize:m_cachepos;
      memcpy(mat->data,m_pcmcache,cpsize);
      int left = chksize - cpsize;
      if(left>0) memcpy(mat->data + cpsize,buf,left);
      //printf("append a %d\n",left);
      m_cachepos -= cpsize;
      cursize -= left;
      curhead += left;
      rst = appenditem(mat);
    }else{
      memcpy(m_pcmcache+ m_cachepos,buf,len);
      m_cachepos += len;
      return 0;
    }
  }
  while(cursize>=m_minsize){
    //printf("pbbb\n");
    int chkblock = cursize / STREAM_BASE_PCM;
    if(chkblock>m_maxcnt)chkblock = m_maxcnt;
    int chksize = chkblock * STREAM_BASE_PCM;

    jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,chkblock,1,0,1,NULL);
    memcpy(mat->data ,curhead,chksize);
    curhead += chksize;
    cursize -= chksize;
    rst = appenditem(mat);
  }
  if(cursize>0){
    //printf("==cursize %d\n",cursize);
    memcpy(m_pcmcache,curhead,cursize);
    m_cachepos = cursize;
  }
  return rst;
}

int PcmSession::conpcm(uint64_t sessid){
  //if(m_finished)return -1;
  if(m_sessid!=sessid)return -2;
  m_cachepos = 0;
  m_finished = 0;
  m_curflag ++;
  return 0;
}

int PcmSession::finpcm(uint64_t sessid){
  if(m_finished)return -1;
  if(m_sessid!=sessid)return -2;
  checkpcmcache();
  if(m_cachepos){
    int block = m_cachepos / STREAM_BASE_PCM;
    int left = m_cachepos % STREAM_BASE_PCM;
    if(left)block++;
    jmat_t* mat = jmat_alloc(STREAM_BASE_PCM,block,1,0,1,NULL);
    memset(mat->data,0,STREAM_BASE_PCM*block);
    memcpy(mat->data,m_pcmcache,m_cachepos);
    appenditem(mat);
  }
  m_finished = 1;
  return 0;
}

int PcmSession::runfirst(uint64_t sessid,WeAI* weai){
  if(m_sessid!=sessid)return -2;
  if(!m_first)return 0;
  if(m_calccnt)return 0;
  PcmItem* item = vec_pcm[m_calccnt];
  if(item){
    item->runWenet(weai);
    m_numcalc += item->numblock();
  }
  m_calccnt ++;
  m_first = 0;
  //
  return 0;
}

int PcmSession::runcalc(uint64_t sessid,WeAI* weai,int mincalc){
  if(m_sessid!=sessid)return -2;
  if(m_first)return -1;
  int rst = 0;
  if(m_calccnt<m_workcnt){
    int dist = m_calccnt - m_readcnt;
    //printf("===disc %d work %d mincalc %d\n",dist,m_workcnt,mincalc);
    if(dist<mincalc){
      PcmItem* item = vec_pcm[m_calccnt];
      if(item){
        item->runWenet(weai);
        m_numcalc += item->numblock();
      }
      m_calccnt ++;
      rst = 1;
    }
  }else if(m_finished){
    rst = -1;
  }else{
    rst = 0;
  }
  if(rst<1){
    int dist = m_readcnt - m_clrcnt;
    if(dist>5){
      for(int k=0;k<m_readcnt-5;k++){
        PcmItem* item = vec_pcm[k];
        vec_pcm[k] = NULL;
        if(item){ 
          delete item;
          m_clrcnt = k;
        }
      }
    }
  }
  return rst;
}

void PcmSession::dump(char* dumpfn){
  FILE* dumpfile = fopen(dumpfn,"wb");
  printf("===dump %ld\n",vec_pcm.size());
  for(int k=0;k<vec_pcm.size();k++){
    PcmItem* item = vec_pcm[k];
    item->dump(dumpfile);
  }
  fclose(dumpfile);
}

int PcmSession::distwait(){
  printf("===calc %d read %d \n",m_numcalc,m_numread);
  return m_numpush - m_numread;
}

int PcmSession::readnext(uint64_t sessid,uint8_t* pcmbuf,int pcmlen,uint8_t* bnfbuf,int bnflen){
  if(m_sessid!=sessid)return -2;
  if(pcmlen!=STREAM_BASE_PCM)return -1;
  if(bnflen!=STREAM_ALL_BNF)return -2;
  jmat_t* mpcm = jmat_alloc(STREAM_BASE_PCM,1,1,0,1,pcmbuf);
  jmat_t* mbnf = jmat_alloc(STREAM_BASE_BNF,20,1,0,4,bnfbuf);
  int rst = readnext(sessid,mpcm,mbnf);
  jmat_free(mpcm);
  jmat_free(mbnf);
  return rst;
}

int PcmSession::readblock(uint64_t sessid,uint8_t* bnfbuf,int bnflen,int inx){
  if(m_sessid!=sessid)return -2;
  if(bnflen!=STREAM_ALL_BNF)return -2;
  jmat_t* mbnf = jmat_alloc(STREAM_BASE_BNF,20,1,0,4,bnfbuf);
  int rst = readblock(sessid,mbnf,inx);
  jmat_free(mbnf);
  return rst;

}

int PcmSession::readblock(uint64_t sessid,jmat_t* mbnf,int inx){
  if(m_sessid!=sessid)return -2;
  if(mbnf->width!=STREAM_BASE_BNF)return -2002; 
  //if(inx>=m_calccnt)return -99;
  //printf("===inx %d num %d\n",inx,m_numcalc);
  if(inx>=m_numcalc)return -99;
  int rst = 0;
  PcmItem* curitem = NULL;
  int newinx = 0;
  if((inx<m_arrmax[m_readcnt])&&(inx>=m_arrmin[m_readcnt])){
    curitem = vec_pcm[m_readcnt];
    newinx = inx - m_arrmin[m_readcnt];
  }else{
    for(int k=0;k<m_calccnt;k++){
      //printf("==k %d max %d min %d\n",k,m_arrmax[k],m_arrmin[k]);
      if((inx<m_arrmax[k])&&(inx>=m_arrmin[k])){
        curitem = vec_pcm[k];
        m_readcnt = k;
        newinx = inx - m_arrmin[k];
        break;
      }
    }
  }
  //printf("===curitem %p inx %d new %d\n",curitem,inx ,newinx);
  if(curitem){
    rst = curitem->readblock(newinx,NULL,mbnf);
    if(rst){
      mbnf->buf.sessid = inx;
    }
    return rst;
  }
  return 0;
}

int PcmSession::readnext(uint64_t sessid,jmat_t* mpcm,jmat_t* mbnf){
  if(mpcm->width!=STREAM_BASE_PCM)return -2001; 
  if(mbnf->width!=STREAM_BASE_BNF)return -2002; 
  //printf("===p %d r %d\n",m_totalpush,m_totalread);
  if(m_totalread<m_totalpush){
    //printf("===q %d r %d\n",m_readcnt,m_calccnt);
    if(m_readcnt<m_calccnt){
      PcmItem* item = vec_pcm[m_readcnt];      
      int rst = item->readblock(mpcm,mbnf);
      if(!rst){
        m_readcnt++;
        return 0;
      }else{
#ifdef PCMDEBUG
        if(1){
          char fn[255];
          sprintf(fn,"out_%d.data",++m_debugout);
          FILE* df = fopen(fn,"wb");
          fwrite(mpcm->data,1,STREAM_BASE_PCM,df);
          fclose(df);
        }
#endif
        m_numread += 1;
        m_totalread+=STREAM_BASE_PCM;
        return item->itemsentid();
      }
    }else{
      return 0;
    }
  }else{
    return m_finished?-1:0;
  }
}




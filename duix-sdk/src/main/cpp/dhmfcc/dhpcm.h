                                                                                        ///*
#pragma once
#include "dh_data.h"
#include "aicommon.h"
#include <mutex>
#include <vector>
#include "dhwenet.h"
#include "wenetai.h"

//#define PCMDEBUG 1
#define AIRUN_FLAG 1
class PcmItem{
  private:
    uint64_t m_sessid = 0;
    int   m_minoff = 0;
    int   m_maxblock = 0;

    int   pcm_allsamp = 0;
    int   bnf_allcnt = 0;
    int   mel_allcnt = 0;
    jmat_t* m_wav = NULL;
    jmat_t* m_pcm = NULL;
    jmat_t* m_mel = NULL;
    jmat_t* m_bnf = NULL;
    jmat_t* m_bnfflip = NULL;
    jmat_t* m_mfcc = NULL;
    int   pcm_block = 0;
    int   pcm_read = 0;
    int   pre_block = 0;
    uint64_t m_pcminx = 0;
    //gjvad_t* m_vad = NULL;
    int   m_ready = 0;
    int m_sentid = 1;
    int m_flip = 0;
    int m_inx = 0;
    jmat_t* mat_flip = NULL;
  public:
    int itemsentid(){return m_sentid;};
    int blocks(){return pcm_block;};
    int ready(){return m_ready;};
    int finished(){return pcm_read>=pcm_block;};
    int reset();
    PcmItem(int sentid,int minoff  ,int maxblock ,int flip,int inx);
    int fillPcm(uint64_t sessid,uint64_t tickinx,jmat_t* premat,jmat_t* mat);
    int checkValid(uint64_t tickinx);
    jmat_t* readlast(int minoff);
    int runWenet(WeAI* weai);
    int readblock(jmat_t* pcm,jmat_t* mfcc);
    int readblock(int inx,jmat_t* pcm,jmat_t* mfcc);
    int readbnf(char* buf);
    int numblock();
    int startinx(){return m_inx;};
    int endinx(){return m_inx+pcm_block;};
    int readblock();
    void dump(FILE* dumpfile);
    ~PcmItem();
};

class PcmFile{
  private:
    int         m_fps = 25;
    int         m_adj = 0;
    float       m_scale = 1.0f;
    int         m_minoff = 0;
    int         m_mincnt = 0;
    int         m_maxcnt = 0;
    int         m_minsize = 0;
    int         m_maxsize = 0;

    int         m_fileblock = 0;
    int         m_calcblock = 0;
    int         m_clrcnt = 0;
    int         m_readcnt = 0;
    int         m_calccnt = 0;
    int         *m_arrmax = NULL;
    int         *m_arrmin = NULL;
    std::vector<PcmItem*>  vec_pcm ;
    int       appenditem(jmat_t* mat,int noone=0);
    PcmItem     *m_lastitem = NULL;
    PcmItem     *m_lastread = NULL;
    int         m_presize = 0;
    int         m_preblock = 0;
    PcmItem     *m_preitem = NULL;;
    int         m_flip = 0;
  public:
    PcmFile(int fps = 25,int minoff = STREAM_BASE_MINOFF,int mincnt = STREAM_BASE_MINBLOCK,int maxcnt = STREAM_BASE_MAXBLOCK);
    int setflip(int flip);
    int prepare(std::string& pcmfn);
    int prepare(char* buf,int size,char* prebuf = NULL,int presize = 0);
    int itemSize();
    int process(int inx,WeAI* ai);
    int readblock(int sinx,jmat_t* pcm,jmat_t* feat);
    jmat_t* readbnf(int sinx);
    int readbnf(char* bnf,int bnfsize);
    int fileBlock(){return m_fileblock*m_scale;};
    int calcBlock(){return m_calcblock*m_scale;};
    virtual ~PcmFile();
};

class PcmSession{
  private:
    int         m_sessid = 0;

    int         m_minoff = 0;
    int         m_mincnt = 0;
    int         m_maxcnt = 0;
    int         m_minsize = 0;
    int         m_maxsize = 0;
    //int         m_basesize = 0;
    //int         m_firstsize = 0;

    int         m_cachepos = 0;
    int         m_cachemax = 0;
    uint8_t      *m_pcmcache = NULL;

    std::mutex  m_lock;
    int         *m_arrflag;
    int         m_curflag = 1;

    std::vector<PcmItem*>  vec_pcm ;
    PcmItem     *m_lastitem = NULL;

    volatile int         m_clrcnt = 0;
    volatile int         m_workcnt = 0;
    volatile int         m_readcnt = 0;
    volatile int         m_calccnt = 0;
    int       appenditem(jmat_t* mat,int noone=0);

    volatile int       m_totalpush = 0;
    volatile int       m_totalread = 0;
    volatile int       m_finished = 0;
    int       m_first = 1;
    int     m_debuginx = 0;
    int     m_debugout = 0;
    int     checkpcmcache(int flash=0);
    int     m_numcalc = 0;
    int     m_numread = 0;
    int     m_numpush = 0;
    int     distwait();
    int     m_checkcnt = 0;
    int     m_flip = 0;
    int         *m_arrmax = NULL;
    int         *m_arrmin = NULL;
    int         m_fileblock = 0;
    int         m_calcblock = 0;
  public:
    int setflip(int flip);
    uint64_t sessid(){return m_sessid;};
    int simppcm(uint64_t sessid,uint8_t* buf,int len);
    int pushpcm(uint64_t sessid,uint8_t* buf,int len);
    int finpcm(uint64_t sessid);
    int conpcm(uint64_t sessid);
    int runcalc(uint64_t sessid,WeAI* weai,int mincalc=1);
    int runfirst(uint64_t sessid,WeAI* weai);
    int readnext(uint64_t sessid,jmat_t* mpcm,jmat_t* mbnf);
    int readnext(uint64_t sessid,uint8_t* pcmbuf,int pcmlen,uint8_t* bnfbuf,int bnflen);
    int readblock(uint64_t sessid,jmat_t* mbnf,int index);
    int readblock(uint64_t sessid,uint8_t* bnfbuf,int bnflen,int inx);
    PcmSession(uint64_t sessid,int minoff = STREAM_BASE_MINOFF,int mincnt = STREAM_BASE_MINBLOCK,int maxcnt = STREAM_BASE_MAXBLOCK);
    ~PcmSession();
    void dump(char* dumpfn);
    int first(){return m_first;};
    int fileBlock(){return m_fileblock;};
    //int calcBlock(){return m_calcblock;};
    int calcBlock(){return m_numcalc;};
};



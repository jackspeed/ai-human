
#ifndef GJ_BOTCORE
#define GJ_BOTCORE
#include "dh_mem.h"
#include "dh_data.h"
#include "dh_que.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct ext_handle_t     ext_handle_t;
  typedef struct ext_env_t     ext_env_t;;

  typedef ext_handle_t* (*func_extcreate)(char* uuid,void* env,void* tag);
  typedef int (*func_extdestroy)(ext_handle_t *exthandle);
  typedef int  (*func_extupsess)(ext_handle_t *handle,uint64_t sessid);
  typedef int  (*func_extstart)(ext_handle_t *handle);
  typedef int  (*func_extstop)(ext_handle_t *handle);
  typedef int  (*func_extrun)(ext_handle_t *handle,uint64_t sessid,jbuf_t* buf);
  typedef int  (*func_extrunex)(ext_handle_t *handle,uint64_t sessid,jbuf_t** buf);

  typedef struct ext_model_t{
    int             m_id;
    char*           m_name;
    func_extcreate  fn_create;
    func_extdestroy fn_destroy;
  }ext_model_t;


  struct ext_handle_t{
    void            *ext_tag;
    char            *m_uuid;
    uint64_t        m_sessid;
    func_extstart   fn_start;
    func_extstop    fn_stop;
    func_extupsess  fn_upsess;
    func_extrun     fn_extrun;
    func_extrun     fn_extrunex;
  };

  struct ext_env_t{
    uint64_t  m_sessid;
    volatile int  m_running;
    jqueue_t* q_arrext[16];//msg pcm asr chat tts mfcc render
  };

#define INX_QMSG 0
#define INX_QPCM 1
#define INX_QASR 2
#define INX_QCHAT 3
#define INX_QANSWER 4
#define INX_QTTS 5
#define INX_QMFCC 6
#define INX_QRENDER 7


#ifdef __cplusplus
}
#endif

#endif

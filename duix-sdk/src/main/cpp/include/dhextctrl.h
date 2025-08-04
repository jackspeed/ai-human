#ifndef GJ_EXTCTRL
#define GJ_EXTCTRL

#include <stdio.h>
#include "dhextend.h"
#include "gj_threadpool.h"

#ifdef __cplusplus
extern "C" {
#endif


  typedef void (*func_extprocess)(void *data);
  typedef struct{
    ext_env_t* env;;
    jqueue_t* q_msg;
    jqueue_t* q_input;
    jqueue_t* q_output;
    uint64_t  tick_process;
    ext_handle_t*   hnd_process;
    func_extrun     fn_run;
    func_extprocess fn_process;
  }ext_process_t;

  typedef struct{
    ext_model_t*  asr_model;
    ext_model_t*  chat_model;
    ext_model_t*  tts_model;
    ext_model_t*  bnf_model;
    ext_model_t*  render_model;
  }extmain_t;

  typedef struct{
    volatile  uint64_t  m_sessid;
    volatile int m_running;
    ext_env_t* env_sess;
    ext_process_t* asr_proc;
    ext_process_t* chat_proc;
    ext_process_t* tts_proc;
    threadpool_t*  pool;
  }extsess_t;

  typedef int (*func_inout)(uint64_t looptick,void* arg);

  int ext_createsess( extmain_t* extmain,char* uuid,extsess_t** pext);
  int ext_startsess(extsess_t* ext,func_inout fn_input,func_inout fn_output,void* tag); 
  int ext_stopsess(extsess_t* ext); 
  int ext_destroysess(extsess_t** pext);


#ifdef __cplusplus
}
#endif

#endif

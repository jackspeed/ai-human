#ifndef GJ_MEDQUE_H
#define GJ_MEDQUE_H
#include "dh_data.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct jqueue_s jqueue_t;
#define GQUE_SIMP  1001
#define GQUE_MUTI  1003
#define GQUE_LOCK  1005

  jqueue_t*   jque_alloc(int size,int cache,int kind);
  int         jque_push(jqueue_t* que,jbuf_t* buf);
  jbuf_t*     jque_pop(jqueue_t* que,uint64_t sessid);
  jbuf_t*     jque_popall(jqueue_t* que);
  int         jque_size(jqueue_t* que);
  int         jque_free(jqueue_t* que);



#ifdef __cplusplus
}
#endif

#endif

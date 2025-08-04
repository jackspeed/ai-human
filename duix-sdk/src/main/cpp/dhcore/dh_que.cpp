#include "dh_que.h"

#include "readerwriterqueue.h"
#include "concurrentqueue.h"
#include "blockingconcurrentqueue.h"
#include "dh_atomic.h"

typedef moodycamel::ReaderWriterQueue<jbuf_t*>  ReaderWriterQueue;
typedef moodycamel::ConcurrentQueue<jbuf_t*> ConcurrentQueue;
typedef moodycamel::BlockingConcurrentQueue<jbuf_t*> BlockingConcurrentQueue;

typedef int (*jqfn_pop)(jqueue_t* que,int flush,jbuf_t** pbuf);
typedef int (*jqfn_push)(jqueue_t* que,int flush,jbuf_t* buf);

struct jqueue_s{
  void        *m_obj;
  int         m_kind;
	DH_ATOMIC uint32_t nrefs; /**< Number of references  */
  int         m_cache;
  //jbuf_t      *m_readcache;
  //jbuf_t      *m_writecache;
  jqfn_push   fn_push;
  jqfn_pop    fn_pop;
  uint64_t    m_lastsess;
};

typedef struct{
  jqueue_t  que;
  jbuf_t    *m_head;
  jbuf_t    *m_tail;
  pthread_mutex_t  m_lock; 
}jlockque_t;

static int simp_push(jqueue_t* que,int flush,jbuf_t* buf){
  void* obj = que->m_obj;
  //if(flush){
  return reinterpret_cast<ReaderWriterQueue*>(obj)->enqueue(buf);
  //}else{
  //return reinterpret_cast<ReaderWriterQueue*>(obj)->try_enqueue(buf);
  //}
}

static int simp_pop(jqueue_t* que,int flush,jbuf_t** pbuf){
  void* obj = que->m_obj;
  return reinterpret_cast<ReaderWriterQueue*>(obj)->try_dequeue(*pbuf);
}

static int muti_push(jqueue_t* que,int flush,jbuf_t* buf){
  void* obj = que->m_obj;
  //if(flush){
  return reinterpret_cast<BlockingConcurrentQueue*>(obj)->enqueue(buf);
  //}else{
  //return reinterpret_cast<BlockingConcurrentQueue*>(obj)->try_enqueue(buf);
  //}
}

static int muti_pop(jqueue_t* que,int flush,jbuf_t** pbuf){
  void* obj = que->m_obj;
  return reinterpret_cast<BlockingConcurrentQueue*>(obj)->try_dequeue(*pbuf);
}

static int lock_push(jqueue_t* que,int flush,jbuf_t* buf){
  jlockque_t* exque = reinterpret_cast<jlockque_t*>(que);
  buf->next = NULL;
  pthread_mutex_lock(&exque->m_lock);
  if(exque->m_tail){
    if(exque->m_head==exque->m_tail){
      exque->m_head->next = buf;
      exque->m_tail = buf;
    }else{
      exque->m_tail->next = buf;
      exque->m_tail = buf;
    }
  }else{
    exque->m_head = buf;
    exque->m_tail = buf;
  }
  pthread_mutex_unlock(&exque->m_lock);
  //printf("===push %p one %d %p\n",que,que->m_size,buf);
  //printf("===que %p head %p tail %p\n",que,exque->m_head,exque->m_tail);
  return 1;  //
}

static int lock_pop(jqueue_t* que,int flush,jbuf_t** pbuf){
  jlockque_t* exque = reinterpret_cast<jlockque_t*>(que);
  jbuf_t* buf = NULL;
  int rst = 0;
  pthread_mutex_lock(&exque->m_lock);
  buf = exque->m_head;
  if(buf){
    if(exque->m_tail==buf){
      exque->m_head = NULL;
      exque->m_tail = NULL;
    }else{
      exque->m_head = buf->next;
    }
    buf->next = NULL;
  }
  pthread_mutex_unlock(&exque->m_lock);
  //printf("===pop %p one %d %p\n",que,que->m_size,buf);
  //printf("===que %p head %p tail %p\n",que,exque->m_head,exque->m_tail);
  *pbuf = buf;
  return rst;
}


void my_jque_destroy(void* arg){
  jqueue_t* que = (jqueue_t*)arg;

  /*
     buf= que->m_readcache;
     while(buf){
     jbuf_t* one = buf->next;
     jbuf_free(buf);
     buf = one;
     }
     buf= que->m_writecache;
     while(buf){
     jbuf_t* one = buf->next;
     jbuf_free(buf);
     buf = one;
     }
     */
  jbuf_t* buf  = NULL;
  que->fn_pop(que,1,&buf);
  while(buf){
    jbuf_free(buf);
    buf = NULL;
    que->fn_pop(que,1,&buf);
    //printf("===free one %p\n",buf);
  }

  if(que->m_kind==GQUE_SIMP){
    delete reinterpret_cast<ReaderWriterQueue*>(que->m_obj);
  }else{
    delete reinterpret_cast<BlockingConcurrentQueue*>(que->m_obj);
  }

}

jqueue_t*  jque_alloc(int size,int cache,int kind){
  jqueue_t* que = NULL;
  //if(kind==GQUE_LOCK){
  if(0){
    jlockque_t* exq = (jlockque_t*)dhmem_alloc(sizeof(jlockque_t),my_jque_destroy);
    memset(exq,0,sizeof(jlockque_t));
    pthread_mutex_init(&exq->m_lock,NULL);
    que = reinterpret_cast<jqueue_t*>(exq);
    que->fn_pop = lock_pop;
    que->fn_push = lock_push;
  }else{
    que = (jqueue_t*)dhmem_alloc(sizeof(jqueue_t),my_jque_destroy);
    memset(que,0,sizeof(jqueue_t));
    if(kind==GQUE_SIMP){
      que->m_obj = new ReaderWriterQueue();
      que->fn_push = simp_push;
      que->fn_pop = simp_pop;
    }else {
      que->m_obj = new BlockingConcurrentQueue();
      que->fn_push = muti_push;
      que->fn_pop = muti_pop;
    }
  }
  if(que){
    que->m_cache = cache;
    que->m_kind = kind;
  }
  return que;
}

int jque_push(jqueue_t* que,jbuf_t* buf){
  if(!buf)return 0;
  if(buf->sessid>que->m_lastsess) que->m_lastsess = buf->sessid;
  /*
     if(que->m_cache){
     while(que->m_writecache){
     jbuf_t* one = que->m_writecache;
     que->m_writecache = one->next;
     que->fn_push(que,1,one);
     }
     }
     */
  int rst = que->fn_push(que,!que->m_cache,buf);
	dh_atomic_rlx_add(&que->nrefs, 1u);
  /*
     if(!rst&&que->m_cache){
     if(que->m_writecache){
     jbuf_t* tail = que->m_writecache;
     while(tail->next)tail = tail->next;
     tail->next = buf;
     }else{
     que->m_writecache = buf;
     }
     rst = 1;
     }
     */
  return rst;
}

jbuf_t* jque_pop(jqueue_t* que,uint64_t sessid){
  if(sessid&&(sessid<que->m_lastsess)){
    //printf("===last %ld of %ld\n",sessid,que->m_lastsess);
    return NULL;
  }
  int rst = 0;
  jbuf_t* buf = NULL;

  /*
     if(que->m_readcache){
     buf = que->m_readcache;
     que->m_readcache = que->m_readcache->next;
     buf->next = NULL;
     rst = 1;
     }else{
     */
  rst = que->fn_pop(que,0,&buf);
  if(buf)dh_atomic_acq_sub(&que->nrefs, 1u);
  /*
     if(rst&&buf){
     que->m_readcache = buf->next;
     buf->next = NULL;
     }
     }
     */
  return buf;
}

jbuf_t* jque_popall(jqueue_t* que){
  return NULL;
}

int jque_size(jqueue_t* que){
  return que->nrefs;
}

int jque_free(jqueue_t* que){
  dhmem_deref(que);
  return 0;
}



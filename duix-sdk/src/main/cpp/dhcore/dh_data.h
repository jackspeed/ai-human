#ifndef GJ_MEDDATA_H
#define GJ_MEDDATA_H
#include <stdint.h>
#include "dh_mem.h"


#ifdef __cplusplus
extern "C" {
#endif

  typedef struct jbuf_s jbuf_t;

  struct jbuf_s{
    char    *data;
    int     size;
    uint64_t  sessid;
    int64_t   tag;
    int     ref;
    jbuf_t  *next;
  };

  jbuf_t* jbuf_alloc(int size);
  jbuf_t* jbuf_strdup(char* txt,int size);
  jbuf_t* jbuf_dupmem(char* mem,int size);
  jbuf_t* jbuf_refmem(char* mem,int size);
  jbuf_t* jbuf_null(uint64_t sessid);
  int       jbuf_zeros(jbuf_t* buf);
  int       jbuf_free(jbuf_t* buf);
  int       jbuf_copy(jbuf_t* dst,jbuf_t* src);

  typedef struct jmat_s jmat_t;
  struct jmat_s{
    jbuf_t    buf;
    char      *data;
    int       width;
    int       height;
    int       channel;
    int       stride;
    int       bit;  
    int       gpu;
    //jmat_t    *rmat;    
    //jmat_t    *bmat;    
  };

  jmat_t* jmat_null();
  jmat_t* jmat_allocex(int w,int h,int c ,int d, int b,void* mem,dhmem_destroy_h fndestroy);
  jmat_t* jmat_alloc(int w,int h,int c ,int d, int b,void* mem);
  jmat_t* jmat_crgb(int w,int h,uint8_t *mem); 
  char*   jmat_row(jmat_t* mat,int row);
  char*   jmat_item(jmat_t* mat,int col,int row);
  int     jmat_free(jmat_t* mat);
  int jmat_reshape(jmat_t* mat,int w,int h);
  jmat_t* jmat_roi(jmat_t* mat,int w,int h,int l,int t);
  int       jmat_reroi(jmat_t* mat,jmat_t* src,int w,int h,int l,int t);
  int       jmat_copy(jmat_t* dst,jmat_t* src);
  int       jmat_dump(jmat_t* mat);

  jmat_t*   jmat_clone(jmat_t* mat);
  //jmat_t* jmat_roi(jmat_t* src,int left,int top,int width,int height);
  int       jmat_zero(jmat_t* src);

  jmat_t* jmat_addref(jmat_t* mat);
  jmat_t* jmat_deref(jmat_t* mat);




  void* jdata_addref(void* data);
  void* jdata_deref(void* data);
  uint64_t jtimer_msstamp();  
  void jtimer_mssleep(int ms) ;

#ifdef __cplusplus
}
#endif

#endif

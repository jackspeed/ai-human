#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "dh_data.h"
#ifdef WIN32
#include <windows.h>
#else
//#include <sys/timeb.h>
#include <unistd.h>
#endif
#include <time.h>
#include "dh_mem.h"

jmat_t* jmat_addref(jmat_t* mat){
  if(mat) dhmem_ref(mat);
  return mat;
}

jmat_t* jmat_deref(jmat_t* mat){
  if(!mat)return NULL;
  return (jmat_t*)dhmem_deref(mat);
}

void* jdata_addref(void* data){
  if(!data)return NULL;
  return dhmem_ref(data);
}

void* jdata_deref(void* data){
  if(!data)return NULL;
  return dhmem_deref(data);
}

static void my_jbuf_destroy(void* arg){
  jbuf_t* buf = (jbuf_t*)arg;
  //printf("===jbuf destroy %p\n",buf);
  //
}

jbuf_t* jbuf_allocex(char* mem,int size,dhmem_destroy_h fndestroy){
  jbuf_t* buf = (jbuf_t*)dhmem_alloc(sizeof(jbuf_t),fndestroy);
  memset(buf,0,sizeof(jbuf_t));

  return buf;
}

jbuf_t* jbuf_alloc(int size){
  int len = size>0?size:0;
  jbuf_t* buf = (jbuf_t*)dhmem_alloc(sizeof(jbuf_t)+len,my_jbuf_destroy);
  //printf("===jbuf alloc %p\n",buf);
  memset(buf,0,len+sizeof(jbuf_t));
  if(size>0){
    buf->data = (char*)buf + sizeof(jbuf_t);
  }else{
    buf->data = NULL;
  }
  buf->size = size;
  return buf;
}

jbuf_t* jbuf_strdup(char* txt,int pos){
  int len = strlen(txt);
  if((pos>0)&&(pos<len))len = pos;
  char* pb = txt; 
  int size = len;
  jbuf_t* buf = jbuf_alloc(size+1);
  memcpy(buf->data,pb,size);
  buf->data[size]=0;
  return buf;
}

jbuf_t* jbuf_dupmem(char* mem,int size){
  jbuf_t* buf = jbuf_alloc(size);
  if(size) memcpy(buf->data,mem,size);
  return buf;
}

jbuf_t* jbuf_refmem(char* mem,int size){
  jbuf_t* buf = jbuf_alloc(0);
  buf->data = mem;
  buf->size = size;
  buf->ref = 1;
  return buf;
}

jbuf_t* jbuf_null(uint64_t sessid){
  jbuf_t* buf = jbuf_alloc(0);
  buf->sessid = sessid;
  buf->data = NULL;
  buf->size = 0;
  buf->ref = 0;
  return buf;
}

int       jbuf_zeros(jbuf_t* buf){
  if(buf->size>0){
    memset(buf->data,0,buf->size);
  }
  return 0;
}

int       jbuf_free(jbuf_t* buf){
  dhmem_deref(buf);
  return 0;
}

int       jbuf_copy(jbuf_t* dst,jbuf_t* src){
  int size = src->size;
  if(size>dst->size)size = dst->size;
  memcpy(dst->data,src->data,size);
  return 0;
}

int       jmat_dump(jmat_t* mat){
  if(mat->gpu){
    printf("===w %d h %d c %d d %d b %d p %p \n",
        mat->width,mat->height,mat->channel,mat->stride,mat->bit,mat->data);
    return 0;
  }
  printf("===w %d h %d c %d d %d b %d p %p [\n",
      mat->width,mat->height,mat->channel,mat->stride,mat->bit,mat->data);
  int rgb = (mat->channel==3)?1:0;
  if(mat->bit == 4){
    for(int m=0;m<3;m++){
      printf("[");
      float* pa = (float*)jmat_row(mat,m);
      for(int k=0;k<3;k++){
        if(rgb){
          printf("[%f %f %f]",pa[0],pa[1],pa[2]);
          pa+=3;
        }else{
          printf(" %f ",*pa++);
        }
      }
      if(rgb){
        pa = (float*)jmat_row(mat,m) + mat->width*mat->channel - 9;
      }else{
        pa = (float*)jmat_row(mat,m) + mat->width*mat->channel - 3;
      }
      //printf("\n====offset %ld\n",(char*)pa - mat->data);
      printf("====");
      for(int k=0;k<3;k++){
        if(rgb){
          printf("[%f %f %f]",pa[0],pa[1],pa[2]);
          pa+=3;
        }else{
          printf(" %f ",*pa++);
        }
      }
      printf("]\n");
    }
    for(int m=3;m>0;m--){
      printf("[");
      float* pa = (float*)jmat_row(mat,mat->height - m);
      for(int k=0;k<3;k++){
        if(rgb){
          printf("[%f %f %f]",pa[0],pa[1],pa[2]);
          pa+=3;
        }else{
          printf(" %f ",*pa++);
        }
      }
      if(rgb){
        pa = (float*)jmat_row(mat,mat->height - m) + mat->width*mat->channel - 9;
      }else{
        pa = (float*)jmat_row(mat,mat->height - m) + mat->width*mat->channel - 3;
      }
      printf("====");
      for(int k=0;k<3;k++){
        if(rgb){
          printf("[%f %f %f]",pa[0],pa[1],pa[2]);
          pa+=3;
        }else{
          printf(" %f ",*pa++);
        }
      }
      printf("]\n");
    }
  }else{
    for(int m=0;m<3;m++){
      printf("[");
      uint8_t* pa = (uint8_t*)jmat_row(mat,m);
      for(int k=0;k<3;k++){
        printf("[%d %d %d]",pa[0],pa[1],pa[2]);
        pa+=3;
      }
      pa = (uint8_t*)jmat_row(mat,m) + mat->width*mat->channel - 9;
      printf("====");
      for(int k=0;k<3;k++){
        printf("[%d %d %d]",pa[0],pa[1],pa[2]);
        pa+=3;
      }
      printf("]\n");
    }
    for(int m=3;m>0;m--){
      printf("[");
      uint8_t* pa = (uint8_t*)jmat_row(mat,mat->height - m);
      for(int k=0;k<3;k++){
        printf("[%d %d %d]",pa[0],pa[1],pa[2]);
        pa+=3;
      }
      pa = (uint8_t*)jmat_row(mat,mat->height - m) + mat->width*mat->channel - 9;
      printf("====");
      for(int k=0;k<3;k++){
        printf("[%d %d %d]",pa[0],pa[1],pa[2]);
        pa+=3;
      }
      printf("]\n");
    }
  }
  printf("]=====\n");
  return 0;
}

static void my_jmat_destroy(void* arg){
  jmat_t* mat = (jmat_t*)arg;
  if(!mat->buf.ref){
    dhmem_deref(mat->data);
    mat->data = NULL;
  }
  jbuf_t* buf = mat->buf.next;
  while(buf){
    jbuf_t* tbuf = buf;
    buf = buf->next;
    dhmem_deref(tbuf);
  }
  //if(mat->rmat)dhmem_deref(mat->rmat);
  //if(mat->bmat)dhmem_deref(mat->bmat);
  //printf("===jmat destroy %p \n",mat);
}

jmat_t* jmat_allocex(int w,int h,int c ,int d, int b,void* mem,dhmem_destroy_h fndestroy){
  int bit = b?b:1;
  int stride = d?d:w*c;
  int size = bit*stride*h;
  int realsize = 0;//mem?0:size;
  realsize = sizeof(jmat_t);
  jmat_t* mat = (jmat_t*)dhmem_alloc(realsize,fndestroy);
  //printf("===jmat alloc %p\n",mat);
  //printf("===jmat alloc %p \n",mat);
  memset(mat,0,realsize);
  jbuf_t* buf = (jbuf_t*)&mat->buf;
  mat->width = w;
  mat->height = h;
  mat->channel = c;
  mat->bit = bit;
  mat->stride = stride;
  buf->data = (char*)mem;
  buf->size = size;
  mat->data = buf->data;
  return mat;
}

jmat_t* jmat_null(){
  jmat_t* mat = (jmat_t*)dhmem_zalloc(sizeof(jmat_t),my_jmat_destroy);
  return mat;
}

jmat_t* jmat_alloc(int w,int h,int c ,int d, int b,void* mem){
  int bit = b?b:1;
  int stride = d?d:w*c;
  int size = bit*stride*h;
  int realsize = sizeof(jmat_t);
  jmat_t* mat = (jmat_t*)dhmem_zalloc(realsize,my_jmat_destroy);
  //printf("===jmat alloc %p\n",mat);
  //printf("===jmat alloc %p \n",mat);
  jbuf_t* buf = (jbuf_t*)&mat->buf;
  mat->width = w;
  mat->height = h;
  mat->channel = c;
  mat->bit = bit;
  mat->stride = stride;
  if(mem){
    buf->data = (char*)mem;
    buf->ref = 1;
  }else{
    buf->data = (char*)dhmem_zalloc(size,NULL);
  }
  buf->size = size;
  mat->data = buf->data;
  return mat;
}

jmat_t* jmat_crgb(int w,int h,uint8_t *mem){
  jmat_t* mat = jmat_alloc(w,h,3,0,1,mem);
  return mat;
}

char*   jmat_row(jmat_t* mat,int row){
  if(row>=mat->height)return NULL;
  int offset =  row*mat->stride*mat->bit;
  //printf("==row %d stride %d offset %d\n",row,mat->stride,offset);
  return mat->data + offset;
}

char*   jmat_item(jmat_t* mat,int col,int row){
  if(row>=mat->height)return NULL;
  if(col>=mat->width)return NULL;
  int offset =  row*mat->stride*mat->bit + col*mat->bit;
  return mat->data + offset;
}

int       jmat_zero(jmat_t* src){
  return jbuf_zeros(&src->buf);
}

jmat_t*   jmat_clone(jmat_t* mat){
  jmat_t* dst = NULL;
  dst = jmat_alloc(mat->width,mat->height,mat->channel,mat->stride,mat->bit,NULL);
  memcpy(dst->data,mat->data,mat->buf.size);
  return dst;
}

int     jmat_free(jmat_t* mat){
  if(mat) dhmem_deref(mat);
  //printf("===jmat free %p \n",mat);
  return 0;
}

int       jmat_copy(jmat_t* dst,jmat_t* src){
  if(dst->buf.size!=src->buf.size)return -1;
  memcpy(dst->data,src->data,dst->buf.size);
  return 0;
}

int jmat_reshape(jmat_t* mat,int w,int h){
  mat->width = w;
  mat->height = h;
  mat->stride = w*mat->channel;
  return 0;
}

int jmat_reroi(jmat_t* mat,jmat_t* src,int w,int h,int l,int t){
  int d = src->stride;
  int c = src->channel;
  int b = src->bit;
  int s = b*d*h;
  char* mem = src->data + t*d*b + l*c*b;
  //
  jbuf_t* buf = (jbuf_t*)&mat->buf;

  mat->width = w;
  mat->height = h;
  mat->channel = c;
  mat->bit = b;
  mat->stride = d;
  buf->data = (char*)mem;
  buf->size = s;
  mat->data = buf->data;
  mat->gpu = src->gpu;
  mat->buf.ref = 1;
  return 0;
}

jmat_t* jmat_roi(jmat_t* mat,int w,int h,int l,int t){
  int d = mat->stride;
  int c = mat->channel;
  int b = mat->bit;
  char* roidata = mat->data + t*d*b + l*c*b;
  jmat_t* roimat = jmat_alloc(w,h,c,d,b,roidata);
  roimat->gpu = mat->gpu;
  return roimat;
}


uint64_t jtimer_msstamp(){
  struct timespec ts;
#ifdef WIN32
  //return clock();
  clock_gettime(0, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
  return (ts.tv_sec*1000l) + (ts.tv_nsec/CLOCKS_PER_SEC);
}


void jtimer_mssleep(int ms) {
#ifdef WIN32
  Sleep(ms);
#else
  /*
     struct timeval delay;
     delay.tv_sec = 0;
     delay.tv_usec = ms * 1000; // 20 ms
     select(0, NULL, NULL, NULL, &delay);
     */
  usleep(ms*1000);
#endif
}

#ifndef GJ_DHMEM_H
#define GJ_DHMEM_H
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (dhmem_destroy_h)(void *data);

char    *dhstr_dup(char* txt);
void    *dhmem_alloc(size_t size, dhmem_destroy_h *dh);
void    *dhmem_zalloc(size_t size, dhmem_destroy_h *dh);
void    *dhmem_realloc(void *data, size_t size);
void    *dhmem_reallocarray(void *ptr, size_t nmemb,
			  size_t membsize, dhmem_destroy_h *dh);
void     dhmem_destructor(void *data, dhmem_destroy_h *dh);
void    *dhmem_ref(void *data);
void    *dhmem_deref(void *data);
uint32_t dhmem_nrefs(const void *data);


#ifdef __cplusplus
}
#endif

#endif

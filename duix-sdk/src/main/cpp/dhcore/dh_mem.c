/**
 * @file mem.c  Memory management with reference counting
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "dh_atomic.h"
#include "dh_mem.h"





/** Defines a reference-counting memory object */
struct dhmem {
	DH_ATOMIC uint32_t nrefs; /**< Number of references  */
	uint32_t size;         /**< Size of memory object */
	dhmem_destroy_h *dh;     /**< Destroy handler       */
};


#define STAT_ALLOC(_m, _size) (_m)->size = (uint32_t)(_size);
#define STAT_REALLOC(_m, _size) (_m)->size = (uint32_t)(_size);
#define STAT_DEREF(_m)
#define MAGIC_CHECK(_m)


enum {
#if defined(__x86_64__)
	/* Use 16-byte alignment on x86-x32 as well */
	dhmem_alignment = 16u,
#else
	dhmem_alignment = sizeof(void*) >= 8u ? 16u : 8u,
#endif
	alignment_mask = dhmem_alignment - 1u,
	dhmem_header_size = (sizeof(struct dhmem) + alignment_mask) &
		(~(size_t)alignment_mask)
};

#define MEM_SIZE_MAX \
	(size_t)(sizeof(size_t) > sizeof(uint32_t) ? \
		(~(uint32_t)0u) : (~(size_t)0u) - dhmem_header_size)


static inline struct dhmem *get_mem(void *p)
{
	return (struct dhmem *)(void *)(((unsigned char *)p) - dhmem_header_size);
}


static inline void *get_dhmem_data(struct dhmem *m)
{
	return (void *)(((unsigned char *)m) + dhmem_header_size);
}

char    *dhstr_dup(char* txt){
  int len = strlen(txt);
  char* str = (char*)dhmem_zalloc(len+1,NULL);
  memcpy(str,txt,len);
  return str;
}

/**
 * Allocate a new reference-counted memory object
 *
 * @param size Size of memory object
 * @param dh   Optional destructor, called when destroyed
 *
 * @return Pointer to allocated object
 */
void *dhmem_alloc(size_t size, dhmem_destroy_h *dh)
{
	struct dhmem *m;

	if (size > MEM_SIZE_MAX)
		return NULL;


	m = (struct dhmem*)malloc(dhmem_header_size + size);
	if (!m)
		return NULL;

	dh_atomic_rlx_set(&m->nrefs, 1u);
	m->dh    = dh;

	STAT_ALLOC(m, size);

	return get_dhmem_data(m);
}


/**
 * Allocate a new reference-counted memory object. Memory is zeroed.
 *
 * @param size Size of memory object
 * @param dh   Optional destructor, called when destroyed
 *
 * @return Pointer to allocated object
 */
void *dhmem_zalloc(size_t size, dhmem_destroy_h *dh)
{
	void *p;

	p = dhmem_alloc(size, dh);
	if (!p)
		return NULL;

	memset(p, 0, size);

	return p;
}


/**
 * Re-allocate a reference-counted memory object
 *
 * @param data Memory object
 * @param size New size of memory object
 *
 * @return New pointer to allocated object
 *
 * @note Realloc NULL pointer is not supported
 */
void *dhmem_realloc(void *data, size_t size)
{
	struct dhmem *m, *m2;

	if (!data)
		return NULL;

	if (size > MEM_SIZE_MAX)
		return NULL;

	m = get_mem(data);

	MAGIC_CHECK(m);

	if (dh_atomic_acq(&m->nrefs) > 1u) {
		void* p = dhmem_alloc(size, m->dh);
		if (p) {
			memcpy(p, data, m->size);
			dhmem_deref(data);
		}
		return p;
	}


	m2 = (struct dhmem*)realloc(m, dhmem_header_size + size);

	if (!m2) {
		return NULL;
	}

	STAT_REALLOC(m2, size);

	return get_dhmem_data(m2);
}


/**
 * Re-allocate a reference-counted array
 *
 * @param ptr      Pointer to existing array, NULL to allocate a new array
 * @param nmemb    Number of members in array
 * @param membsize Number of bytes in each member
 * @param dh       Optional destructor, only used when ptr is NULL
 *
 * @return New pointer to allocated array
 */
void *dhmem_reallocarray(void *ptr, size_t nmemb, size_t membsize,
		       dhmem_destroy_h *dh)
{
	size_t tsize;

	if (membsize && nmemb > MEM_SIZE_MAX / membsize) {
		return NULL;
	}

	tsize = nmemb * membsize;

	if (ptr) {
		return dhmem_realloc(ptr, tsize);
	}
	else {
		return dhmem_alloc(tsize, dh);
	}
}


/**
 * Set or unset a destructor for a memory object
 *
 * @param data Memory object
 * @param dh   called when destroyed, NULL for remove
 */
void dhmem_destructor(void *data, dhmem_destroy_h *dh)
{
	struct dhmem *m;

	if (!data)
		return;

	m = get_mem(data);

	MAGIC_CHECK(m);

	m->dh = dh;
}


/**
 * Reference a reference-counted memory object
 *
 * @param data Memory object
 *
 * @return Memory object (same as data)
 */
void *dhmem_ref(void *data)
{
	struct dhmem *m;

	if (!data)
		return NULL;

	m = get_mem(data);

	MAGIC_CHECK(m);

	dh_atomic_rlx_add(&m->nrefs, 1u);

	return data;
}


/**
 * Dereference a reference-counted memory object. When the reference count
 * is zero, the destroy handler will be called (if present) and the memory
 * will be freed
 *
 * @param data Memory object
 *
 * @return Always NULL
 */
/* coverity[-tainted_data_sink: arg-0] */
void *dhmem_deref(void *data)
{
	struct dhmem *m;

	if (!data)
		return NULL;

	m = get_mem(data);

	MAGIC_CHECK(m);

	if (dh_atomic_acq_sub(&m->nrefs, 1u) > 1u) {
		return NULL;
	}

	if (m->dh)
		m->dh(data);

	/* NOTE: check if the destructor called dhmem_ref() */
	if (dh_atomic_rlx(&m->nrefs) > 0u)
		return NULL;


	STAT_DEREF(m);

	free(m);

	return NULL;
}


/**
 * Get number of references to a reference-counted memory object
 *
 * @param data Memory object
 *
 * @return Number of references
 */
uint32_t dhmem_nrefs(const void *data)
{
	struct dhmem *m;

	if (!data)
		return 0;

	m = get_mem((void*)data);

	MAGIC_CHECK(m);

	return (uint32_t)dh_atomic_acq(&m->nrefs);
}



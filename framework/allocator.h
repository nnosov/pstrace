/*
 * allocator.h
 *
 *  Created on: Jan 28, 2020
 *      Author: nnosov
 */

#ifndef PST_ALLOCATOR_H_
#define PST_ALLOCATOR_H_

#include <stdint.h>
#include <assert.h>

// concatenation
#define CAT(a, ...) CAT2(a, __VA_ARGS__)
#define CAT2(a, ...) a ## __VA_ARGS__

// declaration with initialization
#define pst_decl(TYPE, NAME, ...) \
    TYPE NAME; CAT2(TYPE, _init) (&NAME, __VA_ARGS__);

// allocation with initialization
#define pst_alloc(TYPE, NAME, ...) \
    TYPE* NAME; NAME = CAT2(TYPE, _new) (__VA_ARGS__);

// de-initialization and deletion if was previously allocated
#define pst_free(TYPE, NAME) \
    CAT2(TYPE, _fini) (NAME);

typedef struct __pst_allocator pst_allocator;

typedef void* (*pst_alloc_def)(pst_allocator* alloc, uint32_t size);
typedef void (*pst_free_def)(pst_allocator* alloc, void* buff);
typedef void* (*pst_realloc_def)(pst_allocator* alloc, void* buff, uint32_t new_size);

typedef enum {
    ALLOC_NONE = 0,     // not initialized
    ALLOC_HEAP = 1,     // use libc memory allocator
    ALLOC_CUSTOM = 2    // use custom allocator in predefined range of memory
} pst_alloc_type;

typedef struct __pst_allocator {
    int             type;
    void*           base;
    uint32_t        size;
    // routines
    pst_alloc_def   alloc;
    pst_free_def    free;
    pst_realloc_def realloc;
} pst_allocator;

void pst_alloc_init(pst_allocator* alloc);
void pst_alloc_init_custom(pst_allocator* alloc, void* buff, uint32_t size);
void pst_alloc_fini(pst_allocator* alloc);

char* pst_dup(pst_allocator* alloc, const char* str);

#endif /* PST_ALLOCATOR_H_ */

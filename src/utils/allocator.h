/*
 * allocator.h
 *
 *  Created on: Jan 28, 2020
 *      Author: nnosov
 */

#ifndef __PST_ALLOCATOR_H__
#define __PST_ALLOCATOR_H__

#include <stdint.h>
#include <assert.h>
#include <pthread.h>

// concatenation
#define CAT(a, ...) CAT2(a, __VA_ARGS__)
#define CAT2(a, ...) a ## __VA_ARGS__

// declaration with initialization and parameters of initialization
#define pst_decl(TYPE, NAME, ...) \
    TYPE NAME; CAT2(TYPE, _init) (&NAME, __VA_ARGS__);

// declaration with initialization
#define pst_decl0(TYPE, NAME) \
    TYPE NAME; CAT2(TYPE, _init) (&NAME);

// allocation with initialization
#define pst_new(TYPE, NAME, ...) \
    TYPE* NAME; NAME = CAT2(TYPE, _new) (__VA_ARGS__);

// de-initialization and deletion if was previously allocated
#define pst_fini(TYPE, NAME) \
    CAT2(TYPE, _fini) (NAME);

typedef enum {
    ALLOC_NONE = 0,     // not initialized
    ALLOC_HEAP = 1,     // use libc memory allocator
    ALLOC_CUSTOM = 2    // use custom allocator in predefined range of memory
} pst_alloc_type;

typedef struct __pst_allocator {
    // methods
    void* (*alloc)(struct __pst_allocator* alloc, uint32_t size);
    void (*free)(struct __pst_allocator* alloc, void* buff);
    void* (*realloc)(struct __pst_allocator* alloc, void* buff, uint32_t new_size);

    // fields
    int             type;
    void*           base;
    uint32_t        size;
    pthread_mutex_t lock;

} pst_allocator;

void pst_alloc_init(pst_allocator* alloc);
void pst_alloc_init_custom(pst_allocator* alloc, void* buff, uint32_t size);
void pst_alloc_fini(pst_allocator* alloc);

#endif /* __PST_ALLOCATOR_H__ */

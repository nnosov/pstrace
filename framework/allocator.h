/*
 * allocator.h
 *
 *  Created on: Jan 28, 2020
 *      Author: nnosov
 */

#ifndef FRAMEWORK_ALLOCATOR_H_
#define FRAMEWORK_ALLOCATOR_H_

#include <stddef.h>
#include <stdint.h>

typedef struct __pst_allocator pst_allocator;

typedef void* (*pst_alloc_def)(pst_allocator& alloc, uint32_t size);
typedef void (*pst_free_def)(pst_allocator& alloc, void* buff);
typedef void* (*pst_realloc_def)(pst_allocator& alloc, void* buff, uint32_t new_size);

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

void pst_alloc_init(pst_allocator& alloc);
void pst_alloc_init_custom(pst_allocator& alloc, void* buff, uint32_t size);
void pst_alloc_fini(pst_allocator& alloc);

#endif /* FRAMEWORK_ALLOCATOR_H_ */

/*
 * allocator.cpp
 *
 *  Created on: Jan 28, 2020
 *      Author: nnosov
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "common.h"
#include "allocator.h"

void heap_free(pst_allocator* alloc, void* buff)
{
    alloc->size -= malloc_usable_size(buff);
    free(buff);
}

void* heap_alloc(pst_allocator* alloc, uint32_t size)
{
    void* buff = malloc(size);
    alloc->size += malloc_usable_size(buff);

    return buff;
}

void* heap_realloc(pst_allocator* alloc, void* buff, uint32_t new_size)
{
    alloc->size -= malloc_usable_size(buff);
    void* new_buff = realloc(buff, new_size);
    alloc->size += malloc_usable_size(new_buff);

    return new_buff;
}

void pst_alloc_init(pst_allocator* alloc)
{
    alloc->type = ALLOC_HEAP;
    alloc->base = NULL;
    alloc->size = 0;

    alloc->alloc = heap_alloc;
    alloc->free = heap_free;
    alloc->realloc = heap_realloc;

    return;
}

void pst_alloc_init_custom(pst_allocator* alloc, void* buff, uint32_t size)
{
    // TBD to implement custom allocator
    pst_alloc_init(alloc);
    return;
}

void pst_alloc_fini(pst_allocator* alloc)
{
    if(alloc->type == ALLOC_HEAP || alloc->type == ALLOC_CUSTOM) {
        alloc->type = ALLOC_NONE;
        alloc->base = NULL;
        alloc->size = 0;
    }
}

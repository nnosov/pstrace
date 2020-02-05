/*
 * libpst.c
 *
 *  Created on: Feb 5, 2020
 *      Author: nnosov
 */

#include <ucontext.h>
#include <stdio.h>
#include <stdint.h>

#include "utils/allocator.h"
#include "dwarf/dwarf_handler.h"

// allocate and initialize libpst handler.
pst_handler* pst_new_handler(ucontext_t* hctx)
{
    // global
    pst_alloc_init(&allocator);
    pst_log_init_console(&logger);

    pst_new(pst_handler, handler, hctx);
    return handler;
}

// deallocate libpst handler
void pst_del_handler(pst_handler* h)
{
    pst_free(h);

    // global
    pst_log_fini(&logger);
    pst_alloc_fini(&allocator);
}

// write stack trace information to provided file descriptor
int pst_unwind_simple_fd(pst_handler* h, FILE* fd)
{
    int ret = pst_handler_unwind(h);
    if(ret) {
        fwrite(h->ctx.buff, 1, strlen(h->ctx.buff), fd);
        return 0;
    }

    return -1;
}

// save stack trace information to provided buffer in RAM
int pst_unwind_simple(pst_handler* h, char* buff, uint32_t buff_size)
{
    int ret = pst_handler_unwind(h);
    if(ret) {
        uint32_t cnt = buff_size < sizeof(h->ctx.buff) ? buff_size : sizeof(h->ctx.buff);
        memcpy(buff, h->ctx.buff, cnt - 2);
        buff[cnt - 1] = 0;

        return 0;
    }

    return -1;
}


// write stack trace information to provided file descriptor
int pst_unwind_pretty_fd(pst_handler* h, FILE* fd)
{
    int ret = pst_handler_handle_dwarf(h);
    if(ret) {
        pst_handler_print_dwarf(h);
        fwrite(h->ctx.buff, 1, strlen(h->ctx.buff), fd);
        return 0;
    }

    return -1;
}

// save stack trace information to provided buffer in RAM
int pst_unwind_pretty(pst_handler* h, char* buff, uint32_t buff_size)
{
    int ret = pst_handler_handle_dwarf(h);
    if(ret) {
        pst_handler_print_dwarf(h);
        uint32_t cnt = buff_size < sizeof(h->ctx.buff) ? buff_size : sizeof(h->ctx.buff);
        memcpy(buff, h->ctx.buff, cnt - 2);
        buff[cnt - 1] = 0;

        return 0;
    }

    return -1;
}

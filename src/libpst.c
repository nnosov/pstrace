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
#include "dwarf/dwarf_parameter.h"

// allocate and initialize libpst library
pst_handler* pst_lib_init(ucontext_t* hctx, void* buff, uint32_t size)
{
    // global
    if(!buff || !size) {
        pst_alloc_init(&allocator);
    } else {
        pst_alloc_init_custom(&allocator, buff, size);
    }
    pst_log_init_console(&logger);

    pst_new(pst_handler, handler, hctx);
    return handler;
}

// deallocate libpst handler and library
void pst_lib_fini(pst_handler* h)
{
    pst_free(h);

    // global
    pst_log_fini(&logger);
    pst_alloc_fini(&allocator);
}

// save stack trace information to provided buffer in RAM
int pst_unwind_simple(pst_handler* h)
{
    return pst_handler_unwind(h);
}

// save stack trace information to provided buffer in RAM
int pst_unwind_pretty(pst_handler* h)
{
    return pst_handler_handle_dwarf(h);
}

pst_parameter_info* pst_get_parameter_info(pst_parameter* parameter)
{
    return &parameter->info;
}

pst_function_info* pst_get_function_info(pst_function* function)
{
    return &function->info;
}

pst_function* pst_function_next(pst_handler* handler, pst_function* current)
{
    return pst_handler_next_function(handler, current);
}

pst_parameter* pst_parameter_next(pst_function* function, pst_parameter* current)
{
    return function_next_parameter(function, current);
}

pst_parameter* pst_parameter_next_child(pst_parameter* parent, pst_parameter* current)
{
    return parameter_next_child(parent, current);
}

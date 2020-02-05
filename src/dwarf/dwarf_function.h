/*
 * dwarf_function.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef __PST_DWARF_FUNCTION_H__
#define __PST_DWARF_FUNCTION_H__

#include <elfutils/libdw.h>
#include <libunwind.h>

#include "utils/list_head.h"
#include "context.h"
#include "dwarf_call_site.h"
#include "dwarf_expression.h"
#include "dwarf_parameter.h"

// -----------------------------------------------------------------------------------
// pst_function
// -----------------------------------------------------------------------------------
typedef struct __pst_function {
    list_node               node;

    Dwarf_Addr              lowpc;      // offset to start of the function against base address
    Dwarf_Addr              highpc;     // offset to the next address after the end of the function against base address
    unw_word_t              pc;         // address between LowPC & HighPC (plus base address offset). actually, currently executed command
    Dwarf_Die*              die;        // DWARF DIE containing definition of the function
    char*                   name;       // function's name
    list_head               params;     // function's parameters
    pst_call_site_storage   call_sites;

    unw_word_t              sp;         // SP register in function's frame
    unw_word_t              cfa;        // CFA (Canonical Frame Address) of the function
    unw_cursor_t            cursor;     // copy of stack state of the function
    int                     line;       // line in code where function is defined
    char*                   file;       // file name (DWARF Compilation Unit) where function is defined
    struct __pst_function*  parent;     // parent function in call trace (caller)
    Dwarf_Frame*            frame;      // function's stack frame
    pst_context*            ctx;        // context of unwinding
    bool                    allocated;  // whether this object was allocated or not
} pst_function;
void pst_function_init(pst_function* fn, pst_context* _ctx, pst_function* _parent);
pst_function* pst_function_new(pst_context* _ctx, pst_function* _parent);
void pst_function_fini(pst_function* fn);

bool pst_function_unwind(pst_function* fn, Dwarf_Addr addr);
bool pst_function_handle_dwarf(pst_function * fn, Dwarf_Die* d);
bool pst_function_print_dwarf(pst_function* fn);

#endif /* __PST_DWARF_FUNCTION_H__ */

/*
 * context.h
 *
 *  Created on: Jan 28, 2020
 *      Author: nnosov
 */

#ifndef __PST_CONTEXT_H__
#define __PST_CONTEXT_H__

#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <libunwind.h>
#include <elfutils/libdwfl.h>
#include <stdbool.h>

#include "utils/allocator.h"
#include "utils/log.h"

// uncomment line below to enable debug output to stdout
//#define PST_DEBUG

extern pst_logger       pstlogger;     // logger for whole PST library
extern pst_allocator    allocator;  // custom allocator for PST library

#define pst_alloc(TYPE) (TYPE*)allocator.alloc(&allocator, sizeof(TYPE))
#define pst_free(NAME) allocator.free(&allocator, NAME)

#ifdef PST_DEBUG
#define pst_log(SEVERITY, FORMAT, ...) pstlogger.log(&logger, SEVERITY, FORMAT, ##__VA_ARGS__)
#else
#define pst_log(SEVERITY, FORMAT, ...)
#endif

char* pst_strdup(const char* str);

typedef struct __pst_context {
    // methods
    void (*clean_print)     (struct __pst_context* ctx);
    bool (*print)           (struct __pst_context* ctx, const char* fmt, ...);
    bool (*print_expr)      (struct __pst_context* ctx, Dwarf_Op *exprs, int exprlen, Dwarf_Attribute* attr);
    void (*print_registers) (struct __pst_context* ctx, int from, int to);
    void (*print_stack)     (struct __pst_context* ctx, int max, uint64_t next_cfa);

    // fields
    ucontext_t*                 hcontext;   // context of signal handler
    unw_context_t               context;    // context of stack trace
    unw_cursor_t                cursor;     // libunwind stack frame storage
    unw_cursor_t*               curr_frame; // callee libunwind frame
    Dwarf_Addr                  base_addr;  // base address where process loaded

    Dwarf_Addr                  sp;         // stack pointer of currently processed stack frame
    Dwarf_Addr                  cfa;        // CFA (Canonical Frame Address) of currently processed stack frame

    Dwarf_Frame*                frame;      // currently examined libdwfl frame
    Dwfl*                       dwfl;       // DWARF context
    Dwfl_Module*                module;     // currently processed CU

    char                        buff[8192]; // stack trace buffer
    uint32_t                    offset;     // offset in the 'buff'
} pst_context;

void pst_context_init(pst_context* ctx, ucontext_t* hctx);
void pst_context_fini(pst_context* ctx);

#endif /* __PST_CONTEXT_H__ */

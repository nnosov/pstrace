/*
 * sysutils.cpp
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include <elfutils/libdwfl.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <dwarf.h>
#include <inttypes.h>

#include "sysutils.h"
#include "logger/log.h"

#define USE_LIBUNWIND

#ifdef USE_LIBUNWIND
#include <libunwind.h>
#endif

#include "common.h"
#include "dwarf_operations.h"
#include "allocator.h"
#include "dwarf_stack.h"

// dwfl_addrsegment() possibly can be used to check address validity
// dwarf_getattrs() allows to enumerate all DIE attributes
// dwarf_getfuncs() allows to enumerate functions within CU

bool handler_get_dwarf_function(pst_handler* h, pst_function* fun)
{
    Dwarf_Addr mod_cu = 0;
    // get CU(Compilation Unit) debug definition
    h->ctx.module = dwfl_addrmodule(h->ctx.dwfl, fun->pc);
    Dwarf_Die* cdie = dwfl_module_addrdie(h->ctx.module, fun->pc, &mod_cu);
    //Dwarf_Die* cdie = dwfl_addrdie(dwfl, addr, &mod_bias);
    if(!cdie) {
        h->ctx.log(SEVERITY_INFO, "Failed to find DWARF DIE for address %X", fun->pc);
    	return false;
    }

    if(dwarf_tag(cdie) != DW_TAG_compile_unit) {
        h->ctx.log(SEVERITY_DEBUG, "Skipping non-cu die. DWARF tag: 0x%X, name = %s", dwarf_tag(cdie), dwarf_diename(cdie));
    	return false;
    }

    Dwarf_Die result;
	if(dwarf_child(cdie, &result)) {
	    h->ctx.log(SEVERITY_ERROR, "No child DIE found for CU %s", dwarf_diename(cdie));
		return false;
	}

	bool nret = false;

	do {
		int tag = dwarf_tag(&result);
		if(tag == DW_TAG_subprogram || tag == DW_TAG_entry_point || tag == DW_TAG_inlined_subroutine) {
		    //ctx.log(SEVERITY_DEBUG, "function die name %s", dwarf_diename(&result));
			if(!strcmp(fun->name, dwarf_diename(&result))) {
				return fun->handle_dwarf(fun, &result);
			}
		}
	} while(dwarf_siblingof(&result, &result) == 0);

	return nret;
}

pst_function* handler_add_function(pst_handler* h, pst_function* parent)
{
    pst_new(pst_function, fn, &h->ctx, parent);
    list_add_bottom(&h->functions, &fn->node);

    return fn;
}

void handler_del_function(pst_function* fn)
{
    list_del(&fn->node);
    pst_function_fini(fn);
}

void handler_clear(pst_handler* h)
{
    pst_function*  fn = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(fn, pos, tn, &h->functions, node) {
        list_del(&fn->node);
        pst_function_fini(fn);
    }
}

pst_function* handler_next_function(pst_handler* h, pst_function* fn)
{
    list_node* n = (fn == NULL) ? list_first(&h->functions) : list_next(&fn->node);
    pst_function* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_function, node);
    }

    return ret;
}

pst_function* handler_prev_function(pst_handler* h, pst_function* fn)
{
    list_node* n = (fn == NULL) ? list_last(&h->functions) : list_prev(&fn->node);
    pst_function* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_function, node);
    }

    return ret;
}

pst_function* handler_last_function(pst_handler* h)
{
    list_node* n = list_last(&h->functions);
    if(n) {
        return list_entry(n, pst_function, node);
    }

    return NULL;
}

bool handler_handle_dwarf(pst_handler* h)
{
    h->ctx.clean_print(&h->ctx);
    Dl_info info;

    //for(pst_function* fun = next_function(NULL); fun; fun = next_function(fun)) {
    for(pst_function* fun = (pst_function*)handler_last_function(h); fun; fun = h->prev_function(h, fun)) {
        dladdr((void*)(fun->pc), &info);
        h->ctx.log(SEVERITY_INFO, "Function %s(...): module name: %s, base address: %p, CFA: %#lX", fun->name, info.dli_fname, info.dli_fbase, fun->parent ? fun->parent->sp : 0);

        // setup context
        h->ctx.module       = dwfl_addrmodule(h->ctx.dwfl, fun->pc);
        h->ctx.base_addr    = (uint64_t)info.dli_fbase;
        h->ctx.curr_frame   = &fun->cursor;
        h->ctx.sp           = fun->sp;
        h->ctx.cfa          = fun->cfa;
        h->ctx.curr_frame   = &fun->cursor;

        handler_get_dwarf_function(h, fun);
    }

    return true;
}

void handler_print_dwarf(pst_handler* h)
{
    h->ctx.clean_print(&h->ctx);
    h->ctx.print(&h->ctx, "DWARF-based stack trace information:\n");
    uint32_t idx = 0;
    for(pst_function* fn = h->next_function(h, NULL); fn; fn = h->next_function(h, fn)) {
        h->ctx.print(&h->ctx, "[%-2u] ", idx); idx++;
        fn->print_dwarf(fn);
        h->ctx.print(&h->ctx, "\n");
    }
}

char *debuginfo_path = NULL;
Dwfl_Callbacks callbacks = {
        .find_elf           = dwfl_linux_proc_find_elf,
        .find_debuginfo     = dwfl_standard_find_debuginfo,
        .section_address    = dwfl_offline_section_address,
        .debuginfo_path     = &debuginfo_path,
};

#include <dlfcn.h>

bool handler_unwind(pst_handler* h)
{
    h->ctx.clean_print(&h->ctx);
#ifdef REG_RIP // x86_64
    h->caller = (void *) h->ctx.hcontext->uc_mcontext.gregs[REG_RIP];
    h->ctx.sp = h->ctx.hcontext->uc_mcontext.gregs[REG_RSP];
    h->ctx.log(SEVERITY_DEBUG, "Original caller's SP: %#lX", h->ctx.sp);
#elif defined(REG_EIP) // x86_32
    caller_address = (void *) uctx->uc_mcontext.gregs[REG_EIP]);
#elif defined(__arm__)
    caller_address = (void *) uctx->uc_mcontext.arm_pc);
#elif defined(__aarch64__)
    caller_address = (void *) uctx->uc_mcontext.pc);
#elif defined(__ppc__) || defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__)
    caller_address = (void *) uctx->uc_mcontext.regs->nip);
#elif defined(__s390x__)
    caller_address = (void *) uctx->uc_mcontext.psw.addr);
#elif defined(__APPLE__) && defined(__x86_64__)
    caller_address = (void *) uctx->uc_mcontext->__ss.__rip);
#else
#   error "unknown architecture!"
#endif

    //handle = dlopen(NULL, RTLD_NOW);
	Dl_info info;
	dladdr(h->caller, &info);
	h->ctx.base_addr = (uint64_t)info.dli_fbase;
	h->ctx.log(SEVERITY_INFO, "Process address information: PC address: %p, base address: %p, object name: %s", h->caller, info.dli_fbase, info.dli_fname);

	h->ctx.dwfl = dwfl_begin(&callbacks);
    if(h->ctx.dwfl == NULL) {
        h->ctx.print(&h->ctx, "Failed to initialize libdw session for parse stack frames");
        return false;
    }

    if(dwfl_linux_proc_report(h->ctx.dwfl, getpid()) != 0 || dwfl_report_end(h->ctx.dwfl, NULL, NULL) !=0) {
        h->ctx.print(&h->ctx, "Failed to parse debug section of executable");
    	return false;
    }

    h->ctx.print(&h->ctx, "Stack trace: caller = %p\n", h->caller);

    unw_getcontext(&h->ctx.context);
    unw_init_local(&h->ctx.cursor, &h->ctx.context);
    h->ctx.curr_frame = &h->ctx.cursor;
    for(int i = 0, skip = 1; unw_step(h->ctx.curr_frame) > 0; ++i) {
        if(unw_get_reg(h->ctx.curr_frame, UNW_REG_IP,  &h->addr)) {
            h->ctx.log(SEVERITY_DEBUG, "Failed to get IP value");
            continue;
        }
        unw_word_t  sp;
        if(unw_get_reg(h->ctx.curr_frame, UNW_REG_SP,  &sp)) {
            h->ctx.log(SEVERITY_DEBUG, "Failed to get SP value");
            continue;
        }

        if(h->addr == (uint64_t)h->caller) {
            skip = 0;
        } else if(skip) {
            h->ctx.log(SEVERITY_DEBUG, "Skipping frame #%d: PC = %#lX, SP = %#lX", i, h->addr, sp);
            continue;
        }

        h->ctx.print(&h->ctx, "[%-2d] ", i);
        h->ctx.module = dwfl_addrmodule(h->ctx.dwfl, h->addr);
        pst_function* last = handler_last_function(h);
        pst_function* fn = h->add_function(h, NULL);
        fn->sp = sp;
        h->ctx.log(SEVERITY_DEBUG, "Analyze frame #%d: PC = %#lX, SP = %#lX", i, h->addr, sp);
        if(!fn->unwind(fn, h->addr)) {
            h->del_function(fn);
        } else {
            if(last) {
                last->parent = fn;
            }
            //get_frame(fun);
        }
        h->ctx.print(&h->ctx, "\n");
    }

   return true;
}

void pst_handler_init(pst_handler* h, ucontext_t* hctx)
{
    // methods
    h->clear = handler_clear;
    h->add_function = handler_add_function;
    h->del_function = handler_del_function;
    h->next_function = handler_next_function;
    h->prev_function = handler_prev_function;

    // fields
    pst_context_init(&h->ctx, hctx);
    h->handle = NULL;
    h->addr = 0;
    h->frame = NULL;
    h->caller = NULL;
    list_head_init(&h->functions);
}

void pst_handler_fini(pst_handler* h)
{
    if(h->handle) {
        dlclose(h->handle);
    }

    h->clear(h);
    pst_context_fini(&h->ctx);
}


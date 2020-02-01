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

bool __pst_handler::get_dwarf_function(pst_function* fun)
{
    Dwarf_Addr mod_cu = 0;
    // get CU(Compilation Unit) debug definition
    ctx.module = dwfl_addrmodule(ctx.dwfl, fun->pc);
    Dwarf_Die* cdie = dwfl_module_addrdie(ctx.module, fun->pc, &mod_cu);
    //Dwarf_Die* cdie = dwfl_addrdie(dwfl, addr, &mod_bias);
    if(!cdie) {
    	ctx.log(SEVERITY_INFO, "Failed to find DWARF DIE for address %X", fun->pc);
    	return false;
    }

    if(dwarf_tag(cdie) != DW_TAG_compile_unit) {
    	ctx.log(SEVERITY_DEBUG, "Skipping non-cu die. DWARF tag: 0x%X, name = %s", dwarf_tag(cdie), dwarf_diename(cdie));
    	return false;
    }

    Dwarf_Die result;
	if(dwarf_child(cdie, &result)) {
		ctx.log(SEVERITY_ERROR, "No child DIE found for CU %s", dwarf_diename(cdie));
		return false;
	}

	bool nret = false;

	do {
		int tag = dwarf_tag(&result);
		if(tag == DW_TAG_subprogram || tag == DW_TAG_entry_point || tag == DW_TAG_inlined_subroutine) {
		    //ctx.log(SEVERITY_DEBUG, "function die name %s", dwarf_diename(&result));
			if(!strcmp(fun->name.c_str(), dwarf_diename(&result))) {
				return fun->handle_dwarf(&result);
			}
		}
	} while(dwarf_siblingof(&result, &result) == 0);

	return nret;
}

pst_function* __pst_handler::add_function(__pst_function* parent)
{
    pst_function* f = new pst_function(&ctx, parent);
    functions.InsertLast(f);

    return f;
}

void __pst_handler::del_function(pst_function* f)
{
    functions.Remove(f);
    delete f;
}

void __pst_handler::clear()
{
    for(pst_function* f = (pst_function*)functions.First(); f; f = (pst_function*)functions.First()) {
        functions.Remove(f);
        delete f;
    }
}

pst_function* __pst_handler::next_function(pst_function* f)
{
    pst_function* next = NULL;
    if(!f) {
        next = (pst_function*)functions.First();
    } else {
        next = (pst_function*)functions.Next(f);
    }

    return next;
}

pst_function* __pst_handler::last_function()
{
    return (pst_function*)functions.Last();
}

bool __pst_handler::handle_dwarf()
{
    ctx.clean_print(&ctx);
    Dl_info info;

    //for(pst_function* fun = next_function(NULL); fun; fun = next_function(fun)) {
    for(pst_function* fun = (pst_function*)functions.Last(); fun; fun = (pst_function*)functions.Prev(fun)) {
        dladdr((void*)(fun->pc), &info);
        ctx.module = dwfl_addrmodule(ctx.dwfl, fun->pc);
        ctx.base_addr = (uint64_t)info.dli_fbase;
        ctx.log(SEVERITY_INFO, "Function %s(...): module name: %s, base address: %p, CFA: %#lX", fun->name.c_str(), info.dli_fname, info.dli_fbase, fun->parent ? fun->parent->sp : 0);
        ctx.curr_frame = &fun->cursor;
        ctx.sp = fun->sp;
        ctx.cfa = fun->cfa;

        ctx.curr_frame = &fun->cursor;
        get_dwarf_function(fun);
    }

    return true;
}

void __pst_handler::print_dwarf()
{
    ctx.clean_print(&ctx);
    ctx.print(&ctx, "DWARF-based stack trace information:\n");
    uint32_t idx = 0;
    for(pst_function* function = next_function(NULL); function; function = next_function(function)) {
        ctx.print(&ctx, "[%-2u] ", idx); idx++;
        function->print_dwarf();
        ctx.print(&ctx, "\n");
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

bool __pst_handler::unwind()
{
    ctx.clean_print(&ctx);
#ifdef REG_RIP // x86_64
    caller = (void *) ctx.hcontext->uc_mcontext.gregs[REG_RIP];
    ctx.sp = ctx.hcontext->uc_mcontext.gregs[REG_RSP];
    ctx.log(SEVERITY_DEBUG, "Original caller's SP: %#lX", ctx.sp);
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
	dladdr(caller, &info);
	ctx.base_addr = (uint64_t)info.dli_fbase;
	ctx.log(SEVERITY_INFO, "Process address information: PC address: %p, base address: %p, object name: %s", caller, info.dli_fbase, info.dli_fname);

    ctx.dwfl = dwfl_begin(&callbacks);
    if(ctx.dwfl == NULL) {
    	ctx.print(&ctx, "Failed to initialize libdw session for parse stack frames");
        return false;
    }

    if(dwfl_linux_proc_report(ctx.dwfl, getpid()) != 0 || dwfl_report_end(ctx.dwfl, NULL, NULL) !=0) {
    	ctx.print(&ctx, "Failed to parse debug section of executable");
    	return false;
    }

    ctx.print(&ctx, "Stack trace: caller = %p\n", caller);

    unw_getcontext(&ctx.context);
    unw_init_local(&ctx.cursor, &ctx.context);
    ctx.curr_frame = &ctx.cursor;
    for(int i = 0, skip = 1; unw_step(ctx.curr_frame) > 0; ++i) {
        if(unw_get_reg(ctx.curr_frame, UNW_REG_IP,  &addr)) {
            ctx.log(SEVERITY_DEBUG, "Failed to get IP value");
            continue;
        }
        unw_word_t  sp;
        if(unw_get_reg(ctx.curr_frame, UNW_REG_SP,  &sp)) {
            ctx.log(SEVERITY_DEBUG, "Failed to get SP value");
            continue;
        }

        if(addr == (uint64_t)caller) {
            skip = 0;
        } else if(skip) {
            ctx.log(SEVERITY_DEBUG, "Skipping frame #%d: PC = %#lX, SP = %#lX", i, addr, sp);
            continue;
        }

        ctx.print(&ctx, "[%-2d] ", i);
        ctx.module = dwfl_addrmodule(ctx.dwfl, addr);
        pst_function* last = last_function();
        pst_function* fun = add_function(NULL);
        fun->sp = sp;
        ctx.log(SEVERITY_DEBUG, "Analyze frame #%d: PC = %#lX, SP = %#lX", i, addr, sp);
        if(!fun->unwind(addr)) {
            del_function(fun);
        } else {
            if(last) {
                last->parent = fun;
            }
            //get_frame(fun);
        }
        ctx.print(&ctx, "\n");
    }

   return true;
}

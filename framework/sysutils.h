/*
 * sysutils.h
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */

#ifndef SC_SYSUTILS_H_
#define SC_SYSUTILS_H_

//system
#include <string>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <ucontext.h>
#include <libunwind.h>
#include <elfutils/libdwfl.h>
#include <dlfcn.h>
#include <stdlib.h>

#include "allocator.h"
#include "common.h"
#include "context.h"
#include "allocator.h"
#include "list_head.h"
#include "dwarf_expression.h"
#include "dwarf_parameter.h"


// DW_TAG_call_site_parameter
typedef struct __pst_call_site_param : public SC_ListNode {
    __pst_call_site_param(pst_context* ctx) {
        param = NULL; name = NULL; value = 0;
        pst_dwarf_expr_init(&location, ctx->alloc);
    }

    ~__pst_call_site_param() {
        if(name) {
            free(name); name = NULL;
        }
    }

    void set_location(Dwarf_Op* expr, size_t exprlen);

    Dwarf_Die*                  param;      // reference to parameter DIE in callee (DW_AT_call_parameter)
    char*                       name;       // name of parameter if present (DW_AT_name)
    pst_dwarf_expr              location;   // DWARF stack containing location expression
    uint64_t                    value;      // parameter's value
} pst_call_site_param;

// DW_TAG_call_site
typedef struct __pst_call_site : public SC_ListNode {
    __pst_call_site(pst_context* c, uint64_t tgt, const char* orn) {
        target = tgt;
        if(orn) {
            origin = strdup(orn);
        } else {
            origin = NULL;
        }
        die = NULL;
        ctx = c;
    }

    ~__pst_call_site() {
        if(origin) {
            free(origin);
            origin = NULL;
        }
    }

    pst_call_site_param* add_param();
    void del_param(pst_call_site_param* p);
    pst_call_site_param* next_param(pst_call_site_param* p);
    pst_call_site_param* find_param(pst_dwarf_expr& expr);

    bool handle_dwarf(Dwarf_Die* die);

    uint64_t                    target;     // pointer to callee function's (it's Low PC + base address)
    char*                       origin;     // name of callee function
    Dwarf_Die*                  die;        // DIE of function for which this call site has parameters
    SC_ListHead                 params;     // list of parameters and their values
    pst_context*                ctx;
} pst_call_site;

typedef struct __pst_function : public SC_ListNode {
	__pst_function(pst_context* _ctx, __pst_function* _parent) : ctx(_ctx) {
		pc = 0;
		line = -1;
		die = NULL;
		lowpc = 0;
		highpc = 0;
		memcpy(&cursor, _ctx->curr_frame, sizeof(cursor));
		parent = _parent;
		sp = 0;
		cfa = 0;
		frame = NULL;
	}

	~__pst_function() {
	    clear();
	    if(frame) {
	        free(frame);
	    }
	}

    void clear();
    pst_parameter* add_param();
    void del_param(pst_parameter* p);
    pst_parameter* next_param(pst_parameter* p);

    pst_call_site* add_call_site(uint64_t target, const char* origin);
    void del_call_site(pst_call_site* st);
    pst_call_site* next_call_site(pst_call_site* st);
    pst_call_site* call_site_by_origin(const char* origin);
    pst_call_site* call_site_by_target(uint64_t target);

	bool unwind(Dwarf_Addr addr);
	bool handle_dwarf(Dwarf_Die* d);
	bool print_dwarf();
	bool handle_lexical_block(Dwarf_Die* result);
	bool handle_call_site(Dwarf_Die* result);
	pst_call_site* find_call_site(__pst_function* callee);

	bool get_frame();

	Dwarf_Addr					lowpc;  // offset to start of the function against base address
	Dwarf_Addr					highpc; // offset to the next address after the end of the function against base address
	Dwarf_Die*					die; 	// DWARF DIE containing definition of the function
	std::string					name;	// function's name
	SC_ListHead	                params;	// function's parameters

	// call-site
	SC_ListHead                 call_sites;     // Call-Site definitions
	SC_Dict                     mCallStToTarget; // map pointer to caller to call-site
	SC_Dict                     mCallStToOrigin; // map caller name to call-site

	unw_word_t  				pc;		// address between lowpc & highpc (plus base address offset). actually, currently executed command
	unw_word_t                  sp;     // SP register in function's frame
	unw_word_t                  cfa;    // CFA (Canonical Frame Adress) of the function
	unw_cursor_t                cursor; // copy of stack state of the function
	int							line;	// line in code where function is defined
	std::string					file;	// file name (DWARF Compilation Unit) where function is defined
	__pst_function*             parent; // parent function in call trace (caller)
    Dwarf_Frame*                frame;  // function's stack frame
	pst_context*				ctx;    // context of unwinding
} pst_function;

typedef struct __pst_handler {
	__pst_handler(ucontext_t* hctx)
	{
		pst_context_init(&ctx, hctx);
		caller = NULL;
		frame = NULL;
		addr = 0;
		handle = 0;
	}

	~__pst_handler()
	{
		if(handle) {
			dlclose(handle);
		}

		clear();
		pst_context_fini(&ctx);
	}

    void clear();
    pst_function* add_function(__pst_function* fun);
    void del_function(pst_function* f);
    pst_function* next_function(pst_function* f);
    pst_function* last_function();

    bool handle_dwarf();
	void print_dwarf();
	bool unwind();
	bool get_dwarf_function(pst_function* fun);

	pst_context					ctx;		// context of unwinding
	void*						handle;		// process handle
	Dwarf_Addr 					addr;		// address of currently processed function
	Dwarf_Frame* 				frame;		// currently processed stack frame
	void*						caller;		// pointer to the function which requested to unwind stack
	SC_ListHead	                functions;	// list of functions in stack frame
} pst_handler;

#endif /* SC_SYSUTILS_H_ */

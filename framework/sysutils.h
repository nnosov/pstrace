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
#include "dwarf_utils.h"
#include "dwarf_function.h"

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
    pst_function* add_function(pst_function* fun);
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

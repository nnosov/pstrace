/*
 * sysutils.h
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */

#ifndef SC_SYSUTILS_H_
#define SC_SYSUTILS_H_

//system
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

typedef struct pst_handler {
	pst_context					ctx;		// context of unwinding
	list_head	                functions;	// list of functions in stack frame
} pst_handler;
void pst_handler_init(pst_handler* h, ucontext_t* hctx);
void pst_handler_fini(pst_handler* h);

bool pst_handler_handle_dwarf(pst_handler* h);
void pst_handler_print_dwarf(pst_handler* h);
bool pst_handler_unwind(pst_handler* h);

#endif /* SC_SYSUTILS_H_ */

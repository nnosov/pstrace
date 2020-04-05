/*
 * sysutils.h
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */

#ifndef __PST_DWARF_HANDLER_H__
#define __PST_DWARF_HANDLER_H__

//system
#include <ucontext.h>

#include "utils/list_head.h"
#include "common.h"
#include "context.h"
#include "dwarf_function.h"

typedef struct pst_handler {
	pst_context	    ctx;		// context of unwinding
	list_head	    functions;	// list of functions in stack frame
	bool            allocated;  // whether this object was allocated or not
} pst_handler;

void pst_handler_init(pst_handler* h, ucontext_t* hctx);
pst_handler* pst_handler_new(ucontext_t* hctx);
void pst_handler_fini(pst_handler* h);

bool pst_handler_handle_dwarf(pst_handler* h);
bool pst_handler_unwind(pst_handler* h);
pst_function* pst_handler_next_function(pst_handler* h, pst_function* fn);

#endif /* __PST_DWARF_HANDLER_H__ */

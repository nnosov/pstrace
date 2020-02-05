/*
 * sysutils.h
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */

#ifndef PST_HANDLER_H_
#define PST_HANDLER_H_

//system
#include <ucontext.h>
/*
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <libunwind.h>
#include <elfutils/libdwfl.h>
#include <dlfcn.h>
#include <stdlib.h>
*/

#include "utils/list_head.h"
#include "context.h"

typedef struct pst_handler {
	pst_context					ctx;		// context of unwinding
	list_head	                functions;	// list of functions in stack frame
} pst_handler;
void pst_handler_init(pst_handler* h, ucontext_t* hctx);
void pst_handler_fini(pst_handler* h);

bool pst_handler_handle_dwarf(pst_handler* h);
void pst_handler_print_dwarf(pst_handler* h);
bool pst_handler_unwind(pst_handler* h);

#endif /* PST_HANDLER_H_ */

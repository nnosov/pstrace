/*
 * libpst.h
 *
 * External interface to libpst
 *
 *  Created on: Dec 28, 2019
 *      Author: nnosov
 */

#ifndef __LIBPST_EXTERNAL_H_
#define __LIBPST_EXTERNAL_H_

//system
#include <ucontext.h>
#include <stdio.h>
#include <stdint.h>

typedef struct pst_handler pst_handler;

// allocate libpst handler and initialize the library
pst_handler* pst_lib_init(ucontext_t* hctx);

// deallocate libpst handler and deinitialize the library
void pst_lib_fini(pst_handler* h);


//
// Base unwind routines.
// Unwinds current stack trace using base information about program (i.e. print out address, function name)
//

// write stack trace information to provided file descriptor
int pst_unwind_simple_fd(pst_handler* h, FILE* fd);

// save stack trace information to provided buffer in RAM
int pst_unwind_simple(pst_handler* h, char* buff, uint32_t buff_size);

//
// Advanced unwind routines.
// Additionally to pst_unwind_simple(...) provides types of parameters and variables in functions and their values if possible
//

// write stack trace information to provided file descriptor
int pst_unwind_pretty_fd(pst_handler* h, FILE* fd);

// save stack trace information to provided buffer in RAM
int pst_unwind_pretty(pst_handler* h, char* buff, uint32_t buff_size);

#endif /* __LIBPST_EXTERNAL_H_ */

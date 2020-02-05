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
#include <stdio.h>
#include <stdint.h>

typedef struct pst_handler pst_handler;

// allocate and initialize libpst handler.
pst_handler* pst_new_handler(ucontext_t* hctx);

// deallocate libpst handler
void pst_del_handler(pst_handler* h);


//
// Base unwind routines.
// Unwinds current stack trace using base information about program (i.e. print out address, function name)
//

// write stack trace information to provided file descriptor
int pst_unwind_simple(pst_handler* h, FILE* fd);

// save stack trace information to provided buffer in RAM
int pst_unwind_simple(pst_handler* h, char* buff, uint32_t buff_size);


// Advanced unwind routines.
// Unwinds current stack trace using DWARF debug sections in file if they present and print it to provided file descriptor.
// Additionally to pst_unwind_simple(...) provides types of parameters and variables in functions and their values
//

// write stack trace information to provided file descriptor
int pst_unwind_pretty(pst_handler* h, FILE* fd);

// save stack trace information to provided buffer in RAM
int pst_unwind_pretty(pst_handler* h, char* buff, uint32_t buff_size);

#endif /* PST_HANDLER_H_ */

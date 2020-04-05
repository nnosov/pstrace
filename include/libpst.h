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

/**
 * @brief allocate libpst handler and initialize the library
 * @param context context of process given to signal handler of a program. If NULL, then we are not in signal handler
 * @param buff pointer to buffer by libpst custom allocator(i.e. no malloc() will beused). if NULL, then standard allocator will be used
 * @param size of 'buff'
 *
 * @return pointer to handler to be used for all other operations. NULL in case of error
 */
pst_handler* pst_lib_init(ucontext_t* context, void* buff, uint32_t size);

/**
 * @brief deallocate libpst handler and deinitialize the library
 * @param handler library handler previously initialized by 'pst_lib_init()'
 */
void pst_lib_fini(pst_handler* handler);


/**
 * @defgroup unwind_simple
 * @brief Unwinds current stack trace using base information about program (i.e. function address, function name and line)
 */

/**
 * @ingroup unwind_simple
 * @brief Save stack trace information to provided buffer in RAM
 * @param handler The handler obtained by pst_lib_init()
 * @return 1 on success, 0 on failure
 */
int pst_unwind_simple(pst_handler* handler);

/**
 * @ingroup unwind_simple
 * @brief Print unwound stack trace to internal buffer
 * @param handler The handler obtained by pst_lib_init()
 * @return pointer on zero terminated C string, NULL on failure
 */
const char* pst_print_simple(pst_handler* handler);

//
// Advanced unwind routines.
// Additionally to pst_unwind_simple(...) provides types of parameters and variables in functions and their values if possible
//

// save stack trace information to provided buffer in RAM
int pst_unwind_pretty(pst_handler* h);
const char* pst_print_pretty(pst_handler* h);

#endif /* __LIBPST_EXTERNAL_H_ */

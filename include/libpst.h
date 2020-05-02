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
#include <stdint.h>

#include "libpst-types.h"

typedef struct pst_handler pst_handler;
typedef struct pst_function pst_function;
typedef struct pst_parameter pst_parameter;

//
// Initialization of the library
//

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

//
// Basic unwind routines.
// Allows to retrieve stack trace information about function's name, line and file if possibly
//

/**
 * @brief Unwinds current stack trace using base information about program (i.e. function address, function name and line)
 * @param handler The handler obtained by pst_lib_init()
 * @return 1 on success, 0 on failure
 */
int pst_unwind_simple(pst_handler* handler);

/**
 * @brief Print unwound stack trace to internal buffer
 * @param handler The handler obtained by pst_lib_init()
 * @return pointer to zero terminated C string, NULL on failure
 */
const char* pst_print_simple(pst_handler* handler);

/**
 * @brief Get next function handle in stack trace
 * @param handler The handler obtained by pst_lib_init()
 * @param current pointer to current function. NULL to get first
 * @return pointer to next function's handle, NULL in case of end of list
 */
const pst_function* pst_function_next(pst_handler* handler, pst_function* current);

/**
 * @brief Get function's information (name, address, line)
 * @param function Function's handle obtained by pst_get_next_function()
 * @return pointer to function's info
 */
const pst_function_info* pst_get_function_info(pst_function* function);

/**
 * @brief Get value of CPU register in function's frame
 * @param function function's handler obtained by pst_get_next_function()
 * @param regno index of register
 * @param val pointer to store register's value
 * @return zero on success, UNW_EUNSPEC in case of unspecified error, UNW_EBADREG - register that is either invalid or not accessible in the current frame
 */
int pst_get_register(pst_function* function, int regno, unw_word_t* val);

/**
 * @brief Get next parameter's handle in function
 * @param function Function's handler obtained by pst_get_next_function()
 * @param current pointer to current parameter. NULL to get first
 * @return pointer to next parameter's handle, NULL in case of end of list
 */
const pst_parameter* pst_parameter_next(pst_function* function, pst_parameter* current);

/**
 * @brief Get next parameter's children parameter (for composite types)
 * @param parent parent parameter's handler obtained by pst_get_next_parameter()
 * @param current pointer to current child parameter. NULL to get first
 * @return pointer to next parameter's handle, NULL in case of end of list
 */
const pst_parameter* pst_parameter_next_child(pst_parameter* parent, pst_parameter* current);

/**
 * @brief Get parameter's information (name, type, line etc)
 * @param parameter Parameter's handle obtained by pst_get_next_parameter()
 * @return pointer to parameter's info
 */
const pst_parameter_info* pst_get_parameter_info(pst_parameter* parameter);

//
// Advanced unwind routines.
// Additionally to pst_unwind_simple() provides types of parameters and variables in functions and their values if possible
//

/**
 * @brief Unwinds current stack trace using DWARF information about program if .debug_info section is present in executable
 * @param handler The handler obtained by pst_lib_init()
 * @return 1 on success, 0 on failure
 */
int pst_unwind_pretty(pst_handler* handler);

/**
 * @brief Print unwound Advanced stack trace(using DWARF information) to internal buffer
 * @param handler The handler obtained by pst_lib_init()
 * @return pointer to zero terminated C string, NULL on failure
 */
const char* pst_print_pretty(pst_handler* handler);

#endif /* __LIBPST_EXTERNAL_H_ */

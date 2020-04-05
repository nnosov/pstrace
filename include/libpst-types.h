/*
 * libpst-types.h
 *
 *  Created on: Apr 5, 2020
 *      Author: nnosov
 */

#ifndef INCLUDE_LIBPST_TYPES_H_
#define INCLUDE_LIBPST_TYPES_H_

#include <stdbool.h>
#include <libunwind.h>

typedef struct  pst_function_info {
    char*       name;   ///< demangled name of the function
    char*       file;   ///< file name where the function is defined
    int         line;   ///< line of definition of the function
    unw_word_t  pc;     ///< address between LowPC & HighPC (plus base address offset). actually, address of currently executed command in function
} pst_function_info;

typedef struct pst_parameter_info {
    char*           name;           ///< parameter's name
    uint32_t        line;           ///< line of parameter definition
    unw_word_t      size;           ///< size of parameter in bytes
    unw_word_t      value;          ///< value of parameter. currently only base types supported. use 'size' to determine number of actual bytes of value
    bool            is_return;      ///< whether this parameter is return value of the function
    bool            is_variable;    ///< whether this parameter is function variable or argument of function
    bool            has_value;      ///< whether we got value of parameter or not
} pst_parameter_info;

#endif /* INCLUDE_LIBPST_TYPES_H_ */

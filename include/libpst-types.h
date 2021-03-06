/*
 * libpst-types.h
 *
 *  Created on: Apr 5, 2020
 *      Author: nnosov
 */

#ifndef INCLUDE_LIBPST_TYPES_H_
#define INCLUDE_LIBPST_TYPES_H_

#include <stdint.h>
#include <libunwind.h>

/// @brief bitmask of function's options
typedef enum {
    FUNC_GLOBAL     = 0x00000001,   ///< function has global visibility
    FUNC_LOCAL      = 0x00000002,   ///< function has local visibility
    IS_MEMBER_OF    = 0x00000004,   ///< function is member of class
    IS_INLINED      = 0x00000008,   ///< function was inlined
    IS_PRIVATE      = 0x00000010,   ///< private member of class
    IS_PROTECTED    = 0x00000020,   ///< protected member of class
    IS_PUBLIC       = 0x00000040,   ///< public member of class
} pst_fun_flags;

/// @brief information about function
typedef struct {
    char*           name;   ///< demangled name of the function. NULL if function name resolution failed
    char*           file;   ///< file name where the function is defined. NULL if library failed to get file name
    int             line;   ///< line of definition of the function. -1 if library failed to get file line
    unw_word_t      pc;     ///< address between LowPC & HighPC (plus base address offset). actually, address of currently executed command in function
    unw_word_t      lowpc;  ///< offset to start of the function against base address
    unw_word_t      highpc; ///< offset to the next address after the end of the function against base address
    unw_word_t      sp;     ///< SP register in function's frame
    unw_word_t      cfa;    ///< CFA (Canonical Frame Address) of the function
    pst_fun_flags   flags;  ///< flags of various function options
} pst_function_info;

/// @brief bitmask of parameter's options
typedef enum {
    // scope & access
    PARAM_CONST         = 0x00000001,   ///< constant access
    PARAM_GLOBAL        = 0x00000002,   ///< global scope
    PARAM_STATIC        = 0x00000004,   ///< parameter defined as static
    PARAM_VOLATILE      = 0x00000008,
    PARAM_RETURN        = 0x00000010,   ///< parameter is return value of a function
    PARAM_VARIABLE      = 0x00000020,   ///< parameter is variable inside of a function
    // validity
    PARAM_HAS_VALUE     = 0x00000040,   ///< parameter has a valid value (if not set, then value is undefined)
    PARAM_INVALID       = 0x00000080,   ///< library failed to dereference pointer
    // base types
    PARAM_TYPE_POINTER  = 0x00000100,   ///< pointer
    PARAM_TYPE_INT      = 0x00000200,   ///< integer. to determine size of value, use 'size' field of parameter
    PARAM_TYPE_UINT     = 0x00000400,   ///< unsigned integer. to determine size of value, use 'size' field of parameter
    PARAM_TYPE_FLOAT    = 0x00000800,   ///< float
    PARAM_TYPE_BOOL     = 0x00001000,   ///< boolean
    PARAM_TYPE_VOID     = 0x00002000,   ///< void
    PARAM_TYPE_CHAR     = 0x00004000,   ///< character
    PARAM_TYPE_UCHAR    = 0x00008000,   ///< unsigned character
    PARAM_TYPE_REF      = 0x00010000,   ///< reference
    PARAM_TYPE_RREF     = 0x00020000,   ///< rvalue reference
    PARAM_TYPE_STRING   = 0x00040000,	///< pointer to C zero terminated string
    PARAM_TYPE_UNSPEC   = 0x00080000,	///< variable number of arguments (... in C/C++)
    // complex types
    PARAM_TYPE_ARRAY    = 0x00100000,   ///< array
    PARAM_TYPE_STRUCT   = 0x00200000,   ///< structure
    PARAM_TYPE_UNION    = 0x00400000,   ///< union
    PARAM_TYPE_ENUM     = 0x00800000,   ///< enumeration
    PARAM_TYPE_CLASS    = 0x01000000,   ///< class
    PARAM_TYPE_TYPEDEF  = 0x02000000,   ///< type definition
    PARAM_TYPE_FUNCPTR  = 0x04000000,   ///< pointer to function
} pst_param_flags;

/// @brief information about parameter
typedef struct {
    char*           name;           ///< parameter's name
    char*           type_name;      ///< name of parameter's type
    uint32_t        line;           ///< line of parameter definition
    unw_word_t      size;           ///< size of parameter in bits
    unw_word_t      value;          ///< value of parameter. use 'size' to determine number of actual bits of 'value'
    pst_param_flags flags;          ///< flags of various parameter's options
} pst_parameter_info;

#endif /* INCLUDE_LIBPST_TYPES_H_ */

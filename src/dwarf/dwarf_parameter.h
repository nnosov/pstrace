/*
 * dwarf_parameter.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef __PST_DWARF_PARAMETER_H__
#define __PST_DWARF_PARAMETER_H__

#include <inttypes.h>
#include <elfutils/libdwfl.h>

#include "dwarf_expression.h"
#include "utils/list_head.h"
#include "context.h"

typedef struct __pst_type {
    list_node       node;       // uplink

    char*           name;       // type name
    pst_param_flags type;       // type bit
    bool            allocated;  // whether this struct was allocated or not
} pst_type;
void pst_type_init(pst_type* t, const char* name, pst_param_flags type);
pst_type* pst_type_new(const char* name, pst_param_flags type);
void pst_type_fini(pst_type* t);

typedef struct pst_function pst_function;

typedef struct pst_parameter{
    list_node       node;           // uplink. !!! must be first !!!

    // fields
    Dwarf_Die*          die;            // DWARF DIE containing parameter's definition
    pst_parameter_info  info;
    list_head           types;          // list of parameter's definitions i.e. 'typedef', 'uint32_t'
    pst_context*        ctx;
    pst_dwarf_expr      location;
    list_head           children;       // sub parameters. used in case of complex types (array, pointer to function etc)

    bool                allocated;
} pst_parameter;
void pst_parameter_init(pst_parameter* param, pst_context* ctx);
pst_parameter* pst_parameter_new(pst_context* ctx);
void pst_parameter_fini(pst_parameter* param);

bool pst_parameter_handle_dwarf(pst_parameter* param, Dwarf_Die* result, pst_function* fun);
bool pst_parameter_print_dwarf(pst_parameter* param);
bool pst_parameter_handle_type(pst_parameter* param, Dwarf_Attribute* base);
pst_type* pst_parameter_add_type(pst_parameter* param, const char* name, pst_param_flags type);
pst_parameter* pst_parameter_next_child(pst_parameter* param, pst_parameter* p);

#endif /* __PST_DWARF_PARAMETER_H__ */

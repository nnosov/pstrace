/*
 * dwarf_parameter.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef SRC_DWARF_DWARF_PARAMETER_H_
#define SRC_DWARF_DWARF_PARAMETER_H_

#include <inttypes.h>
#include <elfutils/libdwfl.h>

#include "../src/dwarf/dwarf_expression.h"
#include "../utils/list_head.h"
#include "context.h"

typedef struct __pst_type {
    list_node       node;       // uplink

    char*           name;       // type name
    uint32_t        type;       // DW_AT_XXX type
    bool            allocated;  // whether this struct was allocated or not
} pst_type;
void pst_type_init(pst_type* t, const char* name, uint32_t type);
pst_type* pst_type_new(const char* name, uint32_t type);
void pst_type_fini(pst_type* t);

typedef struct __pst_function pst_function;

typedef struct pst_parameter{
    list_node       node;           // uplink. !!! must be first !!!

    // fields
    Dwarf_Die*      die;            // DWARF DIE containing parameter's definition
    char*           name;           // parameter's name
    uint32_t        line;           // line of parameter definition
    Dwarf_Word      size;           // size of parameter in bytes
    int             type;           // type of parameter in DW_TAG_XXX types enumeration
    Dwarf_Word      enc_type;       // if 'type' is DW_TAG_Base_type, then 'base_type' holds DW_AT_ATE_XXX base type encoding type
    list_head       types;          // list of parameter's definitions i.e. 'typedef', 'uint32_t'
    bool            is_return;      // whether this parameter is return value of the function
    bool            is_variable;    // whether this parameter is function variable or argument of function
    bool            has_value;      // whether we got value of parameter or not
    pst_context*    ctx;
    pst_dwarf_expr  location;

    bool            allocated;
} pst_parameter;
void pst_parameter_init(pst_parameter* param, pst_context* ctx);
pst_parameter* pst_parameter_new(pst_context* ctx);
void pst_parameter_fini(pst_parameter* param);

bool pst_parameter_handle_dwarf(pst_parameter* param, Dwarf_Die* result, pst_function* fun);
bool pst_parameter_print_dwarf(pst_parameter* param);
bool pst_parameter_handle_type(pst_parameter* param, Dwarf_Attribute* base);
pst_type* pst_parameter_add_type(pst_parameter* param, const char* name, int type);

#endif /* SRC_DWARF_DWARF_PARAMETER_H_ */

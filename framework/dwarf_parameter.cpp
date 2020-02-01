/*
 * dwarf_parameter.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#include <dwarf.h>
#include <elfutils/libdw.h>

#include "dwarf_parameter.h"
#include "dwarf_utils.h"
#include "context.h"

//
// pst_type
//
void pst_type_init(pst_type* t, const char* name, uint32_t type)
{
    list_node_init(&t->node);
    t->name = pst_dup(name);
    t->type = type;
    t->allocated = false;
}

pst_type* pst_type_new(const char* name, uint32_t type)
{
    pst_type* nt = pst_alloc(pst_type);
    if(nt) {
        pst_type_init(nt, name, type);
        nt->allocated = true;
    }

    return nt;
}

void pst_type_fini(pst_type* t)
{
    pst_free(t->name);
    if(t->allocated) {
        pst_free(t);
    }
}

//
// pst_parameter
//
bool param_print_dwarf(pst_parameter* param)
{
    if(list_count(&param->types)) {
        if(!param->is_return) {
            if(param->has_value) {
                param->ctx->print(param->ctx, "%s %s = 0x%lX", param->next_type(NULL)->name, param->name, param->location.value);
            } else {
                param->ctx->print(param->ctx, "%s %s = <undefined>", param->next_type(NULL)->name, param->name);
            }
        } else {
            param->ctx->print(param->ctx, "%s", param->next_type(NULL)->name);
        }
    } else {
        if(param->has_value) {
            param->ctx->print(param->ctx, "%s = 0x%lX", param->name, param->location.value);
        } else {
            param->ctx->print(param->ctx, "%s = <undefined>", param->name);
        }
    }
    return true;
}

bool param_handle_type(pst_parameter* param, Dwarf_Attribute* base)
{
    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    // get DIE of return type
    Dwarf_Die ret_die;

    if(!dwarf_formref_die(base, &ret_die)) {
        logger.log(SEVERITY_ERROR, "Failed to get parameter DIE");
        return false;
    }

    switch (dwarf_tag(&ret_die)) {
        case DW_TAG_base_type: {
            // get Size attribute and it's value
            param->size = 0;
            attr = dwarf_attr(&ret_die, DW_AT_byte_size, &attr_mem);
            if(attr) {
                dwarf_formudata(attr, &param->size);
            }
            logger.log(SEVERITY_DEBUG, "base type '%s'(%lu)", dwarf_diename(&ret_die), param->size);
            param->add_type(dwarf_diename(&ret_die), DW_TAG_base_type);
            param->type = DW_TAG_base_type;

            attr = dwarf_attr(&ret_die, DW_AT_encoding, &attr_mem);
            if(attr) {
                param->enc_type = 0;
                dwarf_formudata(attr, &param->enc_type);
            }
            break;
        }
        case DW_TAG_array_type:
            logger.log(SEVERITY_DEBUG, "array type");
            param->add_type("[]", DW_TAG_array_type);
            break;
        case DW_TAG_structure_type:
            logger.log(SEVERITY_DEBUG, "structure type");
            param->add_type("struct", DW_TAG_structure_type);
            break;
        case DW_TAG_union_type:
            logger.log(SEVERITY_DEBUG, "union type");
            param->add_type("union", DW_TAG_union_type);
            break;
        case DW_TAG_class_type:
            logger.log(SEVERITY_DEBUG, "class type");
            param->add_type("class", DW_TAG_class_type);
            break;
        case DW_TAG_pointer_type:
            logger.log(SEVERITY_DEBUG, "pointer type");
            param->add_type("*", DW_TAG_pointer_type);
            break;
        case DW_TAG_enumeration_type:
            logger.log(SEVERITY_DEBUG, "enumeration type");
            param->add_type("enum", DW_TAG_enumeration_type);
            break;
        case DW_TAG_const_type:
            logger.log(SEVERITY_DEBUG, "constant type");
            param->add_type("const", DW_TAG_const_type);
            break;
        case DW_TAG_subroutine_type:
            logger.log(SEVERITY_DEBUG, "Skipping subroutine type");
            break;
        case DW_TAG_typedef:
            logger.log(SEVERITY_DEBUG, "typedef '%s' type", dwarf_diename(&ret_die));
            param->add_type(dwarf_diename(&ret_die), DW_TAG_typedef);
            break;
        default:
            logger.log(SEVERITY_WARNING, "Unknown 0x%X tag type", dwarf_tag(&ret_die));
            break;
    }

    attr = dwarf_attr(&ret_die, DW_AT_type, &attr_mem);
    if(attr) {
        return param->handle_type(attr);
    }

    return true;
}

pst_type* param_add_type(pst_parameter* param, const char* name, int type)
{
    pst_new(pst_type, t, name, type);
    list_add_bottom(&param->types, &t->node);

    return t;
}

void param_clear(pst_parameter* param)
{
    pst_type* t = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(t, pos, tn, &param->types, node) {
        list_del(&t->node);
        pst_type_fini(t);
    }
}

pst_type* param_next_type(pst_parameter* param, pst_type* t)
{
    list_node* n = (t == NULL) ? list_first(&param->types) : list_next(&t->node);
    pst_type* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_type, node);
    }

    return ret;
}

bool param_handle_dwarf(pst_parameter* param, Dwarf_Die* result, pst_function* fun)
{
    param->die = result;

    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    param->name = pst_dup(dwarf_diename(result));
    param->is_variable = (dwarf_tag(result) == DW_TAG_variable);

    dwarf_decl_line(result, (int*)&param->line);
    // Get reference to attribute type of the parameter/variable
    attr = dwarf_attr(result, DW_AT_type, &attr_mem);
    logger.log(SEVERITY_DEBUG, "---> Handle '%s' %s", param->name, dwarf_tag(result) == DW_TAG_formal_parameter ? "parameter" : "variable");
    if(attr) {
        param->handle_type(param, attr);
    }

    if(dwarf_hasattr(result, DW_AT_location)) {
        // determine location of parameter in stack/heap or CPU registers
        attr = dwarf_attr(result, DW_AT_location, &attr_mem);
        Dwarf_Addr pc;
        unw_get_reg(param->ctx->curr_frame, UNW_REG_IP,  &pc);

        if(handle_location(param->ctx, attr, &param->location, pc, fun)) {
            param->has_value = true;
        } else {
            logger.log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", param->ctx->buff);
            return false;
        }
    } else if(dwarf_hasattr(result, DW_AT_const_value)) {
        // no locations definitions, value is constant, known by DWARF directly
        attr = dwarf_attr(result, DW_AT_const_value, &attr_mem);
        switch (dwarf_whatform(attr)) {
            case DW_FORM_string:
                // do nothing for now
                logger.log(SEVERITY_WARNING, "Const value form DW_FORM_string value = %s.", dwarf_formstring(attr));
                break;
            case DW_FORM_data1:
            case DW_FORM_data2:
            case DW_FORM_data4:
            case DW_FORM_data8:
                dwarf_formudata(attr, &param->location.value);
                param->has_value = true;
                break;
            case DW_FORM_sdata:
                dwarf_formsdata(attr, (int64_t*)&param->location.value);
                param->has_value = true;
                break;
            case DW_FORM_udata:
                dwarf_formudata(attr, &param->location.value);
                param->has_value = true;
                break;
        }
        if(param->has_value) {
            logger.log(SEVERITY_DEBUG, "Parameter constant value: 0x%lX", param->location.value);
        }
    }

    // Additionally handle these attributes:
    // 1. DW_AT_default_value to get information about default value for DW_TAG_formal_parameter type of function
    //      A DW_AT_default_value attribute for a formal parameter entry. The value of
    //      this attribute may be a constant, or a reference to the debugging information
    //      entry for a variable, or a reference to a debugging information entry containing a DWARF procedure

    // 2. DW_AT_variable_parameter
    //      A DW_AT_variable_parameter attribute, which is a flag, if a formal
    //      parameter entry represents a parameter whose value in the calling function
    //      may be modified by the callee. The absence of this attribute implies that the
    //      parameter’s value in the calling function cannot be modified by the callee.

    // 3. DW_AT_abstract_origin
    //      In place of these omitted attributes, each concrete inlined instance entry has a DW_AT_abstract_origin attribute that may be used to obtain the
    //      missing information (indirectly) from the associated abstract instance entry. The value of the abstract origin attribute is a reference to the associated abstract
    //      instance entry.


    return true;
}

void pst_parameter_init(pst_parameter*param, pst_context* ctx)
{
    // methods
    param->clear = param_clear;
    param->add_type = param_add_type;
    param->next_type = param_next_type;
    param->handle_dwarf = param_handle_dwarf;
    param->handle_type = param_handle_type;
    param->print_dwarf = param_print_dwarf;

    // fields
    list_node_init(&param->node);
    param->die = NULL;
    param->name = NULL;
    param->line = 0;
    param->size = 0;
    param->type = 0;
    param->enc_type = 0;
    list_head_init(&param->types);
    param->is_return = false;
    param->is_variable = false;
    param->has_value = false;
    param->ctx = ctx;
    pst_dwarf_expr_init(&param->location);
    param->allocated = false;
}

pst_parameter* pst_parameter_new(pst_context* ctx)
{
    pst_parameter* param = pst_alloc(pst_parameter);
    if(param) {
        pst_parameter_init(param, ctx);
        param->allocated = true;
    }

    return param;
}
void pst_parameter_fini(pst_parameter* param)
{
    param->clear();
    if(param->name) {
        pst_free(param->name);
    }
    if(param->allocated) {
        pst_free(param);
    }
}



/*
 * dwarf_parameter.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */


#include <dwarf.h>
#include <elfutils/libdw.h>

#include "dwarf_utils.h"
#include "context.h"
#include "dwarf_parameter.h"

//
// pst_type
//
void pst_type_init(pst_type* t, const char* name, uint32_t type)
{
    list_node_init(&t->node);
    t->name = pst_strdup(name);
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
static void clear(pst_parameter* param)
{
    pst_type* t = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(t, pos, tn, &param->types, node) {
        list_del(&t->node);
        pst_type_fini(t);
    }
}

static pst_type* next_type(pst_parameter* param, pst_type* t)
{
    list_node* n = (t == NULL) ? list_first(&param->types) : list_next(&t->node);
    pst_type* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_type, node);
    }

    return ret;
}

bool pst_parameter_print_dwarf(pst_parameter* param)
{
    if(list_count(&param->types)) {
        if(param->info.flags & PARAM_RETURN) {
            param->ctx->print(param->ctx, "%s", next_type(param, NULL)->name);
        } else {
            if(param->info.flags & PARAM_HAS_VALUE) {
                param->ctx->print(param->ctx, "%s %s = 0x%lX", next_type(param, NULL)->name, param->info.name, param->info.value);
            } else {
                param->ctx->print(param->ctx, "%s %s = <undefined>", next_type(param, NULL)->name, param->info.name);
            }
        }
    } else {
        if(param->info.flags & PARAM_HAS_VALUE) {
            param->ctx->print(param->ctx, "%s = 0x%lX", param->info.name, param->info.value);
        } else {
            param->ctx->print(param->ctx, "%s = <undefined>", param->info.name);
        }
    }
    return true;
}

pst_type* pst_parameter_add_type(pst_parameter* param, const char* name, int type)
{
    pst_new(pst_type, t, name, type);
    list_add_bottom(&param->types, &t->node);

    return t;
}

bool pst_parameter_handle_type(pst_parameter* param, Dwarf_Attribute* base)
{
    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    // get DIE of return type
    Dwarf_Die ret_die;

    if(!dwarf_formref_die(base, &ret_die)) {
        pst_log(SEVERITY_ERROR, "Failed to get parameter DIE");
        return false;
    }

    switch (dwarf_tag(&ret_die)) {
        case DW_TAG_base_type: {
            // get Size attribute and it's value
            param->info.size = 0;
            attr = dwarf_attr(&ret_die, DW_AT_byte_size, &attr_mem);
            if(attr) {
                dwarf_formudata(attr, &param->info.size);
            }
            pst_log(SEVERITY_DEBUG, "base type '%s'(%lu)", dwarf_diename(&ret_die), param->info.size);
            pst_parameter_add_type(param, dwarf_diename(&ret_die), DW_TAG_base_type);
            param->type = DW_TAG_base_type;

            attr = dwarf_attr(&ret_die, DW_AT_encoding, &attr_mem);
            if(attr) {
                param->enc_type = 0;
                dwarf_formudata(attr, &param->enc_type);
            }
            break;
        }
        case DW_TAG_array_type:
            pst_log(SEVERITY_DEBUG, "array type");
            pst_parameter_add_type(param, "[]", DW_TAG_array_type);
            break;
        case DW_TAG_structure_type:
            pst_log(SEVERITY_DEBUG, "structure type");
            pst_parameter_add_type(param, "struct", DW_TAG_structure_type);
            break;
        case DW_TAG_union_type:
            pst_log(SEVERITY_DEBUG, "union type");
            pst_parameter_add_type(param, "union", DW_TAG_union_type);
            break;
        case DW_TAG_class_type:
            pst_log(SEVERITY_DEBUG, "class type");
            pst_parameter_add_type(param, "class", DW_TAG_class_type);
            break;
        case DW_TAG_pointer_type:
            pst_log(SEVERITY_DEBUG, "pointer type");
            pst_parameter_add_type(param, "*", DW_TAG_pointer_type);
            break;
        case DW_TAG_enumeration_type:
            pst_log(SEVERITY_DEBUG, "enumeration type");
            pst_parameter_add_type(param, "enum", DW_TAG_enumeration_type);
            break;
        case DW_TAG_const_type:
            pst_log(SEVERITY_DEBUG, "constant type");
            pst_parameter_add_type(param, "const", DW_TAG_const_type);
            break;
        case DW_TAG_subroutine_type:
            pst_log(SEVERITY_DEBUG, "Skipping subroutine type");
            break;
        case DW_TAG_typedef:
            pst_log(SEVERITY_DEBUG, "typedef '%s' type", dwarf_diename(&ret_die));
            pst_parameter_add_type(param, dwarf_diename(&ret_die), DW_TAG_typedef);
            break;
        default:
            pst_log(SEVERITY_WARNING, "Unknown 0x%X tag type", dwarf_tag(&ret_die));
            break;
    }

    attr = dwarf_attr(&ret_die, DW_AT_type, &attr_mem);
    if(attr) {
        return pst_parameter_handle_type(param, attr);
    }

    return true;
}

bool pst_parameter_handle_dwarf(pst_parameter* param, Dwarf_Die* result, pst_function* fun)
{
    param->die = result;

    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    param->info.name = pst_strdup(dwarf_diename(result));
    param->info.flags |= (dwarf_tag(result) == DW_TAG_variable) ? PARAM_VARIABLE : 0;

    dwarf_decl_line(result, (int*)&param->info.line);
    // Get reference to attribute type of the parameter/variable
    attr = dwarf_attr(result, DW_AT_type, &attr_mem);
    pst_log(SEVERITY_DEBUG, "---> Handle '%s' %s", param->info.name, dwarf_tag(result) == DW_TAG_formal_parameter ? "parameter" : "variable");
    if(attr) {
        pst_parameter_handle_type(param, attr);
    }

    if(dwarf_hasattr(result, DW_AT_location)) {
        // determine location of parameter in stack/heap or CPU registers
        attr = dwarf_attr(result, DW_AT_location, &attr_mem);
        Dwarf_Addr pc;
        unw_get_reg(param->ctx->curr_frame, UNW_REG_IP,  &pc);

        if(handle_location(param->ctx, attr, &param->location, pc, fun)) {
            param->info.value = param->location.value;
            param->info.flags |= PARAM_HAS_VALUE;
        } else {
            pst_log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", param->ctx->buff);
        }
    } else if(dwarf_hasattr(result, DW_AT_const_value)) {
        // no locations definitions, value is constant, known by DWARF directly
        attr = dwarf_attr(result, DW_AT_const_value, &attr_mem);
        switch (dwarf_whatform(attr)) {
            case DW_FORM_string:
                // do nothing for now
                pst_log(SEVERITY_WARNING, "Const value form DW_FORM_string value = %s.", dwarf_formstring(attr));
                break;
            case DW_FORM_data1:
            case DW_FORM_data2:
            case DW_FORM_data4:
            case DW_FORM_data8:
                dwarf_formudata(attr, &param->info.value);
                param->info.flags |= PARAM_HAS_VALUE;
                break;
            case DW_FORM_sdata:
                dwarf_formsdata(attr, (int64_t*)&param->info.value);
                param->info.flags |= PARAM_HAS_VALUE;
                break;
            case DW_FORM_udata:
                dwarf_formudata(attr, &param->info.value);
                param->info.flags |= PARAM_HAS_VALUE;
                break;
        }
        if(param->info.flags & PARAM_HAS_VALUE) {
            pst_log(SEVERITY_DEBUG, "Parameter constant value: 0x%lX", param->info.value);
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

void pst_parameter_init(pst_parameter* param, pst_context* ctx)
{
    list_node_init(&param->node);
    param->die = NULL;
    // info field
    param->info.name = NULL;
    param->info.line = 0;
    param->info.size = 0;
    param->info.flags = 0;

    param->type = 0;
    param->enc_type = 0;
    list_head_init(&param->types);
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
    clear(param);
    if(param->info.name) {
        pst_free(param->info.name);
    }
    if(param->allocated) {
        pst_free(param);
    }
}



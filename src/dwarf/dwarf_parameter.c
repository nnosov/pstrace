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
void pst_type_init(pst_type* t, const char* name, pst_param_flags type)
{
    list_node_init(&t->node);
    if(name) {
        t->name = pst_strdup(name);
    } else {
        t->name = NULL;
    }
    t->type = type;
    t->allocated = false;
}

pst_type* pst_type_new(const char* name, pst_param_flags type)
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
    if(t->name) {
        pst_free(t->name);
    }
    if(t->allocated) {
        pst_free(t);
    }
}

//
// pst_parameter
//
static void clear(pst_parameter* param)
{
    // clean up parameter's types
    pst_type* t = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(t, pos, tn, &param->types, node) {
        list_del(&t->node);
        pst_type_fini(t);
    }

    // clean up children parameters
    pst_parameter* p = NULL;
    list_for_each_entry_safe(p, pos, tn, &param->children, node) {
        list_del(&p->node);
        pst_parameter_fini(p);
    }

    if(param->info.name) {
        pst_free(param->info.name);
    }

    if(param->info.type_name) {
        pst_free(param->info.type_name);
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
    if(param->info.flags & PARAM_RETURN) {
        param->ctx->print(param->ctx, "%s", param->info.type_name ? param->info.type_name : "");
    } else {
        if(param->info.flags & PARAM_TYPE_FUNCPTR) {
            param->ctx->print(param->ctx, "%s (*%s)(", param->info.type_name ? param->info.type_name : "", param->info.name);
            for(pst_parameter* p = pst_parameter_next_child(param, NULL); p; p = pst_parameter_next_child(param, p)) {
                if(p->info.flags & PARAM_RETURN) {
                    // skip return value
                    continue;
                }
                param->ctx->print(param->ctx, "%s%s%s", p->info.type_name ? p->info.type_name : "", p->info.flags & PARAM_TYPE_POINTER ? "*" : "",
                        (pst_parameter_next_child(param, p) != NULL) ? ", " : "");
            }
            param->ctx->print(param->ctx, ") = ");
        } else {
            param->ctx->print(param->ctx, "%s%s %s = ", param->info.type_name ? param->info.type_name : "",
                    param->info.flags & PARAM_TYPE_POINTER ? "*" : "", param->info.name);
        }
        if(param->info.flags & PARAM_HAS_VALUE) {
            param->ctx->print(param->ctx, "0x%lX", param->info.value);
        } else {
            param->ctx->print(param->ctx, "<undefined>");
        }
    }
    return true;
}

pst_type* pst_parameter_add_type(pst_parameter* param, const char* name, pst_param_flags type)
{
    pst_new(pst_type, t, name, type);
    list_add_bottom(&param->types, &t->node);
    param->info.flags |= type;
    if(name && !param->info.type_name) {
        param->info.type_name = pst_strdup(name);
    }

    return t;
}

pst_parameter* pst_parameter_next_child(pst_parameter* param, pst_parameter* p)
{
    struct list_node* n = (p == NULL) ? list_first(&param->children) : list_next(&p->node);

    pst_parameter* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_parameter, node);
    }

    return ret;
}

bool pst_parameter_handle_subroutine(pst_parameter* param, Dwarf_Die* die)
{
	// return value
    pst_new(pst_parameter, p, param->ctx);
    list_add_bottom(&param->children, &p->node);
    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr = dwarf_attr(die, DW_AT_name, &attr_mem);
    if(attr) {
        const char *name = dwarf_formstring(attr);
        if(name) {
            p->info.name = pst_strdup(name);
        }
    }

    // Get reference to attribute type of the parameter/variable
    attr = dwarf_attr(die, DW_AT_type, &attr_mem);
    if(attr) {
        pst_parameter_handle_type(param, attr);
    }

    // hack since DWARF has no ability to determine 'void' it another way
    if(!p->info.type_name) {
        p->info.type_name = pst_strdup("void");
    }

    p->info.flags |= PARAM_RETURN;

    Dwarf_Die result;
    if(dwarf_child(die, &result) != 0) {
        // no parameters defined for subroutine
        return true;
    }

    // went through arguments of subroutine
    do {
        switch (dwarf_tag(&result)) {
            case DW_TAG_formal_parameter:
            {
                pst_new(pst_parameter, p1, param->ctx);
                list_add_bottom(&param->children, &p1->node);

                attr = dwarf_attr(&result, DW_AT_name, &attr_mem);
                if(attr) {
                    const char *name = dwarf_formstring(attr);
                    if(name) {
                        p1->info.name = pst_strdup(name);
                    }
                }

                // Get reference to attribute type of the parameter/variable
                attr = dwarf_attr(&result, DW_AT_type, &attr_mem);
                if(attr) {
                    pst_parameter_handle_type(p1, attr);
                }

                // hack since DWARF has no ability to determine 'void' it another way
                if(!p1->info.type_name) {
                    p1->info.type_name = pst_strdup("void");
                }

                break;
            }
            case DW_TAG_unspecified_parameters:
            {
                pst_new(pst_parameter, p1, param->ctx);
                list_add_bottom(&param->children, &p1->node);
                p1->info.flags |= PARAM_TYPE_UNSPEC;
                p1->info.name = pst_strdup("...");
                break;
            }
            default:
                pst_log(SEVERITY_WARNING, "Unknown TAG of pointer to function: 0x%X", dwarf_tag(&result));
                break;
        }
    } while(dwarf_siblingof(&result, &result) == 0);

    return true;
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

    param->info.size = 64;
    switch (dwarf_tag(&ret_die)) {
        case DW_TAG_base_type: {
            // get Size attribute and it's value
            attr = dwarf_attr(&ret_die, DW_AT_byte_size, &attr_mem);
            if(attr) {
                dwarf_formudata(attr, &param->info.size);
            }
            pst_log(SEVERITY_DEBUG, "base type '%s'(%lu)", dwarf_diename(&ret_die), param->info.size);
            pst_parameter_add_type(param, dwarf_diename(&ret_die), DW_TAG_base_type);

            const char* type_name = dwarf_diename(&ret_die);
            pst_log(SEVERITY_DEBUG, "type_name = %s", type_name);
            attr = dwarf_attr(&ret_die, DW_AT_encoding, &attr_mem);
            if(attr) {
                Dwarf_Word enc_type = 0;
                dwarf_formudata(attr, &enc_type);
                switch (enc_type) {
                    case DW_ATE_boolean:
                        pst_parameter_add_type(param, type_name, PARAM_TYPE_BOOL);
                        break;
                    case DW_ATE_address:
                        pst_parameter_add_type(param, NULL, PARAM_TYPE_POINTER);
                        break;
                    case DW_ATE_signed:
                        pst_parameter_add_type(param, type_name, PARAM_TYPE_INT);
                        break;
                    case DW_ATE_unsigned:
                        pst_parameter_add_type(param, type_name, PARAM_TYPE_UINT);
                        break;
                    case DW_ATE_signed_char:
                        pst_parameter_add_type(param, type_name, PARAM_TYPE_CHAR);
                        break;
                    case DW_ATE_unsigned_char:
                        pst_parameter_add_type(param, type_name, PARAM_TYPE_UCHAR);
                        break;
                    case DW_ATE_float:
                    case DW_ATE_complex_float:
                    case DW_ATE_imaginary_float:
                    case DW_ATE_decimal_float:
                        pst_parameter_add_type(param, type_name, PARAM_TYPE_FLOAT);
                        break;
                }

            }
            break;
        }
        case DW_TAG_array_type:
            pst_parameter_add_type(param, NULL, PARAM_TYPE_ARRAY);
            break;
        case DW_TAG_structure_type:
            pst_parameter_add_type(param, NULL, PARAM_TYPE_STRUCT);
            break;
        case DW_TAG_union_type:
            pst_parameter_add_type(param, NULL, PARAM_TYPE_UNION);
            break;
        case DW_TAG_class_type:
            pst_parameter_add_type(param, NULL, PARAM_TYPE_CLASS);
            break;
        case DW_TAG_pointer_type:
            pst_parameter_add_type(param, NULL, PARAM_TYPE_POINTER);
            break;
        case DW_TAG_enumeration_type:
            pst_parameter_add_type(param, NULL, PARAM_TYPE_ENUM);
            break;
        case DW_TAG_const_type:
            pst_parameter_add_type(param, NULL, PARAM_CONST);
            break;
        case DW_TAG_subroutine_type:
            pst_parameter_handle_subroutine(param, &ret_die);
            pst_parameter_add_type(param, NULL, PARAM_TYPE_FUNCPTR);
            break;
        case DW_TAG_typedef: {
            pst_parameter_add_type(param, dwarf_diename(&ret_die), PARAM_TYPE_TYPEDEF);
            break;
        }
        case DW_TAG_unspecified_type:
            pst_parameter_add_type(param, "void", PARAM_TYPE_VOID);
            break;
        case DW_TAG_reference_type:
            pst_parameter_add_type(param, NULL, PARAM_TYPE_REF);
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
        param->info.flags |= PARAM_CONST;
        // no locations definitions, value is constant, known by DWARF directly
        attr = dwarf_attr(result, DW_AT_const_value, &attr_mem);
        param->info.flags |= PARAM_HAS_VALUE;
        switch (dwarf_whatform(attr)) {
            case DW_FORM_string:
                param->info.value = (unw_word_t)dwarf_formstring(attr);
                break;
            case DW_FORM_data1:
            case DW_FORM_data2:
            case DW_FORM_data4:
            case DW_FORM_data8:
                dwarf_formudata(attr, &param->info.value);
                break;
            case DW_FORM_sdata:
                dwarf_formsdata(attr, (int64_t*)&param->info.value);
                break;
            case DW_FORM_udata:
                dwarf_formudata(attr, &param->info.value);
                break;
            default:
                pst_log(SEVERITY_WARNING, "Unknown form '0x%X' of attribute for Constant value", dwarf_whatform(attr));
                param->info.flags &= ~PARAM_HAS_VALUE;
        }

        if(param->info.flags & PARAM_HAS_VALUE) {
            pst_log(SEVERITY_DEBUG, "Parameter constant value: 0x%lX", param->info.value);
        }
    }

    //param->info.value &=0xFFFFFFFFFFFFFFFF >> (64 - ((param->info.size * 8) & 0x0F));

    // hack since DWARF has no ability to determine 'void' it another way
    if(!param->info.type_name) {
        param->info.type_name = pst_strdup("void");
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
    param->info.type_name = NULL;
    param->info.line = 0;
    param->info.size = 0;
    param->info.flags = 0;

    list_head_init(&param->types);
    list_head_init(&param->children);
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
    if(param->allocated) {
        pst_free(param);
    }
}



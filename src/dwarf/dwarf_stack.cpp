/*
 * dwarf_stack.cpp
 *
 *  Created on: Jan 29, 2020
 *      Author: nnosov
 */

#include "../src/dwarf/dwarf_stack.h"

#include <string.h>
#include <stdlib.h>
#include <dwarf.h>

#include "../src/dwarf/dwarf_operations.h"
#include "../utils/allocator.h"
#include "../utils/list_head.h"
#include "dwarf_handler.h"

// -----------------------------------------------------------------------------------
// DWARF Stack value
// -----------------------------------------------------------------------------------
void pst_dwarf_value_set(pst_dwarf_value* value, void* v, uint32_t s, int t)
{
    pst_assert(value && v && s > 0 && s <= sizeof(value->value));

    // clean-up all bits
    value->value.uint64 = 0;
    value->type = t;

    if(t & DWARF_TYPE_SIGNED) {
        switch(s) {
            case 1:
                value->value.int8 = *(int8_t*)v;
                break;
            case 2:
                value->value.int16 = *(int16_t*)v;
                break;
            case 4:
                value->value.int32 = *(int32_t*)v;
                break;
            default:
                value->value.int64 = *(int64_t*)v;
                break;

        }
    } else {
        // GENERIC, SIGNED etc
        switch(s) {
            case 1:
                value->value.uint8 = *(uint8_t*)v;
                break;
            case 2:
                value->value.uint16 = *(uint16_t*)v;
                break;
            case 4:
                value->value.uint32 = *(uint32_t*)v;
                break;
            default:
                value->value.uint64 = *(uint64_t*)v;
                break;

        }
    }
}

void pst_dwarf_value_init(pst_dwarf_value* dv, char* v, uint32_t s, int t)
{
    pst_assert(dv && s <= sizeof(dv->value));

    list_node_init(&dv->node);
    dv->type = t;
    dv->allocated = false;
    pst_dwarf_value_set(dv, v, s, t);
}

pst_dwarf_value* pst_dwarf_value_new(char* v, uint32_t s, int t)
{
    pst_assert(v);

    pst_dwarf_value* nv = pst_alloc(pst_dwarf_value);
    if(nv) {
        pst_dwarf_value_init(nv, v, s, t);
        nv->allocated = true;
    }


    return nv;
}

void pst_dwarf_value_fini(pst_dwarf_value* value)
{
    pst_assert(value);

    if(value->allocated) {
        pst_free(value);
    } else {
        value->type = DWARF_TYPE_INVALID;
        value->value.uint64 = 0;
    }
}


// -----------------------------------------------------------------------------------
// DWARF stack
// -----------------------------------------------------------------------------------
void pst_dwarf_stack_push(pst_dwarf_stack* st, void* v, uint32_t s, int t)
{
    pst_new(pst_dwarf_value, value, (char*)v, s, t);
    list_add_head(&st->values, &value->node);
}

void pst_dwarf_stack_push_value(pst_dwarf_stack* st, pst_dwarf_value* value)
{
    list_add_head(&st->values, &value->node);
}

pst_dwarf_value* pst_dwarf_stack_pop(pst_dwarf_stack* st)
{
    pst_dwarf_value* value = (pst_dwarf_value*)list_first(&st->values);
    if(value) {
        list_del(&value->node);
    }

    return value;
}

pst_dwarf_value* pst_dwarf_stack_get(pst_dwarf_stack* st, uint32_t idx)
{
    pst_dwarf_value* value = NULL;
    struct list_node  *pos;
    list_for_each_entry(value, pos, &st->values, node) {
        if(!idx) {
            break;
        }
        idx--;
    }

    return value;
}

bool pst_dwarf_stack_get_value(pst_dwarf_stack* st, uint64_t* value)
{
    assert(st && value);

    if(!list_count(&st->values)) {
        return false;
    }

    pst_dwarf_value* v = pst_dwarf_stack_get(st, 0);
    if(v->type & DWARF_TYPE_REGISTER_LOC) {
        // dereference register location
        int ret = unw_get_reg(st->ctx->curr_frame, v->value.uint64, value);
        if(ret) {
            pst_log(SEVERITY_ERROR, "Failed to get value of register 0x%X. Error: %d", v->value.uint64, ret);
            return false;
        }
    } else if(v->type & DWARF_TYPE_MEMORY_LOC) {
        // dereference memory location
        *value = *((uint64_t*)v->value.uint64);
    } else {
        *value = v->value.uint64;
    }

    return true;
}

void pst_dwarf_stack_clear(pst_dwarf_stack* st)
{
    pst_dwarf_value*  value = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(value, pos, tn, &st->values, node) {
        list_del(&value->node);
        pst_dwarf_value_fini(value);
    }

    pst_dwarf_op* op = NULL;
    list_for_each_entry_safe(op, pos, tn, &st->expr, node) {
        list_del(&op->node);
        pst_dwarf_op_fini(op);
    }
}

bool pst_dwarf_stack_calc(pst_dwarf_stack* st, Dwarf_Op *exprs, int expr_len, Dwarf_Attribute* attr, pst_function* fun)
{
    pst_dwarf_stack_clear(st);

    for (int i = 0; i < expr_len; i++) {
        const dwarf_op_map* map = find_op_map(exprs[i].atom);
        if(!map) {
            pst_log(SEVERITY_ERROR, "Unknown operation type 0x%hhX(0x%lX, 0x%lX)", exprs[i].atom, exprs[i].number, exprs[i].number2);
            return false;
        }

        pst_new(pst_dwarf_op, op, exprs[i].atom, exprs[i].number, exprs[i].number2);
        list_add_bottom(&st->expr, &op->node);

        pst_dwarf_value* v = pst_dwarf_stack_get(st, 0);
        // dereference register location there if it is not last in stack
        if(v && (v->type & DWARF_TYPE_REGISTER_LOC)) {
            unw_word_t value = 0;
            uint64_t regno = *((uint64_t*)v->value.uint64);
            int ret = unw_get_reg(st->ctx->curr_frame, regno, &value);
            if(ret) {
                pst_log(SEVERITY_ERROR, "Failed to ger value of register 0x%X. Error: %d", regno, ret);
                return false;
            }
            pst_dwarf_value_set(v, &value, sizeof(value), DWARF_TYPE_GENERIC);
        }

        // handle there because it contains sub-expression of a Location in caller's frame
        if(map->op_num == DW_OP_GNU_entry_value) {
            if(!fun || !fun->parent) {
                pst_log(SEVERITY_ERROR, "Cannot calculate DW_OP_GNU_entry_value expression while function and it's caller is undefined");
                return false;
            }

            // This opcode has two operands, the first one is uleb128 length and the second is block of that length, containing either a
            // simple register or DWARF expression
            Dwarf_Attribute attr_mem;
            if(!dwarf_getlocation_attr(attr, exprs, &attr_mem)) {
                Dwarf_Op *expr;
                size_t exprlen;
                if (dwarf_getlocation(&attr_mem, &expr, &exprlen) == 0) {
                    pst_call_site* cs = pst_call_site_storage_find(&fun->parent->call_sites, fun);
                    if(!cs) {
                        pst_log(SEVERITY_ERROR, "Failed to find call site while calculate DW_OP_GNU_entry_value expression");
                        return false;
                    }
                    pst_dwarf_expr loc;
                    pst_dwarf_expr_init(&loc);
                    pst_dwarf_expr_setup(&loc, expr, exprlen);
                    pst_call_site_param* param = pst_call_site_find(cs, &loc);
                    pst_dwarf_expr_fini(&loc);
                    if(!param) {
                        pst_log(SEVERITY_ERROR, "Failed to find call site parameter while calculate DW_OP_GNU_entry_value expression");
                        return false;
                    }

                    pst_dwarf_stack_push(st, &param->value, sizeof(param->value), DWARF_TYPE_GENERIC);

                    continue;
                } else {
                    pst_log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr location");
                    return false;
                }
            } else {
                pst_log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr expression");
                return false;
            }
        }

        if(!map->operation(st, map, exprs[i].number, exprs[i].number2)) {
            pst_log(SEVERITY_ERROR, "Failed to calculate %s(0x%lX, 0x%lX) operation", map->op_name, exprs[i].number, exprs[i].number2);
            return false;
        }

    }

    return true;
}

void pst_dwarf_stack_init(pst_dwarf_stack* st, pst_context* ctx)
{
    pst_assert(st && ctx);

    list_head_init(&st->expr);
    list_head_init(&st->values);
    st->ctx = ctx;
    st->allocated = false;
}

pst_dwarf_stack* pst_dwarf_stack_new(pst_context* ctx)
{
    pst_assert(ctx);

    pst_new(pst_dwarf_stack, ns, ctx);
    if(ns) {
        pst_dwarf_stack_init(ns, ctx);
        ns->allocated = true;
    }

    return ns;
}

void pst_dwarf_stack_fini(pst_dwarf_stack* st)
{
    pst_assert(st);

    pst_dwarf_stack_clear(st);
    if(st->allocated) {
        pst_free(st);
    } else {
        st->ctx = NULL;
    }
}

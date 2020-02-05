/*
 * dwarf_expression.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#include "../src/dwarf/dwarf_expression.h"

#include <stdarg.h>

#include "../utils/allocator.h"
#include "common.h"
#include "context.h"

//
// DWARF operation
//
void pst_dwarf_op_init(pst_dwarf_op* dwop, uint8_t op, uint64_t a1, uint64_t a2)
{
    //pst_assert(dwop);

    list_node_init(&dwop->node);
    dwop->operation = op;
    dwop->arg1 = a1;
    dwop->arg2 = a2;
    dwop->allocated = false;
}

pst_dwarf_op* pst_dwarf_op_new(uint8_t op, uint64_t a1, uint64_t a2)
{
    pst_dwarf_op* nop = pst_alloc(pst_dwarf_op);
    if(nop) {
        pst_dwarf_op_init(nop, op, a1, a2);
        nop->allocated = true;
    }

    return nop;
}

void pst_dwarf_op_fini(pst_dwarf_op* dwop)
{
    assert(dwop);

    if(dwop->allocated) {
        pst_free(dwop);
    } else {
        dwop->operation = 0;
        dwop->arg1 = 0;
        dwop->arg2 = 0;
    }
}

//
// pst_dwarf_expr
//
pst_dwarf_op* add_op(pst_dwarf_expr* expr, uint8_t operation, uint64_t arg1, uint64_t arg2)
{
    pst_new(pst_dwarf_op, op, operation, arg1, arg2);
    list_add_bottom(&expr->operations, &op->node);

    return op;
}

pst_dwarf_op* next_op(pst_dwarf_expr* expr, pst_dwarf_op* op)
{
    struct list_node* n = (op == NULL) ? list_first(&expr->operations) : list_next(&op->node);

    pst_dwarf_op* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_dwarf_op, node);
    }

    return ret;
}

void clean(pst_dwarf_expr* expr)
{
    pst_dwarf_op* op = NULL;
    struct list_node  *pos, *tn;
    list_for_each_entry_safe(op, pos, tn, &expr->operations, node) {
        list_del(&op->node);
        pst_dwarf_op_fini(op);
    }
}

bool pst_dwarf_expr_print_op(pst_dwarf_expr* expr, const char* fmt, ...)
{
    bool nret = true;

    va_list args;
    va_start(args, fmt);
    int size = sizeof(expr->buff) - expr->offset;
    int ret = vsnprintf(expr->buff + expr->offset, size, fmt, args);
    if(ret >= size || ret < 0) {
        nret = false;
    }
    expr->offset += ret;
    va_end(args);

    return nret;
}

void pst_dwarf_expr_set_value(pst_dwarf_expr* expr, uint64_t v)
{
    expr->has_value = true;
    expr->value = v;
}

bool pst_dwarf_expr_equal(pst_dwarf_expr* lhs, pst_dwarf_expr* rhs)
{
    if(list_count(&lhs->operations) != list_count(&rhs->operations)) {
        return false;
    }

    pst_dwarf_op* lop = next_op(lhs, NULL);
    pst_dwarf_op* rop = next_op(rhs, NULL);
    while(lop && rop) {
        if(lop->operation == rop->operation && lop->arg1 == rop->arg1 && lop->arg2 == rop->arg2) {
            return true;
        }
        lop = next_op(lhs, lop);
        rop = next_op(rhs, rop);
    }

    return false;
}

void pst_dwarf_expr_setup(pst_dwarf_expr* expr, Dwarf_Op* exprs, size_t exprlen)
{
    clean(expr);
    for(size_t i = 0; i < exprlen; ++i) {
        add_op(expr, exprs[i].atom, exprs[i].number, exprs[i].number2);
    }
}

void pst_dwarf_expr_init(pst_dwarf_expr* expr)
{
    list_head_init(&expr->operations);
    expr->has_value = false;
    expr->value = 0;

    expr->buff[0] = 0;
    expr->offset = 0;
    expr->allocated = false;
}

pst_dwarf_expr* pst_dwarf_expr_new()
{
    pst_dwarf_expr* ne = pst_alloc(pst_dwarf_expr);
    if(ne) {
        pst_dwarf_expr_init(ne);
        ne->allocated = true;
    }

    return ne;
}

void pst_dwarf_expr_fini(pst_dwarf_expr* expr)
{
    clean(expr);
    if(expr->allocated) {
        pst_free(expr);
    }
}

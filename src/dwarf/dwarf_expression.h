/*
 * dwarf_expression.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef SRC_DWARF_DWARF_EXPRESSION_H_
#define SRC_DWARF_DWARF_EXPRESSION_H_

#include <inttypes.h>
#include <elfutils/libdwfl.h>

#include "../utils/allocator.h"
#include "../utils/list_head.h"

// DWARF operation (represents our own DW_OP_XXX)
typedef struct {
    list_node       node;       //uplink

    uint8_t         operation;
    uint64_t        arg1;
    uint64_t        arg2;
    bool            allocated;
} pst_dwarf_op;
void pst_dwarf_op_init(pst_dwarf_op* dwop, uint8_t op, uint64_t a1, uint64_t a2);
pst_dwarf_op* pst_dwarf_op_new(uint8_t op, uint64_t a1, uint64_t a2);
void pst_dwarf_op_fini(pst_dwarf_op* dwop);


// DWARF expression
typedef struct pst_dwarf_expr {
    list_head       operations;
    bool            has_value;
    uint64_t        value;
    char            buff[512];
    uint16_t        offset;
    bool            allocated;
} pst_dwarf_expr;
void pst_dwarf_expr_init(pst_dwarf_expr* expr);
pst_dwarf_expr* pst_dwarf_expr_new();
void pst_dwarf_expr_fini(pst_dwarf_expr* expr);

void pst_dwarf_expr_setup(pst_dwarf_expr* expr, Dwarf_Op* exprs, size_t exprlen);
bool pst_dwarf_expr_equal(pst_dwarf_expr* lhs, pst_dwarf_expr* rhs);
bool pst_dwarf_expr_print_op(pst_dwarf_expr* expr, const char* fmt, ...);
void pst_dwarf_expr_set_value(pst_dwarf_expr* expr, uint64_t v);

#endif /* SRC_DWARF_DWARF_EXPRESSION_H_ */

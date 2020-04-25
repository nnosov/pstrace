/*
 * context.cpp
 *
 *  Created on: Jan 28, 2020
 *      Author: nnosov
 */

#include <limits.h>
#include <dwarf.h>

#include "context.h"

#include "dwarf/dwarf_operations.h"
#include "arch/registers.h"


extern dwarf_reg_map    reg_map[];
extern int regnum;

pst_logger      logger;     // logger for library
pst_allocator   allocator;  // custom allocator for PST library

static void clean_print(pst_context* ctx)
{
    ctx->buff[0] = 0;
    ctx->offset = 0;
}

static void print_stack(pst_context* ctx, int max, uint64_t next_cfa)
{
    pst_log(SEVERITY_DEBUG, "CFA = %#lX, NEXT_CFA = %#lX, SP = %#lX", ctx->cfa, next_cfa, ctx->sp);
    ctx->clean_print(ctx);
    int i = 0;
    if(ctx->cfa > ctx->sp) {
        ctx->print(ctx, "Args: ");
        for(; i < max && (ctx->cfa - i) > ctx->sp; ++i) {
            ctx->print(ctx, "#%d 0x%lX ", i, *(uint64_t*)(ctx->cfa - i));
        }
    }
    if((ctx->cfa - i) > next_cfa) {
        ctx->print(ctx, "Vars: ");
        for(; i < max && (ctx->cfa - i) > next_cfa; ++i) {
            ctx->print(ctx, "#%d 0x%lX ", i, *(uint64_t*)(ctx->cfa - i));
        }
    }
}

static void print_registers(pst_context* ctx, int from, int to)
{
    ctx->clean_print(ctx);
    for(int i = from; i < regnum && i <= to; ++i) {
        unw_word_t regval;
        if(!unw_get_reg(ctx->curr_frame, reg_map[i].regno, &regval)) {
            ctx->print(ctx, "%s: %#lX ", reg_map[i].regname, regval);
        } else {
            ctx->print(ctx, "%s: <undef>", reg_map[i].regname);
        }
    }
}

static bool print(pst_context* ctx, const char* fmt, ...)
{
    bool nret = true;

    va_list args;
    va_start(args, fmt);
    int size = sizeof(ctx->buff) - ctx->offset;
    int ret = vsnprintf(ctx->buff + ctx->offset, size, fmt, args);
    if(ret >= size || ret < 0) {
        nret = false;
    }
    ctx->offset += ret;
    va_end(args);

    return nret;
}

static bool print_expr_block (pst_context* ctx, Dwarf_Op *exprs, int exprlen, Dwarf_Attribute* attr)
{
    ctx->clean_print(ctx);
    for (int i = 0; i < exprlen; i++) {
        const dwarf_op_map* map = find_op_map(exprs[i].atom);
        if(map) {
            if(map->op_num >= DW_OP_breg0 && map->op_num <= DW_OP_breg16) {
                int32_t off = decode_sleb128((unsigned char*)&exprs[i].number);
                int regno = map->op_num - DW_OP_breg0;
                unw_word_t ptr = 0;
                unw_get_reg(ctx->curr_frame, regno, &ptr);

                ctx->print(ctx, "%s(*%s%s%d) reg_value: 0x%lX", map->op_name, unw_regname(regno), off >=0 ? "+" : "", off, ptr);
            } else if(map->op_num >= DW_OP_reg0 && map->op_num <= DW_OP_reg16) {
                unw_word_t value = 0;
                int regno = map->op_num - DW_OP_reg0;
                unw_get_reg(ctx->curr_frame, regno, &value);
                ctx->print(ctx, "%s(*%s) value: 0x%lX", map->op_name, unw_regname(regno), value);
            } else if(map->op_num == DW_OP_GNU_entry_value) {
                if(!attr) {
                    pst_log(SEVERITY_ERROR, "No attribute of DW_OP_GNU_entry_value provided");
                    return false;
                }
                uint32_t value = decode_uleb128((unsigned char*)&exprs[i].number);
                ctx->print(ctx, "%s(%u, ", map->op_name, value);
                Dwarf_Attribute attr_mem;
                if(!dwarf_getlocation_attr(attr, exprs, &attr_mem)) {
                    Dwarf_Op *expr;
                    size_t exprlen;
                    if (dwarf_getlocation(&attr_mem, &expr, &exprlen) == 0) {
                        ctx->print_expr(ctx, expr, exprlen, &attr_mem);
                        ctx->print(ctx, ") ");
                    } else {
                        pst_log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr location");
                        return false;
                    }
                } else {
                    pst_log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr expression");
                    return false;
                }
            } else if(map->op_num == DW_OP_stack_value) {
                ctx->print(ctx, "%s", map->op_name);
            } else if(map->op_num == DW_OP_plus_uconst) {
                uint32_t value = decode_uleb128((unsigned char*)&exprs[i].number);
                ctx->print(ctx, "%s(+%u) ", map->op_name, value);
            } else if(map->op_num == DW_OP_bregx) {
                uint32_t regno = decode_uleb128((unsigned char*)&exprs[i].number);
                int32_t off = decode_sleb128((unsigned char*)&exprs[i].number2);
                unw_word_t ptr = 0;
                unw_get_reg(ctx->curr_frame, regno, &ptr);
                //ptr += off;
                ctx->print(ctx, "%s(%s%s%d) reg_value = 0x%lX", map->op_name, unw_regname(regno), off >= 0 ? "+" : "", off, ptr);
            } else if(map->op_num == DW_OP_regx) {
                int32_t reg = decode_sleb128((unsigned char*)&exprs[i].number);

                unw_word_t value = 0;
                unw_get_reg(ctx->curr_frame, reg, &value);

                ctx->print(ctx, "%s(%s) value = 0x%lX", map->op_name, unw_regname(reg), value);
            } else if(map->op_num == DW_OP_addr) {
                ctx->print(ctx, "%s value = %p", map->op_name, (void*)exprs[i].number);
            } else if(map->op_num == DW_OP_fbreg) {
                int32_t off = decode_sleb128((unsigned char*)&exprs[i].number);
                ctx->print(ctx, "%s(SP%s%d) ", map->op_name, off >=0 ? "+" : "", off);
            } else {
                ctx->print(ctx, "%s(0x%lX, 0x%lx) ", map->op_name, exprs[i].number, exprs[i].number2);
            }
        } else {
            ctx->print(ctx, "0x%hhX(0x%lX, 0x%lx)", exprs[i].atom, exprs[i].number, exprs[i].number2);
        }
    }

    return true;
}

void pst_context_init(pst_context* ctx, ucontext_t* hctx)
{
    // methods
    ctx->clean_print = clean_print;
    ctx->print = print;
    ctx->print_expr = print_expr_block;
    ctx->print_registers = print_registers;
    ctx->print_stack = print_stack;

    // fields
    ctx->hcontext = hctx;
    ctx->base_addr = 0;
    ctx->sp = 0;
    ctx->cfa = 0;
    ctx->curr_frame = NULL;
    ctx->frame = NULL;
    ctx->dwfl = NULL;
    ctx->module = NULL;
}

void pst_context_fini(pst_context* ctx)
{
    ctx->hcontext = NULL;
    ctx->clean_print(ctx);
    ctx->base_addr = 0;
    ctx->sp = 0;
    ctx->cfa = 0;
    ctx->curr_frame = NULL;
    ctx->frame = NULL;
    ctx->dwfl = NULL;
    ctx->module = NULL;
}


char* pst_strdup(const char* str)
{
    pst_assert(str);

    uint32_t len = strlen(str);
    char* dst = (char*)allocator.alloc(&allocator, len + 1);
    memcpy(dst, str, len);
    dst[len] = 0;

    return dst;
}

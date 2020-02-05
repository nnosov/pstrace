/*
 * registers.cpp
 *
 *  Created on: Jan 27, 2020
 *      Author: nnosov
 */

#include <dwarf.h>
#include <elfutils/libdwfl.h>

#include "common.h"
#include "registers.h"

dwarf_reg_map   reg_map[] = {
    // GP Registers
    {0x0,  "RAX",   DW_OP_reg0},
    {0x1,  "RDX",   DW_OP_reg1},
    {0x2,  "RCX",   DW_OP_reg2},
    {0x3,  "RBX",   DW_OP_reg3},
    {0x4,  "RSI",   DW_OP_reg4},
    {0x5,  "RDI",   DW_OP_reg5},
    {0x6,  "RBP",   DW_OP_reg6},
    {0x7,  "RSP",   DW_OP_reg7}, // Stack pointer address (SP) mapped to RSP
    // Extended GP Registers
    {0x8,  "R8",    DW_OP_reg8},
    {0x9,  "R9",    DW_OP_reg9},
    {0xA,  "R10",   DW_OP_reg10},
    {0xB,  "R11",   DW_OP_reg11},
    {0xC,  "R12",   DW_OP_reg12},
    {0xD,  "R13",   DW_OP_reg13},
    {0xE,  "R14",   DW_OP_reg14},
    {0xF,  "R15",   DW_OP_reg15},
    {0x10, "RIP",   DW_OP_reg16}, // Return Address (RA) mapped to RIP
    // SSE Vector Registers
    {0x11, "XMM0",  DW_OP_reg17},
    {0x12, "XMM1",  DW_OP_reg18},
    {0x13, "XMM2",  DW_OP_reg19},
    {0x14, "XMM3",  DW_OP_reg20},
    {0x15, "XMM4",  DW_OP_reg21},
    {0x16, "XMM5",  DW_OP_reg22},
    {0x17, "XMM6",  DW_OP_reg23},
    {0x18, "XMM7",  DW_OP_reg24},
    {0x19, "XMM8",  DW_OP_reg25},
    {0x1a, "XMM9",  DW_OP_reg26},
    {0x1b, "XMM10", DW_OP_reg27},
    {0x1c, "XMM11", DW_OP_reg28},
    {0x1d, "XMM12", DW_OP_reg29},
    {0x1e, "XMM13", DW_OP_reg30},
    {0x1f, "XMM14", DW_OP_reg31},

    // GP Registers
    {0x0,  "RAX",   DW_OP_breg0},
    {0x1,  "RDX",   DW_OP_breg1},
    {0x2,  "RCX",   DW_OP_breg2},
    {0x3,  "RBX",   DW_OP_breg3},
    {0x4,  "RSI",   DW_OP_breg4},
    {0x5,  "RDI",   DW_OP_breg5},
    {0x6,  "RBP",   DW_OP_breg6},
    {0x7,  "RSP",   DW_OP_breg7},
    // Extended GP Registers
    {0x8,  "R8",    DW_OP_breg8},
    {0x9,  "R9",    DW_OP_breg9},
    {0xA,  "R10",   DW_OP_breg10},
    {0xB,  "R11",   DW_OP_breg11},
    {0xC,  "R12",   DW_OP_breg12},
    {0xD,  "R13",   DW_OP_breg13},
    {0xE,  "R14",   DW_OP_breg14},
    {0xF,  "R15",   DW_OP_breg15},
    {0x10, "RIP",   DW_OP_breg16}, // Return Address (RA) mapped to RIP
    // SSE Vector Registers
    {0x11, "XMM0",  DW_OP_breg17},
    {0x12, "XMM1",  DW_OP_breg18},
    {0x13, "XMM2",  DW_OP_breg19},
    {0x14, "XMM3",  DW_OP_breg20},
    {0x15, "XMM4",  DW_OP_breg21},
    {0x16, "XMM5",  DW_OP_breg22},
    {0x17, "XMM6",  DW_OP_breg23},
    {0x18, "XMM7",  DW_OP_breg24},
    {0x19, "XMM8",  DW_OP_breg25},
    {0x1a, "XMM9",  DW_OP_breg26},
    {0x1b, "XMM10", DW_OP_breg27},
    {0x1c, "XMM11", DW_OP_breg28},
    {0x1d, "XMM12", DW_OP_breg29},
    {0x1e, "XMM13", DW_OP_breg30},
    {0x1f, "XMM14", DW_OP_breg31},
    {0x20, "XMM15", 0xff}, // no mapping to dwarf registers
};

int regnum = sizeof(reg_map) / sizeof(dwarf_reg_map);

int find_regnum(uint32_t op)
{
    for(int i = 0; i < regnum; ++i) {
        if(reg_map[i].op_num == op) {
            return reg_map[i].regno;
        }
    }

    return -1;
}

pst_reg_error pst_get_reg(pst_context* ctx, int regno, uint64_t& regval)
{
#ifdef USE_LIBUNWIND
    int ret = unw_get_reg(ctx->curr_frame, regno, &regval);
    switch (ret) {
        case UNW_EUNSPEC:
            return REG_CFI_ERROR;
            break;
        case UNW_EBADREG:
            return REG_UNDEFINED;
            break;
        default:
            return REG_UNDEFINED;
            break;
    }

    return REG_OK;
#else
    Dwarf_Op ops_mem[3];
    Dwarf_Op* ops;
    size_t nops;
    char str[512];
    if(dwarf_frame_register(ctx->frame, regno, ops_mem, &ops, &nops) != -1) {
        if(nops != 0 || ops != ops_mem) {
            if(nops != 0 || ops != NULL) {
                str[0] = 0;
                ctx->print_expr_block(ops, nops, str, sizeof(str));
                dwarf_stack st(ctx);
                if(st.calc_expression(ops, nops, NULL) && st.get_value(regval)) {
                    pst_log(SEVERITY_DEBUG, "CFI register 0x%X(%s) expression: %s ==> %#lX", regno, reg_map[regno].regname, str, regval);
                    return REG_OK;
                } else {
                    pst_log(SEVERITY_ERROR, "Failed to calculate register 0x%X(%s) CFI expression %s", regno, reg_map[regno].regname, str);
                    return REG_EXPR_ERROR;
                }
            } else {
                pst_log(SEVERITY_DEBUG, "CFI expression for register 0x%X(%s) is SAME VALUE", regno, reg_map[regno].regname);
                return REG_SAME;
            }
        } else {
            pst_log(SEVERITY_DEBUG, "CFI expression for register 0x%X(%s) is UNDEFINED", regno, reg_map[regno].regname);
            return REG_UNDEFINED;
        }

    } else {
        pst_log(SEVERITY_ERROR, "Failed to get CFI expression for register 0x%X", regno);
    }

    return REG_CFI_ERROR;
#endif
}

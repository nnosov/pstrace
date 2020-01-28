/*
 * context.cpp
 *
 *  Created on: Jan 28, 2020
 *      Author: nnosov
 */

#include <limits.h>
#include <dwarf.h>

#include "context.h"
#include "registers.h"
#include "dwarf_operations.h"


extern SC_LogBase* logger;
extern dwarf_reg_map    reg_map[];
extern int regnum;

void __pst_context::print_stack(int max, uint64_t next_cfa)
{
    log(SEVERITY_DEBUG, "CFA = %#lX, NEXT_CFA = %#lX, SP = %#lX", cfa, next_cfa, sp);
    clean_print();
    int i = 0;
    if(cfa > sp) {
        print("Args: ");
        for(; i < max && (cfa - i) > sp; ++i) {
            print("#%d 0x%lX ", i, *(uint64_t*)(cfa - i));
        }
    }
    if((cfa - i) > next_cfa) {
        print("Vars: ");
        for(; i < max && (cfa - i) > next_cfa; ++i) {
            print("#%d 0x%lX ", i, *(uint64_t*)(cfa - i));
        }
    }
}

void __pst_context::print_registers(int from, int to)
{
    clean_print();
    for(int i = from; i < regnum && i <= to; ++i) {
        unw_word_t regval;
        if(!unw_get_reg(curr_frame, reg_map[i].regno, &regval)) {
            print("%s: %#lX ", reg_map[i].regname, regval);
        } else {
            print("%s: <undef>", reg_map[i].regname);
        }
    }
}

bool __pst_context::print(const char* fmt, ...)
{
    bool nret = true;

    va_list args;
    va_start(args, fmt);
    int size = sizeof(buff) - offset;
    int ret = vsnprintf(buff + offset, size, fmt, args);
    if(ret >= size || ret < 0) {
        nret = false;
    }
    offset += ret;
    va_end(args);

    return nret;
}

void __pst_context::log(SC_LogSeverity severity, const char* fmt, ...)
{
    uint32_t    str_len = 0;
    char        str[PATH_MAX]; str[0] = 0;
    va_list args;
    va_start(args, fmt);
    str_len += vsnprintf(str + str_len, sizeof(str) - str_len, fmt, args);
    va_end(args);

    logger->Log(severity, "%s", str);
}

uint32_t __pst_context::print_expr_block (Dwarf_Op *exprs, int len, char* buff, uint32_t buff_size, Dwarf_Attribute* attr)
{
    uint32_t offset = 0;
    for (int i = 0; i < len; i++) {
        //printf ("%s", (i + 1 < len ? ", " : ""));
        const dwarf_op_map* map = find_op_map(exprs[i].atom);
        if(map) {
            if(map->op_num >= DW_OP_breg0 && map->op_num <= DW_OP_breg16) {
                int32_t off = decode_sleb128((unsigned char*)&exprs[i].number);
                int regno = map->op_num - DW_OP_breg0;
                unw_word_t ptr = 0;
                unw_get_reg(curr_frame, regno, &ptr);
                //ptr += off;

                offset += snprintf(buff + offset, buff_size - offset, "%s(*%s%s%d) reg_value: 0x%lX", map->op_name, unw_regname(regno), off >=0 ? "+" : "", off, ptr);
            } else if(map->op_num >= DW_OP_reg0 && map->op_num <= DW_OP_reg16) {
                unw_word_t value = 0;
                int regno = map->op_num - DW_OP_reg0;
                unw_get_reg(curr_frame, regno, &value);
                offset += snprintf(buff + offset, buff_size - offset, "%s(*%s) value: 0x%lX", map->op_name, unw_regname(regno), value);
            } else if(map->op_num == DW_OP_GNU_entry_value) {
                uint32_t value = decode_uleb128((unsigned char*)&exprs[i].number);
                offset += snprintf(buff + offset, buff_size - offset, "%s(%u, ", map->op_name, value);
                Dwarf_Attribute attr_mem;
                if(!dwarf_getlocation_attr(attr, exprs, &attr_mem)) {
                    Dwarf_Op *expr;
                    size_t exprlen;
                    if (dwarf_getlocation(&attr_mem, &expr, &exprlen) == 0) {
                        offset += print_expr_block (expr, exprlen, buff + offset, buff_size - offset, &attr_mem);
                        offset += snprintf(buff + offset, buff_size - offset, ") ");
                    } else {
                        log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr location");
                    }
                } else {
                    log(SEVERITY_ERROR, "Failed to get DW_OP_GNU_entry_value attr expression");
                }
            } else if(map->op_num == DW_OP_stack_value) {
                offset += snprintf(buff + offset, buff_size - offset, "%s", map->op_name);
            } else if(map->op_num == DW_OP_plus_uconst) {
                uint32_t value = decode_uleb128((unsigned char*)&exprs[i].number);
                offset += snprintf(buff + offset, buff_size - offset, "%s(+%u) ", map->op_name, value);
            } else if(map->op_num == DW_OP_bregx) {
                uint32_t regno = decode_uleb128((unsigned char*)&exprs[i].number);
                int32_t off = decode_sleb128((unsigned char*)&exprs[i].number2);
                unw_word_t ptr = 0;
                unw_get_reg(curr_frame, regno, &ptr);
                //ptr += off;
                offset += snprintf(buff + offset, buff_size - offset, "%s(%s%s%d) reg_value = 0x%lX", map->op_name, unw_regname(regno), off >= 0 ? "+" : "", off, ptr);
            } else if(map->op_num == DW_OP_regx) {
                int32_t reg = decode_sleb128((unsigned char*)&exprs[i].number);

                unw_word_t value = 0;
                unw_get_reg(curr_frame, reg, &value);

                offset += snprintf(buff + offset, buff_size - offset, "%s(%s) value = 0x%lX", map->op_name, unw_regname(reg), value);
            } else if(map->op_num == DW_OP_addr) {
                offset += snprintf(buff + offset, buff_size - offset, "%s value = %p", map->op_name, (void*)exprs[i].number);
            } else if(map->op_num == DW_OP_fbreg) {
                int32_t off = decode_sleb128((unsigned char*)&exprs[i].number);
                offset += snprintf(buff + offset, buff_size - offset, "%s(SP%s%d) ", map->op_name, off >=0 ? "+" : "", off);
            } else {
                offset += snprintf(buff + offset, buff_size - offset, "%s(0x%lX, 0x%lx) ", map->op_name, exprs[i].number, exprs[i].number2);
            }
        } else {
            offset += snprintf(buff + offset, buff_size - offset, "0x%hhX(0x%lX, 0x%lx)", exprs[i].atom, exprs[i].number, exprs[i].number2);
        }
    }

    return offset;
}



/*
 * common.cpp
 *
 *  Created on: Jan 8, 2020
 *      Author: nnosov
 */

#include <inttypes.h>
#include <stddef.h>
#include <dwarf.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <elfutils/libdwfl.h>
#include <libunwind.h>

#include "logger/log.h"
#include "common.h"
#include "dwarf_operations.h"

extern SC_LogBase* logger;

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

				unw_word_t ptr = 0;
				unw_get_reg(&cursor, map->regno, &ptr);
				//ptr += off;

				offset += snprintf(buff + offset, buff_size - offset, "%s(*%s%s%d) reg_value: 0x%lX ", map->op_name, map->regname, off >=0 ? "+" : "", off, ptr);
			} else if(map->op_num >= DW_OP_reg0 && map->op_num <= DW_OP_reg16) {
				unw_word_t value = 0;
				unw_get_reg(&cursor, map->regno, &value);
				offset += snprintf(buff + offset, buff_size - offset, "%s(*%s) value: 0x%lX", map->op_name, map->regname, value);
			} else if(map->op_num == DW_OP_GNU_entry_value) {
				uint32_t value = decode_uleb128((unsigned char*)&exprs[i].number);
				offset += snprintf(buff + offset, buff_size - offset, "%s(%u, ", map->op_name, value);
				Dwarf_Attribute attr_mem;
				if(!dwarf_getlocation_attr(attr, exprs, &attr_mem)) {
					Dwarf_Op *expr;
					size_t exprlen;
					if (dwarf_getlocation(&attr_mem, &expr, &exprlen) == 0) {
						offset += print_expr_block (expr, exprlen, buff + offset, buff_size - offset, attr);
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
				unw_get_reg(&cursor, map->regno, &ptr);
				//ptr += off;

				offset += snprintf(buff + offset, buff_size - offset, "%s(%s%s%d) reg_value = 0x%lX", map->op_name, unw_regname(regno), off >= 0 ? "+" : "", off, ptr);
			} else if(map->op_num == DW_OP_regx) {
				int32_t reg = decode_sleb128((unsigned char*)&exprs[i].number);

				unw_word_t value = 0;
				unw_get_reg(&cursor, reg, &value);

				offset += snprintf(buff + offset, buff_size - offset, "%s(%s) value = 0x%lX", map->op_name, unw_regname(reg), value);
			} else if(map->op_num == DW_OP_addr) {
				offset += snprintf(buff + offset, buff_size - offset, "%s value = 0x%X", map->op_name, *((uint32_t*)exprs[i].number));
			} else {
				offset += snprintf(buff + offset, buff_size - offset, "%s(0x%lX, 0x%lx) ", map->op_name, exprs[i].number, exprs[i].number2);
			}
		} else {
			offset += snprintf(buff + offset, buff_size - offset, "0x%hhX(0x%lX, 0x%lx) ", exprs[i].atom, exprs[i].number, exprs[i].number2);
		}
//		if(exprs[i].atom >= DW_OP_reg0 && exprs[i].atom <= DW_OP_bregx) {
//			print_framereg(exprs[i].atom);
//		}DW_OP_addr
	}

	return offset;
}

bool is_location_form(int form)
{
    if (form == DW_FORM_block1 || form == DW_FORM_block2 || form == DW_FORM_block4 || form == DW_FORM_block ||
    	form == DW_FORM_data4  || form == DW_FORM_data8  || form == DW_FORM_sec_offset) {
        return true;
    }
    return false;
}


int32_t decode_sleb128(uint8_t *sleb128)
{
	int32_t num = 0, shift = 0, size = 0;
	do {
		num |= ((*sleb128 & 0x7f) << shift);
		shift += 7;
		size += 8;
	} while(*sleb128++ & 0x80);
	if((shift < size) && (*(--sleb128) & 0x40)) {
		num |= - (1 << shift);
	}
	return num;
}

uint32_t decode_uleb128(uint8_t *uleb128)
{
	uint32_t num = 0, shift = 0;
	do {
		num |= ((*uleb128 & 0x7f) << shift);
		shift += 7;
	} while(*uleb128++ & 0x80);
	return num;
}

// Utility function to encode a ULEB128 value to a buffer. Returns
// the length in bytes of the encoded value.
inline unsigned encodeULEB128(uint64_t Value, uint8_t *p, unsigned PadTo = 0)
{
	uint8_t *orig_p = p;
	unsigned Count = 0;
	do {
		uint8_t Byte = Value & 0x7f;
		Value >>= 7;
		Count++;
		if (Value != 0 || Count < PadTo)
			Byte |= 0x80; // Mark this byte to show that more bytes will follow.
		*p++ = Byte;
	} while (Value != 0);

	// Pad with 0x80 and emit a null byte at the end.
	if (Count < PadTo) {
		for (; Count < PadTo - 1; ++Count)
			*p++ = '\x80';
		*p++ = '\x00';
	}

	return (unsigned)(p - orig_p);
}

// Utility function to encode a SLEB128 value to a buffer. Returns
// the length in bytes of the encoded value.
inline unsigned encodeSLEB128(int64_t Value, uint8_t *p, unsigned PadTo = 0)
{
	uint8_t *orig_p = p;
	unsigned Count = 0;
	bool More;
	do {
		uint8_t Byte = Value & 0x7f;
		// NOTE: this assumes that this signed shift is an arithmetic right shift.
		Value >>= 7;
		More = !((((Value == 0 ) && ((Byte & 0x40) == 0)) ||
				((Value == -1) && ((Byte & 0x40) != 0))));
		Count++;
		if (More || Count < PadTo)
			Byte |= 0x80; // Mark this byte to show that more bytes will follow.
		*p++ = Byte;
	} while (More);

	// Pad with 0x80 and emit a terminating byte at the end.
	if (Count < PadTo) {
		uint8_t PadValue = Value < 0 ? 0x7f : 0x00;
		for (; Count < PadTo - 1; ++Count)
			*p++ = (PadValue | 0x80);
		*p++ = PadValue;
	}
	return (unsigned)(p - orig_p);
}

// Utility function to get the size of the ULEB128-encoded value.
unsigned getULEB128Size(uint64_t Value)
{
	unsigned Size = 0;
	do {
		Value >>= 7;
		Size += sizeof(int8_t);
	} while (Value);

	return Size;
}

// Utility function to get the size of the SLEB128-encoded value.
unsigned getSLEB128Size(int64_t Value)
{
	unsigned Size = 0;
	int Sign = Value >> (8 * sizeof(Value) - 1);
	bool IsMore;

	do {
		unsigned Byte = Value & 0x7f;
		Value >>= 7;
		IsMore = Value != Sign || ((Byte ^ Sign) & 0x40) != 0;
		Size += sizeof(int8_t);
	} while (IsMore);

	return Size;
}
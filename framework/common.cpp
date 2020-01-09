/*
 * common.cpp
 *
 *  Created on: Jan 8, 2020
 *      Author: nnosov
 */

#include <inttypes.h>
#include <stddef.h>

#include "common.h"

dwarf_op_map dw_op[] = {
		{0x23, 0x0,  0, "DW_OP_plus_uconst"},
		// Register location descriptions. From DWARF 5, section 2.6.1.1.3:
		// Register location descriptions describe an object (or a piece of an object) that resides in a register.
		// A register location description must stand alone as the entire description of an object or a piece of an object.
		{0x50, 0x0,  "RAX", "DW_OP_reg0"},
		{0x51, 0x1,  "RDX", "DW_OP_reg1"},
		{0x52, 0x2,  "RCX", "DW_OP_reg2"},
		{0x53, 0x3,  "RBX", "DW_OP_reg3"},
		{0x54, 0x4,  "RSI", "DW_OP_reg4"},
		{0x55, 0x5,  "RDI", "DW_OP_reg5"},
		{0x56, 0x6,  "RBP", "DW_OP_reg6"},
		{0x57, 0x7,  "RSP", "DW_OP_reg7"},
		{0x58, 0x8,  "R8",  "DW_OP_reg8"},
		{0x59, 0x9,  "R9",  "DW_OP_reg9"},
		{0x5A, 0xA,  "R10", "DW_OP_reg10"},
		{0x5B, 0xB,  "R11", "DW_OP_reg11"},
		{0x5C, 0xC,  "R12", "DW_OP_reg12"},
		{0x5D, 0xD,  "R13", "DW_OP_reg13"},
		{0x5E, 0xE,  "R14", "DW_OP_reg14"},
		{0x5F, 0xF,  "R15", "DW_OP_reg15"},
		{0x60, 0x10, "RIP", "DW_OP_reg16"},

		// Register values. DWARF5, section 2.5.1.2
		// Register values are used to describe an object (or a piece of an object) that is located in memory at an address that is contained in
		// a register (possibly offset by some constant)
		{0x70, 0x0,  "RAX", "DW_OP_breg0"},
		{0x71, 0x1,  "RDX", "DW_OP_breg1"},
		{0x72, 0x2,  "RCX", "DW_OP_breg2"},
		{0x73, 0x3,  "RBX", "DW_OP_breg3"},
		{0x74, 0x4,  "RSI", "DW_OP_breg4"},
		{0x75, 0x5,  "RDI", "DW_OP_breg5"},
		{0x76, 0x6,  "RBP", "DW_OP_breg6"},
		{0x77, 0x7,  "RSP", "DW_OP_breg7"},
		{0x78, 0x8,  "R8",  "DW_OP_breg8"},
		{0x79, 0x9,  "R9",  "DW_OP_breg9"},
		{0x7A, 0xA,  "R10", "DW_OP_breg10"},
		{0x7B, 0xB,  "R11", "DW_OP_breg11"},
		{0x7C, 0xC,  "R12", "DW_OP_breg12"},
		{0x7D, 0xD,  "R13", "DW_OP_breg13"},
		{0x7E, 0xE,  "R14", "DW_OP_breg14"},
		{0x7F, 0xF,  "R15", "DW_OP_breg15"},
		{0x80, 0x10, "RIP", "DW_OP_breg16"},

		// The DW_OP_fbreg operation provides a signed LEB128 offset from the address specified by the location description in the DW_AT_frame_base
		// attribute of the current function. This is typically a stack pointer register plus or minus some offset
		{0x91, -1,   "",     "DW_OP_fbreg"},
		{0x92, -1,   "",     "DW_OP_bregx"},
		{0x9C, -1,   "",     "DW_OP_call_frame_cfa"},
		// DWARF5, Section 2.6.1.1.4:
		// he DW_OP_stack_value operation specifies that the object does not exist in memory but its value is nonetheless known and is at the top of the DWARF
		// expression stack. In this form of location description, the DWARF expression represents the actual value of the object, rather than its location.
		// The DW_OP_stack_value operation terminates the expression.
		{0x9F, -1,   "",     "DW_OP_stack_value"},
		//
		{0xF3, -1,   "",     "DW_OP_GNU_entry_value"},
};

const dwarf_op_map* find_op_map(int op)
{
	for(uint32_t i = 0; i < sizeof(dw_op) / sizeof(dwarf_op_map); ++i) {
		if(dw_op[i].op_num == op) {
			return &dw_op[i];
		}
	}

	return NULL;
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


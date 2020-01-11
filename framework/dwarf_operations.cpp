/*
 * dwarf_operations.cpp
 *
 *  Created on: Jan 11, 2020
 *      Author: nnosov
 */

#include <dwarf.h>
#include "dwarf_operations.h"

bool dw_op_addr(pst_context* ctx, const dwarf_op_map* map, Dwarf_Word op1, Dwarf_Word op2)
{
	return true;
}

dwarf_op_map dw_op[] = {
		{0x03, -1,   0, "DW_OP_addr", dw_op_addr},
		{0x23, 0x0,  0, "DW_OP_plus_uconst"},
		// Register location descriptions. From DWARF 5, section 2.6.1.1.3:
		// Register location descriptions describe an object (or a piece of an object) that resides in a register.
		// A register location description must stand alone as the entire description of an object or a piece of an object.

		// GP Registers
		{0x50, 0x0,  "RAX", "DW_OP_reg0"},
		{0x51, 0x1,  "RDX", "DW_OP_reg1"},
		{0x52, 0x2,  "RCX", "DW_OP_reg2"},
		{0x53, 0x3,  "RBX", "DW_OP_reg3"},
		{0x54, 0x4,  "RSI", "DW_OP_reg4"},
		{0x55, 0x5,  "RDI", "DW_OP_reg5"},
		{0x56, 0x6,  "RBP", "DW_OP_reg6"},
		{0x57, 0x7,  "RSP", "DW_OP_reg7"},
		// Extended GP Registers
		{0x58, 0x8,  "R8",  "DW_OP_reg8"},
		{0x59, 0x9,  "R9",  "DW_OP_reg9"},
		{0x5A, 0xA,  "R10", "DW_OP_reg10"},
		{0x5B, 0xB,  "R11", "DW_OP_reg11"},
		{0x5C, 0xC,  "R12", "DW_OP_reg12"},
		{0x5D, 0xD,  "R13", "DW_OP_reg13"},
		{0x5E, 0xE,  "R14", "DW_OP_reg14"},
		{0x5F, 0xF,  "R15", "DW_OP_reg15"},
		{0x60, 0x10, "RIP", "DW_OP_reg16"}, // Return Address (RA) mapped to RIP
		// SSE Vector Registers
		{0x61, 0x11, "XMM0", "DW_OP_reg17"},
		{0x62, 0x12, "XMM1", "DW_OP_reg18"},
		{0x63, 0x13, "XMM2", "DW_OP_reg19"},
		{0x64, 0x14, "XMM3", "DW_OP_reg20"},
		{0x65, 0x15, "XMM4", "DW_OP_reg21"},
		{0x66, 0x16, "XMM5", "DW_OP_reg22"},
		{0x67, 0x17, "XMM6", "DW_OP_reg23"},
		{0x68, 0x18, "XMM7", "DW_OP_reg24"},
		{0x69, 0x19, "XMM8", "DW_OP_reg25"},
		{0x6a, 0x1a, "XMM9", "DW_OP_reg26"},
		{0x6b, 0x1b, "XMM10", "DW_OP_reg27"},
		{0x6c, 0x1c, "XMM11", "DW_OP_reg28"},
		{0x6d, 0x1d, "XMM12", "DW_OP_reg29"},
		{0x6e, 0x1e, "XMM13", "DW_OP_reg30"},
		{0x6f, 0x1f, "XMM14", "DW_OP_reg31"},

		// Register values. DWARF5, section 2.5.1.2
		// Register values are used to describe an object (or a piece of an object) that is located in memory at an address that is contained in
		// a register (possibly offset by some constant)

		// GP Registers
		{0x70, 0x0,  "RAX", "DW_OP_breg0"},
		{0x71, 0x1,  "RDX", "DW_OP_breg1"},
		{0x72, 0x2,  "RCX", "DW_OP_breg2"},
		{0x73, 0x3,  "RBX", "DW_OP_breg3"},
		{0x74, 0x4,  "RSI", "DW_OP_breg4"},
		{0x75, 0x5,  "RDI", "DW_OP_breg5"},
		{0x76, 0x6,  "RBP", "DW_OP_breg6"},
		{0x77, 0x7,  "RSP", "DW_OP_breg7"},
		// Extended GP Registers
		{0x78, 0x8,  "R8",  "DW_OP_breg8"},
		{0x79, 0x9,  "R9",  "DW_OP_breg9"},
		{0x7A, 0xA,  "R10", "DW_OP_breg10"},
		{0x7B, 0xB,  "R11", "DW_OP_breg11"},
		{0x7C, 0xC,  "R12", "DW_OP_breg12"},
		{0x7D, 0xD,  "R13", "DW_OP_breg13"},
		{0x7E, 0xE,  "R14", "DW_OP_breg14"},
		{0x7F, 0xF,  "R15", "DW_OP_breg15"},
		{0x80, 0x10, "RIP", "DW_OP_breg16"}, // Return Address (RA) mapped to RIP
		// SSE Vector Registers
		{0x81, 0x11, "XMM0", "DW_OP_breg17"},
		{0x82, 0x12, "XMM1", "DW_OP_breg18"},
		{0x83, 0x13, "XMM2", "DW_OP_breg19"},
		{0x84, 0x14, "XMM3", "DW_OP_breg20"},
		{0x85, 0x15, "XMM4", "DW_OP_breg21"},
		{0x86, 0x16, "XMM5", "DW_OP_breg22"},
		{0x87, 0x17, "XMM6", "DW_OP_breg23"},
		{0x88, 0x18, "XMM7", "DW_OP_breg24"},
		{0x89, 0x19, "XMM8", "DW_OP_breg25"},
		{0x8a, 0x1a, "XMM9", "DW_OP_breg26"},
		{0x8b, 0x1b, "XMM10", "DW_OP_breg27"},
		{0x8c, 0x1c, "XMM11", "DW_OP_breg28"},
		{0x8d, 0x1d, "XMM12", "DW_OP_breg29"},
		{0x8e, 0x1e, "XMM13", "DW_OP_breg30"},
		{0x8f, 0x1f, "XMM14", "DW_OP_breg31"},

		// The DW_OP_regx operation has a single unsigned LEB128 literal operand that encodes the name of a register
		{0x90, -1,   0,     "DW_OP_regx"},
		// The DW_OP_fbreg operation provides a signed LEB128 offset from the address specified by the location description in the DW_AT_frame_base
		// attribute of the current function. This is typically a stack pointer register plus or minus some offset
		{0x91, -1,   0,     "DW_OP_fbreg"},
		// The DW_OP_bregx operation provides the sum of two values specified by its two operands.
		// The first operand is a register number which is specified by an unsigned LEB128 number. The second operand is a signed LEB128 offset.
		{0x92, -1,   0,     "DW_OP_bregx"},
		{0x9C, -1,   0,     "DW_OP_call_frame_cfa"},
		// DWARF5, Section 2.6.1.1.4:
		// The DW_OP_stack_value operation specifies that the object does not exist in memory but its value is nonetheless known and is at the top of the DWARF
		// expression stack. In this form of location description, the DWARF expression represents the actual value of the object, rather than its location.
		// The DW_OP_stack_value operation terminates the expression.
		{0x9F, -1,   0,     "DW_OP_stack_value"},
		// This opcode has two operands, the first one is uleb128 length and the second is block of that length, containing either a
		// simple register or DWARF expression
		{0xF3, -1,   0,     "DW_OP_GNU_entry_value"},
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

#pragma once


typedef struct __dwarf_op_map {
	int				op_num;		// DWARF Operation DW_OP_XXX
	int				regno;		// platform-dependent register number if present
	const char*		regname;	// register name if regno specified
	const char* 	op_name;	// string representation of an operation
} dwarf_op_map;

const dwarf_op_map* find_op_map(int op);
int32_t decode_sleb128(uint8_t *sleb128);
uint32_t decode_uleb128(uint8_t *uleb128);

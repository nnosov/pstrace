#pragma once

#include <stdint.h>
#include <string.h>

#include "context.h"

typedef struct __dwarf_reg_map {
    int             regno;      // platform-dependent register number
    const char*     regname;    // register name
    uint32_t        op_num;     // number of DWARF DW_OP_reg(x)/DW_OP_breg(x) operation corresponds to this register
} dwarf_reg_map;

int find_regnum(uint32_t op);

typedef enum {
    REG_OK = 0,
    REG_UNDEFINED,
    REG_SAME,
    REG_CFI_ERROR,
    REG_EXPR_ERROR
} pst_reg_error;

pst_reg_error pst_get_reg(pst_context* ctx, int regno, uint64_t& regval);

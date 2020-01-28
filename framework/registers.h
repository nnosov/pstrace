#pragma once

#include <stdint.h>


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

typedef struct __pst_frame : public SC_ListNode {
    __pst_frame() {
        memset(reg_state, REG_UNDEFINED, sizeof(reg_state));
        memset(regs, 0, sizeof(regs));
        cfa = 0;
    }
    int32_t     reg_state[128];
    uint64_t    regs[128];
    uint64_t    cfa;
} pst_frame;

pst_reg_error pst_get_reg(pst_context* ctx, int regno, uint64_t& regval);

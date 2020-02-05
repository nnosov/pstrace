/*
 * dwarf_utils.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef __PST_DWARF_UTILS_H__
#define __PST_DWARF_UTILS_H__

#include <elfutils/libdw.h>

#include "context.h"
#include "dwarf_expression.h"
#include "dwarf_function.h"

typedef struct __reginfo {
    char regname[32];
    int regno;
} reginfo;

int regname_callback (void *arg, int regno, const char *setname, const char *prefix, const char *regname, int bits, int type);
bool handle_location(pst_context* ctx, Dwarf_Attribute* attr, pst_dwarf_expr* loc, Dwarf_Addr pc, pst_function* fun);
bool is_location_form(int form);

#endif /* __PST_DWARF_UTILS_H__ */

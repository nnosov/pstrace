/*
 * dwarf_utils.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef FRAMEWORK_DWARF_UTILS_H_
#define FRAMEWORK_DWARF_UTILS_H_

#include <elfutils/libdw.h>

#include "dwarf_expression.h"
#include "context.h"
#include "sysutils.h"

typedef struct __reginfo {
    __reginfo() {
        regname[0] = 0;
        regno = -1;
    }
    char regname[32];
    int regno;
} reginfo;

int regname_callback (void *arg, int regno, const char *setname, const char *prefix, const char *regname, int bits, int type);
bool handle_location(pst_context* ctx, Dwarf_Attribute* attr, pst_dwarf_expr* loc, Dwarf_Addr pc, pst_function* fun);

#endif /* FRAMEWORK_DWARF_UTILS_H_ */

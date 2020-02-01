/*
 * dwarf_utils.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#include <dwarf.h>

#include "dwarf_utils.h"
#include "dwarf_stack.h"

int regname_callback (void *arg, int regno, const char *setname, const char *prefix, const char *regname, int bits, int type)
{
    reginfo* info = (reginfo*)arg;
    if(info->regno == regno) {
        snprintf(info->regname, sizeof(info->regname), "%s %s%s", setname, prefix, regname);
    }

    return 0;
}

bool handle_location(pst_context* ctx, Dwarf_Attribute* attr, pst_dwarf_expr* loc, Dwarf_Addr pc, pst_function* fun = NULL)
{
    ctx->clean_print(ctx);
    Dwarf_Addr offset = pc - ctx->base_addr;

    pst_decl(pst_dwarf_stack, stack, ctx);
    Dwarf_Op *expr;
    size_t exprlen;
    bool ret = false;
    if(dwarf_hasform(attr, DW_FORM_exprloc)) {
        // Location expression (exprloc class of location in DWARF terms)
        if(dwarf_getlocation(attr, &expr, &exprlen) == 0) {
            loc->setup(expr, exprlen);
            ctx->print_expr(ctx, expr, exprlen, attr);
            ret = stack.calc(&stack, expr, exprlen, attr, fun);
        }
    } else if(dwarf_hasform(attr, DW_FORM_sec_offset)) {
        // Location list (loclist class of location in DWARF terms)
        Dwarf_Addr base, start, end;
        ptrdiff_t off = 0;

        // handle list of possible locations of parameter
        for(int i = 0; (off = dwarf_getlocations (attr, off, &base, &start, &end, &expr, &exprlen)) > 0; ++i) {
            ctx->print_expr(ctx, expr, exprlen, attr);
            if(offset >= start && offset <= end) {
                loc->setup(expr, exprlen);
                // actual location, try to calculate Location expression
                ret = stack.calc(&stack, expr, exprlen, attr, fun);
            } else {
                // Location skipped due to don't match current PC offset
                ctx->log(SEVERITY_DEBUG, "Skip Location list expression: [%d] (low_offset: 0x%" PRIx64 ", high_offset: 0x%" PRIx64 "), \"%s\"", i, start, end, ctx->buff);
            }
        }

    } else {
        ctx->log(SEVERITY_WARNING, "Unknown location attribute form = 0x%X, code = 0x%X, ", attr->form, attr->code);
    }

    pst_dwarf_stack_fini(&stack);

    return ret;
}


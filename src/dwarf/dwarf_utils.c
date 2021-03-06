/*
 * dwarf_utils.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#include <dwarf.h>

#include "dwarf_utils.h"
#include "dwarf_stack.h"
#include "dwarf_function.h"


bool is_location_form(int form)
{
    if (form == DW_FORM_block1 || form == DW_FORM_block2 || form == DW_FORM_block4 || form == DW_FORM_block ||
        form == DW_FORM_data4  || form == DW_FORM_data8  || form == DW_FORM_sec_offset) {
        return true;
    }
    return false;
}

int regname_callback (void *arg, int regno, const char *setname, const char *prefix, const char *regname, int bits, int type)
{
    reginfo* info = (reginfo*)arg;
    if(info->regno == regno) {
        snprintf(info->regname, sizeof(info->regname), "%s %s%s", setname, prefix, regname);
    }

    return 0;
}

bool handle_location(pst_context* ctx, Dwarf_Attribute* attr, pst_dwarf_expr* loc, Dwarf_Addr pc, pst_function* fun)
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
            pst_dwarf_expr_setup(loc, expr, exprlen);
            ctx->print_expr(ctx, expr, exprlen, attr);
            ret = pst_dwarf_stack_calc(&stack, expr, exprlen, attr, fun);
            pst_dwarf_stack_get_value(&stack, &loc->value);
        }
    } else if(dwarf_hasform(attr, DW_FORM_sec_offset)) {
        // Location list (loclist class of location in DWARF terms)
        Dwarf_Addr base, start, end;
        ptrdiff_t off = 0;

        // handle list of possible locations of parameter
        for(int i = 0; (off = dwarf_getlocations (attr, off, &base, &start, &end, &expr, &exprlen)) > 0; ++i) {
            ctx->print_expr(ctx, expr, exprlen, attr);
            if(offset >= start && offset <= end) {
                pst_dwarf_expr_setup(loc, expr, exprlen);
                // actual location, try to calculate Location expression
                ret = pst_dwarf_stack_calc(&stack, expr, exprlen, attr, fun);
                pst_dwarf_stack_get_value(&stack, &loc->value);
            } else {
                // Location skipped due to don't match current PC offset
                pst_log(SEVERITY_DEBUG, "Skip Location list expression: [%d] (low_offset: 0x%" PRIx64 ", high_offset: 0x%" PRIx64 "), \"%s\"", i, start, end, ctx->buff);
            }
        }

    } else {
        pst_log(SEVERITY_WARNING, "Unknown location attribute form = 0x%X, code = 0x%X, ", attr->form, attr->code);
    }

    pst_dwarf_stack_fini(&stack);

    return ret;
}


/*
 * dwarf_call_site.cpp
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#include <dwarf.h>

#include "dwarf_call_site.h"
#include "dwarf_utils.h"

// -----------------------------------------------------------------------------------
// pst_call_site_param
// -----------------------------------------------------------------------------------
void param_set_location(pst_call_site_param* param, Dwarf_Op* expr, size_t exprlen)
{
    param->location.setup(&param->location, expr, exprlen);
}

void pst_call_site_param_init(pst_call_site_param* param)
{
    // methods
    param->set_location = param_set_location;

    // fields
    param->param = NULL;
    param->name = NULL;
    param->value = 0;
    pst_dwarf_expr_init(&param->location);
    param->allocated = false;
}

pst_call_site_param* pst_call_site_param_new()
{
    pst_call_site_param* param = pst_alloc(pst_call_site_param);
    if(param) {
        pst_call_site_param_init(param);
        param->allocated = true;
    }

    return param;
}

void pst_call_site_param_fini(pst_call_site_param* param)
{
    if(param->name) {
        pst_free(param->name);
        param->name = NULL;
    }

    if(param->allocated) {
        pst_free(param);
    }
}

// -----------------------------------------------------------------------------------
// pst_call_site
// -----------------------------------------------------------------------------------
pst_call_site_param* site_add_param(pst_call_site* site)
{
    pst_call_site_param* p = pst_call_site_param_new();
    pst_call_site_param_init(p);
    list_add_bottom(&site->params, &p->node);

    return p;
}

void site_del_param(pst_call_site*, pst_call_site_param* p)
{
    list_del(&p->node);
    pst_call_site_param_fini(p);
}

pst_call_site_param* site_next_param(pst_call_site* site, pst_call_site_param* p)
{
    list_node* n = (p == NULL) ? list_first(&site->params) : list_next(&p->node);
    pst_call_site_param* ret = NULL;
    if(n) {
        ret = list_entry(n, pst_call_site_param, node);
    }

    return ret;
}

pst_call_site_param* site_find_param(pst_call_site* site, pst_dwarf_expr* expr)
{
    for(pst_call_site_param* param = site_next_param(site, NULL); param; param = site_next_param(site, param)) {
        if(param->location.is_equal(&param->location, expr)) {
            return param;
        }
    }

    return NULL;
}

bool site_handle_dwarf(pst_call_site* site, Dwarf_Die* child)
{
    Dwarf_Attribute attr_mem;
    Dwarf_Attribute* attr;

    do {
        switch(dwarf_tag(child))
        {
            case DW_TAG_GNU_call_site_parameter: {
                Dwarf_Addr pc;
                unw_get_reg(site->ctx->curr_frame, UNW_REG_IP,  &pc);

                // expression represent where callee parameter will be stored
                pst_call_site_param* param = site->add_param(site);
                if(dwarf_hasattr(child, DW_AT_location)) {
                    // handle location expression here
                    // determine location of parameter in stack/heap or CPU registers
                    attr = dwarf_attr(child, DW_AT_location, &attr_mem);
                    if(!handle_location(site->ctx, attr, &param->location, pc, NULL)) {
                        pst_log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", site->ctx->buff);
                        site->del_param(site, param);
                        return false;
                    }
                }

                // expression represents call parameter's value
                if(dwarf_hasattr(child, DW_AT_GNU_call_site_value)) {
                    // handle value expression here
                    attr = dwarf_attr(child, DW_AT_GNU_call_site_value, &attr_mem);
                    pst_dwarf_expr loc;
                    pst_dwarf_expr_init(&loc);
                    if(handle_location(site->ctx, attr, &loc, pc, NULL)) {
                        param->value = loc.value;
                        pst_log(SEVERITY_DEBUG, "\tDW_AT_GNU_call_site_value:\"%s\" ==> 0x%lX", site->ctx->buff, param->value);
                    } else {
                        pst_log(SEVERITY_ERROR, "Failed to calculate DW_AT_location expression: %s", site->ctx->buff);
                        site->del_param(site, param);
                        return false;
                    }
                }
                break;
            }
            default:
                break;
        }
    } while (dwarf_siblingof (child, child) == 0);

    return true;
}

void pst_call_site_init(pst_call_site* site, pst_context* c, uint64_t tgt, const char* orn) {
    // methods


    // fields
    list_node_init(&site->node);

    site->target = tgt;
    if(orn) {
        site->origin = strdup(orn);
    } else {
        site->origin = NULL;
    }
    site->die = NULL;
    list_head_init(&site->params);
    site->ctx = c;
    site->allocated = false;
}

pst_call_site* pst_call_site_new(pst_context* c, uint64_t tgt, const char* orn)
{
    pst_call_site* nc = pst_alloc(pst_call_site);
    if(nc) {
        pst_call_site_init(nc, c, tgt, orn);
        nc->allocated = true;
    }

    return nc;
}

void pst_call_site_fini(pst_call_site* site)
{
    if(site->origin) {
        pst_free(site->origin);
        site->origin = NULL;
    }

    if(site->allocated) {
        pst_free(site);
    }
}


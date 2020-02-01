/*
 * dwarf_call_site.h
 *
 *  Created on: Feb 1, 2020
 *      Author: nnosov
 */

#ifndef FRAMEWORK_DWARF_CALL_SITE_H_
#define FRAMEWORK_DWARF_CALL_SITE_H_

#include <inttypes.h>
#include <elfutils/libdw.h>

#include "list_head.h"
#include "context.h"
#include "dwarf_expression.h"


// -----------------------------------------------------------------------------------
// DW_TAG_call_site_parameter
// -----------------------------------------------------------------------------------
typedef struct pst_call_site_param {
    list_node           node;   // uplink. !!! must be first !!!

    // methods
    void (*set_location) (pst_call_site_param* param, Dwarf_Op* expr, size_t exprlen);

    // fields
    Dwarf_Die*                  param;      // reference to parameter DIE in callee (DW_AT_call_parameter)
    char*                       name;       // name of parameter if present (DW_AT_name)
    pst_dwarf_expr              location;   // DWARF stack containing location expression
    uint64_t                    value;      // parameter's value
    bool                        allocated;  // whether this object was allocated or not
} pst_call_site_param;

void pst_call_site_param_init(pst_call_site_param* param);
pst_call_site_param* pst_call_site_param_new();
void pst_call_site_param_fini(pst_call_site_param* param);

// -----------------------------------------------------------------------------------
// DW_TAG_call_site
// -----------------------------------------------------------------------------------
typedef struct pst_call_site {
    list_node       node;   // uplink. !!! must be first !!!

    // methods
    pst_call_site_param*    (*add_param)    (pst_call_site* site);
    void                    (*del_param)    (pst_call_site* site, pst_call_site_param* p);
    pst_call_site_param*    (*next_param)   (pst_call_site* site, pst_call_site_param* p);
    pst_call_site_param*    (*find_param)   (pst_call_site* site, pst_dwarf_expr& expr);
    bool                    (*handle_dwarf) (pst_call_site* site, Dwarf_Die* die);

    // fields
    uint64_t        target;     // pointer to callee function's (it's Low PC + base address)
    char*           origin;     // name of callee function
    Dwarf_Die*      die;        // DIE of function for which this call site has parameters
    list_head       params;     // list of parameters and their values
    pst_context*    ctx;        // execution context
    bool            allocated;  // whether this object was allocated or not
} pst_call_site;


#endif /* FRAMEWORK_DWARF_CALL_SITE_H_ */

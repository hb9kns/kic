/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright (c) 1986 Wayne A. Christopher, U. C. Berkeley CAD Group 
 *     faustus@cad.berkeley.edu, ucbvax!faustus
 * Permission is granted to modify and re-distribute this code in any manner
 * as long as this notice is preserved.  All standard disclaimers apply.
 *
 *************************************************************************/

#include "prefix.h"
#include "hlpdefs.h"

#if __STDC__
static void killfamily(topic*);
#else
static void killfamily();
#endif


void
hlp_provide(top)

topic *top;
{
    toplink *res;
    topic *parent, *newtop, *pa;

    if (top == NULL) return;

    top->xposition = top->yposition = 0;
    if (!hlp_tdisplay(top)) {
        err_printf("Couldn't display text\n");
        return;
    }
    
    for (;;) {
        res = hlp_thandle(&parent);
        if (!res && !parent) {
            /* No more windows. */
            killfamily(top);
            return;
        }
        if (res) {
            /* Create a new window... */
            if (!(newtop = hlp_read(res->keyword))) {
                err_printf("Internal error: bad link\n");
                continue;
            }
            newtop->next = parent->children;
            parent->children = newtop;
            newtop->parent = parent;
            newtop->xposition = parent->xposition + 50;
            newtop->yposition = parent->yposition + 50;
            if (!hlp_tdisplay(newtop)) {
                fprintf(cp_err, "Couldn't display\n"); 
                return; 
            }
        }
        else {
            /* Blow this one and its descendants away. */
            killfamily(parent);
            if (parent->parent) {
                if (parent->parent->children == parent)
                    parent->parent->children =
                            parent->next;
                else {
                    for (pa = parent->parent->children;
                            pa->next; pa = pa->next)
                        if (pa->next == parent)
                            break;
                    if (!pa->next) {
                        err_printf("bah...\n");
                    }
                    pa->next = pa->next->next;
                }
            }
            if (parent == top)
                return;
        }
    }
}

/* Note that this doesn't actually free the data structures, just gets
 * rid of the window.
 */

static void
killfamily(top)

topic *top;
{
    topic *ch;

    for (ch = top->children; ch; ch = ch->next)
        killfamily(ch);
    hlp_tkillwin(top);
    top->children = NULL;
    return;
}

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

/*
 * The main entry point for the KIC help system.
 */

#include "prefix.h"
#include "hlpdefs.h"

char *hlp_directory;
FILE *cp_in;
FILE *cp_out;
FILE *cp_err;
char *HELPPATH;
int out_isatty;


void
hlp_main(path, word)

char *path;
char *word;
{
    topic *top;

    HELPPATH = path;
    out_isatty = false;

    hlp_directory = path;

    if (!(top = hlp_read(word))) {
        if (word && *word)
            err_printf("Error: No such topic: %s\n",word);
        else
            err_printf("Error: no top level topic\n");

    }
    hlp_provide(top);
    
    hlp_free();
    return;
}


#ifdef MSDOS

void
hlp_pathfix(buf)

char *buf;
{
    char *s, *t;
    int bcnt = 0, ecnt = 0;

    s = t = buf;
    while (*t != '\0') {
        if (*t == '/' || *t == '\\') {
            *s++ = '\\';
            t++;
            bcnt = 0;
            ecnt = 0;
        }
        else if (*t == '.') {
            *s++ = *t++;
            ecnt = 1;
        }
        else if (!ecnt) {
            if (bcnt++ < 8) *s++ = *t++;
            else t++;
        }
        else {
            if (ecnt++ < 4) *s++ = *t++;
            else t++;
        }
    }
    *s = *t;
}

#else

void
hlp_pathfix(buf)

char *buf;
{
    return;
}

#endif

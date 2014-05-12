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
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#ifdef WIN32
#include <io.h>
#endif

static topic *curtop;
static bool quitflag;

#if __STDC__
static void putline(char*);
static int putstuff(toplink*,int);
#else
static void putline();
static int putstuff();
#endif

int hlp_width = 72;


bool
hlp_tdisplay(top)

topic *top;
{
    wordlist *wl;
    int i = 0;
    char *xx;

    curtop = top;

    out_init();
    out_printf("\n");
    if (cp_out == stdout) {
        /* in order for the cursor to display the right color,
         * column 1 must have the default attribute */
        fprintf(cp_out," ");
        fflush(cp_out);
        cprint(15,top->title);
        out_printf("\n");
    }
    else
        out_cprint("%s\n",top->title);
    for (wl = top->text; wl; wl = wl->wl_next)
        putline(wl->wl_word);
    if (top->subtopics) {
        xx = "Sub-Topics:";
        if (cp_out == stdout) {
            fprintf(cp_out," ");
            fflush(cp_out);
            cprint(9,xx);
            out_printf("\n");
        }
        else
            out_cprint("%s\n",xx);
        i = putstuff(top->subtopics, 0);
        out_printf("\n");
    }
    if (top->seealso) {
        xx = "See Also:";
        if (cp_out == stdout) {
            fprintf(cp_out," ");
            fflush(cp_out);
            cprint(10,xx);
            out_printf("\n");
        }
        else
            out_cprint("%s\n",xx);
        (void) putstuff(top->seealso, i);
        out_printf("\n");
    }
    if (!top->subtopics && !top->seealso)
        out_printf("\n");
    return (true);
}


toplink *
hlp_thandle(parent)

topic **parent;
{
    char buf[BSIZE], *s;
    toplink *tl;
    int num;

    quitflag = false;
    if (!curtop) {
        *parent = NULL;
        return (NULL);
    }
    for (;;) {
        s = "Selection (h for help) ";
        if (cp_out == stdout) {
            /* in order for the cursor to display the right color,
             * column 1 must have the default attribute */
            fprintf(cp_out," ");
            fflush(cp_out);
            cprint(11,s);
        }
        else {
            fprintf(cp_out,"%s ",s);
            fflush(cp_out);
        }

#if defined (SCED) || defined (KIC)
        s = GetString(buf,BSIZE,cp_in,
            (curtop->subtopics || curtop->seealso) ? "Selection? " : NULL);
#else
        s = fgets(buf,BSIZE,cp_in);
#endif
        if (!s) {
            clearerr(stdin);
            quitflag = true;
            *parent = NULL;
            return (NULL);
        }

        while (*s == ' ' || *s == '\t') s++;
        switch (*s) {
        char *xx;

        case '?':
        case 'h':
xx = "\nType the number of a sub-topic or see also, or one of:\n\
    h or ?       Print this message\n\
    r [ >file ]  Reprint the current topic [ to file ]\n\
    p or CR      Return to the previous topic\n\
    q            Quit help\n";
            if (cp_out == stdout) {
                cprint(14,xx);
                fprintf(cp_out," \n");
            }
            else
                fprintf(cp_out,"%s\n",xx);
            continue;

        case 'r':
            {
            char *ss;
            FILE *tmp;
            bool ttytmp;

            if ((ss = strchr(s,'\n')) != NULL)
                *ss = '\0';

            if ((ss = strchr(s,'>')) == NULL) {
                (void) hlp_tdisplay(curtop);
                continue;
            }    
            tmp = cp_out;
            ss++;
            if (*ss == '>') {
                ss++;
                cp_out = fopen(ss,"a");
            }
            else
                cp_out = fopen(ss,"w");
            if (cp_out) {
                ttytmp = out_isatty;
                out_isatty = isatty(fileno(cp_out));
                (void) hlp_tdisplay(curtop);
                out_isatty = ttytmp;
                (void) fclose(cp_out);
            }
            else
                perror(ss);
            cp_out = tmp;
            continue;
            }

        case 'q':
            quitflag = true;
            *parent = NULL;
            return NULL;

        case 'p':
        case '\n':
        case '\r':
            *parent = curtop;
            return NULL;
        }
        if (!isdigit(*s)) {
            fprintf(cp_out,"%s\n","Invalid command");
            continue;
        }
        num = atoi(s);
        if (num <= 0) {
            fprintf(cp_out,"%s\n","Bad choice");
            continue;
        }
        for (tl = curtop->subtopics; tl; tl = tl->next)
            if (--num == 0)
                break;
        if (num) {
            for (tl = curtop->seealso; tl; tl = tl->next)
                if (--num == 0)
                    break;
        }
        if (num) {
            fprintf(cp_out,"%s\n","Bad choice");
            continue;
        }
        *parent = curtop;
        return tl;
    }
}


/* ARGSUSED */
void
hlp_tkillwin(top)

topic *top;
{
    if (curtop)
        curtop = curtop->parent;
    if (curtop && !quitflag)
        (void) hlp_tdisplay(curtop);
    return;
}


/* This has to rip out the font changes from the lines... */

static void
putline(s)

char *s;
{
    char buf[BSIZE];
    int i = 0;

    while (*s) {
        if (((*s == '\033') && s[1]) ||
                ((*s == '_') && (s[1] == '\b')))
            s += 2;
        else
            buf[i++] = *s++;
    }
    buf[i] = '\0';
    out_printf("%s\n", buf);
    return;
}


/* Figure out the number of columns we can use.  Assume an entry like
 * nn) word -- add 5 characters to the width...
 */

static int
putstuff(tl, base)

toplink *tl;
int base;
{
    int maxwidth = 0, ncols, nrows, nbuts = 0, i, j, k;
    toplink *tt;

    for (tt = tl; tt; tt = tt->next) {
        if (strlen(tt->description) + 5 > maxwidth)
            maxwidth = strlen(tt->description) + 5;
        nbuts++;
    }
    ncols = hlp_width / maxwidth;
    if (!ncols) {
        err_printf("Help, button too big!!\n");
        return (0);
    }
    if (ncols > nbuts)
        ncols = nbuts;
    maxwidth = hlp_width / ncols;
    nrows = nbuts / ncols;
    if (nrows * ncols < nbuts)
        nrows++;

    for (i = 0; i < nrows; i++) {
        for (tt = tl, j = 0; j < i; j++, tt = tt->next)
            ;
        for (j = 0; j < ncols; j++) {
            if (tt)
                out_printf("%2d) %-*s ", base + j*nrows + i + 1,
                    maxwidth - 5, tt->description);
            for (k = 0; k < nrows; k++)
                if (tt)
                    tt = tt->next;
            
        }
        out_printf("\n");
    }
    return (nbuts);
}

#define MAXLEN 4096


#if __STDC__

void
out_printf(char *fmt, ...)

{
    va_list args;
    char buf[MAXLEN];

    va_start(args,fmt);
    (void) vsprintf(buf, fmt, args);
    va_end(args);

    PutString(buf);
    return;
}


void
err_printf(char *fmt, ...)

{
    va_list args;
    char buf[MAXLEN];

    va_start(args,fmt);
    (void) vsprintf(buf, fmt, args);
    va_end(args);

    PutErrorString(buf);
    return;
}


void
out_cprint(char *fmt, ...)

{
    va_list args;
    char buf[MAXLEN];

    va_start(args,fmt);
    (void) vsprintf(buf, fmt, args);
    va_end(args);

    PutBoldString(buf);
    return;
}


#else


void
out_printf(va_alist)
va_dcl
{
    va_list args;
    char *fmt;
    char buf[MAXLEN];

    va_start(args);
    fmt = va_arg(args,char *);
#if __NDPC__
    (void) vsprintf(buf, fmt, &args);
#else
    (void) vsprintf(buf, fmt, args);
#endif
    va_end(args);

    PutString(buf);
    return;
}


void
err_printf(va_alist)
va_dcl
{
    va_list args;
    char *fmt;
    char buf[MAXLEN];

    va_start(args);
    fmt = va_arg(args,char *);
#if __NDPC__
    (void) vsprintf(buf, fmt, &args);
#else
    (void) vsprintf(buf, fmt, args);
#endif
    va_end(args);

    PutErrorString(buf);
    return;
}


void
out_cprint(va_alist)
va_dcl
{
    va_list args;
    char *fmt;
    char buf[MAXLEN];

    va_start(args);
    fmt = va_arg(args,char *);
#if __NDPC__
    (void) vsprintf(buf, fmt, &args);
#else
    (void) vsprintf(buf, fmt, args);
#endif
    va_end(args);

    PutBoldString(buf);
    return;
}

#endif  /* __STDC__ */


char *
copy(str)

char *str;
{
    char *p;
    
    /* SRW */
    if (!str) return NULL;

    p = TMALLOC(strlen(str) + 1);
    (void) strcpy(p, str);

    return(p);
}


/* Case insensitive prefix. */

bool
ciprefix(p, s)

register char *p, *s;
{
    if (!p) return true;
    if (!s) return false;

    while (*p) {
        if ((isupper(*p) ? tolower(*p) : *p) !=
            (isupper(*s) ? tolower(*s) : *s))
            return(false);
        p++;
        s++;
    }
    return (true);
}


void
wl_free(wlist)

wordlist *wlist;
{
    wordlist *wl, *nw;

    for (wl = wlist; wl; wl = nw) {
        nw = wl->wl_next;
        free(wl->wl_word);
        free(wl);
    }
    return;
}


#ifdef MSDOS

char *
dostemp(s)

/* return full path to temp file */

char *s;
{
    static char buf[128], *c, *getenv();

    c = getenv("TMP");

    if (c && !access(c,0)) {
        strcpy(buf,c);
        strcat(buf,"\\");
        strcat(buf,s);
        return buf;
    }
    return s;
}

#endif


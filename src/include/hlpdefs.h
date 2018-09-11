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

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define KIC

/* name of help database file */
#define DBFILE "kic_help.txt"

typedef int bool;

#define false 0
#define true 1

/* Doubly linked lists of words. */

struct wordlist {
    char *wl_word;
    struct wordlist *wl_next;
    struct wordlist *wl_prev;
};
typedef struct wordlist wordlist;

#define BSIZE 256

#ifndef alloc
#define alloc(zz) (struct zz *) tmalloc(sizeof(struct zz))
#endif
#define TMALLOC tmalloc

#define eq(x,y) (!strcmp((x),(y)))
#define cieq(x,y) (!strcasecmp((x),(y)))

#if __STDC__
extern char *tmalloc(unsigned);
extern char *GetString(char*,int,FILE*,char*);
extern void PutString(char*);
#else
extern char *tmalloc();
extern char *GetString();
extern void PutString();
#endif

typedef struct button {
    char *text;
    char *tag;
    int x;
    int y;
    int width;
    int height;
} button;

typedef struct toplink {
    char *description;
    char *keyword;
    struct toplink *next;
    button button;
} toplink;

typedef struct topic {
    char *title;
    char *keyword;
    wordlist *text;
    toplink *subtopics;
    toplink *seealso;
    int xposition;
    int yposition;
    struct topic *parent;
    struct topic *children;
    struct topic *next;
    struct topic *winlink;
    struct topic *readlink;
    int numlines;
    int maxcols;
    int curtopline;
} topic;

/* External symbols. */

/* help.c */
extern char  *hlp_directory;
extern FILE *cp_in;
extern FILE *cp_out;
extern FILE *cp_err;
extern char *HELPPATH;
extern int  out_isatty;
#if __STDC__
extern void hlp_main(char*,char*);
extern void hlp_pathfix(char*);
#else
extern void hlp_main();
extern void hlp_pathfix();
#endif

/* provide.c */
#if __STDC__
extern void hlp_provide(topic*);
#else
extern void hlp_provide();
#endif

/* readhelp.c */
#if __STDC__
extern topic *hlp_read(char*);
extern void hlp_free(void);
#else
extern topic *hlp_read();
extern void hlp_free();
#endif

/* textdisp.c */
#if __STDC__
extern bool hlp_tdisplay(topic*);
extern toplink *hlp_thandle(topic**);
extern void hlp_tkillwin(topic*);
extern void out_init(void);
extern void out_printf(char*, ...);
extern void err_printf(char*, ...);
extern void out_cprint(char*, ...);
extern char *copy(char*);
extern bool ciprefix(char*,char*);
extern void wl_free(wordlist*);
extern char *dostemp(char*);
extern void cprint(int, char*);
extern void PutErrorString(char*);
extern void PutBoldString(char*);
#else
extern bool hlp_tdisplay();
extern toplink *hlp_thandle();
extern void hlp_tkillwin();
extern void out_init();
extern void out_printf();
extern void err_printf();
extern void out_cprint();
extern char *copy();
extern bool ciprefix();
extern void wl_free();
extern char *dostemp();
extern void cprint();
extern void PutErrorString();
extern void PutBoldString();
#endif


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
 * The main routine for the help system in stand-alone mode.
 */

#include "prefix.h"
#include "hlpdefs.h"
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#if __STDC__
static void moreprompt(void);
extern void cprint(int,char*);
static int  getattr(void);
#else
static void moreprompt();
extern void cprint();
static int  getattr();
#endif

int xsize;
int ysize;


int
main(ac, av)

int ac;
char **av;
{
#ifdef TIOCGWINSZ
    struct winsize ws;

    (void) ioctl(fileno(stdout), TIOCGWINSZ, (char *) &ws);
    xsize = ws.ws_col;
    ysize = ws.ws_row;
#else
#ifdef WIN32
    if (!xsize || !ysize) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE) {
            CONSOLE_SCREEN_BUFFER_INFO sbi;
            if (GetConsoleScreenBufferInfo(h, &sbi)) {
                xsize = sbi.dwSize.X;
                ysize = sbi.dwSize.Y;
            }
        }
    }
#endif
#endif

    if (!xsize) xsize = 80;
    if (!ysize) ysize = 23;


    InitGlobal();
    HELPPATH  = PATH_TO_HELP;
    cp_in = stdin;
    cp_out = stdout;
    cp_err = stderr;

    if (ac > 1)
        hlp_main(HELPPATH,av[1]);
    else
        hlp_main(HELPPATH,"help");
    return (0);
}


FILE *OpenDevice() {return NULL;}


void
fatal(s)

char *s;
{
    fprintf(stderr, "fatal error: %s\n", s);
    exit(1);
}


void
cp_printword(s)

char *s;
{
    printf("%s", s);
    return;
}


/* ARGSUSED */
bool
cp_getvar(n, t, r)

char *n, *r;
int t;
{
    return (false);
}


char *
GetString(s,n,fp,p)

char *s;
int n;
FILE *fp;
char *p;
{ return fgets(s,n,fp); }


int noprint,nopause,xpos,ypos;

void
out_init()
{
    xpos = ypos = 0;
}


void
PutString(string)

char *string;
{

    if (noprint) return;

    if (nopause) {
        fputs(string, cp_out);
        return;
    }
    while (*string) {
        switch (*string) {
            case '\n':
                xpos = 0;
                ypos++;
                break;
            case '\f':
                ypos = ysize;
                xpos = 0;
                break;
            case '\t':
                xpos = xpos / 8 + 1;
                xpos *= 8;
                break;
            default:
                xpos++;
                break;
        }
        if (xpos >= xsize) {
            xpos -= xsize;
            ypos++;
            (void) putc('\n',cp_out);
        }
        else {
            (void) putc(*string, cp_out);
            string++;
        }
        moreprompt();
    }
}


void
PutErrorString(string)

char *string;
{
    fputs(string,cp_err);
}


void
PutBoldString(string)

char *string;
{
    fputs(string,cp_out);
}


static void
moreprompt()

{
    short key;
    char *xx, attr;
    char *menu =
"\nPossible responses:\n\
    q      : Discard the rest of the output.\n\
    c      : Continuously print the rest of the output.\n\
    h,?    : Print this help message.\n\
    other  : Print the next page of output.\n";

    if (ypos >= ysize) {
        attr = getattr();
        for (;;) {
            xx = "        -- More -- (h for help) ";
            if (cp_out == stdout)
                cprint(4,xx);
            else {
                (void) fprintf(cp_out,"%s",xx);
                (void) fflush(cp_out);
            }
#ifdef MSDOS
            while (!(key = getch()));
#else
            key = getc(cp_in);
#endif

            if (cp_out == stdout)
                cprint(attr,"\r                                 \r");

            if (key == 'q') {noprint = true; break;}
            if (key == 'c') {nopause = true; break;}
            if (key == 'h' || key == '?') {
                if (cp_out == stdout) {
                    cprint(2,menu);
                    cprint(attr," \n"); /* keeps the cursor the attr color */
                }
                else
                    (void) fprintf(cp_out,"%s",menu);
                continue;
            }
            break;
        }
        if (cp_out != stdout) {
            (void) putc('\n',cp_out);
            (void) fflush(cp_out);
        }
        ypos = xpos = 0;
    }
}


char *
tmalloc(x)

unsigned x;
{
    char *c = (char*)malloc(x);
    if (c == NULL) {
        fprintf(cp_err,"Out of memory.\n");
        exit(1);
    }
    memset(c,0,x);
    return (c);
}


#ifdef MSDOS
/*************************************************************************
 Graphics and miscellaneous library for protected mode MSDOS.
 Copyright (c) Stephen R. Whiteley 1992
 Author: Stephen R. Whiteley
 *************************************************************************/

#include <dos.h>
#include <ctype.h>
#if __NDPC__
#define REGS REGS16
#endif

#ifdef __STDC__
static short getpage(void);
#else
static short getpage();
#endif


void
cprint(clr,buf)

/* print buf in color clr */
int clr;
char *buf;
{
    union REGS r;

    while (*buf) {
        /* change attribute by printing space */
        r.x.bx = getpage() + clr & 0xff;
        r.x.cx = 1;
        r.x.ax = 0x0900 + ' ';
        int86(0x10,&r,&r);

        /* now print character, advance cursor */
        r.x.bx = getpage();
        r.x.ax = 0x0e00 + *buf;
        int86(0x10,&r,&r);

        /* special case, go to beginning of line */
        if (*buf == '\n') {
            r.x.bx = getpage();
            r.x.ax = 0x0e00 + '\r';
            int86(0x10,&r,&r);
        }
        buf++;
    }
}


static int
getattr()

/* return the attribute at current position */
{
    union REGS r;

    r.x.bx = getpage();
    r.x.ax = 0x0800;
    int86(0x10,&r,&r);
    return (r.x.ax >> 8);
}


static short
getpage()

/* return the current display page in upper byte */
{
    union REGS r;

    r.x.ax = 0x0f00;
    int86(0x10,&r,&r);
    return (r.x.bx & 0xff00);
}

#else

void
cprint(i,s)

int i;
char *s;
{
    (void) fprintf(cp_out,"%s",s);
    (void) fflush(cp_out);
}


static int
getattr()

{
    return (0);
}

#endif


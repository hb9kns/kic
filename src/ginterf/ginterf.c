/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1994
Copyright 1990 Regents of the University of California.  All rights reserved.
Author:  1992 Stephen R. Whiteley
         1994 SRW modified for KIC
****************************************************************************/

/*
 * Hardcopy driver (Postscript and HP Laserjet)
 */

#include <stdio.h>
#ifdef MSDOS
#include <dos.h>
#endif
#include "kic.h"
#include "driver.h"

/* define this for Postscript RLL/Ascii85 encoding (Level 2 feature) */
/*
#define PSRLL85
*/

extern char *tmalloc();

#define NFILLPAT 20

static FILE *plotfile;

struct sGText {
    int x;
    int y;
    int xform;
    char *text;
    struct sGText *next;
};

struct sGParams {
    int bytpline;
    int maxx;
    int maxy;
    char *base;
    struct sGText *textlist;
    unsigned char linestyle;
    unsigned char linestyle_stored;
    int stipples[NFILLPAT][8];
    int curfillpatt;
    int fontwidth;
    int fontheight;
    int numcolors;
    int numlinestyles;
};
static struct sGParams GP;

#define ror(x,n) ((x >> n)  | (x << (8-n)))
#define swap(a,b) {int t=a; a=b; b=t;}
#define pswap(tab,i,j) {int temp; temp = *((tab)+(i));\
            *((tab)+(i)) = *((tab)+(j));\
            *((tab)+(j)) = temp; }

/* dots per inch (75, 100, 150, 300) */
#define RESOL  Parameters.kpHardcopyResolution

/* US Letter defaults for Postscript */
#define Uxoff 18
#define Uyoff 18
#define Uwidth  576
#define Uheight  756

/* how big the font is in points=1/72 inch */
#define FONTPTS 12


#ifdef __STDC__
extern void HP_dump(void);
extern void PS_dump(void);
static void sort(int*,int,int);
static void zoids(int*,int*,int,int,int);
extern void PS_rll85dump(char*,int);
#ifdef MSDOS
static void dos_fopen(char*);
#endif
#else
extern void HP_dump();
extern void PS_dump();
static void sort();
static void zoids();
extern void PS_rll85dump();
#ifdef MSDOS
static void dos_fopen();
#endif
#endif


int
GR_info(int num)
{
    switch (num) {
    case MAXX:             return (GP.maxx);
    case MAXY:             return (GP.maxy);
    case MAXCOLORS:        return (GP.numcolors-1);
    case MAXINTENSITY:     return (255);
    case MAXFILLPATTERNS:  return (0);
    case MAXLINESTYLES:    return (GP.numlinestyles-1);
    case MAXBLINKERS:      return (0);
    case POINTINGDEVICE:   return (0);
    case POINTINGBUTTONS:  return (0);
    case NUMBUTTONS:       return (0);
    case BUTTON1:          return (1);
    case BUTTON2:          return (2);
    case BUTTON3:          return (3);
    case BUTTON4:          return (4);
    case BUTTON5:          return (0);
    case BUTTON6:          return (0);
    case BUTTON7:          return (0);
    case BUTTON8:          return (0);
    case BUTTON9:          return (0);
    case BUTTON10:         return (0);
    case BUTTON11:         return (0);
    case BUTTON12:         return (0);
    case TEXTPOSITIONABLE: return (1);
    case TEXTROTATABLE:    return (1);
    case FONTHEIGHT:       return (GP.fontheight);
    case FONTWIDTH:        return (GP.fontwidth);
    case FONTXOFFSET:      return (0);
    case FONTYOFFSET:      return (0);
    case DESTRUCTIVETEXT:  return (0);
    case OVERSTRIKETEXT:   return (1);
    case VLT:              return (0);
    case BLINKERS:         return (0);
    case FILLEDPOLYGONS:   return (1);
    case DEFFILLPATTERNS:  return (1);
    case DEFLINEPATTERN:   return (1);
    case CURFGCOLOR:       return (1);
    case CURFILLPATTERN:   return (GP.curfillpatt);
    case CURLINESTYLE:     return (GP.linestyle);
    case NUMBITPLANES:     return (1);

    default:               return (-1);
    }
}


int
GR_open(char *name)
{
    struct sGParams *gp = &GP;
    int bpline;

#ifdef MSDOS
    dos_fopen(name);
#else
    plotfile = fopen(name, "w");
#endif

    if (!plotfile) {
        return (1);
    }

    /* 8" X 10.5" drawable area */
    gp->maxx = 8*RESOL - 1;
    gp->maxy = 10*RESOL + RESOL/2 - 1;

    /* bytes per line */
    bpline = gp->maxx/8 + 1;

    gp->base = tmalloc((gp->maxy+1)*bpline);
    memset(gp->base,0,(gp->maxy+1)*bpline);

    gp->bytpline = bpline;
    gp->linestyle = 0xff;
    gp->linestyle_stored = 0xcc;
    gp->curfillpatt = 0;
    gp->textlist = NULL;
    gp->fontheight = FONTPTS*RESOL/72;
    gp->fontwidth = 2*gp->fontheight/3;
    gp->numcolors = 2;
    gp->numlinestyles = 2;
    return (0);
}


int
GR_close(void)
{
    struct sGParams *gp = &GP;
    struct sGText *t, *tn;

    /* dump the stuff */
    switch (*Parameters.kpHardcopyFormat) {
    case HPLASER:
        HP_dump();
        break;
    case POSTSC:
        PS_dump();
        break;
    default:
        break;
    }
    for (t = gp->textlist; t; t = tn) {
        tn = t->next;
        free(t->text);
        free(t);
    }
    gp->textlist = NULL;
    free(gp->base);
    gp->base = NULL;
    fclose(plotfile);
    return (0);
}


/* upper left of image in pcl coordinates */
#define OFFSETX 0
#define OFFSETY 90

void
HP_dump(void)
{
    struct sGParams *gp = &GP;
    struct sGText *t;
    char *buf, *rgen, *c;
    int len, i;
    int x, y;

    /* dump the stuff */

    /* reset printer
     * top margin 10
     * X cursor position OFFSETX
     * Y cursor position OFFSETY
     * resolution RESOL dpi
     * start raster graphics at current position
     */
    fprintf(plotfile,
        "\033E\033&l0E\033*p%dX\033*p%dY\033*t%dR\033*r1A",
        OFFSETX,OFFSETY,RESOL);

    buf = tmalloc(gp->bytpline+8);
    sprintf(buf,"\033*b%dW",gp->bytpline);
    len = strlen(buf);
    c = buf + len;

    rgen = gp->base;
    for (i = 0; i <= gp->maxy; i++) {
        memcpy(c,rgen,gp->bytpline);
        rgen += gp->bytpline;

        if (fwrite(buf,1,gp->bytpline+len,plotfile) != gp->bytpline+len) {
            break;
        }
        
    }
    /* end raster graphics */
    fprintf(plotfile,"\033*rB");

    /* now for the text */

    for (t = gp->textlist; t; t = t->next) {
        x = t->x*(300/RESOL) + OFFSETX;
        y = (gp->maxy - t->y)*(300/RESOL) + OFFSETY;
        /* x position, y position, text */
        fprintf(plotfile,"\033*p%dX\033*p%dY%s",x,y,t->text);
    }

    /* form feed
     * reset printer
     */
    fprintf(plotfile,"\033*rB\014\033E");

    free(buf);
}


static char hexc[16] =
    { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

void
PS_dump(void)
{
    struct sGParams *gp = &GP;
    struct sGText *t;
    int x, y;
    int w, h, i, j;
    int deg;
    int cnt;
    char *ptr, *s, *text, tbuf[512];
    unsigned char c;

    w = gp->maxx + 1;
    h = gp->maxy + 1;

    /* start file off with a % */
    fprintf(plotfile, "%%! kic plot\n");

    /* set up a reasonable font */
    fprintf(plotfile, "/Helvetica findfont %d scalefont setfont\n",
        (int)(FONTPTS*((double)w/Uwidth)));

#ifndef PSRLL85
    fprintf(plotfile, "/pixbuf %d string def\n",gp->bytpline);
#endif
    fprintf(plotfile, "%d %d translate\n",Uxoff,Uyoff);
    fprintf(plotfile, "%f %f scale\n",(double)Uwidth/w,(double)Uheight/h);
    fprintf(plotfile, "%d %d 1\n",w,h);
    fprintf(plotfile, "matrix\n");
#ifdef PSRLL85
    fprintf(plotfile, "currentfile\n");
    fprintf(plotfile, "/Ascii85Decode filter\n");
    fprintf(plotfile, "/RunLengthDecode filter\n");
    fprintf(plotfile, "image\n");
#else
    fprintf(plotfile, "{currentfile pixbuf readhexstring pop}\nimage\n");
#endif

    /* write data */

#ifdef PSRLL85
    PS_rll85dump(gp->base,(gp->maxy+1)*gp->bytpline);
#else
    ptr = gp->base + gp->maxy*gp->bytpline;
    cnt = 0;
    for (j = gp->maxy; j >= 0; j--) {
        for (i = 0; i < gp->bytpline; i++) {
            c = ~*(unsigned char*)ptr; /* reverse black/white */
            putc(hexc[c>>4],plotfile);
            putc(hexc[c&0xf],plotfile);
            ptr++;
            cnt++;
            if (cnt > 38) {
                cnt = 0;
                putc('\n',plotfile);
            }
        }
        ptr -= gp->bytpline << 1;
    }
#endif
    /*
    fprintf(plotfile, "\n~>\n");
    */
    fprintf(plotfile, "\n");

    /* now for the text */
    for (t = gp->textlist; t; t = t->next) {
        x = t->x;
        y = t->y;

        s = tbuf;
        text = t->text;
        while (*text) {
            if (*text == '(' || *text == ')' || *text == '\\')
                *s++ = '\\';
            *s++ = *text++;
        }
        *s = '\0';
        fprintf(plotfile,"gsave\n");
        fprintf(plotfile,"%d %d translate\n",x,y);
        if (t->xform & 12)
            fprintf(plotfile,"%d %d scale\n",
                (t->xform & 8) ? -1 : 1, (t->xform & 4) ? -1 : 1);
        deg = (t->xform & 3) * 90;
        if (deg)
            fprintf(plotfile,"%d rotate\n",deg);
        fprintf(plotfile,"0 0 moveto\n");
        fprintf(plotfile,"(%s) show\n",tbuf);
        fprintf(plotfile,"grestore\n");
    }
    fprintf(plotfile, "showpage\n");
}


int
GR_update(void)
{
    return (0);
}


void
GR_pixel(int x, int y)
{
	unsigned char c;
    struct sGParams *gp = &GP;

    c = 0x80 >> (x & 7);
	*(gp->base + ((x >> 3) + (gp->maxy-y)*gp->bytpline)) |= c;
}


void
GR_line(int x1, int y1, int x2, int y2)
{

    struct sGParams *gp = &GP;
	int dx, dy, dy2, errterm = 0, next, lcnt;
	char *rgen;
	unsigned char cbuf, left, right;

	if (x2 < x1) {
		swap(x1,x2);
		swap(y1,y2);
	}
	dx = x2 - x1;

	next = gp->bytpline;
	dy = y1 - y2;

	lcnt = gp->maxy - y1;
	rgen = gp->base + (x1 >> 3) + lcnt*next;
	left = 0x80 >> ((lcnt+x1) & 7);
	right = gp->linestyle;

	if (y1 < y2) {
		next = -next;
		dy = -dy;
	}
	dy2 = dy;

	cbuf = 0x80 >> (x1 & 7);

    for (dy++; dy; dy--) {
		errterm += dx;
		if (errterm <= 0) {
			if (left & right) *rgen |= cbuf;
			rgen += next;
			left = ror(left,1);
			continue;
		}
		while (errterm > 0 && x1 != x2) {
			if (left & right) *rgen |= cbuf;
			left = ror(left,1);
			cbuf = ror(cbuf,1);
			if (cbuf & 0x80) rgen++;
			x1++;
			errterm -= dy2;
		}
		rgen += next;
		left = ror(left,1);
	}
}


void
GR_box(int xl, int yl, int xu, int yu)
{
    struct sGParams *gp = &GP;
    int lnum, dx, tx, dy, next, *st = NULL;
    char *rgen;
    unsigned char left, right, cbuf;

    if (yu < yl) swap(yu,yl);
    if (xu < xl) swap(xu,xl);

    /* expand box so that it covers a box formed of lines with the
     * same coordinates
     */
    xu++;
    yl--;

    if (xl < 0)
        xl = 0;
    else if (xl > gp->maxx)
        return;

    if (xu > gp->maxx)
        xu = gp->maxx;
    else if (xu < 0)
        return;

    if (yl < 0)
        yl = 0;
    else if (yl > gp->maxy)
        return;

    if (yu > gp->maxy)
        yu = gp->maxy;
    else if (yu < 0)
        return;


    if (gp->curfillpatt)
    st = gp->stipples[gp->curfillpatt];

    lnum = (gp->maxy - yu);
    dy = yu - yl;

    left  =  (0xff >> (xl & 7));
    right = ~(0xff >> (xu & 7));
    dx = (xu >> 3) - (xl >>= 3) - 1;
    if (dx < 0) { left &= right; dx = 0; right = 0; }
    rgen = gp->base + xl + (int) lnum*gp->bytpline;
    next = gp->bytpline - 1 - dx;

    cbuf = 0xff;

    while (dy--) {
        if (st)
            cbuf = st[lnum++ & 7];
        *rgen |= left & cbuf;
        rgen++;
        tx = dx;
        while (tx--) {
            *rgen |= cbuf;
            rgen++;
        }
        *rgen |= right & cbuf;
        rgen += next;
    }
}


void
GR_polygon(Poly *poly)
{
    int i, twonvert, *line, *line0 = NULL, stored = 0;
    int *xup, *ll, ny;
    int *xlp, *xip;
    int yl, yu;
    int *ylist;
    int *xulist;
    int *xllist;
    int *xilist;
    
    twonvert = poly->nvertices << 1;
    line = (int *) poly->xy;
    if ((line[0] != line[twonvert-2]) || (line[1] != line[twonvert-1])) {
        /* no closure, copy and add first point */
        twonvert += 2;
        line0 = (int*)tmalloc(twonvert*2*sizeof(int));
        line = line0;
        memcpy(line,(int*)poly->xy,(twonvert-2)*2*sizeof(int));
        line[twonvert-2] = line[0];
        line[twonvert-1] = line[1];
        stored = 1;
    }
    ylist = (int*)tmalloc(twonvert*4*sizeof(int));
    xulist = ylist + twonvert;
    xllist = xulist + twonvert;
    xilist = xllist + twonvert;

    xip = line + twonvert;
    ll = ylist;
    while (line < xip) {
        *ll++ = *(line+1);
        line += 2;
    }
    ny = ll-ylist-1;
    sort(ylist,0,ny);

    xip = xilist;
    ll = ylist;
    while (ny--) {
        yl = *ll;
        yu = *(++ll);
        while (yu == yl) {
            if (!ny) goto done;
            ny--;
            yu = *(++ll);
        }
        memcpy(xllist,xilist,(int)((char *) xip - (char *) xilist));
        xlp = xllist + (int) (xip - xilist);
        xup = xulist;
        xip = xilist;
        
        for (i = twonvert - 4; i >= 0; i -= 2) {
            line = (int *) poly->xy + i;
            if (((line[1] > yu) && (line[3] < yu)) ||
                ((line[3] > yu) && (line[1] < yu))) {

                *xup++ = *xip++ = line[0] +
                    ((int)(yu-line[1])*(line[2]-line[0])) /
                    (line[3]-line[1]);
            }
            if (line[1] == yu) {

                if (line[3] < yu)
                    *xup++ = line[0];
                
                if (i) {
                    if (*(line-1) < yu)
                        *xup++ = line[0];
                }
                else
                    if (line[twonvert-3] < yu)
                        *xup++ = line[0];
            }
            if (line[1] == yl) {

                if (line[3] > yl)
                    *xlp++ = line[0];
                
                if (i) {
                    if (*(line-1) > yl)
                        *xlp++ = line[0];
                }
                else
                    if (line[twonvert-3] > yl)
                        *xlp++ = line[0];
            }
        }

        sort(xulist,0,(int)(xup-xulist-1));
        sort(xllist,0,(int)(xlp-xllist-1));

        zoids(xllist,xulist,(int)(xlp-xllist),yl,yu);
    }
done:
    if (stored)
        free(line0);
    free(ylist);
}


static void
sort(int *tab, int left, int right)
{
    int i, last;

    if (right <= left) return;
    i = (left + right) >> 1;
    pswap(tab,left,i);
    last = left;
    for (i = left + 1; i <= right; i++)
        if (tab[i] < tab[left]) {
            last++;
            pswap(tab,last,i);
        }
    pswap(tab,left,last);
    sort(tab,left,last-1);
    sort(tab,last+1,right);
}


static void
zoids(int *xll, int *xul, int num, int yl, int yu)
{
    struct sGParams *gp = &GP;
    int x1, x2, x3, x4;
    int dx, dy, dy2, dx1, dx2, errterm1, errterm2;
    int i, s1, s2, lnum, lcnt, next, *st = NULL;
    char *rgen, *rgen0, *rgen1;
    unsigned char left, right, cbuf;

    if (gp->curfillpatt)
        st = gp->stipples[gp->curfillpatt];

    lnum = (gp->maxy - yu);

    dy2 = yu - yl;

    next = gp->bytpline;
    rgen1 = gp->base + (int) lnum*next;

    for (i = 0; i < num; i += 2) {

        rgen0 = rgen1;

        x1 = xll[i];
        x4 = xll[i+1];
        x2 = xul[i];
        x3 = xul[i+1];

        dy = dy2 + 1;

        dx1 = x1 - x2;
        dx2 = x3 - x4;
        s1 = 1;
        s2 = -1;
        if (x2 >= x1) {    dx1 = -dx1; s1 = -1; }
        if (x4 >= x3) {    dx2 = -dx2; s2 = 1; }
        errterm1 = errterm2 = 0;
        x3++;
        x4++;

        cbuf = 0xff;
        lcnt = lnum;
        while (dy--) {
            left  =  (0xff >> (x2 & 7));
            right = ~(0xff >> (x3 & 7));
            dx = (x3 >> 3) - (x2 >> 3) - 1;
            if (dx < 0) { left &= right; dx = 0; right = 0; }
            rgen = rgen0 + (x2 >> 3);
            if (st)
                cbuf = st[lcnt++ & 7];
            if (rgen >= gp->base) {
                *rgen |= cbuf & left;
            }
            rgen++;
            while (dx--) {
                if (rgen >= gp->base) {
                    *rgen |= cbuf;
                }
                rgen++;
            }
            if (rgen >= gp->base) {
                *rgen |= cbuf & right;
            }
            rgen0 += next;

            errterm1 += dx1;
            errterm2 += dx2;
            while (errterm1 > 0 && x1 != x2) {
                errterm1 -= dy2;
                x2 += s1;
            }
            while (errterm2 > 0 && x3 != x4) {
                errterm2 -= dy2;
                x3 += s2;
            }
        }
    }
}


int
GR_defLs(int linestyleid, int mask)
{
    struct sGParams *gp = &GP;

    if (linestyleid) {
        gp->linestyle_stored = mask;
        gp->linestyle = mask;
    }
    return (0);
}


int
GR_setLs(int linestyleid)
{
    struct sGParams *gp = &GP;
    
    if (linestyleid)
        gp->linestyle = gp->linestyle_stored;
    else
        gp->linestyle = 0xff;
    return (0);
}


int
GR_defFp(int num, int *pat)
{
    struct sGParams *gp = &GP;
    int i = 0, j;
    int ba, sp;

    if (num < 1 || num >= NFILLPAT)
        return (1);
    /* reverse bytes and bits */
    pat += 7;
    while (i < 8) {
        ba = *(pat--);
        sp = 0;
        for (j = 0; j < 8; j++) {
            sp <<= 1;
            if (ba & 1) sp |= 1;
            ba >>= 1;
        }
        gp->stipples[num][i++] = sp;
    }
    gp->curfillpatt = num;
    return (0);
}


int
GR_setFp( int num)
{
    struct sGParams *gp = &GP;

    if (num < 0 || num >= NFILLPAT)
        return (1);
    gp->curfillpatt = num;
    return (0);
}


/* ARGSUSED */
int
GR_defColor(int num, int r, int g, int b)
{
    return (0);
}


/* ARGSUSED */
int
GR_setColor(int num)
{
    return (0);
}


void
GR_text(char *text, int x, int y, int xform)
{
    struct sGParams *gp = &GP;
    struct sGText *gpt;

    gpt = (struct sGText *)tmalloc(sizeof(struct sGText));
    gpt->x = x;
    gpt->y = y;
    gpt->xform = xform;
    gpt->text = CopyString(text);
    gpt->next = gp->textlist;
    gp->textlist = gpt;
}


/* ARGSUSED */
void
GR_scText(char *text, int x, int y, int Xform, int sc)
{
    GR_text(text,x,y,Xform);
}


#ifdef PSRLL85

/**************************************************
 *                                                *
 * Run length and ascii85 encoding for Postscript *
 *                                                *
 **************************************************/

struct sRL {
    unsigned char key;
    unsigned char data[128];
};

#ifdef __STDC__
static struct sRL *get_rllrec(char**,char*);
static void ascii85dump(struct sRL*);
static void enc85(unsigned char *, unsigned char*);
#else
static struct sRL *get_rllrec();
static void ascii85dump();
static void enc85();
#endif

static int Ocnt;
static int I4;


void
PS_rll85dump(char *data, int size)
{
    char *end;
    struct sRL *rl;

    end = data + size;
    Ocnt = 0;
    I4 = 0;
    

    for (;;) {
        rl = get_rllrec(&data,end);
        ascii85dump(rl);
        if (rl->key == 128)
            break;
    }
    return;
}


static struct sRL *
get_rllrec(char **data, char *end)
{
    static struct sRL rl;
    unsigned char c1, c2, *d;
    int i;

    d = (unsigned char *)*data;

    if (d == (unsigned char*)end) {
        rl.key = 128;
        rl.data[0] = 0;
        return (&rl);
    }

    c1 = *d++;
    if (d == (unsigned char*)end) {
        rl.key = 129;
        rl.data[0] = c1;
        *data = (char*)d;
        return (&rl);
    }
    if (*d == c1) {
        for (i = 0; i < 127; i++) {
            c2 = *d++;
            if (c2 != c1) {
                break;
            }
            if (d == (unsigned char*)end)
                break;
        }
        rl.key = 256 - i;
        rl.data[0] = c1;
        *data = (char*)d;
        return (&rl);
    }
    rl.data[0] = c1;
    for (i = 1; i < 128; i++) {
        if (*d == *(d+1))
            break;
        rl.data[i] = *d;
        d++;
    }
    rl.key = i-1;
    *data = (char*)d;
    return (&rl);
}


#define outc(c) { \
putc(c,plotfile); \
Ocnt++; \
if (Ocnt==72){putc('\n',plotfile); Ocnt = 0;} \
}

static void
ascii85dump(struct sRL *rl)
{
    unsigned char cout[5];
    static unsigned char cin[4];
    unsigned int l;
    int i, n;

    if (rl->key == 128) {
        for (i = I4; i < 4; i++)
            cin[i] = 0;
        enc85(cout,cin);
        if (cout[0] == 'z') {
            cout[0] = cout[1] = cout[2] = cout[3] = cout[4] = '!';
        }
        outc(cout[0]);
        outc(cout[1]);
        if (I4 > 0)
            outc(cout[2]);
        if (I4 > 1)
            outc(cout[3]);
        if (I4 > 2)
            outc(cout[4]);
        return;
    }

    if (rl->key > 128)
        n = 257 - rl->key;
    else
        n = rl->key + 1;

    for (i = 0; i < n; i++) {
        cin[I4] = rl->key > 128 ? rl->data[0] : rl->data[i];
        I4++;
        if (I4 == 4) {
            I4 = 0;
            enc85(cout,cin);
            outc(cout[0]);
            if (cout[0] != 'z') {
                outc(cout[1]);
                outc(cout[2]);
                outc(cout[3]);
                outc(cout[4]);
            }
        }
    }
}
                

#define A4 (85*85*85*85)
#define A3 (85*85*85)
#define A2 (85*85)
#define A1 85


static void
enc85(unsigned char *co, unsigned char *ci)
{
    unsigned int l, n;

    l = (ci[0] << 24) + (ci[1] << 16) + (ci[2] << 8) + ci[3];
    if (l == 0) {
        co[0] = 'z';
        return;
    }
    n = l/A4;
    co[0] = n+33;
    l -= n*A4;
    n = l/A3;
    co[1] = n+33;
    l -= n*A3;
    n = l/A2;
    co[2] = n+33;
    l -= n*A2;
    n = l/A1;
    co[3] = n+33;
    l -= n*A1;
    co[4] = l+33;
}


#endif /* PSRLL85 */

#ifdef MSDOS

static void
dos_fopen(char *name)
{

#ifdef NDPC
    /* silliness */
    union REGS16 r;

    if (*Parameters.kpHardcopyFormat == POSTSC) {
        _pmode = 0x4000; /* O_TEXT */
        plotfile = fopen(name, "w");
        return;
    }
    _pmode = 0x8000; /* O_BINARY */
    plotfile = fopen(name, "w");
    _pmode = 0x4000;
#else
    union REGS r;

    if (*Parameters.kpHardcopyFormat == POSTSC) {
        plotfile = fopen(name, "w");
        return;
    }
    plotfile = fopen(name, "wb");
#endif
    if (plotfile == NULL)
        return;

    /* put the port driver into binary mode,
     * if we are outputting to the printer
     */
    r.x.bx = fileno(plotfile);
    r.x.ax = 0x4400;
    int86(0x21,&r,&r);
    if (r.x.dx & 0x80) { /* output is a device */

        /* put driver in raw mode */
        r.x.bx = fileno(plotfile);
        r.x.dx = 0xa0;
        r.x.ax = 0x4401;
        int86(0x21,&r,&r);
    }
}

#endif

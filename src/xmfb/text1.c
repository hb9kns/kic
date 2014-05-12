/*************************************************************************
 MFB graphics and miscellaneous library
 Copyright (c) Stephen R. Whiteley 1992
 Author: Stephen R. Whiteley
 *************************************************************************/

#include "mfb.h"
#include <ctype.h>
#include <string.h>

static int Xl,Yl, Xu = 0x7fff, Yu = 0x7fff;

#if __STDC__
static int setcode(int);
static void xform_cell(unsigned short*,unsigned short*,int,int,int,int);
#else
static int setcode();
static void xform_cell();
#endif

extern unsigned short mfb_6X8font[];
extern unsigned short mfb_8X8font[];
extern unsigned short mfb_8X14font[];
extern unsigned short mfb_8X16font[];


void
MFBSetTextClip(xl,yl,xu,yu)

/* set up a clipping window for text */
int xl,yl,xu,yu;
{
    Xl = xl;
    Yl = yl;
    Xu = xu;
    Yu = yu;
}


int
MFBScaledText(text,x,y,degrees,scale)

char *text;
int x, y, degrees, scale;
{
    int i, ii, j, k, tx;
    int xnow, ynow;
    int fheight, fwidth;
    unsigned short rotbuf[16], *ctab, mask;
    char *str0, *str1;

    if (scale < 1 || scale > 10) return(-1);
    if (text == NULL) return(-1);
    if (mfb_chartab == NULL) return(-1);

    if (degrees >= 90) degrees /= 90;

    if (degrees & 1) {
        fheight = mfb_charwidth;
        fwidth = mfb_charheight;
        if (degrees & 2) {
            if (!(degrees & 4))
                y -= strlen(text)*fheight*scale;
            if (degrees & 8)
                x -= fwidth*scale;
        }
        else {
            if (!(degrees & 8))
                x -= fwidth*scale;
            if (degrees & 4)
                y -= strlen(text)*fheight*scale;
        }
    }
    else {
        fheight  = mfb_charheight;
        fwidth = mfb_charwidth;
        if (degrees & 2) {
            if (!(degrees & 8))
                x -= strlen(text)*fwidth*scale;
            if (!(degrees & 4))
                y -= fheight*scale;
        }
        else {
            if (degrees & 8)
                x -= strlen(text)*fwidth*scale;
            if (degrees & 4)
                y -= fheight*scale;
        }
    }
    degrees = setcode(degrees);

    str0 = text;
    str1 = text + strlen(text);
    while (str0 != str1) {

        if (degrees & 8) {
            if ((tx = (*--str1 & 0x7f) - ' ') < 0) tx = 0;
        }
        else
            if ((tx = (*str0++ & 0x7f) - ' ') < 0) tx = 0;

        ctab = mfb_chartab + tx*mfb_charheight;
        xform_cell(rotbuf,ctab,fwidth,fheight,degrees,fwidth);
        ctab = rotbuf + fheight - 1;

        ynow = y;
        for (i = fheight; i; i--, ctab--) {
            for (j = scale; j; j--) {
                if (ynow >= Yl && ynow <= Yu) {
                    xnow = x;
                    for (mask = 0x8000,ii = fwidth; ii; mask >>= 1,ii--) {
                        for (k = scale; k; k--) {
                            if (xnow >= Xl && xnow <= Xu) {
                                if (*ctab & mask)
                                    MFBPixel(xnow,ynow);
                            }
                            xnow++;
                        }
                    }
                }
                else
                    x += fwidth*scale;
                ynow++;
            }
        }
        if (degrees & 4) {
            y += fheight*scale;
            continue;
        }
        x += fwidth*scale;
    }
    return MFBOK;
}


static int
setcode(deg)

int deg;
{
    /*
     * returned bit field:
     * 0x8: load text backward
     * 0x4: rotation
     * 0x2: reverse y data
     * 0x1: reverse x data
     *
     * Assumes input bit field:
     * 0x8: mirror x
     * 0x4: mirror y
     * 0x3: 0-no rotation, 1-90, 2-180, 3-270
     */

    int mx = 0,my = 0;
    int rotn;
    int xf = 0;

    rotn = deg & 3;
    if (deg & 8) mx = 1;
    if (deg & 4) my = 1;

    if ((mx && rotn == 0) || (!mx && rotn == 2) ||
        (my && rotn == 1) || (!my && rotn == 3))
        xf |= 8;
    if (rotn & 1)
        xf |= 4;
    if ((my && (rotn == 0 || rotn == 1)) ||
        (!my && (rotn == 2 || rotn == 3)))
        xf |= 2;
    if ((mx && (rotn == 0 || rotn == 3)) ||
        (!mx && (rotn == 1 || rotn == 2)))
        xf |= 1;
    return xf;
}


static void
xform_cell(out,in,w,h,code,shift)

unsigned short *out,*in;
int w,h,code,shift;
{
    int i, j, k, l;

    for (i = 0; i < h; i++) {
        k = ((code & 2) ? h-i-1 : i);
        if (code & 4) {
            for (j = 0,out[i] = 0; j < w; j++) {
                l = ((code & 1) ? (w-j-1) : j);
                out[i] |= ((in[j] >> k) & 1) << l;
            }
        }
        else {
            if (code & 1)
                for (j = 0,out[i] = 0; j < w; j++)
                    out[i] |= ((in[k] >> j) & 1) << (w-j-1);
            else
                 out[i] = in[k];
        }
    }
    if (shift)
        for (i = 0,j = 16-shift; i < h; i++)
            out[i] <<= j;
}


void
MFBTextBB(string,x,y)

char *string;
int *x,*y;
{
    *x = strlen(string) * mfb_charwidth;
    *y = mfb_charheight;
}

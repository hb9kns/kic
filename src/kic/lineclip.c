/*************************************************************************
 Graphics and miscellaneous library
 Copyright (c) Stephen R. Whiteley 1994
 Author: Unknown Hero from Ancient Berkeley History
 *************************************************************************/

#include "prefix.h"
#include "kic.h"

/* Line clipping routines. */

#define CODELEFT 1
#define CODEBOTTOM 2
#define CODERIGHT 4
#define CODETOP 8
#define CODE(x,y,c)  c = 0;\
                     if(x < l)\
                         c = CODELEFT;\
                     else if(x > r)\
                         c = CODERIGHT;\
                     if(y < b)\
                         c |= CODEBOTTOM;\
                     else if(y > t)\
                         c |= CODETOP;

void
Y_Intercept(x1,y1,x2,y2,e,yi)

int x1,y1,x2,y2;                /* two points on line */
int e;                          /* vertical line of intercept */
int *yi;                        /* y coordinate of intercept */
{
    /*
     * Y_Intercept will return the value 'yi' where the the coordinate
     * (e,yi) is the intersection of the vertical line x = e and the line
     * determined by the coordinates (x1,y1) and (x2,y2).
     */
    *yi = y1;
    if (x1 == x2) return;            /* vertical line */
    *yi = y1 + ((e - x1) * (y2 - y1))/(x2 - x1);
}


void
X_Intercept(x1,y1,x2,y2,e,xi)

int x1,y1,x2,y2;                /* two point on line */
int e;                          /* horizontal line of intercept */
int *xi;                        /* x coordinate of intercept */
{
    /*
     * X_Intercept will return the value 'xi' where the the coordinate
     * (xi,e) is the intersection of the horizontal line y = e and the line
     * determined by the coordinates (x1,y1) and (x2,y2).
     */
    *xi = x1;
    if (y1 == y2) return;            /* horizontal line */
    *xi = x1 + ((e - y1) * (x2 - x1))/(y2 - y1);
}


int
LineClip(pX1,pY1,pX2,pY2,l,b,r,t)

int *pX1,*pY1,*pX2,*pY2,l,b,r,t;
{
    /*
     * LineClip will clip a line to a rectangular area.  The returned
     * value is 'True' if the line is out of the AOI (therefore does not
     * need to be displayed) and 'False' if the line is in the AOI.
     */
    int x1 = *pX1;
    int y1 = *pY1;
    int x2 = *pX2;
    int y2 = *pY2;
    int x = 0,y = 0;
    int  c,c1,c2;

    CODE(x1,y1,c1)
    CODE(x2,y2,c2)
    while (c1 != 0 || c2 != 0) {
        if ((c1 & c2) != 0) return (True); /* Line is invisible. */
        if ((c = c1) == 0) c = c2;
        if (c & CODELEFT) {
            y = y1+(y2-y1)*(l-x1)/(x2-x1);
            x = l;
        }
        else if (c & CODERIGHT) {
            y = y1+(y2-y1)*(r-x1)/(x2-x1);
            x = r;
        }
        else if (c & CODEBOTTOM) {
            x = x1+(x2-x1)*(b-y1)/(y2-y1);
            y = b;
        }
        else if (c & CODETOP) {
            x = x1+(x2-x1)*(t-y1)/(y2-y1);
            y = t;
        }
        if (c == c1) {
            x1 = x; y1 = y; CODE(x,y,c1)
        }
        else {
            x2 = x; y2 = y; CODE(x,y,c2)
        }
    }
    *pX1 = x1; *pY1 = y1;
    *pX2 = x2; *pY2 = y2;
    return (False); /* Line is at least partially visible. */
}

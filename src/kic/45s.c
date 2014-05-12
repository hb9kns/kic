/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Kenneth H. Keller, Giles C. Billingsley
 *
 *     KIC is a graphics editor that was developed by the integrated
 * circuits group of the Electronics Research Laboratory and the
 * Department of Electrical Engineering and Computer Sciences at
 * the University of California, Berkeley, California.  The program
 * KIC is available free of charge to any interested party.
 * The sale, resale, or use of this program for profit without the
 * express written consent of the Department of Electrical Engineering
 * and Computer Sciences, University of California, Berkeley, California,
 * is forbidden.
 *
 *************************************************************************/

#include "prefix.h"
#include "kic.h"


void
To45(x1,y1,x2,y2)

/*
 * Changes one end of a line to make it vertical,horizontal, or diagonal.
 *
 * To45(x1,y1,x2,y2)
 *
 *    x1,y1 = coordinates of one end of line (fixed)
 *    x2,y2 = pointers to coordinates of other end of line (movable)
 *
 * New end point will have the same x or y coordinate as old point
 */
int x1,y1,*x2,*y2;
{
    int d[4];
    int c,i;

    d[2] = x1 - *x2;
    d[0] = y1 - *y2;
    d[1] = (*y2-y1)-(*x2-x1);
    d[3] = (*y2-y1)+(*x2-x1);

    if (d[0]*d[1]*d[2]*d[3] == 0) return;
    c = 0;
    for (i = 1; i <= 3; i++)
        if (abs(d[i]) < abs(d[c])) c = i;

    switch (c) {
    case 0:
        *y2 = y1;
        break;
    case 2:
        *x2 = x1;
        break;
    case 1:
        if (d[1] > 0) {
            if (x1 > *x2) *x2 = *x2 + d[1];
            else *y2 = *y2 - d[1];
        }
        else{
            if (x1 > *x2) *y2 = *y2 - d[1];
            else *x2 = *x2 + d[1];
        }
        break;
    case 3:
        if (d[3] > 0) {
            if (x1 > *x2) *y2 = *y2 - d[3];
            else *x2 = *x2 - d[3];
        }
        else{
            if (x1 > *x2) *x2 = *x2 - d[3];
            else *y2 = *y2 - d[3];
        }
        break;
    }
}


int
IsManhattan(X1,Y1,X2,Y2)

int X1,Y1,X2,Y2;
{
    if (X1 == X2 Or Y1 == Y2)
        return (True);
    return (False);
}


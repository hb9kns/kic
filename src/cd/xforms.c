/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Kenneth H. Keller, Giles C. Billingsley
 *
 *     CD is a CIF database package that was developed by the integrated
 * circuits group of the Electronics Research Laboratory and the
 * Department of Electrical Engineering and Computer Sciences at
 * the University of California, Berkeley, California.  The programs in
 * CD are available free of charge to any interested party.
 * The sale, resale, or use of these programs for profit without the
 * express written consent of the Department of Electrical Engineering
 * and Computer Sciences, University of California, Berkeley, California,
 * is forbidden.
 *
 *************************************************************************/
 
/*
 * Transforms package.
 * 
 */

#include "prefix.h"
#include "cd.h"

#ifdef __STDC__
extern void MallocFailed(void);
#else
extern void MallocFailed();
#endif

struct tt {
    int ttMatrix[3][3];
    struct tt *ttNext;
};
static struct tt *Transforms;


void
TInit()

{
    struct tt *Tmp;

    for (Tmp = Transforms; Tmp; Tmp = Transforms) {
        Transforms = Tmp->ttNext;
        afree(Tmp,tt);
    }
    Transforms = alloc(tt);
    if (Transforms == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    Transforms->ttNext = NULL;
    TIdentity();
}


int
TEmpty()

{
    if (Transforms == NULL)
        return (True);
    else
        return (False);
}


int
TFull()

{
    return (False);
}


void
TPush()

{
    struct tt *Tmp;

    Tmp = alloc(tt);
    if (Tmp == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    Tmp->ttNext = Transforms;
    Transforms = Tmp;
}


void
TPop()

{
    struct tt *Tmp;

    Tmp = Transforms;
    Transforms = Tmp->ttNext;
    afree(Tmp,tt);
}


void
TCurrent(TFP)

int *TFP;
{
    int i,j;

    for(i = 0; i < 3; ++i)
        for(j = 0; j<3; ++j)
            TFP[(3 * i) + j] = Transforms->ttMatrix[i][j];
}


void
TLoadCurrent(TFP)

int *TFP;
{
    int i,j;

    for(i = 0; i < 3; ++i)
        for(j = 0; j < 3; ++j)
            Transforms->ttMatrix[i][j] = TFP[(3 * i) + j];
}


void
TTranslate(X,Y)

int X,Y;
{
    Transforms->ttMatrix[2][0] += X;
    Transforms->ttMatrix[2][1] += Y;
}


void
TMY()

{
    Transforms->ttMatrix[0][1] = -Transforms->ttMatrix[0][1];
    Transforms->ttMatrix[1][1] = -Transforms->ttMatrix[1][1];
    Transforms->ttMatrix[2][1] = -Transforms->ttMatrix[2][1];
}


void
TMX()

{
    Transforms->ttMatrix[0][0] = -Transforms->ttMatrix[0][0];
    Transforms->ttMatrix[1][0] = -Transforms->ttMatrix[1][0];
    Transforms->ttMatrix[2][0] = -Transforms->ttMatrix[2][0];
}


void
TRotate(XDirection,YDirection)

int XDirection,YDirection;
{
    /*
    Rotation angle is expressed as a CIF-style direction vector.
    */
    int Int1;

    if (XDirection == 0) {
        if (abs(YDirection) > 1) {
            if (YDirection < 0)
                YDirection = -1;
            else
                YDirection = 1;
        }
    }
    elif (YDirection == 0) {
        if (abs(XDirection) > 1) {
            if (XDirection < 0)
                XDirection = -1;
            else
                XDirection = 1;
        }
    }
    if (XDirection == 1 And YDirection == 0)
        /*
        Don't rotate at all.
        */
        return;
    elif (XDirection == 0 And YDirection == -1) {
        /*
        Rotate ccw by 270 degrees.
        */
        Int1 = Transforms->ttMatrix[0][0];
        Transforms->ttMatrix[0][0] = Transforms->ttMatrix[0][1];
        Transforms->ttMatrix[0][1] = -Int1;
        Int1 = Transforms->ttMatrix[1][0];
        Transforms->ttMatrix[1][0] = Transforms->ttMatrix[1][1];
        Transforms->ttMatrix[1][1] = -Int1;
        Int1 = Transforms->ttMatrix[2][0];
        Transforms->ttMatrix[2][0] = Transforms->ttMatrix[2][1];
        Transforms->ttMatrix[2][1] = -Int1;
    }
    elif (XDirection == 0 And YDirection == 1) {
        /*
        Rotate ccw by 90 degrees.
        */
        Int1 = Transforms->ttMatrix[0][0];
        Transforms->ttMatrix[0][0] = -Transforms->ttMatrix[0][1];
        Transforms->ttMatrix[0][1] = Int1;
        Int1 = Transforms->ttMatrix[1][0];
        Transforms->ttMatrix[1][0] = -Transforms->ttMatrix[1][1];
        Transforms->ttMatrix[1][1] = Int1;
        Int1 = Transforms->ttMatrix[2][0];
        Transforms->ttMatrix[2][0] = -Transforms->ttMatrix[2][1];
        Transforms->ttMatrix[2][1] = Int1;
    }
    elif (XDirection == -1 And YDirection == 0) {
        /*
        Rotate ccw by 180 degrees.
        */
        Transforms->ttMatrix[0][0]    = -Transforms->ttMatrix[0][0];
        Transforms->ttMatrix[0][1]    = -Transforms->ttMatrix[0][1];
        Transforms->ttMatrix[1][0]    = -Transforms->ttMatrix[1][0];
        Transforms->ttMatrix[1][1]    = -Transforms->ttMatrix[1][1];
        Transforms->ttMatrix[2][0]    = -Transforms->ttMatrix[2][0];
        Transforms->ttMatrix[2][1]    = -Transforms->ttMatrix[2][1];
    }
}


void
TIdentity()

{
    int *l;

    l = (int *)Transforms->ttMatrix;
    *l++ = 1; *l++ = 0; *l++ = 0;
    *l++ = 0; *l++ = 1; *l++ = 0;
    *l++ = 0; *l++ = 0; *l++ = 1;
}


void
TPoint(X,Y)

int *X,*Y;
{
    /*
    Transform the point.
    */
    int Int1;

    Int1 = *X*Transforms->ttMatrix[0][0] + *Y*Transforms->ttMatrix[1][0] +
        Transforms->ttMatrix[2][0];
    *Y = *X*Transforms->ttMatrix[0][1] + *Y*Transforms->ttMatrix[1][1] +
        Transforms->ttMatrix[2][1];
    *X = Int1;
}


void
TPremultiply()

{
    /*
     * Form the instance transform.
     * This is done by computing
     *   Transforms->ttMatrix * Transforms->ttNext->ttMatrix and
     * placing the product in Transforms.ttMatrix.    
     * So, the scenario for transforming the coordinates of a master follows.
     * TPush();
     * TIdentity();
     * Invoke TMX, Translate, etc. to build instance transform.
     * Form the instance transform.
     * TPremultiply();
     * Invoke TPoint to transform master points to instance points.
     * TPop();
     */

    int Int1,Int2,Int3,Int4,Int5,Int6;
    struct tt *Next;

    Next = Transforms->ttNext;

    Int1 = Transforms->ttMatrix[0][0]*Next->ttMatrix[0][0] +
        Transforms->ttMatrix[0][1]*Next->ttMatrix[1][0];

    Int2 = Transforms->ttMatrix[0][0]*Next->ttMatrix[0][1] +
        Transforms->ttMatrix[0][1]*Next->ttMatrix[1][1];

    Int3 = Transforms->ttMatrix[1][0]*Next->ttMatrix[0][0] +
        Transforms->ttMatrix[1][1]*Next->ttMatrix[1][0];

    Int4 = Transforms->ttMatrix[1][0]*Next->ttMatrix[0][1] +
        Transforms->ttMatrix[1][1]*Next->ttMatrix[1][1];

    Int5 = Transforms->ttMatrix[2][0]*Next->ttMatrix[0][0] +
        Transforms->ttMatrix[2][1]*Next->ttMatrix[1][0] +
        Next->ttMatrix[2][0];

    Int6 = Transforms->ttMatrix[2][0]*Next->ttMatrix[0][1] +
        Transforms->ttMatrix[2][1]*Next->ttMatrix[1][1] +
        Next->ttMatrix[2][1];

    Transforms->ttMatrix[0][0] = Int1;
    Transforms->ttMatrix[0][1] = Int2;
    Transforms->ttMatrix[1][0] = Int3;
    Transforms->ttMatrix[1][1] = Int4;
    Transforms->ttMatrix[2][0] = Int5;
    Transforms->ttMatrix[2][1] = Int6;
}

static int Storage[3][3];
static int InverseMatrix[3][3];


void
TInverse()

{
    /*
    Compute the inverse transform of the current transform.
    Because all transformations are Manhattan, the
    det of the current transform matrix is always -1 or +1.
    */

    int Det;

    Det = 
        Transforms->ttMatrix[0][0]*Transforms->ttMatrix[1][1] -
        Transforms->ttMatrix[1][0]*Transforms->ttMatrix[0][1];

    if (Det == 1) {
        InverseMatrix[0][0] = Transforms->ttMatrix[1][1];
        InverseMatrix[0][1] = -Transforms->ttMatrix[0][1];
        InverseMatrix[1][0] = -Transforms->ttMatrix[1][0];
        InverseMatrix[1][1] = Transforms->ttMatrix[0][0];
        InverseMatrix[2][0] =
            Transforms->ttMatrix[1][0]*Transforms->ttMatrix[2][1] -
            Transforms->ttMatrix[2][0]*Transforms->ttMatrix[1][1];
        InverseMatrix[2][1] =
            - Transforms->ttMatrix[0][0]*Transforms->ttMatrix[2][1] +
            Transforms->ttMatrix[0][1]*Transforms->ttMatrix[2][0];
    }
    else {
        InverseMatrix[0][0] = -Transforms->ttMatrix[1][1];
        InverseMatrix[0][1] = Transforms->ttMatrix[0][1];
        InverseMatrix[1][0] = Transforms->ttMatrix[1][0];
        InverseMatrix[1][1] = -Transforms->ttMatrix[0][0];
        InverseMatrix[2][0] =
            - Transforms->ttMatrix[1][0]*Transforms->ttMatrix[2][1] +
            Transforms->ttMatrix[2][0]*Transforms->ttMatrix[1][1];
        InverseMatrix[2][1] =
            Transforms->ttMatrix[0][0]*Transforms->ttMatrix[2][1] -
            Transforms->ttMatrix[0][1]*Transforms->ttMatrix[2][0];
    }
    InverseMatrix[0][2] = 0;
    InverseMatrix[1][2] = 0;
    InverseMatrix[2][2] = 1;
}


void
TInversePoint(X,Y)

int *X,*Y;
{
    /*
    Transform the point.
    */
    int Int1;

    Int1 = *X*InverseMatrix[0][0] + *Y*InverseMatrix[1][0]
        + InverseMatrix[2][0];
    *Y = *X*InverseMatrix[0][1] + *Y*InverseMatrix[1][1]
        + InverseMatrix[2][1];
    *X = Int1;
}


void
TStore()

{
    memcpy((char *)Storage,
        (char *)Transforms->ttMatrix,9*sizeof(int));
}


void
TLoad()

{
    memcpy((char *)Transforms->ttMatrix,
        (char *)Storage,9*sizeof(int));
}


void
TLoadInverse()

{
    memcpy((char *)Transforms->ttMatrix,
        (char *)InverseMatrix,9*sizeof(int));
}

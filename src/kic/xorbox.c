/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

#include "prefix.h"
#include "kic.h"

#if __STDC__
static void do_xor(struct ks*,int);
static void xor_erase(struct ks*,struct ka*);
static void xor_new_box(struct ks**,int,int,int,int,int);
static void xor_restore(struct ks*);
static void xor_final(struct ks*);
#else
static void do_xor();
static void xor_erase();
static void xor_new_box();
static void xor_restore();
static void xor_final();
#endif

/* This file supports a command which adds boxes to the database
 * such as to invert a region on a layer.  Previously existing
 * boxes of the same layer which intersect the added box are deleted,
 * and become clear areas in the added box.
 */

extern char *MenuUNDO;
extern char *MenuXOR;


void
XORbox(LookedAhead)

int *LookedAhead;
{
    struct ka BB;
    struct o *Pointer = NULL;
    struct ks *SList = NULL;
    int Undo = False;
    int FirstTime = True;
    int Modified = 0;
    int X = 0,Y = 0;

    MenuSelect(MenuXOR);
    if (SelectQHead != NULL)
        Desel();

top:
    loop {
        if (FirstTime)
            ShowPrompt("Point to diagonal's endpoints.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit; 
        case PL_UND:
            if (FirstTime == True) goto quit;
            MenuSelect(MenuUNDO);
            if (Undo == False) {
                xor_restore(SList);
                SList = NULL;
                Modified--;
                Undo = True;
            }
            else {
                if (Not CDMakeBox(Parameters.kpCellDesc,
                    Parameters.kpLayer,
                    (BB.kaRight-BB.kaLeft),
                    (BB.kaTop-BB.kaBottom),
                    (BB.kaRight-BB.kaLeft)/2+BB.kaLeft,
                    (BB.kaTop-BB.kaBottom)/2+BB.kaBottom,
                    &Pointer)) MallocFailed();
                Modified++;
                SList = alloc(ks);
                SList->ksPointer = Pointer;
                SList->ksSucc = NULL;
                do_xor(SList,Parameters.kpLayer);
                Undo = False;
            }
            EraseBox(&BB);
            Redisplay(&BB);
            MenuDeselect(MenuUNDO);
            continue;
        case PL_PCW:
            X = KicCursor.kcX;
            Y = KicCursor.kcY;
            SetRelative(X,Y,True);
            FBSetRubberBanding('r');
            break;
        }
        loop {
            if (FirstTime)
                ShowPrompt("Point to second endpoint.");
            switch (PointLoop(LookedAhead)) {
            case PL_ESC:
            case PL_CMD:
                goto quit; 
            case PL_UND:
                FBSetRubberBanding(0);
                goto top;
            case PL_PCW:
                BB.kaLeft = min(X,KicCursor.kcX);
                BB.kaBottom = min(Y,KicCursor.kcY);
                BB.kaRight = max(X,KicCursor.kcX);
                BB.kaTop = max(Y,KicCursor.kcY);
                if (BB.kaRight - BB.kaLeft <
                    LayerTable[Parameters.kpLayer].klMinDimensions ||
                    BB.kaRight - BB.kaLeft == 0 ||
                    BB.kaTop - BB.kaBottom <
                    LayerTable[Parameters.kpLayer].klMinDimensions ||
                    BB.kaTop - BB.kaBottom == 0) {
                    ShowPromptAndWait(
                    "Can't make box with side less than minimum dimension.");
                    continue;
                }
                FBSetRubberBanding(0);
                SetRelative(0L,0L,False);
                break;
            }
            break;
        }
        ErasePrompt();

        if (Not CDMakeBox(Parameters.kpCellDesc,
            Parameters.kpLayer,
            (BB.kaRight-BB.kaLeft),
            (BB.kaTop-BB.kaBottom),
            (BB.kaRight-BB.kaLeft)/2+BB.kaLeft,
            (BB.kaTop-BB.kaBottom)/2+BB.kaBottom,
            &Pointer)) MallocFailed();
        Modified++;

        xor_final(SList);
        SList = alloc(ks);
        SList->ksPointer = Pointer;
        SList->ksSucc = NULL;
        do_xor(SList,Parameters.kpLayer);

        FirstTime = False;
        Undo = False;

        /* make sure the new box is displayed */
        CDReflect(Parameters.kpCellDesc);

        EraseBox(&BB);
        Redisplay(&BB);
    }

quit:
    if (Modified)
        Parameters.kpModified = True;
    FBSetRubberBanding(0);
    SetRelative(0L,0L,False);
    ErasePrompt();
    xor_final(SList);
    MenuDeselect(MenuXOR);
}


static void
do_xor(SList,Layer)

struct ks *SList;
int Layer;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct ka BB, AOI;
    struct ks *SQDesc;

    (void)CDBB(Parameters.kpCellDesc,SList->ksPointer,&AOI.kaLeft,
        &AOI.kaBottom,&AOI.kaRight,&AOI.kaTop);

    /* conditionally delete overlapping boxes, store list in Select Q */
    if (Not CDInitGen(Parameters.kpCellDesc,Layer,AOI.kaLeft,AOI.kaBottom,
        AOI.kaRight,AOI.kaTop,&GenDesc)) MallocFailed();
    loop {

        CDGen(Parameters.kpCellDesc,GenDesc,&Pointer);
        if (Pointer == NULL) break;
        if (Pointer == SList->ksPointer) continue;

        if (Pointer->oType == CDBOX) {
            Pointer->oInfo = SQ_GONE;
            SQInsert(Pointer);
            continue;
        }
    }

    /* cut the holes, SList has new rectangles created */
    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oInfo != SQ_GONE) continue;
        if (SQDesc->ksPointer->oType != CDBOX) continue;
        CDStatusInt = CDBB(Parameters.kpCellDesc,SQDesc->ksPointer,
            &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);
        xor_erase(SList,&BB);
    }

    /* replace the part of the box outside of the AOI */
    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oInfo != SQ_GONE) continue;
        if (SQDesc->ksPointer->oType != CDBOX) continue;
        CDStatusInt = CDBB(Parameters.kpCellDesc,SQDesc->ksPointer,
            &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);

        if (BB.kaTop > AOI.kaTop) {
            xor_new_box(&SList,BB.kaLeft,AOI.kaTop,
                BB.kaRight,BB.kaTop,Layer);
            BB.kaTop = AOI.kaTop;
        }
        if (BB.kaBottom < AOI.kaBottom) {
            xor_new_box(&SList,BB.kaLeft,BB.kaBottom,
                BB.kaRight,AOI.kaBottom,Layer);
            BB.kaBottom = AOI.kaBottom;
        }
        if (BB.kaLeft < AOI.kaLeft) {
            xor_new_box(&SList,BB.kaLeft,BB.kaBottom,
                AOI.kaLeft,BB.kaTop,Layer);
        }
        if (BB.kaRight > AOI.kaRight) {
            xor_new_box(&SList,AOI.kaRight,BB.kaBottom,
                BB.kaRight,BB.kaTop,Layer);
        }
    }
}


static void
xor_erase(SList,BB)

struct ks *SList;
struct ka *BB;
{
    struct ks *SQDesc;
    struct ka BoxBB;
    int X,Y,Length,Width;
    int Layer;
    char Type;

    for (SQDesc = SList; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer == NULL) continue;
        Type = SQDesc->ksPointer->oType;

        if (Type == CDBOX) {
            CDBox(SQDesc->ksPointer,&Layer,&Length,&Width,&X,&Y);
            BoxBB.kaLeft = X - Length/2;
            BoxBB.kaRight = X + Length/2;
            BoxBB.kaTop = Y + Width/2;
            BoxBB.kaBottom = Y - Width/2;

            if (BoxBB.kaBottom >= BB->kaTop) continue;
            if (BoxBB.kaTop <= BB->kaBottom) continue;
            if (BoxBB.kaRight <= BB->kaLeft) continue;
            if (BoxBB.kaLeft >= BB->kaRight) continue;

            Layer = SQDesc->ksPointer->oLayer;
            CDDelete(Parameters.kpCellDesc,SQDesc->ksPointer);
            SQDesc->ksPointer = NULL;

            if (BoxBB.kaTop > BB->kaTop) {
                xor_new_box(&SQDesc,BoxBB.kaLeft,BB->kaTop,
                    BoxBB.kaRight,BoxBB.kaTop,Layer);
                BoxBB.kaTop = BB->kaTop;
            }
            if (BoxBB.kaBottom < BB->kaBottom) {
                xor_new_box(&SQDesc,BoxBB.kaLeft,BoxBB.kaBottom,
                    BoxBB.kaRight,BB->kaBottom,Layer);
                BoxBB.kaBottom = BB->kaBottom;
            }
            if (BoxBB.kaLeft < BB->kaLeft) {
                xor_new_box(&SQDesc,BoxBB.kaLeft,BoxBB.kaBottom,
                    BB->kaLeft,BoxBB.kaTop,Layer);
            }
            if (BoxBB.kaRight > BB->kaRight) {
                xor_new_box(&SQDesc,BB->kaRight,BoxBB.kaBottom,
                    BoxBB.kaRight,BoxBB.kaTop,Layer);
            }
        }
    }
}


static void
xor_new_box(SQD,L,B,R,T,Layer)

struct ks **SQD;
int L,B,R,T;
int Layer;
{
    int X,Y,DX,DY;
    struct o *Pointer;
    struct ks *SQTmp;

    X = (L+R)/2;
    Y = (B+T)/2;
    DX = R-L;
    DY = T-B;
    if (DX < 0) DX = -DX;
    if (DY < 0) DY = -DY;

    if (CDMakeBox(Parameters.kpCellDesc,Layer,DX,DY,
        X,Y,&Pointer) == 0) MallocFailed();

    Pointer->oInfo = SQ_OLD;
    if ((*SQD)->ksPointer == NULL)
        (*SQD)->ksPointer = Pointer;
    else {
        SQTmp = alloc(ks);
        SQTmp->ksPointer = Pointer;
        SQTmp->ksSucc = (*SQD)->ksSucc;
        (*SQD)->ksSucc = SQTmp;
        (*SQD) = SQTmp;
    }
}


static void
xor_restore(SList)

struct ks *SList;
{
    struct ks *SQTmp, *SQPrev;

    while (SList) {
        if (SList->ksPointer) {
            CDDelete(Parameters.kpCellDesc,SList->ksPointer);
            SList->ksPointer = NULL;
        }
        SQTmp = SList;
        SList = SList->ksSucc;
        afree(SQTmp,ks);
    }

    for (SList = SelectQHead; SList; SList = SQTmp) {
        SQTmp = SList->ksSucc;
        if (SList->ksPointer->oInfo == SQ_GONE) {
            SList->ksPointer->oInfo = SQ_OLD;
            SList->ksPointer = 0;
        }
    }
    SQPrev = NULL;
    for (SList = SelectQHead; SList; SList = SQTmp) {
        SQTmp = SList->ksSucc;
        if (!SList->ksPointer) {
            if (SQPrev)
                SQPrev->ksSucc = SQTmp;
            else
                SelectQHead = SQTmp;
            afree(SList,ks);
            continue;
        }
        SQPrev = SList;
    }
}


static void
xor_final(SList)

struct ks *SList;
{
    struct ks *SQTmp, *SQPrev;

    while (SList) {
        SQTmp = SList->ksSucc;
        afree(SList,ks);
        SList = SQTmp;
    }

    for (SList = SelectQHead; SList; SList = SList->ksSucc) {
        struct ks *s;
        for (s = SList->ksSucc; s; s = s->ksSucc) {
            if (SList->ksPointer && SList->ksPointer == s->ksPointer)
                s->ksPointer = NULL;
        }
    }

    for (SList = SelectQHead; SList; SList = SList->ksSucc) {
        if (SList->ksPointer && SList->ksPointer->oInfo == SQ_GONE) {
            CDDelete(Parameters.kpCellDesc,SList->ksPointer);
            SList->ksPointer = NULL;
        }
    }

    SQPrev = NULL;
    for (SList = SelectQHead; SList; SList = SQTmp) {
        SQTmp = SList->ksSucc;
        if (!SList->ksPointer) {
            if (SQPrev)
                SQPrev->ksSucc = SQTmp;
            else
                SelectQHead = SQTmp;
            afree(SList,ks);
            continue;
        }
        SQPrev = SList;
    }
}

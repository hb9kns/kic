/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

/*
 * Erase operator.
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuERASE;
extern char *MenuUNDO;

#ifdef __STDC__
static void do_erase(struct ks*,struct ka*);
static void sq_swap(void);
static void erase_poly(struct o*,struct ka*);
#else
static void do_erase();
static void sq_swap();
static void erase_poly();
#endif


void
Erase(LookedAhead)

int *LookedAhead;
{
    struct ka BB, SBB;
    struct ks *SList;
    int Xfirst = 0,Yfirst = 0;
    int Undo = False;
    int Modified = 0;
    int FirstTime = True;
    char TTmp[8];
    char Types[4];

    MenuSelect(MenuERASE);

    Types[0] = CDBOX;
    Types[1] = CDPOLYGON;
    Types[3] = '\0';

    strcpy(TTmp,Parameters.kpSelectTypes);
    strcpy(Parameters.kpSelectTypes,Types);

    ShowPrompt("Point to diagonal of rectangle to erase.");

top:
    loop {
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            if (FirstTime)
                goto quit;
            sq_swap();
            EraseBox(&SBB);
            Redisplay(&SBB);
            FBTransfer();
            if (Undo) {
                Undo = False;
                Modified++;
            }
            else {
                Undo = True;
                Modified--;
            }
            continue;
        case PL_PCW:
            Xfirst = KicCursor.kcX;
            Yfirst = KicCursor.kcY;
            FBSetRubberBanding('r');
            break;
        }

        loop {
            switch (PointLoop(LookedAhead)) {
            case PL_ESC:
            case PL_CMD:
                goto quit;
            case PL_UND:
                FBSetRubberBanding(0);
                goto top;
            case PL_PCW:
                if (KicCursor.kcX == Xfirst &&
                    KicCursor.kcY == Yfirst) continue;
                FBSetRubberBanding(0);
                break;
            }
            break;
        }

        BB.kaLeft = min(Xfirst,KicCursor.kcX);
        BB.kaRight = max(Xfirst,KicCursor.kcX);
        BB.kaBottom = min(Yfirst,KicCursor.kcY);
        BB.kaTop = max(Yfirst,KicCursor.kcY);

        SList = SelectItems(&BB,False);
        if (SList == NULL) continue;
        SLBB(SList,&SBB);
        SQRestore(False);
        do_erase(SList,&BB);
        SLFree(SList);
        EraseBox(&SBB);
        Redisplay(&SBB);
        FBTransfer();
        Modified++;
        Undo = False;
        FirstTime = False;
    }

quit:

    FBSetRubberBanding(0);
    strcpy(Parameters.kpSelectTypes,TTmp);
    SQRestore(False);
    MenuDeselect(MenuERASE);
    ErasePrompt();
    if (Modified)
        Parameters.kpModified = True;
}


void
NewBox(L,B,R,T,Layer)

int L,B,R,T;
int Layer;
{
    int X,Y,DX,DY;
    struct o *Pointer;

    X = (L+R)/2;
    Y = (B+T)/2;
    DX = R-L;
    DY = T-B;
    if (DX < 0) DX = -DX;
    if (DY < 0) DY = -DY;

    if (CDMakeBox(Parameters.kpCellDesc,Layer,DX,DY,
        X,Y,&Pointer) == 0) MallocFailed();

    Pointer->oInfo = SQ_NEW;
    SQInsert(Pointer);
}


void
NewPoly(p,Layer)

Poly *p;
int Layer;
{
    struct p *pp,*Path;
    struct o *NewPointer = NULL;
    int *xy;
    int i,n;

    xy = p->xy;
    Path = pp = alloc(p);
    pp->pX = *xy++;
    pp->pY = *xy++;
    n = p->nvertices - 1;

    for (i = 0; i < n; i++) {
        pp->pSucc = alloc(p);
        pp = pp->pSucc;
        pp->pX = *xy++;
        pp->pY = *xy++;
    }
    pp->pSucc = NULL;

    if (Not CDMakePolygon(Parameters.kpCellDesc,Layer,Path,&NewPointer))
        MallocFailed();
    if (NewPointer) {
        NewPointer->oInfo = SQ_NEW;
        SQInsert(NewPointer);
    }
}


static void
do_erase(SList,BB)

struct ks *SList;
struct ka *BB;
{
    struct ks *SQDesc;
    char Type;
    int X,Y,Length,Width;
    int Layer;
    struct ka BoxBB;

    for (SQDesc = SList; SQDesc; SQDesc = SQDesc->ksSucc) {
        Type = SQDesc->ksPointer->oType;

        /* ignore selected objects */
        if (SQDesc->ksPointer->oInfo == SQ_OLDSEL) continue;

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
            SQDesc->ksPointer->oInfo = SQ_GONE;
            SQInsert(SQDesc->ksPointer);

            Layer = SQDesc->ksPointer->oLayer;

            if (BoxBB.kaTop > BB->kaTop) {
                NewBox(BoxBB.kaLeft,BB->kaTop,
                    BoxBB.kaRight,BoxBB.kaTop,Layer);
                BoxBB.kaTop = BB->kaTop;
            }
            if (BoxBB.kaBottom < BB->kaBottom) {
                NewBox(BoxBB.kaLeft,BoxBB.kaBottom,
                    BoxBB.kaRight,BB->kaBottom,Layer);
                BoxBB.kaBottom = BB->kaBottom;
            }
            if (BoxBB.kaLeft < BB->kaLeft) {
                NewBox(BoxBB.kaLeft,BoxBB.kaBottom,
                    BB->kaLeft,BoxBB.kaTop,Layer);
            }
            if (BoxBB.kaRight > BB->kaRight) {
                NewBox(BB->kaRight,BoxBB.kaBottom,
                    BoxBB.kaRight,BoxBB.kaTop,Layer);
            }
            continue;
        }
        if (Type == CDPOLYGON) {
            erase_poly(SQDesc->ksPointer,BB);
            continue;
        }
    }
}


static void
sq_swap()

{
    struct ks *S;

    for (S = SelectQHead; S != NULL; S = S->ksSucc) {
        if (S->ksPointer->oInfo == SQ_GONE) {
            S->ksPointer->oInfo = SQ_NEW;
            continue;
        }
        if (S->ksPointer->oInfo == SQ_NEW)
            S->ksPointer->oInfo = SQ_GONE;
    }
}


static void
erase_poly(Pointer,NewBB)

struct o *Pointer;
struct ka *NewBB;
{
    int i;
    struct p *path, *pp;
    struct  ka BB;
    Poly p1,p2;
    int *xy;

    path = ((struct po *)Pointer->oRep)->poPath;

    for (i = 0, pp = path; pp; i++,pp = pp->pSucc) ;
    p1.nvertices = i;
    p1.xy = (int*) tmalloc(i*10*sizeof(int));
    p2.xy = p1.xy + 2*i;

    xy = p1.xy;
    for (pp = path; pp; pp = pp->pSucc) {
        *xy++ = pp->pX;
        *xy++ = pp->pY;
    }

    CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,&BB.kaLeft,
        &BB.kaBottom,&BB.kaRight,&BB.kaTop);

    Pointer->oInfo = SQ_GONE;
    SQInsert(Pointer);

    PolygonClip(&p1,BB.kaLeft,NewBB->kaTop,BB.kaRight,BB.kaTop);
    while (NewPolygon(&p2))
        NewPoly(&p2,Pointer->oLayer);

    PolygonClip(&p1,BB.kaLeft,BB.kaBottom,BB.kaRight,NewBB->kaBottom);
    while (NewPolygon(&p2))
        NewPoly(&p2,Pointer->oLayer);

    PolygonClip(&p1,BB.kaLeft,NewBB->kaBottom,
        NewBB->kaLeft,NewBB->kaTop);
    while (NewPolygon(&p2))
        NewPoly(&p2,Pointer->oLayer);

    PolygonClip(&p1,NewBB->kaRight,NewBB->kaBottom,
        BB.kaRight,NewBB->kaTop);
    while (NewPolygon(&p2))
        NewPoly(&p2,Pointer->oLayer);
    free(p1.xy);
}

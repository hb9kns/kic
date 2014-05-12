/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

/*
 * Break selection operator.
 */

#include "prefix.h"
#include "kic.h"


extern char *MenuBREAK;
extern char *MenuUNDO;

#ifdef __STDC__
static int  do_break(int,int,int);
static int  break_wire(int,int,struct o*,int);
static int  break_poly(int,int,struct o*,int);
static struct p *isec_y(struct p*,struct p*,int);
static struct p *isec_x(struct p*,struct p*,int);
#else
static int  do_break();
static int  break_wire();
static int  break_poly();
static struct p *isec_y();
static struct p *isec_x();
#endif


void
Break(LookedAhead)

int *LookedAhead;
{
    int GotOne = False;
    int Undo = False;
    int FirstTime = True;
    int Modified = 0;
    struct ka OldSelectQBB;
    int OldX = 0, OldY = 0;
    char Types[4];
    char pl;

    MenuSelect(MenuBREAK);

    Types[0] = CDBOX;
    Types[1] = CDPOLYGON;
    Types[2] = CDWIRE;
    Types[3] = '\0';

    if (AreTypesInQ(Types))
        GotOne = True;
top:

    if (Not GotOne) {
        ShowPrompt("Point to object to break.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            if (FirstTime) goto quit;
            MenuSelect(MenuUNDO);
            if (Undo == False) {
                SQRestore(True);
                EraseBox(&OldSelectQBB);
                Redisplay(&OldSelectQBB);
                Modified--;
                Undo = True;
                MenuDeselect(MenuUNDO);
                break;
            }
            else {
                (void) do_break(OldX,OldY,True);
                EraseBox(&OldSelectQBB);
                Redisplay(&OldSelectQBB);
                Modified++;
                Undo = False;
                MenuDeselect(MenuUNDO);
                goto top;
            }
        case PL_PCW:
            SelectTypes(Types);
            if (Not AreTypesInQ(Types))
                goto top;;
        }
    }

    ShowPrompt("Point on the break line.");

next:
    FBSetRubberBanding('l');
    pl = PointLoop(LookedAhead);
    FBSetRubberBanding(0);
    switch (pl) {
    case PL_ESC:
    case PL_CMD:
        goto quit;
    case PL_UND:
        MenuSelect(MenuUNDO);
        if (Not GotOne) {
            SQDesel(Types);
            EraseBox(&OldSelectQBB);
            Redisplay(&OldSelectQBB);
            MenuDeselect(MenuUNDO);
            goto top;
        }
        if (FirstTime)
            goto quit;
        if (Undo == False) {
            MenuSelect(MenuBREAK);
            ShowPrompt("Point on the break line.");
            SQRestore(True);
            EraseBox(&OldSelectQBB);
            Redisplay(&OldSelectQBB);
            Modified--;
            Undo = True;
        }
        else {
            (void) do_break(OldX,OldY,True);
            EraseBox(&OldSelectQBB);
            Redisplay(&OldSelectQBB);
            ErasePrompt();
            MenuDeselect(MenuBREAK);
            Modified++;
            Undo = False;
        }
        MenuDeselect(MenuUNDO);
        goto next;
    case PL_PCW:
        if (Not FirstTime And GotOne And Not Undo) goto next;
        if (do_break(KicCursor.kcX,KicCursor.kcY,False)) {
            SQRestore(False);
            do_break(KicCursor.kcX,KicCursor.kcY,True);
            OldX = KicCursor.kcX;
            OldY = KicCursor.kcY;
            OldSelectQBB = SelectQBB;
            if (Not GotOne)
                /* deselect anything left over */
                SQDesel(Types);
            EraseBox(&OldSelectQBB);
            Redisplay(&OldSelectQBB);
            FirstTime = False;
            Modified++;
            Undo = False;
            if (Not GotOne)
                goto top;
            ErasePrompt();
            MenuDeselect(MenuBREAK);
            goto next;
        }
    }

quit:
    SQRestore(False);
    if (Not GotOne And AreTypesInQ(Types)) {
        SQComputeBB();
        SQDesel(Types);
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
    }
    if (Modified)
        Parameters.kpModified = True;
    ErasePrompt();
    MenuDeselect(MenuUNDO);
    MenuDeselect(MenuBREAK);
}


struct p *
CopyPath(path)
struct p *path;
{
    struct p *pcopy,*pc = NULL,*pp = path;

    if (path == NULL) return (NULL);
    pcopy = pc = alloc(p);
    pc->pX = pp->pX;
    pc->pY = pp->pY;
    pp = pp->pSucc;
    while (pp) {
        pc->pSucc = alloc(p);
        pc = pc->pSucc;
        pc->pX = pp->pX;
        pc->pY = pp->pY;
        pp = pp->pSucc;
    }
    pc->pSucc = NULL;
    return (pcopy);
}


static int
do_break(RefX,RefY,Flag)

int RefX,RefY;
int Flag;
{
    struct ks *SQDesc;
    int X,Y,Length,Width,Right,Left,Top,Bottom;
    int RotationAngle,Layer;
    int DidBreak = False;
    char Type;

    /* If Flag is False, returns True if a break would be performed,
     * but operation is not done.
     */
    RotationAngle = Parameters.kpRotationAngle;
    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oInfo == SQ_GONE) continue;
        Type = SQDesc->ksPointer->oType;

        if (Type == CDSYMBOLCALL) {
            /* can't break symbols */
            continue;
        }
        if (Type == CDLABEL) {
            continue;
        }
        if (Type == CDBOX) {
            CDBox(SQDesc->ksPointer,&Layer,&Length,&Width,&X,&Y);
            Left = X - Length/2;
            Right = X + Length/2;
            Top = Y + Width/2;
            Bottom = Y - Width/2;
            if (RotationAngle == 0 Or RotationAngle == 180) {
                if (RefX <= Left Or RefX >= Right)
                    continue;
                if (Not Flag) return (True);
                SQDesc->ksPointer->oInfo = SQ_GONE;

                NewBox(Left,Bottom,RefX,Top,Layer);
                NewBox(RefX,Bottom,Right,Top,Layer);
            }
            else {
                if (RefY <= Bottom Or RefY >= Top)
                    continue;
                if (Not Flag) return (True);
                SQDesc->ksPointer->oInfo = SQ_GONE;

                NewBox(Left,Bottom,Right,RefY,Layer);
                NewBox(Left,RefY,Right,Top,Layer);
            }
            DidBreak = True;
        }
        if (Type == CDWIRE) {
            DidBreak = break_wire(RefX,RefY,SQDesc->ksPointer,Flag);
            continue;
        }
        if (Type == CDPOLYGON) {
            DidBreak = break_poly(RefX,RefY,SQDesc->ksPointer,Flag);
            continue;
        }
    }
    return (DidBreak);
}


static int
break_wire(RefX,RefY,Pointer,Flag)

int RefX,RefY;
struct o *Pointer;
int Flag;
{
    struct p *pp,*px,*pTemp,*pNew,*pInt;
    struct o *NewPointer;
    struct ka BB;

    CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,&BB.kaLeft,
        &BB.kaBottom,&BB.kaRight,&BB.kaTop);


    if (Parameters.kpRotationAngle == 0 Or
        Parameters.kpRotationAngle == 180) {
        if (RefX <= BB.kaLeft Or RefX >= BB.kaRight) return (False);
        if (Not Flag) return (True);
        px = CopyPath(((struct w *)Pointer->oRep)->wPath);
        for (pNew = pp = px; pp And pp->pSucc; pp = pTemp) {
            pTemp = pp->pSucc;
            if ((pp->pX <= RefX And pp->pSucc->pX > RefX) Or
                (pp->pX >= RefX And pp->pSucc->pX < RefX)) {
                pp->pSucc = NULL;
                pInt = isec_y(pp,pTemp,RefX);
                if (pp->pX != pInt->pX Or pp->pY != pInt->pY) {
                    pp->pSucc = pInt;
                    pInt->pSucc = NULL;
                    pInt = CopyPath(pInt);
                }
                if (Not CDMakeWire(Parameters.kpCellDesc,Pointer->oLayer,
                    ((struct w *)Pointer->oRep)->wWidth,pNew,&NewPointer))
                    MallocFailed();
                NewPointer->oInfo = SQ_NEW;
                SQInsert(NewPointer);
                pInt->pSucc = pTemp;
                pNew = pInt;
            }
        }
    }
    else {
        if (RefY <= BB.kaBottom Or RefY >= BB.kaTop) return (False);
        if (Not Flag) return (True);
        px = CopyPath(((struct w *)Pointer->oRep)->wPath);
        for (pNew = pp = px; pp And pp->pSucc; pp = pp->pSucc) {
            pTemp = pp->pSucc;
            if ((pp->pY <= RefY And pp->pSucc->pY > RefY) Or
                (pp->pY >= RefY And pp->pSucc->pY < RefY)) {
                pp->pSucc = NULL;
                pInt = isec_x(pp,pTemp,RefY);
                if (pp->pX != pInt->pX Or pp->pY != pInt->pY) {
                    pp->pSucc = pInt;
                    pInt->pSucc = NULL;
                    pInt = CopyPath(pInt);
                }
                if (Not CDMakeWire(Parameters.kpCellDesc,Pointer->oLayer,
                    ((struct w *)Pointer->oRep)->wWidth,pNew,&NewPointer))
                    MallocFailed();
                NewPointer->oInfo = SQ_NEW;
                SQInsert(NewPointer);
                pInt->pSucc = pTemp;
                pNew = pInt;
            }
        }
    }
    if (Not CDMakeWire(Parameters.kpCellDesc,Pointer->oLayer,
        ((struct  w *)Pointer->oRep)->wWidth,pNew,&NewPointer))
        MallocFailed();
    NewPointer->oInfo = SQ_NEW;
    SQInsert(NewPointer);
    Pointer->oInfo = SQ_GONE;
    return (True);
}


static int
break_poly(RefX,RefY,Pointer,Flag)

int RefX,RefY;
struct o *Pointer;
int Flag;
{
    int i;
    struct p *path, *pp;
    struct  ka BB;
    Poly p1,p2;
    int *xy;
    int L1,R1,B1,T1;
    int L2,R2,B2,T2;

    CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,&BB.kaLeft,
        &BB.kaBottom,&BB.kaRight,&BB.kaTop);

    if (Parameters.kpRotationAngle == 0 Or
        Parameters.kpRotationAngle == 180) {
        if (RefX <= BB.kaLeft Or RefX >= BB.kaRight)
            return (False);
        if (Not Flag)
            return (True);
        L1 = BB.kaLeft;
        B1 = BB.kaBottom;
        R1 = RefX;
        T1 = BB.kaTop;
        L2 = RefX;
        B2 = BB.kaBottom;
        R2 = BB.kaRight;
        T2 = BB.kaTop;
    }
    else {
        if (RefY <= BB.kaBottom Or RefY >= BB.kaTop)
            return (False);
        if (Not Flag)
            return (True);
        L1 = BB.kaLeft;
        B1 = BB.kaBottom;
        R1 = BB.kaRight;
        T1 = RefY;
        L2 = BB.kaLeft;
        B2 = RefY;
        R2 = BB.kaRight;
        T2 = BB.kaTop;
    }

    path = ((struct po *)Pointer->oRep)->poPath;

    for (i = 0, pp = path; pp; i++,pp = pp->pSucc) ;
    p1.nvertices = i;
    p1.xy = (int*) tmalloc(i*10*sizeof(int));
    p2.xy = p1.xy + i*2;

    xy = p1.xy;
    for (pp = path; pp; pp = pp->pSucc) {
        *xy++ = pp->pX;
        *xy++ = pp->pY;
    }

    PolygonClip(&p1,L1,B1,R1,T1);
    while (NewPolygon(&p2))
        NewPoly(&p2,Pointer->oLayer);

    PolygonClip(&p1,L2,B2,R2,T2);
    while (NewPolygon(&p2))
        NewPoly(&p2,Pointer->oLayer);

    Pointer->oInfo = SQ_GONE;
    free(p1.xy);
    return (True);
}


static struct p *
isec_y(p1,p2,X)
struct p *p1,*p2;
int X;
{
    struct p *pp;

    pp = alloc(p);
    pp->pX = X;
    pp->pY = p1->pY + ((X - p1->pX)*(p2->pY - p1->pY))/(p2->pX - p1->pX);
    return (pp);
}

static struct p *
isec_x(p1,p2,Y)
struct p *p1,*p2;
int Y;
{
    struct p *pp;

    pp = alloc(p);
    pp->pX = p1->pX + ((Y - p1->pY)*(p2->pX - p1->pX))/(p2->pY - p1->pY);
    pp->pY = Y;
    return (pp);
}


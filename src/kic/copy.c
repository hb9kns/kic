/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Giles C. Billingsley
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

/*
 * Copy and move selection operators.
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuCOPY;
extern char *MenuMOVE;
extern char *MenuUNDO;

#define MOVE 1
#define COPY 0

#ifdef __STDC__
static void show_move(struct ka*,struct ka*);
static int  copy_ok(int,int);
static void do_copy(int,int,int,int,struct ka*,int);
static char fix_xform(int);
static void do_copy_call(int,int,int,int,struct o*,struct o**);
static void add_transform(struct o*,int*);
static void free_path(struct p*);
#else
static void show_move();
static int  copy_ok();
static void do_copy();
static char fix_xform();
static void do_copy_call();
static void add_transform();
static void free_path();
#endif


void
Copy(LookedAhead)

int *LookedAhead;
{
    struct ka BB;
    int RefX,RefY,CopX = 0,CopY = 0;
    int i;
    int Undo = False;
    int FirstTime = True;
    int GotOne = False;
    int Modified = 0;

    MenuSelect(MenuCOPY);

    if (SelectQHead != NULL)
        GotOne = True;
top:

    if (Not GotOne) {
        ShowPrompt("Point to object to copy.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
        case PL_UND:
            goto quit;
        case PL_PCW:
            BB.kaLeft = BB.kaRight = KicCursor.kcRawX;
            BB.kaBottom = BB.kaTop = KicCursor.kcRawY;
            Selection(&BB);
            if (SelectQHead == NULL)
            goto top;
        }
    }

Next:
    ShowPrompt("Point to the reference point.");

    switch (PointLoop(LookedAhead)) {
    case PL_ESC:
    case PL_CMD:
        goto quit;
    case PL_UND:
        if (Not GotOne) {
            SQComputeBB();
            SQDesel((char*)NULL);
            EraseBox(&SelectQBB);
            Redisplay(&SelectQBB);
            goto top;
        }
        goto quit;
    }

    FBSetRubberBanding('m');
    RefX = KicCursor.kcX;
    RefY = KicCursor.kcY;
    SetRelative(RefX,RefY,True);

    ShowPrompt("Point to locations where the selected items will be copied.");

    loop {
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            MenuSelect(MenuUNDO);
            if (FirstTime) {
                MenuDeselect(MenuUNDO);
                FBSetRubberBanding(0);
                goto Next;
            }
            if (Undo == False) {
                SQRestore(True);
                EraseBox(&BB);
                Redisplay(&BB);
                Modified--;
                Undo = True;
            }
            else {
                do_copy(RefX,RefY,CopX,CopY,&BB,COPY);
                Redisplay(&BB);
                Modified++;
                Undo = False;
            }
            MenuDeselect(MenuUNDO);
            continue;
        case PL_PCW:
            CopX = KicCursor.kcX;
            CopY = KicCursor.kcY;
            i = copy_ok(CopX-RefX,CopY-RefY);
            if (i == -1) goto quit;
            if (i == 0)  continue;
            SQRestore(False);
            do_copy(RefX,RefY,CopX,CopY,&BB,COPY);
            FirstTime = False;
            Undo = False;
            Modified++;
            Redisplay(&BB);
            continue;
        }
    }

quit:
    FBSetRubberBanding(0);
    SQRestore(False);
    if (Not GotOne And AreTypesInQ((char*)NULL)) {
        SQComputeBB();
        SQDesel((char*)NULL);
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
    }
    if (Modified)
        Parameters.kpModified = True;
    SetRelative(0L,0L,False);
    ErasePrompt();
    MenuDeselect(MenuUNDO);
    MenuDeselect(MenuCOPY);
}


void
Move(LookedAhead)

int *LookedAhead;
{
    struct ka OldBB,NewBB;
    int LastRefX,LastRefY;
    int RefX = 0,RefY = 0,MovX = 0,MovY = 0;
    int  FirstTime = True;
    int GotOne = False;
    int Modified = 0;
    int Undo = False;

    MenuSelect(MenuMOVE);

    if (SelectQHead != NULL)
        GotOne = True;

    loop {
top:
        if (Not GotOne) {
            ShowPrompt("Point to object to move.");
            switch (PointLoop(LookedAhead)) {
            case PL_ESC:
            case PL_CMD:
                goto quit;
            case PL_UND:
                if (FirstTime) goto quit;
                MenuSelect(MenuUNDO);
                if (Undo == False) {
                    SQRestore(True);
                    /* restored objects have Info = SQ_NEWSEL */
                    show_move(&NewBB,&OldBB);
                    Modified--;
                    Undo = True;
                    MenuDeselect(MenuUNDO);
                    break;
                }
                else {
                    /* should have only Info = SQ_NEW objects here */
                    do_copy(RefX,RefY,MovX,MovY,&NewBB,MOVE);
                    SQDesel((char*)NULL);
                    show_move(&OldBB,&NewBB);
                    Modified++;
                    Undo = False;
                    MenuDeselect(MenuUNDO);
                    continue;
                }
            case PL_PCW:
                SelectTypes((char*)NULL);
                if (Not AreTypesInQ((char*)NULL))
                    goto top;
            }
        }

next:
        ShowPrompt("Point to the reference point.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            MenuSelect(MenuUNDO);
            if (Not GotOne) {
                SQComputeBB();
                SQDesel((char*)NULL);
                /* newly selected objects deleted, restored objects
                 * have Info = SQ_NEW.
                 */
                EraseBox(&SelectQBB);
                Redisplay(&SelectQBB);
            }
            else {
                if (FirstTime) {
                    MenuDeselect(MenuUNDO);
                    goto quit;
                }
                if (Undo == False) {
                    SQRestore(True);
                    /* restored objects have Info = SQ_NEWSEL */
                    show_move(&OldBB,&NewBB);
                    Modified--;
                    Undo = True;
                }
                else {
                    /* should have only Info = SQ_NEWSEL objects here */
                    do_copy(RefX,RefY,MovX,MovY,&NewBB,MOVE);
                    show_move(&OldBB,&NewBB);
                    Modified++;
                    Undo = False;
                }
            }
            MenuDeselect(MenuUNDO);
            goto top;
        }

        FBSetRubberBanding('m');
        LastRefX = RefX;
        LastRefY = RefY;
        RefX = KicCursor.kcX;
        RefY = KicCursor.kcY;
        SetRelative(RefX,RefY,True);

        ShowPrompt("Point to where the selected items will be moved.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            MenuSelect(MenuUNDO);
            RefX = LastRefX;
            RefY = LastRefY;
            MenuDeselect(MenuUNDO);
            FBSetRubberBanding(0);
            goto next;
        }
        FBSetRubberBanding(0);
        SQComputeBB();
        OldBB = SelectQBB;
        MovX = KicCursor.kcX;
        MovY = KicCursor.kcY;
        SetRelative(0L,0L,False);
        SQRestore(False);
        do_copy(RefX,RefY,MovX,MovY,&NewBB,MOVE);
        FirstTime = False;
        Modified++;
        Undo = False;
        if (Not GotOne)
            SQDesel((char*)NULL);
        show_move(&OldBB,&NewBB);
    }
quit:
FBSetRubberBanding(0);
    SetRelative(0L,0L,False);
    SQRestore(False);
    if (Not GotOne And AreTypesInQ((char*)NULL)) {
        SQComputeBB();
        SQDesel((char*)NULL);
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
    }
    if (Modified)
        Parameters.kpModified = True;
    ErasePrompt();
    MenuDeselect(MenuUNDO);
    MenuDeselect(MenuMOVE);
}


void
CopyPathWithXForm(Path)

struct p **Path;
{
    struct p *OldPair,*NewPair;
    /*
     * Copy Path with transform and return pointer to new path
     */
    OldPair = *Path;
    if ((NewPair = alloc(p)) == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    *Path = NewPair;
    NewPair->pX = OldPair->pX; 
    NewPair->pY = OldPair->pY; 
    TPoint(&NewPair->pX,&NewPair->pY);
    NewPair->pSucc = NULL;
    OldPair = OldPair->pSucc;
    while(OldPair != NULL) {
        if ((NewPair->pSucc = alloc(p)) == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        NewPair = NewPair->pSucc;
        NewPair->pX = OldPair->pX; 
        NewPair->pY = OldPair->pY; 
        NewPair->pSucc = NULL;
        TPoint(&NewPair->pX,&NewPair->pY);
        OldPair = OldPair->pSucc;
    }
}


void
ShowMove(RefX,RefY,NewX,NewY)
int RefX,RefY,NewX,NewY;
{
    struct ks *SQDesc;
    struct p *Path;
    struct ka BB;
    int  Layer;
    int Width;
    int TFold[9], TFnew[9];

    TCurrent(TFold);
    TPush();
    TIdentity();
    SetNewTransform(RefX,RefY,NewX,NewY);
    TCurrent(TFnew);
    TPop();

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oInfo != SQ_OLDSEL &&
            SQDesc->ksPointer->oInfo != SQ_NEWSEL) continue;
        /* prevent drawing duplicate entries twice */
        SQDesc->ksPointer->oInfo += 20;

        switch (SQDesc->ksPointer->oType) {

        case CDBOX:
        case CDSYMBOLCALL:
        case CDLABEL:
            GetBB(SQDesc->ksPointer,&BB);
            TLoadCurrent(TFnew);
            TPoint(&BB.kaLeft,&BB.kaBottom);
            TPoint(&BB.kaRight,&BB.kaTop);
            TLoadCurrent(TFold);
            ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
            break;

        case CDWIRE:
            CDWire(SQDesc->ksPointer,&Layer,&Width,&Path);
            TLoadCurrent(TFnew);
            CopyPathWithXForm(&Path);
            TLoadCurrent(TFold);
            ShowWire(ColorTable[HighlightingColor].Ent,Width,Path);
            free_path(Path);
            break;

        case CDPOLYGON:
            CDPolygon(SQDesc->ksPointer,&Layer,&Path);
            TLoadCurrent(TFnew);
            CopyPathWithXForm(&Path);
            TLoadCurrent(TFold);
            ShowPath(ColorTable[HighlightingColor].Ent,Path,True);
            free_path(Path);
            break;
        }
    }
    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc)
        if (SQDesc->ksPointer->oInfo > 20)
            SQDesc->ksPointer->oInfo -= 20;
}


static void
show_move(OBB,NBB)

struct ka *OBB,*NBB;
{
    int L1,R1,B1,T1,L2,R2,B2,T2;
    struct ka BB;

    L1 = NBB->kaLeft;
    R1 = NBB->kaRight;
    B1 = NBB->kaBottom;
    T1 = NBB->kaTop;
    L2 = OBB->kaLeft;
    R2 = OBB->kaRight;
    B2 = OBB->kaBottom;
    T2 = OBB->kaTop;
    if (L1 > R1) SwapInts(L1,R1);
    if (B1 > T1) SwapInts(B1,T1);
    if (L2 > R2) SwapInts(L2,R2);
    if (B2 > T2) SwapInts(B2,T2);
    if (L1 > R2 Or L2 > R1 Or B1 > T2 Or B2 > T1) {
        /*
         * Old BB is not within the new BB
         * So, redisplay twice.
         */
        EraseBox(OBB);
        Redisplay(OBB);
        EraseBox(NBB);
        Redisplay(NBB);
        return;
    }
    /*
     * Old BB intersects new BB
     */
    BB.kaLeft   = min(L1,L2);
    BB.kaRight  = max(R1,R2);
    BB.kaBottom = min(B1,B2);
    BB.kaTop    = max(T1,T2);
    EraseBox(&BB);
    Redisplay(&BB);
}


static int
copy_ok(X,Y)

int X,Y;
{
    char *TypeIn;

    if (Not Parameters.kpMX And Not Parameters.kpMY And
        Parameters.kpRotationAngle == 0 And
        X == 0 And Y == 0 ) {

        ShowPrompt(
         "This will copy objects directly over themselves.  Continue(N)?");
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) return (-1);
        if ((TypeIn[0] != 'Y' And TypeIn[0] != 'y'))
            return (0);
    }
    return (1);
}


static void
do_copy(RefX,RefY,NewX,NewY,NewBB,MoveOrCopy)

int RefX,RefY,NewX,NewY;
struct ka *NewBB;
int MoveOrCopy;
{
    struct prpty *PrptyDesc;
    struct ks *SQDesc;
    struct p *Path;
    struct ka BB;
    struct o *Pointer;
    int  Layer;
    char Xform;
    char *Label;
    int X,Y,Length,Width;

    BB.kaLeft = BB.kaRight = NewX;
    BB.kaTop = BB.kaBottom = NewY;
    NewBB->kaLeft = NewBB->kaRight = NewX;
    NewBB->kaTop = NewBB->kaBottom = NewY;
    TPush();
    TIdentity();
    SetNewTransform(RefX,RefY,NewX,NewY);

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oInfo == SQ_GONE) continue;

        switch (SQDesc->ksPointer->oType) {
        case CDBOX:
            CDBox(SQDesc->ksPointer,&Layer,&Length,&Width,&X,&Y);
            TPoint(&X,&Y);
            if (Parameters.kpRotationAngle == 90 Or
                Parameters.kpRotationAngle == 270) {
                SwapInts(Width,Length);
            }
            if (Not CDMakeBox(Parameters.kpCellDesc,Layer,Length,Width,
                X,Y,&Pointer)) MallocFailed();
            break;

        case CDLABEL:
            CDLabel(SQDesc->ksPointer,&Layer,&Label,&X,&Y,&Xform);
            TPoint(&X,&Y);

            if (Not CDMakeLabel(Parameters.kpCellDesc,Layer,Label,
                X,Y,fix_xform(Xform),&Pointer)) MallocFailed();
            break;

        case CDWIRE:
            CDWire(SQDesc->ksPointer,&Layer,&Width,&Path);
            CopyPathWithXForm(&Path);
            if (Not CDMakeWire(Parameters.kpCellDesc,Layer,Width,Path,
                &Pointer)) MallocFailed();
            break;

        case CDPOLYGON:
            CDPolygon(SQDesc->ksPointer,&Layer,&Path);
            CopyPathWithXForm(&Path);
            if (Not CDMakePolygon(Parameters.kpCellDesc,Layer,Path,
                &Pointer)) MallocFailed();
            break;

        case CDSYMBOLCALL:
            do_copy_call(RefX,RefY,NewX,NewY,SQDesc->ksPointer,&Pointer);
            break;
        }

        GetBB(Pointer,&BB);

        if (MoveOrCopy == MOVE)
            Pointer->oInfo = SQ_NEWSEL;
        else
            Pointer->oInfo = SQ_NEW;
        SQInsert(Pointer);

        if (BB.kaLeft   < NewBB->kaLeft)   NewBB->kaLeft   = BB.kaLeft;
        if (BB.kaRight  > NewBB->kaRight)  NewBB->kaRight  = BB.kaRight;
        if (BB.kaBottom < NewBB->kaBottom) NewBB->kaBottom = BB.kaBottom;
        if (BB.kaTop    > NewBB->kaTop)    NewBB->kaTop    = BB.kaTop;

        if (MoveOrCopy == MOVE)
            SQDesc->ksPointer->oInfo = SQ_GONE;

        PrptyDesc = SQDesc->ksPointer->oPrptyList;
        while (PrptyDesc) {
            CDAddProperty(Parameters.kpCellDesc,Pointer,
                PrptyDesc->prpty_Value,PrptyDesc->prpty_String);
            PrptyDesc = PrptyDesc->prpty_Succ;
        }
    }
    TPop();
}


static char
fix_xform(Xform)

int Xform;
{
    int TF[9];
    char xf;

    TPush();
    TIdentity();
    xf = Xform & 3;
    if (xf != 0) {
        if (xf == 1)   TRotate(0L,1L);
        elif (xf == 2) TRotate(-1L,0L);
        elif (xf == 3) TRotate(0L,-1L);
    }
    if (Xform & 4) TMY();
    if (Xform & 8) TMX();
    if (Parameters.kpRotationAngle != 0) {
        if (Parameters.kpRotationAngle == 180)   TRotate(-1L,0L);
        elif (Parameters.kpRotationAngle == 90)  TRotate(0L,1L);
        elif (Parameters.kpRotationAngle == 270) TRotate(0L,-1L);
    }
    if (Parameters.kpMX) TMX();
    if (Parameters.kpMY) TMY();
    TCurrent(TF);
    Xform = SetXform(TF);
    TPop();
    return ((char)Xform);
}


static void
do_copy_call(RefX,RefY,NewX,NewY,OPointer,NPointer)

int RefX,RefY,NewX,NewY;
struct o *OPointer;
struct o **NPointer;
{
    char *SymbolName;
    char Type;
    int NumX,NumY;
    int X,Y;
    int DX,DY;
    int TF[9];
    struct o *Pointer;
    struct t *TGen;

    CDCall(OPointer,&SymbolName,&NumX,&DX,&NumY,&DY);
    /* only possible error is CDMALLOCFAILED */
    if (Not CDBeginMakeCall(Parameters.kpCellDesc,SymbolName,
        NumX,DX,NumY,DY,&Pointer))
        MallocFailed();

    TPush();
    TIdentity();
    CDInitTGen(OPointer,&TGen);
    loop {
        /* first determine transform of the placed cell */
        CDTGen(&TGen,&Type,&X,&Y);
        if (TGen == NULL) break;

        elif (Type == CDTRANSLATE) TTranslate(X,Y);
        elif (Type == CDROTATE)    TRotate(X,Y);
        elif (Type == CDMIRRORY)   TMY();
        elif (Type == CDMIRRORX)   TMX();
    }

    /* now add the previous transform to that of the move */
    SetNewTransform(RefX,RefY,NewX,NewY);
    TCurrent(TF);
    add_transform(Pointer,TF);
    if (Not CDEndMakeCall(Parameters.kpCellDesc,Pointer))
        MallocFailed();
    *NPointer = Pointer;
    TPop();
}


void
SetNewTransform(RefX,RefY,NewX,NewY)

int RefX,RefY,NewX,NewY;
{
    TTranslate(-RefX,-RefY);
    if (Parameters.kpMX) TMX();
    if (Parameters.kpMY) TMY();
    if (Parameters.kpRotationAngle != 0) {
        if (Parameters.kpRotationAngle == 90)    TRotate(0L,1L);
        elif (Parameters.kpRotationAngle == 180) TRotate(-1L,0L);
        elif (Parameters.kpRotationAngle == 270) TRotate(0L,-1L);
    }
    TTranslate(NewX,NewY);
}


static void
add_transform(Pointer,TF)

struct o *Pointer;
int *TF;
{
    int A,B,C,D,TX,TY;
    int ret;
    /*
     * Take the transformation defined in TF and add
     * it to the symbol call currently being created.
     *
     *                  | a    c    0  |
     * Transform = TM = | b    d    0  |
     *                  | TX   TY   1  |
     *
     * A = TM[0][0] = TF[0];
     * B = TM[1][0] = TF[3];
     * C = TM[0][1] = TF[1];
     * D = TM[1][1] = TF[4];
     * TX = TM[2][0] = TF[6];
     * TY = TM[2][1] = TF[7];
     */
    A = TF[0]; B = TF[3]; C = TF[1]; D = TF[4];
    TX = TF[6]; TY = TF[7];
    if (A == 0 And B == 1 And C == 1 And D == 0) {
        /* MX R 0 -1 T tx,ty */
        ret = CDT(Pointer,CDMIRRORX,0L,0L);
        ret &= CDT(Pointer,CDROTATE,0L,-1L);
        ret &= CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    elif (A == 0 And B == -1 And C == -1 And D == 0) {
        /* MX R 0 1 T tx,ty */
        ret = CDT(Pointer,CDMIRRORX,0L,0L);
        ret &= CDT(Pointer,CDROTATE,0L,1L);
        ret &= CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    elif (A == 0 And B == 1 And C == -1 And D == 0) {
        /* R 0 -1 T tx,ty */
        ret = CDT(Pointer,CDROTATE,0L,-1L);
        ret &= CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    elif (A == 0 And B == -1 And C == 1 And D == 0) {
        /* R 0 1 T tx,ty */
        ret = CDT(Pointer,CDROTATE,0L,1L);
        ret &= CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    elif (A == 1 And B == 0 And C == 0 And D == 1) {
        /* T tx,ty */
        ret = CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    elif (A == -1 And B == 0 And C == 0 And D == -1) {
        /* R -1 0 T tx,ty */
        ret = CDT(Pointer,CDROTATE,-1L,0L);
        ret &= CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    elif (A == -1 And B == 0 And C == 0 And D == 1) {
        /* MX T tx,ty */
        ret = CDT(Pointer,CDMIRRORX,0L,0L);
        ret &= CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    else {
        /* MY T tx,ty */
        ret = CDT(Pointer,CDMIRRORY,0L,0L);
        ret &= CDT(Pointer,CDTRANSLATE,TX,TY);
    }
    if (Not ret)
        MallocFailed();
}


static void
free_path(Path)

struct p *Path;
{
    struct p *Next;

    for (; Path; Path = Next) {
        Next = Path->pSucc;
        afree(Path,p);
    }
}


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
 * Wire management.
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuWIRES;
extern char *MenuUNDO;
extern char *MenuWIDTH;


#ifdef __STDC__
static void allocate_wire(int,struct p*,int,struct ka*,struct o**,int);
static void record_wire(struct o*);
static void copy_path(struct p **);
#else
static void allocate_wire();
static void record_wire();
static void copy_path();
#endif


void
Wires(LookedAhead)

int *LookedAhead;
{
    struct p *Path;
    struct ka BB,OldBB;
    struct o *Pointer = NULL,*OldPointer = NULL;
    int Width;
    int X,Y,FX = 0,FY = 0;
    int NumVertices = 0;
    int ExpectFirstPoint = True;
    int Modified = 0;
    int Undo = False;

    MenuSelect(MenuWIRES);
    Path = NULL;

    loop {
        Width = LayerTable[Parameters.kpLayer].klWireWidth;
        sprintf(TypeOut,"Point to the reference points (current width %g).",
            (double)Width/RESOLUTION);
        ShowPrompt(TypeOut);

        switch (PointLoopCreate(LookedAhead)) {
        case PL_PLT:
            if (NumVertices > 1) {
                Path = CopyPath(Path);
                CDDelete(Parameters.kpCellDesc,Pointer);
                EraseBox(&BB);
                Redisplay(&BB);
                Width = LayerTable[Parameters.kpLayer].klWireWidth;
                allocate_wire(Parameters.kpLayer,Path,Width,&BB,
                    &Pointer,SQ_INCMPLT);
                ShowWire(Parameters.kpLayer,Width,Path);
            }
            continue;
        case PL_UND:
            MenuSelect(MenuUNDO);
            if (NumVertices == 0) {
                if (OldPointer == NULL) {
                    MenuDeselect(MenuUNDO);
                    goto quit;
                }
                if (Undo == False) {
                    OldPointer->oInfo = SQ_GONE;
                    Modified--;
                    Undo = True;
                }
                else {
                    OldPointer->oInfo = SQ_NEW;
                    Modified++;
                    Undo = False;
                }
                EraseBox(&OldBB);
                Redisplay(&OldBB);
                MenuDeselect(MenuUNDO);
                continue;
            }
            if (NumVertices > 1)
                RemoveLastPointInPath(&Path);
            CDDelete(Parameters.kpCellDesc,Pointer);
            EraseBox(&BB);
            Redisplay(&BB);
            if (NumVertices == 1) {
                ExpectFirstPoint = True;
                ShowMarker(ERASE,0,FX,FY,200);
            }
            else {
                allocate_wire(Parameters.kpLayer,Path,Width,&BB,
                    &Pointer,SQ_INCMPLT);
                ShowWire(Parameters.kpLayer,Width,Path);
                LastPointInPath(&X,&Y,Path);
            }
            NumVertices--;
            MenuDeselect(MenuUNDO);
            continue;
        case PL_ESC:
            if (Not ExpectFirstPoint) {
                CDDelete(Parameters.kpCellDesc,Pointer);
                Pointer = NULL;
                EraseBox(&BB);
                Redisplay(&BB);
            }
            goto quit;
        case PL_CMD:
            if (Not ExpectFirstPoint) {
                if (NumVertices == 1) {
                    CDDelete(Parameters.kpCellDesc,Pointer);
                }
                else {
                    Modified++;
                    record_wire(Pointer);
                }
                Pointer = NULL;
                EraseBox(&BB);
                Redisplay(&BB);
            }
            goto quit;
        }
        if (ExpectFirstPoint) {
            NumVertices++;
            ExpectFirstPoint = False;
            FX = X = KicCursor.kcX;
            FY = Y = KicCursor.kcY;
            Path = AllocatePath(X,Y);
            SetRelative(X,Y,True);
            allocate_wire(Parameters.kpLayer,Path,Width,&BB,
                &Pointer,SQ_INCMPLT);
            ShowWire(Parameters.kpLayer,Width,Path);
            ShowMarker(DISPLAY,ColorTable[HighlightingColor].Ent,X,Y,200);
            continue;
        }
        if (KicCursor.kcX == X And KicCursor.kcY == Y) {
            if (NumVertices == 1) {
                /* click twice to exit */
                CDDelete(Parameters.kpCellDesc,Pointer);
                ShowMarker(ERASE,0,FX,FY,200);
                Pointer = NULL;
                EraseBox(&BB);
                Redisplay(&BB);
                goto quit;
            }
            else {
                Pointer->oInfo = SQ_NEW;
                record_wire(OldPointer);
                OldPointer = Pointer;
                Pointer = NULL;
                OldBB = BB;
                Modified++;
            }
            NumVertices = 0;
            ExpectFirstPoint = True;
            EraseBox(&BB);
            Redisplay(&BB);
            Undo = False;
        }
        else { 
            X = KicCursor.kcX;
            Y = KicCursor.kcY;
            AppendPointToPath(&X,&Y,&Path);
            CDDelete(Parameters.kpCellDesc,Pointer);
            allocate_wire(Parameters.kpLayer,Path,Width,&BB,
                &Pointer,SQ_INCMPLT);
            ShowMarker(ERASE,0,FX,FY,200);
            ShowWire(Parameters.kpLayer,Width,Path);
            NumVertices++;
        }
    }
quit:
    record_wire(OldPointer);
    record_wire(Pointer);
    if (Modified)
        Parameters.kpModified = True;
    SetRelative(0L,0L,False);
    ErasePrompt();
    if (NumVertices == 1)
        ShowMarker(ERASE,0,FX,FY,200);
    MenuDeselect(MenuWIRES);
}


static void
allocate_wire(Layer,Path,Width,BB,Pointer,Info)

int Layer;
struct p *Path;
int Width;
struct ka *BB;
struct o **Pointer;
int Info;
{
    if (Not CDMakeWire(Parameters.kpCellDesc,
        Layer,Width,Path,Pointer)) MallocFailed();
    (*Pointer)->oInfo = Info;
    CDStatusInt = CDBB(Parameters.kpCellDesc,*Pointer,
        &BB->kaLeft,&BB->kaBottom,&BB->kaRight,&BB->kaTop);
    OversizeBox(BB,Width);
}


static void
record_wire(Pointer)

struct o *Pointer;
{
    if (Pointer == NULL) return;
    if (Pointer->oInfo == SQ_GONE)
        CDDelete(Parameters.kpCellDesc,Pointer);
    else
        Pointer->oInfo = SQ_OLD;
}


void
ShowWire(Layer,Width,Path)

int Layer;
int Width;
struct p *Path;
{
    struct p *Pair;
    struct ka BB;
    struct p PolyPath[4];
    int C,D;
    int Wc,Wf;
    int PredX,X,PredY,Y;
    int Width2;
    double alpha;
    char OldRD;

    if (Width <= 0) {
        ShowPath(Layer,Path,0);
        return;
    }

    Wc = View->kvCoarseRatio*Width;
    Wf = View->kvFineRatio*Width;

    Width2 = Width >> 1;
    Pair = Path;
    PredX = Pair->pX;
    PredY = Pair->pY;
    Pair = Pair->pSucc;
    while (Pair != NULL) {
        BB.kaLeft = PredX-Width2;
        BB.kaBottom = PredY-Width2;
        BB.kaRight = PredX+Width2;
        BB.kaTop = PredY+Width2;
        ShowBox(Layer,&BB);
        X = Pair->pX;
        Y = Pair->pY;
        if (IsManhattan(PredX,PredY,X,Y)) {
            if (PredY == Y) {
                /* Horizontal thick line */
                BB.kaLeft = min(PredX,X);
                BB.kaBottom = Y-Width2; 
                BB.kaRight = max(PredX,X);
                BB.kaTop = Y+Width2; 
            }
            else {
                /* Vertical thick line */
                BB.kaLeft = X-Width2;
                BB.kaBottom = min(PredY,Y); 
                BB.kaRight = X+Width2;
                BB.kaTop = max(PredY,Y); 
            }
            ShowBox(Layer,&BB);
        }
        else {
            /* Non-manhattan Wire */
            alpha = -atan2((double)(PredY - Y),(double)(PredX - X));
            C = (double)(Width2) * sin(alpha);
            D = (double)(Width2) * cos(alpha);
            PolyPath[0].pX = PredX + C;
            PolyPath[0].pY = PredY + D;
            PolyPath[0].pSucc = &PolyPath[1];
            PolyPath[1].pX = X + C;
            PolyPath[1].pY = Y + D;
            PolyPath[1].pSucc = &PolyPath[2];
            PolyPath[2].pX = X - C;
            PolyPath[2].pY = Y - D;
            PolyPath[2].pSucc = &PolyPath[3];
            PolyPath[3].pX = PredX - C;
            PolyPath[3].pY = PredY - D;
            PolyPath[3].pSucc = NULL;
            OldRD = Parameters.kpRedisplayControl;
            if (OldRD != FINEVIEWPORTONLY) {
                Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
                if (Wc < 1 || Layer == ColorTable[HighlightingColor].Ent)
                    ShowPath(Layer,PolyPath,True);
                else
                    ShowPolygon(Layer,PolyPath);
            }
            if (OldRD != COARSEVIEWPORTONLY) {
                Parameters.kpRedisplayControl = FINEVIEWPORTONLY;
                if (Wf < 1 || Layer == ColorTable[HighlightingColor].Ent)
                    ShowPath(Layer,PolyPath,True);
                else
                    ShowPolygon(Layer,PolyPath);
            }
            Parameters.kpRedisplayControl = OldRD;
        }
        Pair = Pair->pSucc;
        PredX = X;
        PredY = Y;
    }
    BB.kaLeft = PredX-Width2;
    BB.kaBottom = PredY-Width2;
    BB.kaRight = PredX+Width2;
    BB.kaTop = PredY+Width2;
    ShowBox(Layer,&BB);
}


void
Width(LookedAhead)

int *LookedAhead;
{
    char *TypeIn;
    int Layer, Undo = False;
    int Width, dummy;
    double d;
    struct ks *sq;
    struct o *Pointer, *NewPointer;
    struct p *Path;

    MenuSelect(MenuWIDTH);

    for (sq = SelectQHead; sq; sq = sq->ksSucc) {
        Pointer = sq->ksPointer;
        if (Pointer->oInfo == SQ_GONE)
            continue;
        if (Pointer->oLayer != Parameters.kpLayer &&
            Parameters.kpLayerSpecificSelection)
            continue;
        if (Pointer->oType == CDWIRE)
            break;
    }
    if (sq) {
        /* There is a wire selected. */
        sprintf(TypeOut,
            "Enter new width for selected wires (default width %g): ",
            (double)LayerTable[Parameters.kpLayer].klWireWidth/RESOLUTION);
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (!TypeIn){
            MenuDeselect(MenuWIDTH);
            ErasePrompt();
            return;
        }

        Width = LayerTable[Parameters.kpLayer].klWireWidth;
        if (*TypeIn != '\0' && *TypeIn != '\n') {
            if (sscanf(TypeIn,"%lg",&d) == 1 && d >= 0) {
                Width = d*RESOLUTION;
            }
        }
again:

        for (sq = SelectQHead; sq; sq = sq->ksSucc) {
            Pointer = sq->ksPointer;
            if (Pointer->oType != CDWIRE)
                continue;
            if (Pointer->oInfo == SQ_GONE)
                continue;
            if (Pointer->oLayer != Parameters.kpLayer &&
                Parameters.kpLayerSpecificSelection)
                continue;
            CDWire(Pointer,&Layer,&dummy,&Path);
            copy_path(&Path);
            if (Not CDMakeWire(Parameters.kpCellDesc,Layer,Width,Path,
                &NewPointer)) MallocFailed();
            NewPointer->oInfo = SQ_NEWSEL;
            SQInsert(NewPointer);
            Pointer->oInfo = SQ_GONE;
        }
        SQComputeBB();
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
        MenuDeselect(MenuWIDTH);
        ErasePrompt();

top:
        switch (PointLoop(LookedAhead)) {
        case PL_UND:
            if (!Undo) {
                MenuSelect(MenuUNDO);
                SQComputeBB();

                /* put everything back... */
                SQRestore(True);
                for (sq = SelectQHead; sq; sq = sq->ksSucc) {
                    if (sq->ksPointer->oInfo == SQ_NEWSEL)
                        sq->ksPointer->oInfo = SQ_OLDSEL;
                }

                EraseBox(&SelectQBB);
                Redisplay(&SelectQBB);
                MenuDeselect(MenuUNDO);
                Undo = True;
            }
            else {
                Undo = False;
                goto again;
            }
        case PL_ESC:
        case PL_PCW:
            goto top;
        case PL_CMD:
            SQRestore(False);
            break;
        }
        return;
    }

    sprintf(TypeOut,"Wire width for current layer is %g, enter new width: ",
        (double)LayerTable[Parameters.kpLayer].klWireWidth/RESOLUTION);
    ShowPrompt(TypeOut);
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL && *TypeIn != '\0' && *TypeIn != '\n') {
        if (sscanf(TypeIn,"%lg",&d) == 1 && d >= 0) {
            Width = d*RESOLUTION;
            if (Width < LayerTable[Parameters.kpLayer].klMinDimensions) {
                sprintf(TypeOut,
                    "Can't, the minimum dimension on this layer is %g.",
                    (double)LayerTable[Parameters.kpLayer].klMinDimensions/
                    RESOLUTION);
                ShowPromptAndWait(TypeOut);
            }
            else
                LayerTable[Parameters.kpLayer].klWireWidth = Width;
        }
        else
            ShowPromptAndWait("Bad value, width not changed.");
    }
    ErasePrompt();
    MenuDeselect(MenuWIDTH);
}


static void
copy_path(Path)

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
        OldPair = OldPair->pSucc;
    }
}


void
RemoveLastPointInPath(Path)

struct p **Path;
{
    struct p *OldPair;
    struct p *NewPair;

    OldPair = *Path;
    if (OldPair == NULL Or OldPair->pSucc == NULL)
        return;
    if ((NewPair = alloc(p)) == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    *Path = NewPair;
    NewPair->pX = OldPair->pX; 
    NewPair->pY = OldPair->pY; 
    NewPair->pSucc = NULL;
    OldPair = OldPair->pSucc;
    while (OldPair->pSucc != NULL) {
        if ((NewPair->pSucc = alloc(p)) == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        NewPair = NewPair->pSucc;
        NewPair->pX = OldPair->pX; 
        NewPair->pY = OldPair->pY; 
        NewPair->pSucc = NULL;
        OldPair = OldPair->pSucc;
    }
}


void
AppendPointToPath(X,Y,Path)

struct p **Path;
int *X,*Y;
{
    struct p *OldPair;
    struct p *NewPair;

    if (Path == NULL || *Path == NULL)
        return;
    OldPair = *Path;
    if ((NewPair = alloc(p)) == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    *Path = NewPair;
    NewPair->pX = OldPair->pX; 
    NewPair->pY = OldPair->pY; 
    NewPair->pSucc = NULL;
    OldPair = OldPair->pSucc;
    while (OldPair != NULL) {
        if ((NewPair->pSucc = alloc(p)) == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        NewPair = NewPair->pSucc;
        NewPair->pX = OldPair->pX; 
        NewPair->pY = OldPair->pY; 
        NewPair->pSucc = NULL;
        OldPair = OldPair->pSucc;
    }
    /* Append the new reference point */
    if (Parameters.kp45s)
        To45(NewPair->pX,NewPair->pY,X,Y);
    if ((NewPair->pSucc = alloc(p)) == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    NewPair = NewPair->pSucc;
    NewPair->pX = *X;
    NewPair->pY = *Y;
    NewPair->pSucc = NULL;
}

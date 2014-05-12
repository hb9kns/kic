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
 * Polygon management.
 */

#include "prefix.h"
#include "kic.h"
#include <math.h>

extern char *MenuPOLYG;
extern char *MenuFLASH;
extern char *MenuDONUT;
extern char *MenuARC;
extern char *MenuUNDO;

#ifndef PI
#define PI        3.14159265358979323846
#endif

#define RADTODEG 57.29577951

#ifdef __STDC__
static void allocate_poly(int,struct p*,struct ka*,struct o**,int);
static void record_poly(struct o*);
static double compute_radius(int,int);
static void clip_message(void);
#else
static void allocate_poly();
static void record_poly();
static double compute_radius();
static void clip_message();
#endif


void
Polygons(LookedAhead)

int *LookedAhead;
{
    struct p *Path = NULL;
    struct ka BB,OldBB;
    struct o *Pointer = NULL,*OldPointer = NULL;
    int ExpectFirstPoint = True;
    int NumVertices = 0;
    int Modified = 0;
    int Undo = False;
    int X,Y,FirstX, FirstY;
    char *errmsg = "Can't allow a degenerate polygon.";

    MenuSelect(MenuPOLYG);
    SetCurrentAOI(View->kvCoarseWindow);

    loop {
        ShowPrompt("Point to vertices.");

        switch (PointLoopCreate(LookedAhead)) {
        case PL_PLT:
            if (NumVertices > 1) {
                Path = CopyPath(Path);
                CDDelete(Parameters.kpCellDesc,Pointer);
                EraseBox(&BB);
                Redisplay(&BB);
                allocate_poly(Parameters.kpLayer,Path,&BB,
                    &Pointer,SQ_INCMPLT);
                ShowPath(Parameters.kpLayer,Path,False);
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
                ShowMarker(ERASE,0,FirstX,FirstY,200);
            }
            else {
                allocate_poly(Parameters.kpLayer,Path,&BB,&Pointer,SQ_INCMPLT);
                LastPointInPath(&X,&Y,Path);
                ShowPath(Parameters.kpLayer,Path,False);
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
                if (NumVertices <= 2) {
                    CDDelete(Parameters.kpCellDesc,Pointer);
                    Pointer = NULL;
                }
                else {
                    AppendPointToPath(&FirstX,&FirstY,&Path);
                    CDDelete(Parameters.kpCellDesc,Pointer);
                    allocate_poly(Parameters.kpLayer,Path,&BB,&Pointer,0);
                    Modified++;
                }
                EraseBox(&BB);
                Redisplay(&BB);
            }
            goto quit;
        }
        if (ExpectFirstPoint) {
            NumVertices++;
            ExpectFirstPoint = False;
            FirstX = X = KicCursor.kcX;
            FirstY = Y = KicCursor.kcY;
            Path = AllocatePath(X,Y);
            SetRelative(X,Y,True);
            allocate_poly(Parameters.kpLayer,Path,&BB,&Pointer,SQ_INCMPLT);
            ShowMarker(DISPLAY,ColorTable[HighlightingColor].Ent,X,Y,200);
            continue;
        }
        if (KicCursor.kcX == X And KicCursor.kcY == Y) {
            X = FirstX;
            Y = FirstY;
        }
        else {
            X = KicCursor.kcX;
            Y = KicCursor.kcY;
        }
        AppendPointToPath(&X,&Y,&Path);
        CDDelete(Parameters.kpCellDesc,Pointer);
        allocate_poly(Parameters.kpLayer,Path,&BB,&Pointer,SQ_INCMPLT);

        if (X == FirstX And Y == FirstY) {
            if (NumVertices <= 2) {
                CDDelete(Parameters.kpCellDesc,Pointer);
                ShowMarker(ERASE,0,FirstX,FirstY,200);
                ShowPromptAndWait(errmsg);
            }
            else {
                Pointer->oInfo = SQ_NEW;
                record_poly(OldPointer);
                OldPointer = Pointer;
                OldBB = BB;
                Modified++;
            }
            Pointer = NULL;
            NumVertices = 0;
            ExpectFirstPoint = True;
            EraseBox(&BB);
            Redisplay(&BB);
            Undo = False;
        }
        else {
            ShowMarker(ERASE,0,FirstX,FirstY,200);
            ShowPath(Parameters.kpLayer,Path,False);
            NumVertices++;
        }
    }
quit:
    record_poly(OldPointer);
    record_poly(Pointer);
    if (Modified)
        Parameters.kpModified = True;
    SetRelative(0L,0L,False);
    ErasePrompt();
    if (NumVertices == 1)
        ShowMarker(ERASE,0,FirstX,FirstY,200);
    MenuDeselect(MenuPOLYG);
}


void
Flash(LookedAhead)

int *LookedAhead;
{
    struct ka BB;
    struct p *Path, *NewPath;
    struct o *Pointer, *OldPointer = NULL;
    double Angle,Rad = 0.0,DPhi;
    int CenX,CenY,PerX,PerY;
    int i;
    int Undo = False;
    int Modified = 0;

    MenuSelect(MenuFLASH);
    SetCurrentAOI(View->kvCoarseWindow);

    DPhi = 2*PI/Parameters.kpNumRoundFlashSides;

    loop {
        ShowPrompt("Point at center.");
        switch (PointLoop(LookedAhead)) {
        case PL_UND:
            if (OldPointer == NULL)
                goto quit;
            MenuSelect(MenuUNDO);
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
            EraseBox(&BB);
            Redisplay(&BB);
            MenuDeselect(MenuUNDO);
            continue;
        case PL_ESC:
        case PL_CMD:
            goto quit;
        }
        CenX = KicCursor.kcX;
        CenY = KicCursor.kcY;
        ShowMarker(DISPLAY,ColorTable[HighlightingColor].Ent,CenX,CenY,200);
        SetRelative(CenX,CenY,True);

next:
        ShowPrompt("Point at the perimeter.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            ShowMarker(ERASE,0,CenX,CenY,200);
            goto quit;
        case PL_UND:
            ShowMarker(ERASE,0,CenX,CenY,200);
            continue;
        case PL_PCW:
            PerX = KicCursor.kcX;
            PerY = KicCursor.kcY;
            Rad = compute_radius(CenX - PerX,CenY - PerY);
            if (Rad < (double)
                LayerTable[Parameters.kpLayer].klMinDimensions/
                    RESOLUTION) {
                ShowPromptAndWait(
                "Sorry.  The radius is less than the minimum dimension.");
                goto next;
            }
        }
        clip_message();
        ShowMarker(ERASE,0,CenX,CenY,200);

        Angle = 0;
        Path = NewPath = AllocatePath(CenX,CenY+(int)Rad);
        for (i = 1; i < Parameters.kpNumRoundFlashSides; i++) {
            Angle += DPhi;
            NewPath = NewPath->pSucc = AllocatePath(
                CenX + (int)(Rad*sin(Angle)),
                CenY + (int)(Rad*cos(Angle)));
        }
        NewPath = NewPath->pSucc = AllocatePath(CenX,CenY+(int)Rad);

        if (Parameters.kpClipVerticesToGrid)
            for (NewPath = Path; NewPath; NewPath = NewPath->pSucc)
                ClipToGridPoint(&NewPath->pX,&NewPath->pY);

        if (Not CDMakePolygon(Parameters.kpCellDesc,Parameters.kpLayer,
            Path,&Pointer)) MallocFailed();

        Pointer->oInfo = SQ_NEW;
        record_poly(OldPointer);
        OldPointer = Pointer;

        BB.kaLeft   = CenX - Rad;
        BB.kaRight  = CenX + Rad;
        BB.kaBottom = CenY - Rad;
        BB.kaTop    = CenY + Rad;
        Modified++;
        Undo = False;
        EraseBox(&BB);
        Redisplay(&BB);
    }

quit:
    record_poly(OldPointer);
    if (Modified)
        Parameters.kpModified = True;
    SetRelative(0L,0L,False);
    ErasePrompt();
    MenuDeselect(MenuFLASH);
}


void
Doughnut(LookedAhead)

int *LookedAhead;
{
    struct ka BB;
    struct o *Pointer1,*Pointer2,*OldPointer1 = NULL,*OldPointer2 = NULL;
    struct p *Path1, *Path2, *NewPath1, *NewPath2;
    double Angle,DPhi,Rad1 = 0.0,Rad2 = 0.0;
    int CenX = 0,CenY = 0;
    int i,Sides;
    int Modified = 0;
    int Undo = False;

    MenuSelect(MenuDONUT);
    SetCurrentAOI(View->kvCoarseWindow);
    OldPointer1 = NULL;
    Sides = Parameters.kpNumRoundFlashSides;
    Sides = ((Sides+1)/2);
    DPhi = PI/Sides;

    loop {
        ShowPrompt("Point at center.");
        switch (PointLoop(LookedAhead)) {
        case PL_UND:
            if (OldPointer1 == NULL)
                goto quit;
            MenuSelect(MenuUNDO);
            if (Undo == False) {
                OldPointer1->oInfo = SQ_GONE;
                OldPointer2->oInfo = SQ_GONE;
                Modified--;
                Undo = True;
            }
            else {
                OldPointer1->oInfo = SQ_NEW;
                OldPointer2->oInfo = SQ_NEW;
                Modified++;
                Undo = False;
            }
            EraseBox(&BB);
            Redisplay(&BB);
            MenuDeselect(MenuUNDO);
            continue;
        case PL_ESC:
        case PL_CMD:
            goto quit;
        }

        CenX = KicCursor.kcX;
        CenY = KicCursor.kcY;
        ShowMarker(DISPLAY,ColorTable[HighlightingColor].Ent,CenX,CenY,200);
        SetRelative(CenX,CenY,True);

inner:
        ShowPrompt("Point at inner radius.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            ShowMarker(ERASE,0,CenX,CenY,200);
            continue;
        case PL_PCW:
            Rad1 = compute_radius(CenX-KicCursor.kcX,CenY-KicCursor.kcY);
            if (Rad1 == 0) {
                ShowPromptAndWait(
                    "Inner diameter is zero, use Flash to make a disk.");
                goto inner;
            }
            if (Rad1 < (double)
                LayerTable[Parameters.kpLayer].klMinDimensions/
                    (2*RESOLUTION)) {
                ShowPromptAndWait(
                    "Inner diameter is less than the minimum dimension.");
                goto inner;
            }
        }
outer:

        ShowPrompt("Point at outer radius.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            goto inner;
        case PL_PCW:
            Rad2 = compute_radius(CenX-KicCursor.kcX,CenY-KicCursor.kcY);
            if (Rad2 <= Rad1) {
                ShowPromptAndWait("The width is zero or negative.");
                goto outer;
            }
            if (Rad2 - Rad1 < (double)
                LayerTable[Parameters.kpLayer].klMinDimensions/
                    RESOLUTION) {
                ShowPromptAndWait(
                    "The width is less than the minimum dimension.");
                goto outer;
            }
        }

        clip_message();
        ShowMarker(ERASE,ColorTable[HighlightingColor].Ent,CenX,CenY,200);

        Angle = 0;
        Path1 = NewPath1 = AllocatePath(CenX,CenY+(int)Rad1);
        Path2 = NewPath2 = AllocatePath(CenX,CenY-(int)Rad2);
        for (i = 1; i < Sides; i++) {
            Angle += DPhi;
            NewPath1 = NewPath1->pSucc = AllocatePath(
                CenX + (int)(Rad1*sin(Angle)),
                CenY + (int)(Rad1*cos(Angle)));
            NewPath2 = NewPath2->pSucc = AllocatePath(
                CenX + (int)(Rad2*sin(Angle)),
                CenY - (int)(Rad2*cos(Angle)));
        }
        NewPath1 = NewPath1->pSucc = AllocatePath(CenX,CenY-(int)Rad1);
        NewPath2 = NewPath2->pSucc = AllocatePath(CenX,CenY+(int)Rad2);
        NewPath1->pSucc = Path2;
        NewPath2->pSucc = AllocatePath(CenX,CenY+(int)Rad1);

        if (Parameters.kpClipVerticesToGrid)
            for (NewPath1 = Path1; NewPath1; NewPath1 = NewPath1->pSucc)
                ClipToGridPoint(&NewPath1->pX,&NewPath1->pY);

        if (Not CDMakePolygon(Parameters.kpCellDesc,Parameters.kpLayer,Path1,
            &Pointer1)) MallocFailed();

        Angle = 0;
        Path1 = NewPath1 = AllocatePath(CenX,CenY+(int)Rad1);
        Path2 = NewPath2 = AllocatePath(CenX,CenY-(int)Rad2);
        for (i = 1; i < Sides; i++) {
            Angle += DPhi;
            NewPath1 = NewPath1->pSucc = AllocatePath(
                CenX - (int)(Rad1*sin(Angle)),
                CenY + (int)(Rad1*cos(Angle)));
            NewPath2 = NewPath2->pSucc = AllocatePath(
                CenX - (int)(Rad2*sin(Angle)),
                CenY - (int)(Rad2*cos(Angle)));
        }
        NewPath1 = NewPath1->pSucc = AllocatePath(CenX,CenY-(int)Rad1);
        NewPath2 = NewPath2->pSucc = AllocatePath(CenX,CenY+(int)Rad2);
        NewPath1->pSucc = Path2;
        NewPath2->pSucc = AllocatePath(CenX,CenY+(int)Rad1);

        if (Parameters.kpClipVerticesToGrid)
            for (NewPath1 = Path1; NewPath1; NewPath1 = NewPath1->pSucc)
                ClipToGridPoint(&NewPath1->pX,&NewPath1->pY);

        if (Not CDMakePolygon(Parameters.kpCellDesc,Parameters.kpLayer,Path1,
            &Pointer2)) MallocFailed();

        Pointer1->oInfo = SQ_NEW;
        Pointer2->oInfo = SQ_NEW;
        record_poly(OldPointer1);
        record_poly(OldPointer2);
        OldPointer1 = Pointer1;
        OldPointer2 = Pointer2;

        BB.kaLeft   = CenX - Rad2;
        BB.kaRight  = CenX + Rad2;
        BB.kaBottom = CenY - Rad2;
        BB.kaTop    = CenY + Rad2;
        Modified++;
        Undo = False;
        EraseBox(&BB);
        Redisplay(&BB);
    }
quit:
    ShowMarker(ERASE,0,CenX,CenY,200);
    record_poly(OldPointer1);
    record_poly(OldPointer2);
    if (Modified)
        Parameters.kpModified = True;
    SetRelative(0L,0L,False);
    ErasePrompt();
    MenuDeselect(MenuDONUT);
}


void
Arcs(LookedAhead)

int *LookedAhead;
{
    struct ka BB;
    struct p *Path1,*Path2,*NewPath1,*NewPath2;
    struct o *Pointer,*OldPointer = NULL;
    double A1,A2,Rad1 = 0.0,Rad2 = 0.0;
    double Angle1 = 0.0,Angle2 = 0.0,DPhi;
    int CenX = 0, CenY = 0;
    int i,Sides;
    int Undo = False;
    int Modified = 0;

    MenuSelect(MenuARC);
    SetCurrentAOI(View->kvCoarseWindow);

    loop {
        ShowPrompt("Point at center of arc.");
        switch (PointLoop(LookedAhead)) {
        case PL_UND:
            if (OldPointer  == NULL)
                goto quit;
            MenuSelect(MenuUNDO);
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
            EraseBox(&BB);
            Redisplay(&BB);
            MenuDeselect(MenuUNDO);
            continue;
        case PL_ESC:
        case PL_CMD:
            goto quit;
        }
        CenX = KicCursor.kcX;
        CenY = KicCursor.kcY;
        ShowMarker(DISPLAY,ColorTable[HighlightingColor].Ent,CenX,CenY,200);
        SetRelative(CenX,CenY,True);

inner:
        ShowPrompt("Point at inner radius.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            ShowMarker(ERASE,0,CenX,CenY,200);
            continue;
        case PL_PCW:
            Rad1 = compute_radius(CenX-KicCursor.kcX,CenY-KicCursor.kcY);
            if (Rad1 < (double)
                LayerTable[Parameters.kpLayer].klMinDimensions/
                    (2*RESOLUTION)) {
                ShowPromptAndWait(
                    "Inner diameter is less than the minimum dimension.");
                goto inner;
            }
        }

outer:
        ShowPrompt("Point at outer radius.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            goto inner;
        case PL_PCW:
            Rad2 = compute_radius(CenX-KicCursor.kcX,CenY-KicCursor.kcY);
            if (Rad2 - Rad1 < (double)
                LayerTable[Parameters.kpLayer].klMinDimensions/
                    RESOLUTION) {
                ShowPromptAndWait(
                    "The width is less than the minimum dimension.");
                goto outer;
            }
        }

begin:
        ShowPrompt("Point to beginning of arc in a clockwise path.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            goto outer;
        case PL_PCW:
            Angle2 = atan2((double)(KicCursor.kcY-CenY),
                (double)(KicCursor.kcX-CenX));
        }
        ShowPrompt("Point to end of arc in a clockwise path.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            goto begin;
        case PL_PCW:
            Angle1 = atan2((double)(KicCursor.kcY-CenY),
                (double)(KicCursor.kcX-CenX));
        }
        clip_message();
        ShowMarker(ERASE,ColorTable[HighlightingColor].Ent,CenX,CenY,200);

        if (Angle2 < Angle1) Angle2 += 2*PI;

        Sides = ((Angle2 - Angle1)/(2*PI))*Parameters.kpNumRoundFlashSides;
        if (Sides < 3) Sides = 3;

        DPhi = (Angle2 - Angle1)/Sides;

        A1 = Angle1;
        A2 = Angle2;
        Path1 = NewPath1 = AllocatePath(
               CenX + (int)(Rad1*cos(Angle1)),
               CenY + (int)(Rad1*sin(Angle1)));
        Path2 = NewPath2 = AllocatePath(
               CenX + (int)(Rad2*cos(Angle2)),
               CenY + (int)(Rad2*sin(Angle2)));
        for (i = 1; i < Sides; i++) {
            A1 += DPhi;
            A2 -= DPhi;
            NewPath1 = NewPath1->pSucc = AllocatePath(
                CenX + (int)(Rad1*cos(A1)),
                CenY + (int)(Rad1*sin(A1)));
            NewPath2 = NewPath2->pSucc = AllocatePath(
                CenX + (int)(Rad2*cos(A2)),
                CenY + (int)(Rad2*sin(A2)));
        }
        NewPath1 = NewPath1->pSucc = AllocatePath(
               CenX + (int)(Rad1*cos(Angle2)),
               CenY + (int)(Rad1*sin(Angle2)));
        NewPath2 = NewPath2->pSucc = AllocatePath(
               CenX + (int)(Rad2*cos(Angle1)),
               CenY + (int)(Rad2*sin(Angle1)));
        NewPath1->pSucc = Path2;
        NewPath2->pSucc = AllocatePath(
               CenX + (int)(Rad1*cos(Angle1)),
               CenY + (int)(Rad1*sin(Angle1)));

        if (Parameters.kpClipVerticesToGrid)
            for (NewPath1 = Path1; NewPath1; NewPath1 = NewPath1->pSucc)
                ClipToGridPoint(&NewPath1->pX,&NewPath1->pY);

        if (Not CDMakePolygon(Parameters.kpCellDesc,Parameters.kpLayer,Path1,
            &Pointer)) MallocFailed();

        Pointer->oInfo = SQ_NEW;
        record_poly(OldPointer);
        OldPointer = Pointer;
        BB.kaLeft   = CenX - Rad2;
        BB.kaRight  = CenX + Rad2;
        BB.kaBottom = CenY - Rad2;
        BB.kaTop    = CenY + Rad2;
        Modified++;
        Undo = False;
        EraseBox(&BB);
        Redisplay(&BB);
    }
quit:
    record_poly(OldPointer);
    ShowMarker(ERASE,0,CenX,CenY,200);
    if (Modified)
        Parameters.kpModified = True;
    SetRelative(0L,0L,False);
    ErasePrompt();
    MenuDeselect(MenuARC);
}


void
ShowPath(Layer,Path,Terminate)

int Layer;
struct p *Path;
int Terminate;     /* If True, the path is closed */
{
    struct p *Pair;
    int firstX,firstY,X,Y,lastX,lastY;

    Pair = Path;
    firstX = lastX = Pair->pX;
    firstY = lastY = Pair->pY;
    Pair = Pair->pSucc;
    while (Pair != NULL) {
        X = Pair->pX;
        Y = Pair->pY;
        ShowLine(Layer,lastX,lastY,X,Y);
        lastX = X;
        lastY = Y;
        Pair = Pair->pSucc;
    }
    if (Terminate)
        ShowLine(Layer,firstX,firstY,lastX,lastY);
}


void
ShowPolygon(Layer,Path)

int Layer;
struct p *Path;
{
    struct p *Pair;
    int *PolygonBuffer;
    int i,n,nfine,ncoarse,n2;
    int X,Y;


    for (n = 0,Pair = Path; Pair != NULL; n++,Pair = Pair->pSucc) ;
    if (Layer == ColorTable[HighlightingColor].Ent Or
        !(LayerTable[Layer].klAttributes & FILLED) Or
        !FB.fFilledPolygons Or n < 3) {
        ShowPath(Layer,Path,True);
        return;
    }
    n2 = n << 1;
    ncoarse = nfine = n;
    PolygonBuffer = (int*) tmalloc(n2*4*sizeof(int));

    if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY And
        CurrentAOI.aInCoarse) {

        for (i = 0,Pair = Path; i < n2; i += 2,Pair = Pair->pSucc) {
            X = Pair->pX;
            Y = Pair->pY;
            TPoint(&X,&Y);
            CoarseLToP(X,Y,PolygonBuffer[i],PolygonBuffer[i+1]);
        }

        FBPolygonClip(PolygonBuffer,&ncoarse,(struct ka*)&CurrentAOI.aLC);
        if (LayerTable[Layer].klAttributes & COARSE_FILL) {
            FBPolygon(Layer,FILL,LayerTable[Layer].klStyleID,
                PolygonBuffer,ncoarse);
            if (LayerTable[Layer].klAttributes & OUTLINED)
                ShowPath(Layer,Path,True);
        }
        else
            FBPolygon(Layer,FILL,0,PolygonBuffer,ncoarse);
    }

    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY And
        CurrentAOI.aInFine) {

        for (i = 0,Pair = Path; i < n2; i += 2,Pair = Pair->pSucc) {
            X = Pair->pX;
            Y = Pair->pY;
            TPoint(&X,&Y);
            FineLToP(X,Y,PolygonBuffer[i],PolygonBuffer[i+1]);
        }

        FBPolygonClip(PolygonBuffer,&nfine,(struct ka*)&CurrentAOI.aLF);
        if (LayerTable[Layer].klAttributes & FINE_FILL) {
            FBPolygon(Layer,FILL,LayerTable[Layer].klStyleID,
                PolygonBuffer,nfine);
            if (LayerTable[Layer].klAttributes & OUTLINED)
                ShowPath(Layer,Path,True);
        }
        else
            FBPolygon(Layer,FILL,0,PolygonBuffer,nfine);
    }
    free(PolygonBuffer);
}


void
LastPointInPath(X,Y,Path)

int *X,*Y;
struct p *Path;
{
    struct p *p;

    for (p = Path; p && p->pSucc; p = p->pSucc) ;
    if (p) {
        *X = p->pX;
        *Y = p->pY;
    }
}


struct p *
AllocatePath(X,Y)

int X,Y;
{
    struct p *Path;

    if ((Path = alloc(p)) == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    Path->pX = X;
    Path->pY = Y;
    Path->pSucc = NULL;
    return Path;
}


static void
allocate_poly(Layer,Path,BB,Pointer,Info)

int Layer;
struct p *Path;
struct ka *BB;
struct o **Pointer;
int Info;
{
    CDBogusPoly = True;
    if (Not CDMakePolygon(Parameters.kpCellDesc,
        Layer,Path,Pointer)) MallocFailed();
    CDBogusPoly = False;
    (*Pointer)->oInfo = Info;
    CDStatusInt = CDBB(Parameters.kpCellDesc,*Pointer,
        &BB->kaLeft,&BB->kaBottom,&BB->kaRight,&BB->kaTop);
}


static void
record_poly(Pointer)

struct o *Pointer;
{
    if (Pointer == NULL) return;
    if (Pointer->oInfo == SQ_GONE)
        CDDelete(Parameters.kpCellDesc,Pointer);
    else
        Pointer->oInfo = SQ_OLD;
}


static double
compute_radius(x,y)

int x,y;
{
    return (int)sqrt(x*(double)x + y*(double)y);
}


static void
clip_message()

{
    if (Parameters.kpClipVerticesToGrid)
        ShowPrompt("Vertices will be clipped to the nearest grid point.");
}

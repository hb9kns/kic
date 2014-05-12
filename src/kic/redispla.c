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
 * The KIC redisplay code.
 */

#include "prefix.h"
#include "kic.h"

/* cells smaller than this are never expanded */
static int MinWidth;

#define ka_copy(BB1,BB2) BB1.kaLeft = BB2->kaLeft; \
    BB1.kaRight = BB2->kaRight; \
    BB1.kaBottom = BB2->kaBottom; \
    BB1.kaTop = BB2->kaTop; \
    TPoint(&BB1.kaLeft,&BB1.kaBottom); \
    TPoint(&BB1.kaRight,&BB1.kaTop); \
    if(BB.kaLeft > BB.kaRight) \
        SwapInts(BB1.kaLeft,BB1.kaRight); \
    if(BB.kaBottom > BB.kaTop) \
        SwapInts(BB1.kaBottom,BB1.kaTop);

#ifdef __STDC__
static void redisplay_layer(struct s*,struct ka*,int,int,int,int);
#else
static void redisplay_layer();
#endif


void
Redisplay(AOI)

struct ka *AOI;
{
    struct ka FineAOI;
    int Layer,Expanding;
    char Temp;
    
    if (Parameters.kpRedisplayControl == SPLITSCREEN) {
        SetCurrentAOI(View->kvCoarseWindow);
        XORfineViewport();
    }

    SetCurrentAOI(AOI);

    if (CurrentAOI.aInCoarse == 0 And CurrentAOI.aInFine == 0)
        goto quit;

    Parameters.kpNumGeometries = 0;
    if (Parameters.kpShowBandwidth)
        StartTiming();

    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY) {

        FineAOI.kaX      = AOI->kaX;
        FineAOI.kaY      = AOI->kaY;
        FineAOI.kaLeft   = max(AOI->kaLeft,View->kvFineWindow->kaLeft);
        FineAOI.kaRight  = min(AOI->kaRight,View->kvFineWindow->kaRight);
        FineAOI.kaTop    = min(AOI->kaTop,View->kvFineWindow->kaTop);
        FineAOI.kaBottom = max(AOI->kaBottom,View->kvFineWindow->kaBottom);
    }

    if (!Parameters.kpGridOnTop)
        ShowGrid();

    if (Parameters.kpCellDesc != NULL) {
        for (Layer = 1; Layer <= NumLayerTable; ++Layer) {
            if (!(LayerTable[Layer].klAttributes & VISIBLE)) continue;

            if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY) {
                Temp = Parameters.kpRedisplayControl;
                Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
                MinWidth = 4.0/View->kvCoarseRatio;
                Expanding = Parameters.kpExpandInstances;
                if (Parameters.kpShowContext &&
                        (Parameters.kpCellDesc != Parameters.kpTopDesc)) {
                    TPush();
                    TLoad();
                    Parameters.kpHierarchyLevel = 0;
                    redisplay_layer(Parameters.kpTopDesc,AOI,Layer,
                        True,Layer == NumLayerTable,True);
                    TPop();
                    if (Parameters.kpSIGINTERRUPT) {
                        RedisplayAfterInterrupt();
                        goto quit;
                    }
                }
                Parameters.kpHierarchyLevel = 0;
                redisplay_layer(Parameters.kpCellDesc,AOI,Layer,
                    Expanding,Layer == NumLayerTable,False);
                Parameters.kpRedisplayControl = Temp;
                if (Parameters.kpSIGINTERRUPT) {
                    RedisplayAfterInterrupt();
                    goto quit;
                }
            }

            if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY) {
                Temp = Parameters.kpRedisplayControl;
                Parameters.kpRedisplayControl = FINEVIEWPORTONLY;
                MinWidth = 4.0/View->kvFineRatio;
                Expanding = Parameters.kpExpandInstances ||
                    Parameters.kpExpandFineViewportOnly;
                if (Parameters.kpShowContext &&
                        (Parameters.kpCellDesc != Parameters.kpTopDesc)) {
                    TPush();
                    TLoad();
                    Parameters.kpHierarchyLevel = 0; 
                    redisplay_layer(Parameters.kpTopDesc,&FineAOI,Layer,
                        True,Layer == NumLayerTable,True);
                    if (Parameters.kpSIGINTERRUPT) {
                        RedisplayAfterInterrupt();
                        goto quit;
                    }
                    TPop();
                }
                Parameters.kpHierarchyLevel = 0;
                redisplay_layer(Parameters.kpCellDesc,&FineAOI,Layer,
                    Expanding,Layer == NumLayerTable,False);
                Parameters.kpRedisplayControl = Temp;
                if (Parameters.kpSIGINTERRUPT) {
                    RedisplayAfterInterrupt();
                    goto quit;
                }
            }
        }
    }

    if (Parameters.kpGridOnTop)
        ShowGrid();
    else
        ShowAxes();

    /*
     * Highlight selected objects.
     */
    SQShow();

    /* SRW ** show markers */
    ShowMarker(DISPLAY,ColorTable[HighlightingColor].Ent,0L,0L,0);

    if (Parameters.kpShowBandwidth) {
        StopTiming();
        ShowRatio("geometries",Parameters.kpNumGeometries,"seconds(real)",
            ElapsedRealTime());
        ShowRatio("geometries",Parameters.kpNumGeometries,"seconds(user)",
            ElapsedUserTime());
        ShowRatio("geometries",Parameters.kpNumGeometries,"seconds(system)",
            ElapsedSystemTime());
    }

quit:
    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY) {
        FBForeground(DISPLAY,ColorTable[HighlightingColor].Ent);
        OutlineBox(View->kvFineViewport);
    }
    SetCurrentAOI(View->kvCoarseWindow);
    if (Parameters.kpRedisplayControl == SPLITSCREEN)
        XORfineViewport();
}


static void
redisplay_layer(CellDesc,AOI,Layer,Expand,ShowBB,Context)


/* if Expand, show subcells in expanded form.
 * if ShowBB, if !Expand, show bounding box of subcells.
 * if Context, don't show current cell or descendents.
 */
struct s *CellDesc;
struct ka *AOI;
int Layer,Expand,ShowBB,Context;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct p *Path;
    struct ka BB;
    struct ka MasterBB;
    struct s *MasterDesc;
    char Type,Xform;
    char *MasterName;
    char *Label;
    int NumX,NumY;
    int Int1,Int2;
    int X,Y;
    int Width;
    int DX,DY;

    ++Parameters.kpHierarchyLevel;

    if (Not CDInitGen(CellDesc,Layer,AOI->kaLeft,AOI->kaBottom,
        AOI->kaRight,AOI->kaTop,&GenDesc)) MallocFailed();
    loop {
        /* Test for user interrupt */
        if (Parameters.kpSIGINTERRUPT) return;

        CDGen(CellDesc,GenDesc,&Pointer);
        if (Pointer == NULL) break;
        /* Don't display if conditionally deleted. */
        if (Pointer->oInfo == SQ_GONE) continue;
        ++Parameters.kpNumGeometries;

        Type = Pointer->oType;
        if (Type == CDBOX) {
            CDStatusInt = CDBB(CellDesc,Pointer,&BB.kaLeft,
                &BB.kaBottom,&BB.kaRight,&BB.kaTop);
            ShowBox(Layer,&BB);
            continue;
        }
        if (Type == CDWIRE) {
            CDWire(Pointer,&Layer,&Width,&Path);
            if (Pointer->oInfo == SQ_INCMPLT)
                ShowPath(Layer,Path,False);
            else
                ShowWire(Layer,Width,Path);
            continue;
        }
        if (Type == CDPOLYGON) {
            CDPolygon(Pointer,&Layer,&Path);
            if (Pointer->oInfo == SQ_INCMPLT)
                ShowPath(Layer,Path,False);
            else
                ShowPolygon(Layer,Path);
            continue;
        }
        if (Type == CDLABEL) {
            CDLabel(Pointer,&Layer,&Label,&X,&Y,&Xform);
            ShowLabel(Layer,Label,X,Y,Xform,Parameters.kpDisplayAllLabels);
            continue;
        }
    }
    if (TFull()) {
        --Parameters.kpHierarchyLevel;
        return;
    }

    /*
     * If we aren't expanding instances, we want to show symbolic
     * layers on next lower hierarchy level only.
     */
    if (!Expand And Parameters.kpHierarchyLevel != 1) {
        --Parameters.kpHierarchyLevel;
        return;
    }

    if (Expand || (LayerTable[Layer].klAttributes & SYMBOLIC)) {

        if (Not CDInitGen(CellDesc,0,AOI->kaLeft,AOI->kaBottom,AOI->kaRight,
            AOI->kaTop,&GenDesc)) MallocFailed();
        loop {
            /* Test for user interrupt */
            if (Parameters.kpSIGINTERRUPT) break;

            CDGen(CellDesc,GenDesc,&Pointer);

            if (Pointer == NULL)
                break;
            if (Pointer->oInfo == SQ_GONE)
                continue;
            if (Context && (CellDesc == Parameters.kpCellDesc))
                continue;

            CDStatusInt = CDBB(CellDesc,Pointer,&BB.kaLeft,&BB.kaBottom,
                &BB.kaRight,&BB.kaTop);

            if (BB.kaRight < BB.kaLeft) SwapInts(BB.kaLeft,BB.kaRight);
            if (BB.kaTop < BB.kaBottom) SwapInts(BB.kaBottom,BB.kaTop);
            X = BB.kaRight - BB.kaLeft;
            Y = BB.kaTop - BB.kaBottom;
            if (Y < X) X = Y;
            if (X < MinWidth) {
                /*
                 * Outline BB of Instance.
                 */
                if (ShowBB)
                    ShowEmptyBox(ColorTable[InstanceBBColor].Ent,&BB);
                continue;
            }

            CDCall(Pointer,&MasterName,&NumX,&DX,&NumY,&DY);
            if (OpenCell(MasterName,&MasterDesc)) break;

            if (Not CDBB(MasterDesc,(struct o *)NULL,&MasterBB.kaLeft,
                &MasterBB.kaBottom,&MasterBB.kaRight,&MasterBB.kaTop))
                MallocFailed();

            SetTransform(Pointer);

            for (Int1 = NumY-1; Int1 >= 0; --Int1) {
                for (Int2 = 0; Int2 < NumX; ++Int2 ) {
                    TPush();
                    TIdentity();
                    TTranslate(Int2*DX,Int1*DY);
                    TPremultiply();
                    /*
                     * Box test instance array element with AOI.
                     */
                    ka_copy(BB,(&MasterBB));
                    if (Not (BB.kaLeft > AOI->kaRight Or 
                        BB.kaRight  < AOI->kaLeft Or
                        BB.kaBottom > AOI->kaTop Or
                        BB.kaTop    < AOI->kaBottom))
                        redisplay_layer(MasterDesc,
                            AOI,Layer,Expand,ShowBB,Context); 
                    TPop();
                }
            }
            TPop();
        }
    }
        
    if (!Expand && ShowBB) {

        if (Not CDInitGen(CellDesc,0,AOI->kaLeft,AOI->kaBottom,AOI->kaRight,
            AOI->kaTop,&GenDesc)) MallocFailed();
        loop {
            /* Test for user interrupt */
            if (Parameters.kpSIGINTERRUPT) break;

            CDGen(CellDesc,GenDesc,&Pointer);

            if (Pointer == NULL)
                break;
            if (Pointer->oInfo == SQ_GONE)
                continue;
            if (Context && (CellDesc == Parameters.kpCellDesc))
                continue;

            /*
             * Unexpanded form of an instance array is its BB with the
             * name of its master shown in its center. 
             * If it is not 1 by 1, tell the user its dimensions. 
             * BB dimensions are shown also.
             * CDBB will always return True if Pointer != NULL.
             */
            CDStatusInt = CDBB(CellDesc,Pointer,&BB.kaLeft,&BB.kaBottom,
                &BB.kaRight,&BB.kaTop);

            if (BB.kaRight < BB.kaLeft) SwapInts(BB.kaLeft,BB.kaRight);
            if (BB.kaTop < BB.kaBottom) SwapInts(BB.kaBottom,BB.kaTop);

            /*
             * Outline BB of Instance.
             */
            ShowEmptyBox(ColorTable[InstanceBBColor].Ent,&BB);

            CDCall(Pointer,&MasterName,&NumX,&DX,&NumY,&DY);
            if (NumX != 1 Or NumY != 1)
                sprintf(TypeOut,"%d/%d %s",NumX,NumY,MasterName);
            else sprintf(TypeOut,"%s",MasterName);
            ShowLabel(ColorTable[InstanceNameColor].Ent,TypeOut,BB.kaLeft,
                BB.kaBottom + (BB.kaTop-BB.kaBottom)/2,0,
                Parameters.kpLabelAllInstances);

            sprintf(TypeOut,"%g/%g",(double)(BB.kaRight-BB.kaLeft)/RESOLUTION,
                (double)(BB.kaTop-BB.kaBottom)/RESOLUTION);
            ShowLabel(ColorTable[InstanceSizeColor].Ent,TypeOut,BB.kaLeft,
                BB.kaBottom,0,Parameters.kpLabelAllInstances);
        }
    }

    --Parameters.kpHierarchyLevel;
    return;
}


void
RedisplayAfterInterrupt()

{
    TInit();
    ShowPrompt("Interrupted.  Type Ctrl A to abort.");
    InitSignals();
    ShowCommandMenu();
    ShowLayerTable();
    ShowParameters();
}


int
TCheck()

{
    if (TFull()) {
        ShowPrompt("Cell hierarchy is too deep. MORE");
        (void)FBGetchar(ERASE);
        ShowPrompt("Probably you have a recursive hierarchy.");
        TInit();
        return True;
    }
    return False;
}


void
SetTransform(Pointer)

struct o  *Pointer;
{
    struct t *TGen;
    int X,Y;
    char Type;

    TPush();
    TIdentity();
    CDInitTGen(Pointer,&TGen);
    loop {
        CDTGen(&TGen,&Type,&X,&Y);
        if (TGen == NULL)
            break;
        if (Type == CDROTATE)
            TRotate(X,Y);
        elif (Type == CDTRANSLATE)
            TTranslate(X,Y);
        elif (Type == CDMIRRORX)
            TMX();
        elif (Type == CDMIRRORY)
            TMY();
    }
    TPremultiply();
}

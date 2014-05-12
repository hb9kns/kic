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

/*
 * The KIC selection code.
 * 
 * 
 * The Selection code extensively uses the CD Info field.  The following
 * convention is used:
 * 
 *   Info = SQ_OLD     (0) Object is unselected.
 *  *Info = SQ_OLDSEL  (1) Object is selected and in SelectionQ.
 *   Info = SQ_GONE    (2) Object is conditionally deleted and in SelectionQ.
 *   Info = SQ_NEW     (3) Object is conditionally created and in SelectionQ.
 *  *Info = SQ_NEWSEL  (4) Object is conditionally created and in SelectionQ.
 *   Info = SQ_INCMPLT (5) Object is being created.
 *
 *   Info = 6-11           Reserved.
 *  *Info = 11-255         Object has conditionally new layer and is in
 *                         SelectionQ.  OldLayer = Info - 10.
 *
 *  * means that SQShow will highlight these objects.
 *
 * Giles Billingsley
 */

#include "prefix.h"
#include "kic.h"


#ifdef __STDC__
static void redisplay_edges(struct ka*);
static struct ks *which_cell(struct ks*);
static void sq_set_NEW(struct o*);
static void sq_delete_dups(void);
static void sq_display_selected(struct o*);
static int  is_point_in_poly(int,struct p*,int,int);
static int  overlap_path(struct p*,struct ka*);
static int  overlap_line(struct ka*,struct ka*);
static int  cross_line(struct ka*,struct ka*);
#else
static void redisplay_edges();
static struct ks *which_cell();
static void sq_set_NEW();
static void sq_delete_dups();
static void sq_display_selected();
static int  is_point_in_poly();
static int  overlap_path();
static int  overlap_line();
static int  cross_line();
#endif



/***********************************************************************
 *
 * Current transform code.
 *
 ***********************************************************************/

extern char *MenuMX;
extern char *MenuMY;
extern char *Menu0;
extern char *Menu90;
extern char *Menu180;
extern char *Menu270;


void
MX()

{
    if (Parameters.kpMX) {
        Parameters.kpMX = False;
        MenuDeselect(MenuMX);
    }
    else {
        Parameters.kpMX = True;
        MenuSelect(MenuMX);
    }
}


void
MY()

{
    if (Parameters.kpMY) {
        Parameters.kpMY = False;
        MenuDeselect(MenuMY);
    }
    else {
        Parameters.kpMY = True;
        MenuSelect(MenuMY);
    }
}


void
Rotat0()

{
    Parameters.kpRotationAngle = 90;
    AlterMenuEntries(Menu0,Menu90);
    MenuSelect(Menu90);
}


void
Rotat90()

{
    Parameters.kpRotationAngle = 180;
    AlterMenuEntries(Menu90,Menu180);
    MenuSelect(Menu180);
}


void
Rotat180()

{
    Parameters.kpRotationAngle = 270;
    AlterMenuEntries(Menu180,Menu270);
    MenuSelect(Menu270);
}


void
Rotat270()

{
    Parameters.kpRotationAngle = 0;
    AlterMenuEntries(Menu270,Menu0);
    MenuDeselect(Menu0);
}


/***********************************************************************
 *
 * Selection operator code.
 *
 ***********************************************************************/

extern char *MenuAREA;
extern char *MenuDESEL;
extern char *MenuSELEC;
extern char *MenuLAYER;
extern char *MenuUNDO;

struct ka SelectQBB;
struct ks *SelectQHead;

#define UpdateBB(BB2,BB1) \
    if (BB1.kaLeft < BB2.kaLeft)     BB2.kaLeft = BB1.kaLeft; \
    if (BB1.kaBottom < BB2.kaBottom) BB2.kaBottom = BB1.kaBottom; \
    if (BB1.kaRight > BB2.kaRight)   BB2.kaRight = BB1.kaRight; \
    if (BB1.kaTop > BB2.kaTop)       BB2.kaTop = BB1.kaTop;


void
Sel(LookedAhead)

int *LookedAhead;
{
    struct ka AOI;
    int FirstTime = True;

    MenuSelect(MenuSELEC);
    ShowPrompt("Point to select.");
    loop {
        switch (PointLoop(LookedAhead)) {
        case PL_CMD:
        case PL_ESC:
            goto quit;
        case PL_UND:
            if (FirstTime == True) goto quit;
            MenuSelect(MenuUNDO);
            Selection(&AOI);
            MenuDeselect(MenuUNDO);
            continue;
        case PL_PCW:
            AOI.kaLeft   = AOI.kaRight = KicCursor.kcRawX;
            AOI.kaBottom = AOI.kaTop   = KicCursor.kcRawY;
            ErasePrompt();
            Selection(&AOI);
            FirstTime = False;
        }
    }
quit:
    MenuDeselect(MenuSELEC);
    ErasePrompt();
}


void
Area(LookedAhead)

int *LookedAhead;
{
    struct ka AOI;
    int OldRawX = 0,OldRawY = 0;
    int FirstTime = True;

    MenuSelect(MenuAREA);
top:
    loop {
        ShowPrompt("Point to endpoints of diagonal.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            if (FirstTime == True) goto quit;
            MenuSelect(MenuUNDO);
            Selection(&AOI);
            MenuDeselect(MenuUNDO);
            goto top;
        case PL_PCW:
            FBSetRubberBanding('R');
            OldRawX = KicCursor.kcRawX;
            OldRawY = KicCursor.kcRawY;
        }
        ShowPrompt("Point to second endpoint.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            FBSetRubberBanding(0);
            goto top;
        case PL_PCW:
            FBSetRubberBanding(0);
            break;
        }
        AOI.kaLeft   = min(OldRawX,KicCursor.kcRawX);
        AOI.kaBottom = min(OldRawY,KicCursor.kcRawY);
        AOI.kaRight  = max(OldRawX,KicCursor.kcRawX);
        AOI.kaTop    = max(OldRawY,KicCursor.kcRawY);
        Selection(&AOI);
        FirstTime = False;
    }

quit:
    FBSetRubberBanding(0);
    MenuDeselect(MenuAREA);
    ErasePrompt();
}


void
Desel()

{
    MenuSelect(MenuDESEL);
    SQComputeBB();
    if (SelectQHead != NULL) {
        SQClear();
        /* Take care of Instance markers */
        OversizeBox(&SelectQBB,200);
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
    }
    MenuDeselect(MenuDESEL);
}


int
Layer()

{
    if (strcmp(Parameters.kpCommand,MenuLAYER) == 0) {
        if (Parameters.kpLayerSpecificSelection) {
            MenuDeselect(MenuLAYER);
            Parameters.kpLayerSpecificSelection = False;
        }
        else{
            MenuSelect(MenuLAYER);
            Parameters.kpLayerSpecificSelection = True;
        }
        return (True);
    }
    return (False);
}


void
Selection(AOI)

/* Select items and link into SelectionQ, compute BB. */
struct ka *AOI;
{
    struct ks *SList, *S;
    struct ka BB,OldSelectQBB;

    if (AOI->kaLeft == AOI->kaRight) {
        /* Point selection. */

        SList = SelectItems(AOI,True);
        if (SList == NULL) return;
        if (SList->ksPointer->oType == CDSYMBOLCALL) {
            /* no geometry found, get one symbol */
            S = which_cell(SList);
            if (S) {
                if (S->ksPointer->oInfo == SQ_OLDSEL) {
                    /* already selected, deselect and break */
                    S->ksPointer->oInfo = SQ_OLD;
                    ShowInstanceMarker(ERASE,0,S->ksPointer);
                    SQDelete(S->ksPointer);
                    GetBB(S->ksPointer,&BB);
                    redisplay_edges(&BB);
                }
                else {
                    /* add to select Q */
                    S->ksPointer->oInfo = SQ_OLDSEL;
                    SQInsert(S->ksPointer);
                    sq_display_selected(S->ksPointer);
                }
            }
            SQComputeBB();
            SLFree(SList);
            return;
        }
        for (S = SList; S != NULL; S = S->ksSucc) {
            /* keep only geometry */
            if (S->ksPointer->oType == CDSYMBOLCALL) break;

            if (S->ksPointer->oInfo == SQ_OLDSEL) {
                /* already selected, deselect and break */
                S->ksPointer->oInfo = SQ_OLD;
                SQDelete(S->ksPointer);
                GetBB(S->ksPointer,&BB);
                EraseBox(&BB);
                Redisplay(&BB);
                FBTransfer();
                break;

            }
            else {
                /* add to select Q */
                S->ksPointer->oInfo = SQ_OLDSEL;
                SQInsert(S->ksPointer);
                sq_display_selected(S->ksPointer);
            }
        }
        SQComputeBB();
    }
    else {
        /* Area select. */
        SList = SelectItems(AOI,False);
        if (SList == NULL) return;
        SQComputeBB();
        OldSelectQBB = SelectQBB;

        for (S = SList; S != NULL; S = S->ksSucc) {
            if (S->ksPointer->oType == CDSYMBOLCALL &&
                !BBVisible(S->ksPointer)) continue;

            if (S->ksPointer->oInfo == SQ_OLDSEL) {
                /* already selected, deselect */
                S->ksPointer->oInfo = SQ_OLD;
                SQDelete(S->ksPointer);
            }
            else {
                /* add to select Q */
                S->ksPointer->oInfo = SQ_OLDSEL;
                SQInsert(S->ksPointer);
            }
        }
        SQComputeBB();
        UpdateBB(SelectQBB,OldSelectQBB);
        if (SelectQBB.kaLeft == CDINFINITY) return;
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
        FBTransfer();
    }
    SLFree(SList);
}

struct ks *
SelectItems(AOI,PointSelect)

/* Return a list of visible objects in the neighborhood of AOI as returned
 * from the generator.  Only types in Parameters.kpSelectTypes are listed,
 * all are listed if this is NULL.  The flag PointSelect is set for
 * a point selection.
 */
struct ka *AOI;
int PointSelect;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct ka BB;
    struct ks *SPointer = NULL, *S = NULL;
    struct p *Path;
    int Width;
    int Delta = 0,Layer,Layer1;

    if (PointSelect) {
        /* expand the point to finite size */
        if (KicCursor.kcInFine == True)
            Delta = 3.0/View->kvFineRatio;
        else
            Delta = 3.0/View->kvCoarseRatio;
        OversizeBox(AOI,Delta);
    }

    if (Parameters.kpLayerSpecificSelection) {
        Layer = Parameters.kpLayer;
        Layer1 = Layer;
    }
    else {
        Layer = 1;
        Layer1 = NumLayerTable;
    }
    for ( ; Layer <= Layer1; Layer++) {
        if (!(LayerTable[Layer].klAttributes & VISIBLE)) continue;

        if (Not CDInitGen(Parameters.kpCellDesc,Layer,AOI->kaLeft,AOI->kaBottom,
            AOI->kaRight,AOI->kaTop,&GenDesc)) MallocFailed();
        loop {
            CDGen(Parameters.kpCellDesc,GenDesc,&Pointer);
            if (Pointer == NULL) break;
            if (Pointer->oInfo == SQ_GONE) continue;
            if (Pointer->oInfo == SQ_INCMPLT) continue;
            if (Parameters.kpSelectTypes &&
                !strchr(Parameters.kpSelectTypes,Pointer->oType)) continue;

            switch (Pointer->oType) {
                case CDWIRE:
                    CDWire(Pointer,&Layer,&Width,&Path);
                    if (PointSelect) {
                        if (InPath(Delta+Width/2,Path,
                            AOI->kaLeft+Delta,AOI->kaBottom+Delta) == NULL)
                            continue;
                    }
                    else {
                        CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
                            &BB.kaLeft,&BB.kaBottom,
                            &BB.kaRight,&BB.kaTop);
                        if (InBox(BB.kaLeft,BB.kaBottom,AOI) &&
                            InBox(BB.kaRight,BB.kaTop,AOI))
                            /* wire is entirely inside AOI, select it */
                            break;
                        if (!overlap_path(Path,AOI)) continue;
                            /* wire overlaps AOI, select it */
                    }
                    break;

                case CDPOLYGON:
                    CDPolygon(Pointer,&Layer,&Path);
                    if (PointSelect) {
                        if (!is_point_in_poly(Delta,Path,
                            AOI->kaLeft+Delta,AOI->kaBottom+Delta))
                            continue;
                    }
                    else {
                        CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
                            &BB.kaLeft,&BB.kaBottom,
                            &BB.kaRight,&BB.kaTop);
                        if (InBox(BB.kaLeft,BB.kaBottom,AOI) &&
                            InBox(BB.kaRight,BB.kaTop,AOI))
                            /* poly is entirely inside AOI, select it */
                            break;
                        if (!is_point_in_poly(0,Path,
                            AOI->kaLeft,AOI->kaBottom))
                            if (!overlap_path(Path,AOI)) continue;
                            /* If an AOI corner is in the poly, select it.
                             * This catches the case where the AOI is
                             * entirely in the poly.
                             * Otherwise select poly only if the poly
                             * boundary intersects the AOI.
                             */
                    }
                    break;

                case CDLABEL:
                case CDBOX:
                    break;

                default:
                    continue;
            }
            if (SPointer == NULL)
                S = SPointer = alloc(ks);
            else {
                S->ksSucc = alloc(ks);
                S = S->ksSucc;
            }
            if (S == NULL) {
                CDStatusInt = CDMALLOCFAILED;
                MallocFailed();
            }
            S->ksPointer = Pointer;
            S->ksSucc = NULL;
        }
    }

    /* Now for the instances... */

    if (Parameters.kpSelectTypes &&
        !strchr(Parameters.kpSelectTypes,CDSYMBOLCALL)) {
        if (PointSelect) OversizeBox(AOI,-Delta);
        return SPointer;
    }

    if (Not CDInitGen(Parameters.kpCellDesc,0,AOI->kaLeft,AOI->kaBottom,
        AOI->kaRight,AOI->kaTop,&GenDesc)) MallocFailed();
    loop {
        CDGen(Parameters.kpCellDesc,GenDesc,&Pointer);
        if (Pointer == NULL) break;

        if (Pointer->oInfo == SQ_GONE) continue;
        if (SPointer == NULL)
            S = SPointer = alloc(ks);
        else {
            S->ksSucc = alloc(ks);
            S = S->ksSucc;
        }
        if (S == NULL) {
            MallocFailed();
            CDStatusInt = CDMALLOCFAILED;
        }
        S->ksPointer = Pointer;
        S->ksSucc = NULL;
    }
    if (PointSelect) OversizeBox(AOI,-Delta);
    return (SPointer);
}


void
SLFree(SList)

/* Free a list as returned from SelectItems(). */
struct ks *SList;
{
    struct ks *SQDesc,*SQNext;

    for (SQDesc = SList; SQDesc; SQDesc = SQNext) {
        SQNext = SQDesc->ksSucc;
        afree(SQDesc,ks);
    }
}


void
SLBB(SList,BB)

/* Compute the BB of the objects in SList. */
struct ks *SList;
struct ka *BB;
{
    struct ks *S;
    struct ka NBB,OBB;

    NBB.kaLeft   = CDINFINITY;
    NBB.kaBottom = CDINFINITY;
    NBB.kaRight  = -CDINFINITY;
    NBB.kaTop    = -CDINFINITY;

    NBB.kaHeight = 0;
    NBB.kaWidth = 0;
    NBB.kaX = 0;
    NBB.kaY = 0;

    for (S = SList; S != NULL; S = S->ksSucc) {
        GetBB(S->ksPointer,&OBB);
        UpdateBB(NBB,OBB);
    }
    *BB = NBB;
}


void
GetBB(Pointer,BB)

struct o *Pointer;
struct ka *BB;
{
    if (Pointer->oType == CDLABEL)
        BBLabel(View->kvCoarseWindow,Pointer,BB);
    else
        CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,&BB->kaLeft,
            &BB->kaBottom,&BB->kaRight,&BB->kaTop);
}


int
BBVisible(Pointer)

struct o *Pointer;
{
    struct ka BB;

    /* will edges show in coarse window? */
    CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
        &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);
    if (BB.kaLeft   <= View->kvCoarseWindow->kaLeft   &&
        BB.kaRight  >= View->kvCoarseWindow->kaRight  &&
        BB.kaBottom <= View->kvCoarseWindow->kaBottom &&
        BB.kaTop    >= View->kvCoarseWindow->kaTop)
        return (False);
    return (True);
}


static void
redisplay_edges(BB)

/* redisplay the edges of BB */ 
struct ka *BB;
{
    struct ka EdgeOfBB;

    /* Left edge. */
    EdgeOfBB.kaLeft   = BB->kaLeft-300;
    EdgeOfBB.kaRight  = BB->kaLeft+300;
    EdgeOfBB.kaTop    = BB->kaTop+300;
    EdgeOfBB.kaBottom = BB->kaBottom-300;
    EraseBox(&EdgeOfBB);
    Redisplay(&EdgeOfBB);

    /* Right edge. */
    EdgeOfBB.kaRight  = BB->kaRight+300;
    EdgeOfBB.kaLeft   = BB->kaRight-300;
    EdgeOfBB.kaTop    = BB->kaTop+300;
    EdgeOfBB.kaBottom = BB->kaBottom-300;
    EraseBox(&EdgeOfBB);
    Redisplay(&EdgeOfBB);

    /* Bottom edge. */
    EdgeOfBB.kaBottom = BB->kaBottom-300; 
    EdgeOfBB.kaTop    = BB->kaBottom+300;
    EdgeOfBB.kaRight  = BB->kaRight;
    EdgeOfBB.kaLeft   = BB->kaLeft;
    EraseBox(&EdgeOfBB);
    Redisplay(&EdgeOfBB);

    /* Top edge. */
    EdgeOfBB.kaBottom = BB->kaTop-300; 
    EdgeOfBB.kaTop    = BB->kaTop+300;
    EdgeOfBB.kaRight  = BB->kaRight;
    EdgeOfBB.kaLeft   = BB->kaLeft;
    EraseBox(&EdgeOfBB);
    Redisplay(&EdgeOfBB);

    FBTransfer();
}


static struct ks *
which_cell(SList)

/* Resolve ambiguity (multiple instances selected) */
struct ks *SList;
{
    struct ks *S,*Sret;
    char *SymbolName;
    struct ka BB;
    double A,Area;

    /* find the smallest cell */
    SLBB(SList,&BB);
    Area = BB.kaRight - BB.kaLeft;
    Area *= BB.kaTop - BB.kaBottom;
    Sret = SList;
    for (S = SList; S != NULL; S = S->ksSucc) {
        CDStatusInt = CDBB(Parameters.kpCellDesc,S->ksPointer,
            &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);
        A = BB.kaRight - BB.kaLeft;
        A *= BB.kaTop - BB.kaBottom;
        if (A < Area) {
            Sret = S;
            Area = A;
        }
    }
    if (!BBVisible(Sret->ksPointer)) return (NULL);

    SymbolName = ((struct c *)Sret->ksPointer->oRep)->cMaster->mName;
    sprintf(TypeOut,"You have selected an instance of %s.",SymbolName);
    ShowPrompt(TypeOut);
    return (Sret);
}


int
AreTypesInQ(Types)

/* Returns True if one of Types is in SelectionQ and is selected,
 * or of anything selected is in the Q if Types is NULL.
 */
char *Types;
{
    struct ks *SQDesc;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (Types == NULL || strchr(Types,SQDesc->ksPointer->oType))
            if (SQDesc->ksPointer->oInfo == SQ_OLDSEL) return (True);
    }
    return (False);
}


void
SelectTypes(Types)

/* Perform a point select, selecting only Types, or anything if
 * Types is NULL.
 */
char *Types;
{
    struct ka BB;
    char TTmp[8];

    BB.kaLeft   = BB.kaRight = KicCursor.kcRawX;
    BB.kaBottom = BB.kaTop   = KicCursor.kcRawY;
    if (Types == NULL) {
        Selection(&BB);
        return;
    }
    strcpy(TTmp,Parameters.kpSelectTypes);
    strncpy(Parameters.kpSelectTypes,Types,8);
    Parameters.kpSelectTypes[7] = '\0';
    Selection(&BB);
    strcpy(Parameters.kpSelectTypes,TTmp);
}


void
SQInit()

{
    SelectQHead = NULL;
}


void
SQClear()

/* Clear the SelectionQ. */
{
    struct ks *SQDesc,*SQNext;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQNext) {
        SQNext = SQDesc->ksSucc;
        SQDesc->ksPointer->oInfo = SQ_OLD;
        afree(SQDesc,ks);
    }
    SelectQHead = NULL;
}


void
SQInsert(Pointer)

/* Insert Pointer into the SelectionQ (no checking for duplication). */
struct o *Pointer;
{
    struct ks *SQDesc;

    if ((SQDesc = alloc(ks)) == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }

    /* SelectQHead is most recent addition */
    SQDesc->ksPointer = Pointer; 
    SQDesc->ksSucc = SelectQHead;
    SelectQHead = SQDesc;
}


void
SQDelete(Pointer)

/* Delete Pointer from SelectionQ if it is there. */
struct o *Pointer;
{
    struct ks *SQDesc,*SQPrev,*SQNext;

    SQPrev = NULL;

    for (SQDesc = SelectQHead; SQDesc; SQPrev = SQDesc,SQDesc = SQNext) {
        SQNext = SQDesc->ksSucc;

        if (SQDesc->ksPointer != Pointer) continue;
        Pointer->oInfo = SQ_OLD;

        if (SQPrev == NULL)
            SelectQHead = SQNext;
        else
            SQPrev->ksSucc = SQNext;
        afree(SQDesc,ks);
        return;
    }
}


void
SQComputeBB()

/* Compute the BB of the queued and selected objects. */
{
    struct ks *SQDesc;
    struct ka BB;
    int Info;

    SelectQBB.kaLeft   = CDINFINITY;
    SelectQBB.kaBottom = CDINFINITY;
    SelectQBB.kaRight  = -CDINFINITY;
    SelectQBB.kaTop    = -CDINFINITY;

    for (SQDesc = SelectQHead; SQDesc != NULL; SQDesc = SQDesc->ksSucc) {
        Info = SQDesc->ksPointer->oInfo;
        if (Info == SQ_OLDSEL || Info == SQ_NEWSEL ||
            (Info > 10 && Info <= 255)) {
            GetBB(SQDesc->ksPointer,&BB);
            UpdateBB(SelectQBB,BB);
        }
    }
}

/* Theory behind SQRestore() and SQDesel();
 *
 * In commands which modify objects such as Move, the original object(s)
 * are conditionally deleted, and new objects are conditionally created.
 * All objects are left in the SelectionQ for a time to allow Undo.
 * For example, suppose the SelectionQ is empty, and the user points at
 * an object.
 *
 *  operation:         SelectionQ:
 *  select             OLD (Info = SQ_OLDSEL)
 *  move               NEW (Info = SQ_NEW), OLD (Info = SQ_GONE)
 * User now selects newly moved object (the complicated case):
 *  select NEW         NEW (Info = SQ_OLDSEL), NEW (Info = SQ_OLDSEL),
 *                     OLD (Info = SQ_GONE)
 * Now undo undoes the selection
 *  Undo (SQDesel())   NEW (Info = SQ_NEW), OLD (Info = SQ_GONE)
 * Next undo undoes the move, leaving original item selected (conditionally).
 *  Undo (SQRestore(1)) OLD (Info = SQ_NEWSEL)
 * Next undo undoes the selection
 *  Undo (SQDesel())   OLD (Info = SQ_NEW)
 * The next undo repeats the move, etc.
 *
 * SQRestore(0) unsets the conditionality of objects in the SelectionQ
 * and should be called when things are "final," i.e., before function
 * exit or next operation.
 */


void
SQRestore(Undo)

/* Restore the conditionally created objects in the SelectionQ
 * and delete duplicates if Undo is False.  Otherwise undo the
 * last operation.
 */

/* If Undo is True:
 *  Previously deleted objects become conditionally selected:
 *   Info = SQ_GONE   -> Info = SQ_NEWSEL.
 *  New objects are deleted from SelectionQ and database:
 *   Info = SQ_NEW    deleted from queue and database.
 *   Info = SQ_NEWSEL deleted from queue and database.
 * Else
 *  Conditionally deleted objects are really deleted:
 *   Info = SQ_GONE   deleted from queue and database.
 *  New objects are now Old objects, selected or otherwise:
 *   Info = SQ_NEW    -> Info = SQ_OLD, deleted from queue.
 *   Info = SQ_NEWSEL -> Info = SQ_OLDSEL.
 */

int Undo;
{
    struct ks *SQDesc,*SQNext;
    int Info;
    struct o *Pointer;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQNext) {
        SQNext = SQDesc->ksSucc;
        Pointer = SQDesc->ksPointer;
        Info = Pointer->oInfo;

        if (Info == SQ_GONE) {
            if (Undo == True)
                Pointer->oInfo = SQ_NEWSEL;
            else {
                SQDelete(Pointer);
                CDDelete(Parameters.kpCellDesc,Pointer);
            }
            continue;
        }
        if (Info == SQ_NEW) {
            SQDelete(Pointer);
            if (Undo == True) {  
                Pointer->oInfo = SQ_GONE;
                CDDelete(Parameters.kpCellDesc,Pointer);
            }
            else
                Pointer->oInfo = SQ_OLD;
            continue;
        }
        if (Info == SQ_NEWSEL) {
            if (Undo == True) {  
                SQDelete(Pointer);
                Pointer->oInfo = SQ_GONE;
                CDDelete(Parameters.kpCellDesc,Pointer);
            }
            else
                Pointer->oInfo = SQ_OLDSEL;
        }
    }
    if (Undo == False)
        sq_delete_dups();
}


void
SQDesel(Types)

/* Undo a selection operation. */

/* Info = SQ_OLDSEL deleted from queue, and duplicates set to Info = SQ_NEW.
 * Info = SQ_NEWSEL -> Info = SQ_NEW.
 * Ignores objects with type not listed in Types.
 */

char *Types;
{
    struct ks *SQDesc, *SQNext;
    struct o *Pointer;
    int Info;
    char Type;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQNext) {
        SQNext = SQDesc->ksSucc;
        Type = SQDesc->ksPointer->oType;
        if (Types And !strchr(Types,Type)) continue;
        Info = SQDesc->ksPointer->oInfo;

        /* Have to be careful here.  If selected object is already
         * in queue have to reset Info of second entry to SQ_NEW.
         */

        if (Info == SQ_OLDSEL) {
            Pointer = SQDesc->ksPointer;
            Pointer->oInfo = SQ_OLD;
            SQDelete(Pointer);
            sq_set_NEW(Pointer);
            continue;
        }
        if (Info == SQ_NEWSEL)
            SQDesc->ksPointer->oInfo = SQ_NEW;
    }
}


void
SQShow()

/* Show selected objects by highlighting their BBs. */
{
    struct ks *SQDesc;
    int Info;

    if (Not Parameters.kpEnableSelectQRedisplay) return;
    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        Info = SQDesc->ksPointer->oInfo;
        /* Test for user interrupt */
        if (Parameters.kpSIGINTERRUPT) {
            RedisplayAfterInterrupt();
            return;
        }
        if (Info == SQ_OLDSEL || Info == SQ_NEWSEL ||
            (Info > 10 && Info <= 255)) {
            /* Show Selected Objects */
            sq_display_selected(SQDesc->ksPointer);
        }
    }
}


static void
sq_set_NEW(Pointer)

/* If object is in queue, set Info to SQ_NEW. */
struct o *Pointer;
{
    struct ks *SQDesc;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer == Pointer) {
            SQDesc->ksPointer->oInfo = SQ_NEW;
            return;
        }
    }
}


static void
sq_delete_dups()

/* Delete duplicate entries in selection queue. */
{
    struct ks *SQDesc,*SQDesc1,*SQNext;
    int Info;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQNext) {
        SQNext = SQDesc->ksSucc;
        Info = SQDesc->ksPointer->oInfo;
        for (SQDesc1 = SQDesc->ksSucc; SQDesc1; SQDesc1 = SQDesc1->ksSucc)
            if (SQDesc->ksPointer == SQDesc1->ksPointer) {
                SQDelete(SQDesc->ksPointer);
                /* SQDelete sets Info to SQ_OLD, have to undo this */
                SQDesc1->ksPointer->oInfo = Info;
                break;
            }
    }
}


static void
sq_display_selected(Pointer)

struct o *Pointer;
{
    int Layer;
    struct p *Path;
    int Width;
    char OldRD;
    struct ka BB;

    if (Pointer->oType == CDWIRE) {
        CDWire(Pointer,&Layer,&Width,&Path);
        ShowWire(ColorTable[HighlightingColor].Ent,Width,Path);
        return;
    }
    if (Pointer->oType == CDSYMBOLCALL) {
        CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
            &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);
        ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
        ShowInstanceMarker(DISPLAY,ColorTable[HighlightingColor].Ent,
            Pointer);
        return;
    }
    if (Pointer->oType == CDLABEL) {

        /* BB of labels must be special-cased.  Can't use CDBB. */

        OldRD = Parameters.kpRedisplayControl;
        if (OldRD != FINEVIEWPORTONLY) {
            Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
            BBLabel(View->kvCoarseWindow,Pointer,&BB);
            ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
        }
        if (OldRD != COARSEVIEWPORTONLY) {
            Parameters.kpRedisplayControl = FINEVIEWPORTONLY;
            BBLabel(View->kvFineWindow,Pointer,&BB);
            ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
        }
        Parameters.kpRedisplayControl = OldRD;
        return;
    }
    if (Pointer->oType == CDPOLYGON) {
        CDPolygon(Pointer,&Layer,&Path);
        ShowPath(ColorTable[HighlightingColor].Ent,Path,True);
        return;
    }
    if (Pointer->oType == CDBOX) {
        CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
            &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);
        ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
        return;
    }
}


int *
InPath(Delta,Path,X,Y)

/* Is (X,Y) on the path described by <Path>?
 * If yes, return a pointer to x,y that are exactly in path.
 */
int Delta;
int X,Y;
struct p *Path;
{
    struct p *Pair;
    struct ka BB;
    double x1,x2,y1,y2,d0,d1,d2,d3,w;
    static int xy[2];

    if (Delta < 10) Delta = 10;

    if (Path == NULL) return (NULL);
    for (Pair = Path; Pair->pSucc != NULL; Pair = Pair->pSucc) {

        if (Pair->pX < Pair->pSucc->pX) {
            BB.kaLeft = Pair->pX;
            BB.kaRight = Pair->pSucc->pX;
        }
        else {
            BB.kaRight = Pair->pX;
            BB.kaLeft = Pair->pSucc->pX;
        }
        if (Pair->pY < Pair->pSucc->pY) {
            BB.kaBottom = Pair->pY;
            BB.kaTop = Pair->pSucc->pY;
        }
        else {
            BB.kaBottom = Pair->pSucc->pY;
            BB.kaTop = Pair->pY;
        }
        OversizeBox(&BB,Delta);
        if (!InBox(X,Y,&BB)) continue;

        x1 = Pair->pX - Pair->pSucc->pX;
        y1 = Pair->pY - Pair->pSucc->pY;
        d0 = x1*x1 + y1*y1;

        x1 = Pair->pX - X;
        y1 = Pair->pY - Y;
        d1 = x1*x1 + y1*y1;
        x2 = Pair->pSucc->pX - X;
        y2 = Pair->pSucc->pY - Y;
        d2 = x2*x2 + y2*y2;

        d3 = (d2 - d1)/(2*sqrt(d0));
        w = (d1+d2)/2 - d0/4 - d3*d3;

        if (w <= Delta*Delta) {
            /* should be positive, fabs() just in case */
            d1 = sqrt(fabs(d1-w)/d0);
            xy[0] = Pair->pX + (Pair->pSucc->pX - Pair->pX)*d1;
            xy[1] = Pair->pY + (Pair->pSucc->pY - Pair->pY)*d1;
            return (xy);
        }
    }
    return (NULL);
}

#ifndef PI
#define PI        3.14159265358979323846
#endif


static int
is_point_in_poly(Delta,Path,X,Y)

/* Return True if point is enclosed in polygon, or near a vertex.
 * Algorithm is to sum angle differences to reference point around
 * path.  If the reference point is inside, the sum is 2*PI, otherwise
 * the sum is zero.
 */
int Delta;
struct p *Path;
int X,Y;
{
    struct p *p;
    double Xp,Yp,R,Theta,ThetaLast = 0.0,Sum,zz;

    Sum =  0;

    for (p = Path; p != NULL; p = p->pSucc) {

        Xp = p->pX - X;
        Yp = p->pY - Y;
        R = sqrt(Xp*Xp + Yp*Yp);
        if (R <= Delta) return (True);
        Theta = asin(Yp/R);
        if (Xp >= 0) {
            if (Yp < 0)
                Theta = 2*PI + Theta;
        }
        else
            Theta = PI - Theta;
        if (p != Path) {
            zz = (Theta - ThetaLast);
            if (zz > PI) zz -= 2*PI;
            if (zz < -PI) zz += 2*PI;
            Sum += zz;
        }
        ThetaLast = Theta;
    }
    if (fabs(Sum) >= 1.99*PI) return (True);
    return (False);
}


static int
overlap_path(Path,BB)

/* return True if the path intersects the BB */ 
struct p *Path;
struct ka *BB;
{
    struct ka Line;

    Line.kaLeft = Path->pX;
    Line.kaBottom = Path->pY;
    Path = Path->pSucc;
    for (; Path != NULL; Path = Path->pSucc) {
        Line.kaRight = Path->pX;
        Line.kaTop = Path->pY;
        if (overlap_line(&Line,BB)) return (True);
        Line.kaLeft = Line.kaRight;
        Line.kaBottom = Line.kaTop;
    }
    return (False);
}


static int
overlap_line(Line,BB)

/* return True if Line intersects BB */
struct ka *Line,*BB;
{
    struct ka LBB;

    LBB.kaLeft = BB->kaLeft;
    LBB.kaRight = BB->kaLeft;
    LBB.kaBottom = BB->kaBottom;
    LBB.kaTop = BB->kaTop;
    if (cross_line(Line,&LBB)) return (True);

    LBB.kaRight = BB->kaRight;
    LBB.kaBottom = BB->kaTop;
    if (cross_line(Line,&LBB)) return (True);

    LBB.kaLeft = BB->kaRight;
    LBB.kaBottom = BB->kaBottom;
    if (cross_line(Line,&LBB)) return (True);
        
    LBB.kaLeft = BB->kaLeft;
    LBB.kaTop = BB->kaBottom;
    if (cross_line(Line,&LBB)) return (True);

    return (False);
}


static int
cross_line(Line,BB)

/* return True if line segments stored as diagonal of BB, Line
 * intersect.  The line in BB is Manhattan.
 */
struct ka *Line,*BB;
{
    struct ka LineBB,MBB;
    int X,Y;

    MBB = *BB;
    if (MBB.kaTop < MBB.kaBottom)
        SwapInts(MBB.kaTop,MBB.kaBottom);
    if (MBB.kaRight < MBB.kaLeft)
        SwapInts(MBB.kaRight,MBB.kaLeft);
    LineBB = *Line;
    if (LineBB.kaTop < LineBB.kaBottom)
        SwapInts(LineBB.kaTop,LineBB.kaBottom);
    if (LineBB.kaRight < LineBB.kaLeft)
        SwapInts(LineBB.kaRight,LineBB.kaLeft);

    /* return False if BB's don't overlap */
    if (LineBB.kaLeft > MBB.kaRight ||
        LineBB.kaRight < MBB.kaLeft ||
        LineBB.kaBottom > MBB.kaTop ||
        LineBB.kaTop < MBB.kaBottom)
        return (False);

    /* if Line is Manhattan, return True */
    if (Line->kaLeft == Line->kaRight || Line->kaBottom == Line->kaTop)
        return (True);

    if (BB->kaBottom == BB->kaTop) {

        X = (BB->kaBottom - Line->kaBottom)*
            ((double)(Line->kaRight - Line->kaLeft)/
            (Line->kaTop - Line->kaBottom)) +
            Line->kaLeft;
        if (X < BB->kaLeft || X > BB->kaRight) return (False);
    }
    else {

        Y = (BB->kaLeft - Line->kaLeft)*
            ((double)(Line->kaTop - Line->kaBottom)/
            (Line->kaRight - Line->kaLeft))
            + Line->kaBottom;
        if (Y < BB->kaBottom || Y > BB->kaTop) return (False);
    }
    return (True);
}

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
 * Box management.
 *
 * CIFToKIC--the CDTo routine in the CD package--transforms non-Manhattan
 * boxes to polygons, so the direction vector parameter in the routines
 * here are superfluous.
 *
 * KIC only knows about Manhattan boxes.
 * To make a 45, the user must use a wire or polygon.
 */

#include "prefix.h"
#include "kic.h"


extern char *MenuBOXES;
extern char *MenuUNDO;

int MakingBoxes;

#define ka_copy(BB1,BB2) BB1.kaLeft = BB2->kaLeft; \
    BB1.kaRight = BB2->kaRight; \
    BB1.kaBottom = BB2->kaBottom; \
    BB1.kaTop = BB2->kaTop; \
    TPoint(&BB1.kaLeft,&BB1.kaBottom); \
    TPoint(&BB1.kaRight,&BB1.kaTop); \
    if (BB1.kaLeft > BB1.kaRight) \
        SwapInts(BB1.kaLeft,BB1.kaRight); \
    if (BB1.kaBottom > BB1.kaTop) \
        SwapInts(BB1.kaBottom,BB1.kaTop);


int
InBox(X,Y,AOI)

int X,Y;
struct ka *AOI;
{
    if (((AOI->kaLeft <= X And X <= AOI->kaRight) Or
        (AOI->kaRight <= X And X <= AOI->kaLeft)) And
        ((AOI->kaBottom <= Y And Y <= AOI->kaTop) Or
        (AOI->kaTop <= Y And Y <= AOI->kaBottom)))
        return (True);
    return (False);
}


void
ShowBox(Layer,boxBB)

int Layer;
struct ka *boxBB;
{
    struct ka BB, BB1, BB2;
    int ShowLeft;
    int ShowBottom;
    int ShowRight;
    int ShowTop;
    int PixWidth = 2;

    if (Layer == ColorTable[HighlightingColor].Ent) {
        ShowEmptyBox(Layer,boxBB);
        return;
    }
    ka_copy(BB,boxBB);

    if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY And
        CurrentAOI.aInCoarse) {

        CoarseLToP(BB.kaLeft,BB.kaBottom,BB1.kaLeft,BB1.kaBottom);
        CoarseLToP(BB.kaRight,BB.kaTop,BB1.kaRight,BB1.kaTop);

        if (BB1.kaLeft <= CurrentAOI.aRC And BB1.kaRight >= CurrentAOI.aLC And
            BB1.kaBottom <= CurrentAOI.aTC And BB1.kaTop >= CurrentAOI.aBC) {

            ShowLeft   = True;
            ShowBottom = True;
            ShowRight  = True;
            ShowTop    = True;

            if (LayerTable[Layer].klAttributes & FILLED) {
                if (BB1.kaLeft < CurrentAOI.aLC) {
                    BB1.kaLeft = CurrentAOI.aLC;
                    ShowLeft = False;
                }
                if (BB1.kaBottom < CurrentAOI.aBC) {
                    BB1.kaBottom = CurrentAOI.aBC;
                    ShowBottom = False;
                }
                if (BB1.kaRight > CurrentAOI.aRC) {
                    BB1.kaRight = CurrentAOI.aRC;
                    ShowRight = False;
                }
                if (BB1.kaTop > CurrentAOI.aTC) {
                    BB1.kaTop = CurrentAOI.aTC;
                    ShowTop = False;
                }
                if (LayerTable[Layer].klAttributes & COARSE_FILL) {
                    FBFilledBox(Layer,DISPLAY,
                        LayerTable[Layer].klStyleID,
                        BB1.kaLeft,BB1.kaBottom,BB1.kaRight,BB1.kaTop);

                    if (LayerTable[Layer].klAttributes & OUTLINED) { 

                        if (ShowBottom)
                            FBLine(BB1.kaRight,BB1.kaBottom,
                                BB1.kaLeft,BB1.kaBottom);
                        if (ShowLeft)
                            FBLine(BB1.kaLeft,BB1.kaBottom,
                                BB1.kaLeft,BB1.kaTop);
                        if (ShowRight)
                            FBLine(BB1.kaRight,BB1.kaTop,
                                BB1.kaRight,BB1.kaBottom);
                        if (ShowTop)
                            FBLine(BB1.kaLeft,BB1.kaTop,
                                BB1.kaRight,BB1.kaTop);
                    }
                }
                else {
                    FBFilledBox(Layer,DISPLAY,0,
                        BB1.kaLeft,BB1.kaBottom,BB1.kaRight,BB1.kaTop);
                }
            }
            else {

                BB2.kaLeft = BB1.kaLeft + PixWidth;
                BB2.kaBottom = BB1.kaBottom + PixWidth;
                BB2.kaRight = BB1.kaRight - PixWidth;
                BB2.kaTop = BB1.kaTop - PixWidth;

                if (BB1.kaLeft < CurrentAOI.aLC) {
                    BB1.kaLeft = CurrentAOI.aLC;
                    if (BB2.kaLeft < CurrentAOI.aLC)
                        ShowLeft = False;
                }
                if (BB1.kaBottom < CurrentAOI.aBC) {
                    BB1.kaBottom = CurrentAOI.aBC;
                    if (BB2.kaBottom < CurrentAOI.aBC)
                        ShowBottom = False;
                }
                if (BB1.kaRight > CurrentAOI.aRC) {
                    BB1.kaRight = CurrentAOI.aRC;
                    if (BB2.kaRight > CurrentAOI.aRC)
                        ShowRight = False;
                }
                if (BB1.kaTop > CurrentAOI.aTC) {
                    BB1.kaTop = CurrentAOI.aTC;
                    if (BB2.kaTop > CurrentAOI.aTC)
                        ShowTop = False;
                }

                if (ShowLeft) {
                    if (BB2.kaLeft > CurrentAOI.aRC)
                        BB2.kaLeft = CurrentAOI.aRC;
                    FBFilledBox(Layer,DISPLAY,0,BB1.kaLeft,
                        BB1.kaBottom,BB2.kaLeft,BB1.kaTop);
                }
                if (ShowBottom)    {
                    if (BB2.kaBottom > CurrentAOI.aTC)
                        BB2.kaBottom = CurrentAOI.aTC;
                    FBFilledBox(Layer,DISPLAY,0,BB1.kaLeft,
                        BB1.kaBottom,BB1.kaRight,BB2.kaBottom);
                }
                if (ShowRight) {
                    if (BB2.kaRight < CurrentAOI.aLC)
                        BB2.kaRight = CurrentAOI.aLC;
                    FBFilledBox(Layer,DISPLAY,0,BB2.kaRight,
                        BB1.kaBottom,BB1.kaRight,BB1.kaTop);
                }
                if (ShowTop) {
                    if (BB2.kaTop < CurrentAOI.aBC)
                        BB2.kaTop = CurrentAOI.aBC;
                    FBFilledBox(Layer,DISPLAY,0,BB1.kaLeft,
                        BB2.kaTop,BB1.kaRight,BB1.kaTop);
                }
            }
        }
    }


    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY And
        CurrentAOI.aInFine) {

        FineLToP(BB.kaLeft,BB.kaBottom,BB1.kaLeft,BB1.kaBottom);
        FineLToP(BB.kaRight,BB.kaTop,BB1.kaRight,BB1.kaTop);

        if (BB1.kaLeft <= CurrentAOI.aRF And BB1.kaRight >= CurrentAOI.aLF And
            BB1.kaBottom <= CurrentAOI.aTF And BB1.kaTop >= CurrentAOI.aBF) {

            ShowLeft   = True;
            ShowBottom = True;
            ShowRight  = True;
            ShowTop    = True;
            if (LayerTable[Layer].klAttributes & FILLED) {
                if (BB1.kaLeft < CurrentAOI.aLF) {
                    BB1.kaLeft = CurrentAOI.aLF;
                    ShowLeft = False;
                }
                if (BB1.kaBottom < CurrentAOI.aBF) {
                    BB1.kaBottom = CurrentAOI.aBF;
                    ShowBottom = False;
                }
                if (BB1.kaRight > CurrentAOI.aRF) {
                    BB1.kaRight = CurrentAOI.aRF;
                    ShowRight = False;
                }
                if (BB1.kaTop > CurrentAOI.aTF) {
                    BB1.kaTop = CurrentAOI.aTF;
                    ShowTop = False;
                }
                if (LayerTable[Layer].klAttributes & FINE_FILL) {
                    FBFilledBox(Layer,DISPLAY,
                        LayerTable[Layer].klStyleID,
                        BB1.kaLeft,BB1.kaBottom,BB1.kaRight,BB1.kaTop);
                    if (LayerTable[Layer].klAttributes & OUTLINED) {

                        if (ShowLeft)
                            FBLine(BB1.kaLeft,BB1.kaBottom,
                                BB1.kaLeft,BB1.kaTop);
                        if (ShowBottom)
                            FBLine(BB1.kaRight,BB1.kaBottom,
                                BB1.kaLeft,BB1.kaBottom);
                        if (ShowRight)
                            FBLine(BB1.kaRight,BB1.kaTop,
                                BB1.kaRight,BB1.kaBottom);
                        if (ShowTop)
                            FBLine(BB1.kaLeft,BB1.kaTop,
                                BB1.kaRight,BB1.kaTop);
                    }
                }
                else {
                    FBFilledBox(Layer,DISPLAY,0,
                        BB1.kaLeft,BB1.kaBottom,BB1.kaRight,BB1.kaTop);
                }
            }
            else {

                BB2.kaLeft = BB1.kaLeft + PixWidth;
                BB2.kaBottom = BB1.kaBottom + PixWidth;
                BB2.kaRight = BB1.kaRight - PixWidth;
                BB2.kaTop = BB1.kaTop - PixWidth;

                if (BB1.kaLeft < CurrentAOI.aLF) {
                    BB1.kaLeft = CurrentAOI.aLF;
                    if (BB2.kaLeft < CurrentAOI.aLF)
                        ShowLeft = False;
                }
                if (BB1.kaBottom < CurrentAOI.aBF) {
                    BB1.kaBottom = CurrentAOI.aBF;
                    if (BB2.kaBottom < CurrentAOI.aBF)
                        ShowBottom = False;
                }
                if (BB1.kaRight > CurrentAOI.aRF) {
                    BB1.kaRight = CurrentAOI.aRF;
                    if (BB2.kaRight > CurrentAOI.aRF)
                        ShowRight = False;
                }
                if (BB1.kaTop > CurrentAOI.aTF) {
                    BB1.kaTop = CurrentAOI.aTF;
                    if (BB2.kaTop > CurrentAOI.aTF)
                        ShowTop = False;
                }
                if (ShowLeft) {
                    if (BB2.kaLeft > CurrentAOI.aRF)
                        BB2.kaLeft = CurrentAOI.aRF;
                    FBFilledBox(Layer,DISPLAY,0,BB1.kaLeft,
                        BB1.kaBottom,BB2.kaLeft,BB1.kaTop);
                }

                if (ShowBottom)    {
                    if (BB2.kaBottom > CurrentAOI.aTF)
                        BB2.kaBottom = CurrentAOI.aTF;
                    FBFilledBox(Layer,DISPLAY,0,BB1.kaLeft,
                        BB1.kaBottom,BB1.kaRight,BB2.kaBottom);
                }
                if (ShowRight) {
                    if (BB2.kaRight < CurrentAOI.aLF)
                        BB2.kaRight = CurrentAOI.aLF;
                    FBFilledBox(Layer,DISPLAY,0,BB2.kaRight,
                        BB1.kaBottom,BB1.kaRight,BB1.kaTop);
                }
                if (ShowTop) {
                    if (BB2.kaTop < CurrentAOI.aBF)
                        BB2.kaTop = CurrentAOI.aBF;
                    FBFilledBox(Layer,DISPLAY,0,BB1.kaLeft,
                        BB2.kaTop,BB1.kaRight,BB1.kaTop);
                }
            }
        }
    }
}


void
ShowEmptyBox(Layer,boxBB)

/* show a thin line box, no fill */
int Layer;
struct ka *boxBB;
{
    struct ka BB,BB1;
    int ShowLeft   = True;
    int ShowBottom = True;
    int ShowRight  = True;
    int ShowTop    = True;

    ka_copy(BB,boxBB);

    if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY And
        CurrentAOI.aInCoarse) {

        CoarseLToP(BB.kaLeft,BB.kaBottom,BB1.kaLeft,BB1.kaBottom);
        CoarseLToP(BB.kaRight,BB.kaTop,BB1.kaRight,BB1.kaTop);

        if (BB1.kaLeft <= CurrentAOI.aRC And BB1.kaRight >= CurrentAOI.aLC And
            BB1.kaBottom <= CurrentAOI.aTC And BB1.kaTop >= CurrentAOI.aBC) {

            if (BB1.kaLeft < CurrentAOI.aLC) {
                BB1.kaLeft = CurrentAOI.aLC;
                ShowLeft = False;
            }
            if (BB1.kaBottom < CurrentAOI.aBC) {
                BB1.kaBottom = CurrentAOI.aBC;
                ShowBottom = False;
            }
            if (BB1.kaRight > CurrentAOI.aRC) {
                BB1.kaRight = CurrentAOI.aRC;
                ShowRight = False;
            }
            if (BB1.kaTop > CurrentAOI.aTC) {
                BB1.kaTop = CurrentAOI.aTC;
                ShowTop = False;
            }
            FBForeground(DISPLAY,Layer);

            if (ShowTop)
                FBLine(BB1.kaLeft,BB1.kaTop,
                    BB1.kaRight,BB1.kaTop);
            if (ShowRight)
                FBLine(BB1.kaRight,BB1.kaTop,
                    BB1.kaRight,BB1.kaBottom);
            if (ShowBottom)
                FBLine(BB1.kaRight,BB1.kaBottom,
                    BB1.kaLeft,BB1.kaBottom);
            if (ShowLeft)
                FBLine(BB1.kaLeft,BB1.kaBottom,
                    BB1.kaLeft,BB1.kaTop);
        }
    }

    ShowLeft   = True;
    ShowBottom = True;
    ShowRight  = True;
    ShowTop    = True;

    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY And
        CurrentAOI.aInFine) {

        FineLToP(BB.kaLeft,BB.kaBottom,BB1.kaLeft,BB1.kaBottom);
        FineLToP(BB.kaRight,BB.kaTop,BB1.kaRight,BB1.kaTop);

        if (BB1.kaLeft <= CurrentAOI.aRF And BB1.kaRight >= CurrentAOI.aLF And
            BB1.kaBottom <= CurrentAOI.aTF And BB1.kaTop >= CurrentAOI.aBF) {

            if (BB1.kaLeft < CurrentAOI.aLF) {
                BB1.kaLeft = CurrentAOI.aLF;
                ShowLeft = False;
            }
            if (BB1.kaBottom < CurrentAOI.aBF) {
                BB1.kaBottom = CurrentAOI.aBF;
                ShowBottom = False;
            }
            if (BB1.kaRight > CurrentAOI.aRF) {
                BB1.kaRight = CurrentAOI.aRF;
                ShowRight = False;
            }
            if (BB1.kaTop > CurrentAOI.aTF) {
                BB1.kaTop = CurrentAOI.aTF;
                ShowTop = False;
            }
            FBForeground(DISPLAY,Layer);

            if (ShowTop)
                FBLine(BB1.kaLeft,BB1.kaTop,
                    BB1.kaRight,BB1.kaTop);
            if (ShowRight)
                FBLine(BB1.kaRight,BB1.kaTop,
                    BB1.kaRight,BB1.kaBottom);
            if (ShowBottom)
                FBLine(BB1.kaRight,BB1.kaBottom,
                    BB1.kaLeft,BB1.kaBottom);
            if (ShowLeft)
                FBLine(BB1.kaLeft,BB1.kaBottom,
                    BB1.kaLeft,BB1.kaTop);
        }
    }
}


void
EraseBox(boxBB)

struct ka *boxBB;
{
    struct ka BB,BB1;

    SetCurrentAOI(boxBB);
    ka_copy(BB,boxBB);

    if (Parameters.kpRedisplayControl == SPLITSCREEN)
        XORfineViewport();

    if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY And
        CurrentAOI.aInCoarse) {

        CoarseLToP(BB.kaLeft,BB.kaBottom,BB1.kaLeft,BB1.kaBottom);
        CoarseLToP(BB.kaRight,BB.kaTop,BB1.kaRight,BB1.kaTop);

        if (BB1.kaLeft <= CurrentAOI.aRC And BB1.kaRight >= CurrentAOI.aLC And
            BB1.kaBottom <= CurrentAOI.aTC And BB1.kaTop >= CurrentAOI.aBC) {

            if (BB1.kaLeft < CurrentAOI.aLC)
                BB1.kaLeft = CurrentAOI.aLC;
            if (BB1.kaBottom < CurrentAOI.aBC)
                BB1.kaBottom = CurrentAOI.aBC;
            if (BB1.kaRight > CurrentAOI.aRC)
                BB1.kaRight = CurrentAOI.aRC;
            if (BB1.kaTop > CurrentAOI.aTC)
                BB1.kaTop = CurrentAOI.aTC;
            FBFilledBox(0,ERASE,0,BB1.kaLeft,BB1.kaBottom,
                BB1.kaRight,BB1.kaTop);
        }
    }

    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY And
        CurrentAOI.aInFine) {

        FineLToP(BB.kaLeft,BB.kaBottom,BB1.kaLeft,BB1.kaBottom);
        FineLToP(BB.kaRight,BB.kaTop,BB1.kaRight,BB1.kaTop);

        if (BB1.kaLeft <= CurrentAOI.aRF And BB1.kaRight >= CurrentAOI.aLF And
            BB1.kaBottom <= CurrentAOI.aTF And BB1.kaTop >= CurrentAOI.aBF) {

            if (BB1.kaLeft < CurrentAOI.aLF)
                BB1.kaLeft = CurrentAOI.aLF;
            if (BB1.kaBottom < CurrentAOI.aBF)
                BB1.kaBottom = CurrentAOI.aBF;
            if (BB1.kaRight > CurrentAOI.aRF)
                BB1.kaRight = CurrentAOI.aRF;
            if (BB1.kaTop > CurrentAOI.aTF)
                BB1.kaTop = CurrentAOI.aTF;
            FBFilledBox(0,ERASE,0,BB1.kaLeft,BB1.kaBottom,
                BB1.kaRight,BB1.kaTop);
        }
    }
    if (Parameters.kpRedisplayControl == SPLITSCREEN)
        XORfineViewport();
}


void
Boxes(LookedAhead)

int *LookedAhead;
{
    struct ka BB;
    struct o *Pointer = NULL;
    int Undo = False;
    int FirstTime = True;
    int Modified = 0;
    int X = 0,Y = 0;

    MakingBoxes = True;
    MenuSelect(MenuBOXES);

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
                    CDDelete(Parameters.kpCellDesc,Pointer);
                    Modified--;
                    Undo = True;
                    EraseBox(&BB);
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
                    Undo = False;
                }
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
        if (LayerTable[Parameters.kpLayer].klElectrical == NULL)
            ErasePrompt();

        if (Not CDMakeBox(Parameters.kpCellDesc,
            Parameters.kpLayer,
            (BB.kaRight-BB.kaLeft),
            (BB.kaTop-BB.kaBottom),
            (BB.kaRight-BB.kaLeft)/2+BB.kaLeft,
            (BB.kaTop-BB.kaBottom)/2+BB.kaBottom,
            &Pointer)) MallocFailed();
        Modified++;;
        FirstTime = False;
        Undo = False;

        /* make sure the new box is displayed */
        CDReflect(Parameters.kpCellDesc);
        Redisplay(&BB);
    }

quit:
    if (Modified)
        Parameters.kpModified = True;
    FBSetRubberBanding(0);
    SetRelative(0L,0L,False);
    ErasePrompt();
    MakingBoxes = False;
    MenuDeselect(MenuBOXES);
}


void
OversizeBox(BB,Delta)

struct ka *BB;
int Delta;
{
    BB->kaTop += Delta;
    BB->kaRight += Delta;
    BB->kaBottom -= Delta;
    BB->kaLeft -= Delta;
}


void
OutlineBox(AOI)

struct ka *AOI;
{
    FBLine(AOI->kaLeft,AOI->kaTop,
        AOI->kaRight,AOI->kaTop);
    FBLine(AOI->kaRight,AOI->kaTop,
        AOI->kaRight,AOI->kaBottom);
    FBLine(AOI->kaRight,AOI->kaBottom,
        AOI->kaLeft, AOI->kaBottom);
    FBLine(AOI->kaLeft, AOI->kaBottom,
        AOI->kaLeft, AOI->kaTop);
}

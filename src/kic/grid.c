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
 * Shows a grid in a layout viewport.
 */

#include "prefix.h"
#include "kic.h"


void
ShowGrid()

{
    /*
     * Show grid for Window within AOI.
     * Window maps to Viewport.
     */
    int YInL,XInL;
    int XInP,YInP;
    int Left,Top,Right,Bottom;
    int Step;

    if (!Parameters.kpGridDisplayed Or Parameters.kpGrid <= 0) {
        ShowAxes();
        return;
    }

    FBDefineLineStyle(1,Parameters.kpGridLineStyle);

    /*
     *
     * The grid is computed relative to (0,0).
     *
     */
    Step = Parameters.kpGrid*5;

    if (((Parameters.kpRedisplayControl == COARSEVIEWPORTONLY And
        View->kvCoarseViewport == View->kvLargeCoarseViewport) Or
        (Parameters.kpRedisplayControl == SPLITSCREEN And
        Parameters.kpShowGridInLargeViewport)) And
        Parameters.kpGrid*View->kvCoarseRatio
        > Parameters.kpPointingThreshold And CurrentAOI.aInCoarse) {

        Top = View->kvCoarseWindow->kaTop -
            (View->kvCoarseWindow->kaTop % Step);

        if (View->kvCoarseWindow->kaTop >= 0)
            Top += Step;

        Bottom = View->kvCoarseWindow->kaBottom;

        Left = View->kvCoarseWindow->kaLeft -
            (View->kvCoarseWindow->kaLeft % Step);

        if (View->kvCoarseWindow->kaLeft < 0)
            Left -= Step;
        
        Right = View->kvCoarseWindow->kaRight;

        if (Parameters.kpGridLineStyle == 0) {

            /* point grid */
            for (YInL = Top; YInL >= Bottom; YInL -= Parameters.kpGrid) {
                YInP = .5+(YInL - View->kvCoarseWindow->kaBottom)*
                    View->kvCoarseRatio;
                YInP += View->kvCoarseViewport->kaBottom;
                if (YInP < CurrentAOI.aBC) break;
                if (YInP > CurrentAOI.aTC) continue;
                for (XInL = Left; XInL <= Right; XInL += Parameters.kpGrid) {
                    XInP = .5+(XInL - View->kvCoarseWindow->kaLeft)*
                        View->kvCoarseRatio;
                    XInP += View->kvCoarseViewport->kaLeft;
                    if (XInP > CurrentAOI.aRC) break;
                    if (XInP < CurrentAOI.aLC) continue;
                    if ((Top-YInL) % Step == 0 || (XInL-Left) % Step == 0)
                        FBForeground(DISPLAY,ColorTable[CoarseGridColor].Ent);
                    else
                        FBForeground(DISPLAY,ColorTable[FineGridColor].Ent);

                    FBPixel(XInP,YInP);
                }
            }
        }
        else {

            /* Horizontal grid lines */
            for (YInL = Top; YInL >= Bottom; YInL -= Parameters.kpGrid) {

                YInP = .5+(YInL - View->kvCoarseWindow->kaBottom)*
                    View->kvCoarseRatio;
                YInP += View->kvCoarseViewport->kaBottom;
                if (YInP < CurrentAOI.aBC) break;
                if (YInP > CurrentAOI.aTC) continue;

                if ((Top-YInL) % Step == 0)
                    FBForeground(DISPLAY,ColorTable[CoarseGridColor].Ent);
                else
                    FBForeground(DISPLAY,ColorTable[FineGridColor].Ent);

                FBLine(CurrentAOI.aLC,YInP,
                    CurrentAOI.aRC,YInP);
            
            }

            /* Vertical grid lines */
            for (XInL = Left; XInL <= Right; XInL += Parameters.kpGrid) {

                XInP = .5+(XInL - View->kvCoarseWindow->kaLeft)*
                    View->kvCoarseRatio;
                XInP += View->kvCoarseViewport->kaLeft;
                if (XInP > CurrentAOI.aRC) break;
                if (XInP < CurrentAOI.aLC) continue;

                if ((XInL-Left) % Step == 0)
                    FBForeground(DISPLAY,ColorTable[CoarseGridColor].Ent);
                else
                    FBForeground(DISPLAY,ColorTable[FineGridColor].Ent);

                FBLine(XInP,CurrentAOI.aBC,
                    XInP,CurrentAOI.aTC);
            
            }
        }
    }

    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY And
        Parameters.kpGrid*View->kvFineRatio
            > Parameters.kpPointingThreshold And CurrentAOI.aInFine) {


        Top = View->kvFineWindow->kaTop -
            (View->kvFineWindow->kaTop % Step);

        if (View->kvFineWindow->kaTop >= 0)
            Top += Step;

        Bottom = View->kvFineWindow->kaBottom;

        Left = View->kvFineWindow->kaLeft -
            (View->kvFineWindow->kaLeft % Step);

        if (View->kvFineWindow->kaLeft < 0)
            Left -= Step;

        Right = View->kvFineWindow->kaRight;

        if (Parameters.kpGridLineStyle == 0) {

            /* point grid */
            for (YInL = Top; YInL >= Bottom; YInL -= Parameters.kpGrid) {
                YInP = .5+(YInL - View->kvFineWindow->kaBottom)*
                    View->kvFineRatio;
                YInP += View->kvFineViewport->kaBottom;
                if (YInP < CurrentAOI.aBF) break;
                if (YInP > CurrentAOI.aTF) continue;
                for (XInL = Left; XInL <= Right; XInL += Parameters.kpGrid) {
                    XInP = .5+(XInL - View->kvFineWindow->kaLeft)*
                        View->kvFineRatio;
                    XInP += View->kvFineViewport->kaLeft;
                    if (XInP > CurrentAOI.aRF) break;
                    if (XInP < CurrentAOI.aLF) continue;
                    if ((Top-YInL) % Step == 0 || (XInL-Left) % Step == 0)
                        FBForeground(DISPLAY,ColorTable[CoarseGridColor].Ent);
                    else
                        FBForeground(DISPLAY,ColorTable[FineGridColor].Ent);

                    FBPixel(XInP,YInP);
                }
            }
        }
        else {
            /* Horizontal grid lines */
            for (YInL = Top; YInL >= Bottom; YInL -= Parameters.kpGrid) {

                YInP = .5+(YInL - View->kvFineWindow->kaBottom)*
                    View->kvFineRatio;
                YInP += View->kvFineViewport->kaBottom;
                if (YInP < CurrentAOI.aBF) break;
                if (YInP > CurrentAOI.aTF) continue;

                if ((Top-YInL) % Step == 0)
                       FBForeground(DISPLAY,ColorTable[CoarseGridColor].Ent);
                else
                       FBForeground(DISPLAY,ColorTable[FineGridColor].Ent);
                FBLine(CurrentAOI.aLF,YInP,
                    CurrentAOI.aRF,YInP);
            }

            /* Vertical grid lines */
            for (XInL = Left; XInL <= Right; XInL += Parameters.kpGrid){

                XInP = .5+(XInL - View->kvFineWindow->kaLeft)*
                    View->kvFineRatio;
                XInP += View->kvFineViewport->kaLeft;
                if (XInP > CurrentAOI.aRF) break;
                if (XInP < CurrentAOI.aLF) continue;

                if ((XInL-Left) % Step == 0)
                    FBForeground(DISPLAY,ColorTable[CoarseGridColor].Ent);
                else
                    FBForeground(DISPLAY,ColorTable[FineGridColor].Ent);
                FBLine(XInP,CurrentAOI.aBF,
                    XInP,CurrentAOI.aTF);
            }
        }
    }
    FBSetLineStyle(0);
    ShowAxes();
}


void
ShowAxes()

{
    int XInP,YInP;

    /*
     * Display the axes if they are visible.
     */

    if (Parameters.kpDoingHardcopy && !Parameters.kpGridDisplayed)
        return;

    FBForeground(DISPLAY,ColorTable[HighlightingColor].Ent);

    if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY) {
        CoarseLToP(0,0,XInP,YInP);

        /* x axis */
        if (View->kvCoarseViewport->kaBottom <= YInP And
            YInP <= View->kvCoarseViewport->kaTop)
            FBLine(View->kvCoarseViewport->kaLeft,YInP,
                View->kvCoarseViewport->kaRight,YInP);

        /* y axis */
        if (View->kvCoarseViewport->kaLeft <= XInP And
            XInP <= View->kvCoarseViewport->kaRight)
            FBLine(XInP,View->kvCoarseViewport->kaBottom,
                XInP,View->kvCoarseViewport->kaTop);
    }
    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY) {
        FineLToP(0,0,XInP,YInP);

        /* x axis */
        if (View->kvFineViewport->kaBottom <= YInP And
            YInP <= View->kvFineViewport->kaTop)
            FBLine(View->kvFineViewport->kaLeft,YInP,
                View->kvFineViewport->kaRight,YInP);

        /* y axis */
        if (View->kvFineViewport->kaLeft <= XInP And
            XInP <= View->kvFineViewport->kaRight)
            FBLine(XInP,View->kvFineViewport->kaBottom,
                XInP,View->kvFineViewport->kaTop);
    }
}

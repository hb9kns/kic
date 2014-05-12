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
 * Line management.
 */

#include "prefix.h"
#include "kic.h"


void
ShowLine(Layer,X1,Y1,X2,Y2)

int Layer;
int X1,Y1,X2,Y2;
{
    int X1P,Y1P,X2P,Y2P;

    TPoint(&X1,&Y1);
    TPoint(&X2,&Y2);

    FBForeground(DISPLAY,Layer);

    if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY And
        CurrentAOI.aInCoarse) {

        CoarseLToP(X1,Y1,X1P,Y1P);
        CoarseLToP(X2,Y2,X2P,Y2P);
        if (Not LineClip(&X1P,&Y1P,&X2P,&Y2P,CurrentAOI.aLC,
            CurrentAOI.aBC,CurrentAOI.aRC,CurrentAOI.aTC)){

            FBLine(X1P,Y1P,X2P,Y2P);
        }
    }

    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY And
        CurrentAOI.aInFine) {

        FineLToP(X1,Y1,X1P,Y1P);
        FineLToP(X2,Y2,X2P,Y2P);
        if (Not LineClip(&X1P,&Y1P,&X2P,&Y2P,CurrentAOI.aLF,
            CurrentAOI.aBF,CurrentAOI.aRF,CurrentAOI.aTF)) {

            FBLine(X1P,Y1P,X2P,Y2P);
        }
    }
}


void
ShowManhattanLine(Layer,X1,Y1,X2,Y2)

int Layer;
int X1,Y1,X2,Y2;
{ 
    int X1P,Y1P,X2P,Y2P;

    FBForeground(DISPLAY,Layer);

    if (Parameters.kpRedisplayControl != FINEVIEWPORTONLY And
        CurrentAOI.aInCoarse) {

        CoarseLToP(X1,Y1,X1P,Y1P);
        CoarseLToP(X2,Y2,X2P,Y2P);

        if (X1P == X2P) {
            if (X1P < CurrentAOI.aLC Or X1P > CurrentAOI.aRC) return;
            if (Y1P > Y2P) SwapInts(Y1P,Y2P);
            if (Y1P > CurrentAOI.aTC Or Y2P < CurrentAOI.aBC) return;
            if (Y1P < CurrentAOI.aBC) Y1P = CurrentAOI.aBC;
            if (Y2P > CurrentAOI.aTC) Y2P = CurrentAOI.aTC;
        }
        elif (Y1P == Y2P) {
            if (Y1P < CurrentAOI.aBC Or Y1P > CurrentAOI.aTC) return;
            if (X1P > X2P) SwapInts(X1P,X2P);
            if (X1P > CurrentAOI.aRC Or X2P < CurrentAOI.aLC) return;
            if (X1P < CurrentAOI.aLC) X1P = CurrentAOI.aLC;
            if (X2P > CurrentAOI.aRC) X2P = CurrentAOI.aRC;
        }
        else return;

        FBLine(X1P,Y1P,X2P,Y2P);
    }
    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY And
        CurrentAOI.aInFine) {

        FineLToP(X1,Y1,X1P,Y1P);
        FineLToP(X2,Y2,X2P,Y2P);

        if (X1P == X2P) {
            if (X1P < CurrentAOI.aLF Or X1P > CurrentAOI.aRF) return;
            if (Y1P > Y2P) SwapInts(Y1P,Y2P);
            if (Y1P > CurrentAOI.aTF Or Y2P < CurrentAOI.aBF) return;
            if (Y1P < CurrentAOI.aBF) Y1P = CurrentAOI.aBF;
            if (Y2P > CurrentAOI.aTF) Y2P = CurrentAOI.aTF;
        }
        elif (Y1P == Y2P) {
            if (Y1P < CurrentAOI.aBF Or Y1P > CurrentAOI.aTF) return;
            if (X1P > X2P) SwapInts(X1P,X2P);
            if (X1P > CurrentAOI.aRF Or X2P < CurrentAOI.aLF) return;
            if (X1P < CurrentAOI.aLF) X1P = CurrentAOI.aLF;
            if (X2P > CurrentAOI.aRF) X2P = CurrentAOI.aRF;
        }
        else return;

        FBLine(X1P,Y1P,X2P,Y2P);
    }
}

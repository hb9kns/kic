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


#define CoarseLToP(X,Y,XT,YT) \
    XT = .5+(X-View->kvCoarseWindow->kaLeft)*View->kvCoarseRatio; \
    XT += View->kvCoarseViewport->kaLeft; \
    YT = .5+(Y-View->kvCoarseWindow->kaBottom)*View->kvCoarseRatio; \
    YT += View->kvCoarseViewport->kaBottom;

#define FineLToP(X,Y,XT,YT) \
    XT = .5+(X-View->kvFineWindow->kaLeft)*View->kvFineRatio; \
    XT += View->kvFineViewport->kaLeft; \
    YT = .5+(Y-View->kvFineWindow->kaBottom)*View->kvFineRatio; \
    YT += View->kvFineViewport->kaBottom;

#define ClipVP(Viewport,X,Y) \
    if(X < Viewport->kaLeft) X = Viewport->kaLeft; \
    else if(X > Viewport->kaRight) X = Viewport->kaRight; \
    if(Y < Viewport->kaBottom) Y = Viewport->kaBottom; \
    else if(Y > Viewport->kaTop) Y = Viewport->kaTop;

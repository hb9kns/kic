/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1994
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
 
/* Misc interfage routines for KIC, X graphics */

#include "prefix.h"
#include "kic.h"
#include "mfb.h"

#if __STDC__
extern void out_printf(char*, ...);
#else
extern void out_printf();
#endif


void
XORfineViewport()

{
    char tmp;
    GC tmpGC;
    /*
     * Erase or draw the fine positioning window in coarse window.
     */
    tmp = Parameters.kpRedisplayControl;
    Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
    tmpGC = mfb_lineGC;
    mfb_lineGC = mfb_dragGC;
    ShowEmptyBox(ColorTable[HighlightingColor].Ent,View->kvFineWindow);
    mfb_lineGC = tmpGC;
    Parameters.kpRedisplayControl = tmp;
}


int
GetWindowCoords(X,Y,Clip)

/* replace X,Y (in pixel coords) with the window coords if pointing
 * in the Coarse or Fine viewports, and return True.  Otherwise
 * return False.  Clip to grid if Clip is True.
 */
int Clip;
int *X,*Y;
{
    int X1,Y1;

    X1 = *X;
    Y1 = TRAN(*Y);
    if (InBox(X1,Y1,View->kvCoarseViewport)) {
        PToL(View->kvCoarseWindow,&X1,&Y1);
        if (Clip)
            ClipToGridPoint(&X1,&Y1);
        *X = X1;
        *Y = Y1;
        return (True);
    }
    if (InBox(X1,Y1,View->kvFineViewport)) {
        PToL(View->kvFineWindow,&X1,&Y1);
        if (Clip)
            ClipToGridPoint(&X1,&Y1);
        *X = X1;
        *Y = Y1;
        return (True);
    }
    return (False);
}


int *
SetButtonMask()

{
    MFBCurrent->buttonMask[0]   = FBInfo(BUTTON1);
    MFBCurrent->buttonMask[1]   = FBInfo(BUTTON2);
    MFBCurrent->buttonMask[2]   = FBInfo(BUTTON3);
    MFBCurrent->buttonMask[3]   = FBInfo(BUTTON4);
    MFBCurrent->buttonMask[4]   = FBInfo(BUTTON5);
    MFBCurrent->buttonMask[5]   = FBInfo(BUTTON6);
    MFBCurrent->buttonMask[6]   = FBInfo(BUTTON7);
    MFBCurrent->buttonMask[7]   = FBInfo(BUTTON8);
    MFBCurrent->buttonMask[8]   = FBInfo(BUTTON9);
    MFBCurrent->buttonMask[9]   = FBInfo(BUTTON10);
    MFBCurrent->buttonMask[10]  = FBInfo(BUTTON11);
    MFBCurrent->buttonMask[11]  = FBInfo(BUTTON12);
    return (MFBCurrent->buttonMask);
}


extern char *MenuFONT, *MenuCURSR;
 
void
SelectKicFont()

{
    char oldRedisplayControl;
    int select;
 
    MenuSelect(MenuFONT);
    oldRedisplayControl = Parameters.kpRedisplayControl;
    Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
    FBForeground(DISPLAY, 0);
    select = FBSelectFont(View->kvFineViewport->kaLeft-1,
        View->kvFineViewport->kaBottom-1,
        View->kvFineViewport->kaRight,
    View->kvFineViewport->kaTop);
    Parameters.kpRedisplayControl = oldRedisplayControl;
    if (select) {
        strncpy(Parameters.kpFontName, (char *)MFBInfo(FONTNAME), 80);
        Parameters.kpFontName[80] = EOS;
        FullRedisplay();
    }  
    else
        ShowFineViewport();
    MenuDeselect(MenuFONT);
}


void
SelectKicCursor()

{
    char oldRedisplayControl;
 
    MenuSelect(MenuCURSR);
    oldRedisplayControl = Parameters.kpRedisplayControl;
    Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
    FBForeground(DISPLAY, 0);
    FBSelectCursor(View->kvFineViewport->kaLeft-1,
        View->kvFineViewport->kaBottom-1,
        View->kvFineViewport->kaRight,
    View->kvFineViewport->kaTop);
    Parameters.kpCursorShape = MFBInfo(CURSORSHAPE);
    Parameters.kpFullScreenCursor = MFBInfo(FULLSCREENCURSOR);
    Parameters.kpRedisplayControl = oldRedisplayControl;
    ShowFineViewport();
    MenuDeselect(MenuCURSR);
}
 

#ifdef notdef
SetBeepVolume()
{
    char *TypeIn;
    int volume;
       
    MenuSelect(MenuBEEPV);
    sprintf(TypeOut, "New beep volume [%d]?", Parameters.kpPointBeepVolume);
    ShowPrompt(TypeOut);
    FBKeyboard(&TypeIn);
    if (sscanf(TypeIn, "%d", &Parameters.kpPointBeepVolume)) {
    if (Parameters.kpPointBeepVolume < 0)
        Parameters.kpPointBeepVolume = 0;
    if (Parameters.kpPointBeepVolume > 100)
        Parameters.kpPointBeepVolume = 100;
    }
    MenuDeselect(MenuBEEPV);
}
#endif

unsigned int
FBTime()
{
    return (MFBInfo(ACTIONTIME));
}


/* ARGSUSED */
void
FBFuncKeys(y,dx) { }


int
Xcheck()
{
    return (1);
}


/* ARGSUSED */
void
cprint(i,s)

int i;
char *s;
{
    out_printf(s);
}


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
 
#include "prefix.h"
#include "kic.h"


void
Debug(LookedAhead)

int *LookedAhead;
{
    *LookedAhead = False;
    Parameters.kpMenu = DEBUGMENU;
    FixMenuPrefix(DebugMenu);
    ShowCommandMenu();
}


void
DoBW()

{
    extern char *MenuBW;

    if(Parameters.kpShowBandwidth) {
        MenuDeselect(MenuBW);
        Parameters.kpShowBandwidth = False;
    }
    else {
        MenuSelect(MenuBW);
        Parameters.kpShowBandwidth = True;
    }
}


void
DoAlloc()

{
    extern char *MenuALLOC;
    /*
     * Show # of symbol descs CD has allocated.
     */
    MenuSelect(MenuALLOC);
    sprintf(TypeOut,"%d symbols allocated so far by CD package.",
        CDDesc.dNumSymbolsAllocated);
    ShowPrompt(TypeOut);
    MenuDeselect(MenuALLOC);
}


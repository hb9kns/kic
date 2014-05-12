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
 * Delete selection operator.
 */

#include "prefix.h"
#include "kic.h"

#ifdef __STDC__
static void do_delete(int,struct ka*);
#else
static void do_delete();
#endif

extern char *MenuDELET;
extern char *MenuUNDO;


void
Del(LookedAhead)

int *LookedAhead;
{
    struct ks *SQDesc;
    int Info = SQ_GONE;

    MenuSelect(MenuDELET);
    if (SelectQHead == NULL) {
        ShowPrompt("There are no selected objects to delete."); 
        MenuDeselect(MenuDELET);
        return;
    }
    SQComputeBB();
    /* take care of instance markers */
    SelectQBB.kaRight  += 600;
    SelectQBB.kaLeft   -= 600;
    SelectQBB.kaTop    += 600;
    SelectQBB.kaBottom -= 600;

    do_delete(SQ_GONE,&SelectQBB);
    MenuDeselect(MenuDELET);

top:
    switch (PointLoop(LookedAhead)) {
    case PL_UND:
        MenuSelect(MenuUNDO);
        if (Info == SQ_GONE) Info = SQ_OLDSEL;
        else Info = SQ_GONE;
        do_delete(Info,&SelectQBB);
        MenuDeselect(MenuUNDO);
    case PL_ESC:
    case PL_PCW:
        goto top;
    case PL_CMD:
        break;
    }
    if (Info == SQ_GONE) {
        struct ks *SQNext;
        for (SQDesc = SelectQHead; SQDesc; SQDesc = SQNext) {
            SQNext = SQDesc->ksSucc;
            SQDesc->ksPointer->oInfo = SQ_OLD;
            CDDelete(Parameters.kpCellDesc,SQDesc->ksPointer);
            afree(SQDesc,ks);
        }
        SelectQHead = NULL;
        SelectQBB.kaLeft = SelectQBB.kaBottom =
            SelectQBB.kaRight = SelectQBB.kaTop = 0;
        Parameters.kpModified = True;
    }
    else
        SQComputeBB();
}


static void
do_delete(Info,BB)

int Info;
struct ka *BB;
{
    struct ks *SQDesc;
    /*
     * Change Info field and redisplay.
     */
    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc)
        SQDesc->ksPointer->oInfo = Info;

    EraseBox(BB);
    Redisplay(BB);
    FBTransfer();
}


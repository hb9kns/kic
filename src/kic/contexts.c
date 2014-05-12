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
 * KIC context code.
 */

#include "prefix.h"
#include "kic.h"

struct cc {
    int ccX,ccY,ccWidth;
    int ccModified;
    struct kw *ccSaveWindow;
    struct o *ccInst;
    char *ccMaster;
    int ccXform[9];
    struct cc *ccNext;
    struct cc *ccPrev;
};
static struct cc *Context;

static int WroteCell;

#ifdef __STDC__
static void edit_cell(int);
static int  pop_context(void);
static void sq_first_call(struct o**);
static void select_call(void);
#else
static void edit_cell();
static int  pop_context();
static void sq_first_call();
static void select_call();
#endif


void
Push(LookedAhead)

int *LookedAhead;
{
    struct o *Pointer;
    struct cc *New;
    char Type;
    char *MasterName;
    int  NumX,NumY;
    int DX,DY;
    extern char *MenuPUSH;

    MenuSelect(MenuPUSH);
    WroteCell = False;
    loop {
        /*
         * Fetch first call desc in select Q if any.
         * Its master is the cell we will push to.
         * Otherwise, point to select an instance;
         */
        sq_first_call(&Pointer);
        if (Pointer != NULL) {
            CDType(Pointer,&Type);
            if (Type == CDSYMBOLCALL)
                break;
        }
        ShowPrompt("Point to instance to push to ");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
        case PL_UND:
            ErasePrompt();
            MenuDeselect(MenuPUSH);
            return;
        case PL_PCW:
            select_call();
        }
    }
    ErasePrompt();
    CDCall(Pointer,&MasterName,&NumX,&DX,&NumY,&DY);
    New = alloc(cc);
    if (New == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    New->ccMaster = malloc(strlen(Parameters.kpCellName)+1);
    if (New->ccMaster == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }

    strcpy(New->ccMaster,Parameters.kpCellName);
    New->ccInst = Pointer;
    New->ccX = View->kvCoarseWindow->kaX;
    New->ccY = View->kvCoarseWindow->kaY;
    New->ccModified = Parameters.kpModified;
    New->ccWidth = View->kvCoarseWindow->kaWidth;
    New->ccSaveWindow = Parameters.kpWindowStack;
    New->ccNext = Context;
    Context = New;
    strcpy(Parameters.kpCellName,MasterName);
    Parameters.kpModified = False;
    Parameters.kpWindowStack = NULL;

    /* find the inverse transform of the cell, used when displaying
     * context */

    TPush();
    TLoad();         /* load the current inverse transform */
    TCurrent(New->ccXform);
    TInverse();      /* compute the inverse (actual transform) */
    TLoadInverse();  /* load it */

    SetTransform(Pointer);

    TInverse();      /* compute the inverse */
    TLoadInverse();  /* load it */
    TStore();        /* save it for use in redisplay() */
    TPop();
    TPop();

    edit_cell(False);
    TitleWindow();
    MenuDeselect(MenuPUSH);
}


void
ShowContext()
{
    extern char *MenuCNTXT;

    if (Parameters.kpShowContext) {
        Parameters.kpShowContext = False;
        MenuDeselect(MenuCNTXT);
    }
    else {
        Parameters.kpShowContext = True;
        MenuSelect(MenuCNTXT);
    }
    if (Parameters.kpCellDesc != Parameters.kpTopDesc)
        FullRedisplay();
}


static void
edit_cell(Modified)

int Modified;
{
    SQClear();
    ShowPrompt("Building database.  Please wait.");
    if (Not CDOpen(Parameters.kpCellName,&Parameters.kpCellDesc,'w')) {
        sprintf(TypeOut,"Can't edit cell %s. MORE",Parameters.kpCellName); 
        ShowPrompt(TypeOut);
        (void)FBGetchar(ERASE);
        sprintf(TypeOut,"%s",CDStatusString);
        ShowPrompt(TypeOut);
        Parameters.kpCellName[0] = EOS;
    }
    else {
        if (CDStatusInt == CDNEWSYMBOL)    {
            ShowPrompt("Internal error: cell not found");
            return;
        }
        CenterFullView();
        CDUnmark(Parameters.kpCellDesc);
        RedisplayViewports();
        ShowPrompt("Cell is ready to be edited.");
        Parameters.kpModified = Modified;
    }
    ShowParameters();
}


void
UpdateParent(NewName)

char *NewName;
{
    char *TypeIn,*MasterName,Type;
    int NumX,NumY;
    int X,Y,DX,DY;
    struct s *CellDesc;
    struct t *TGen;
    struct o *Pointer;

    if (Context == NULL)
        return;

    ShowPrompt("Update parent cell to call new cell name? (n) ");
    TypeIn = FBEdit(NULL);
    if (TypeIn And (*TypeIn == 'y' Or *TypeIn == 'Y')) {
        /*Change the instance pushed from so its master is NewName.*/
        if (Not CDOpen(Context->ccMaster,&CellDesc,'w')) {
            sprintf(TypeOut,"Can't display %s. MORE",
                Context->ccMaster); 
            ShowPrompt(TypeOut);
            (void)FBGetchar(ERASE);
            ShowPrompt(CDStatusString);
            return;
        }
        CDCall(Context->ccInst,&MasterName,&NumX,&DX,&NumY,&DY);
        if (Not CDBeginMakeCall(
            CellDesc,NewName,NumX,DX,NumY,DY,&Pointer)) {
            if (CDStatusInt ==
                CDPARSEFAILED Or CDStatusInt == CDNEWSYMBOL) {
                sprintf(TypeOut,"Can't write cell %s. MORE",NewName);
                ShowPrompt(TypeOut);
                (void)FBGetchar(ERASE);
                ShowPrompt(CDStatusString); 
                return;
            }
            elif (CDStatusInt == CDMALLOCFAILED)
                MallocFailed();
        }
        CDInitTGen(Context->ccInst,&TGen);
        loop {
            CDTGen(&TGen,&Type,&X,&Y);
            if (TGen == NULL)
                break;
            if (Not CDT(Pointer,Type,X,Y))
                MallocFailed();
        }
        if (Not CDEndMakeCall(CellDesc,Pointer))
            MallocFailed();
        CDDelete(CellDesc,Context->ccInst);
        WroteCell = True;
        Context->ccModified = True; 
        Context->ccInst = Pointer;
    }
}


void
Pop()

{
    char *TypeIn;
    extern char *MenuPOP;

    MenuSelect(MenuPOP);
    if (Context == NULL)
        ShowPrompt("There isn't a context to pop to.");

    else {
        SQClear();
        if (WroteCell) {
            /* 
             * Restore cell to its old state, because we don't want
             *  a global change, but we want a change to the instance
             *  pushed from.
             */
            if (Not CDClose(Parameters.kpCellDesc))
                MallocFailed();
            Parameters.kpModified = False;
        }
        elif (Parameters.kpModified) {
            ShowPrompt("You've modified this cell.  Do you want to save it (y)?");
            TypeIn = FBEdit(NULL);
            if (TypeIn == NULL) {
                MenuDeselect(MenuPOP);
                ErasePrompt();
                return;
            }
            if (TypeIn[0] == 'n' Or TypeIn[0] == 'N') {
                if (Not CDClose(Parameters.kpCellDesc))
                    MallocFailed();
            }
            else
                Save();
        }
        edit_cell(pop_context());
    }
    TitleWindow();
    MenuDeselect(MenuPOP);
}


static int
pop_context()

{
    struct kw *Tmp;
    struct cc *Old;
    int Modified;

    strcpy(Parameters.kpCellName,Context->ccMaster);
    InitCoarseWindow(Context->ccX,Context->ccY,Context->ccWidth);
    InitFineWindow(Context->ccX,Context->ccY);

    for (Tmp = Parameters.kpWindowStack; Tmp;
        Tmp = Parameters.kpWindowStack) {
        Parameters.kpWindowStack = Tmp->kwNext;
        afree(Tmp,kw);
    }
    Parameters.kpWindowStack = Context->ccSaveWindow;
    free(Context->ccMaster);
    TPush();
    TLoadCurrent(Context->ccXform);
    TStore();
    TPop();
    SetPositioning();
    Parameters.kpModified = False;
    Modified = Context->ccModified;
    Old = Context;
    Context = Context->ccNext;
    afree(Old,cc);
    return (Modified);
}


int
CheckModified()

/* called on exit */
{
    struct s *SDesc;
    struct cc *Cx = Context;
    char *TypeIn;

    while (Cx) {
        if (Cx->ccModified) {
            sprintf(TypeOut,"Cell %s has been modified. Save it? (y) ",
                Cx->ccMaster);
            ShowPrompt(TypeOut);
            TypeIn = FBEdit(NULL);
            if (TypeIn == NULL) return (True);
            if (*TypeIn != 'n' && *TypeIn != 'N') {
                CDSymbol(Cx->ccMaster,&SDesc);
                if (SDesc && CDUpdate(SDesc,(char *)NULL)) {
                    Cx->ccModified = False;
                    if (Not CDReflect(SDesc))
                        MallocFailed();
                }
                else {
                    /* shouldn't happen */
                    sprintf(TypeOut,"Can't save %s.",Cx->ccMaster);
                    ShowPromptAndWait(TypeOut);
                    return (True);
                }
            }
        }
        Cx = Cx->ccNext;
    }
    ClearContext();
    return (False);
}


void
ClearContext()

/*  called from Edit() */
{
    while (Context) pop_context();
}


static void
sq_first_call(Pointer)

struct o **Pointer;
{
    struct ks *SQDesc;
    char Type;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        CDType(SQDesc->ksPointer,&Type);
        if (Type == CDSYMBOLCALL) {
            *Pointer = SQDesc->ksPointer;
            return;
        }
    }
    *Pointer = NULL;
}


static void
select_call()

{
    struct ka BB;
    char TTmp[8];

    BB.kaLeft   = BB.kaRight = KicCursor.kcRawX;
    BB.kaBottom = BB.kaTop   = KicCursor.kcRawY;
    strcpy(TTmp,Parameters.kpSelectTypes);
    Parameters.kpSelectTypes[0] = CDSYMBOLCALL;
    Parameters.kpSelectTypes[1] = EOS;
    Selection(&BB);
    strcpy(Parameters.kpSelectTypes,TTmp);
}

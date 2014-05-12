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
 * The KIC basic menu code.
 */

#include "prefix.h"
#include "kic.h"
#include <time.h>

#define Matching(string) !strcmp(Parameters.kpCommand,string)


void
Basic(LookedAhead)

int *LookedAhead;
{
    *LookedAhead = False;
    Parameters.kpMenu = BASICMENU;
    FixMenuPrefix(BasicMenu);
    ShowCommandMenu();
}


void
Rdraw()

{
    extern char *MenuRDRAW;

    MenuSelect(MenuRDRAW);
    FullRedisplay();
    MenuDeselect(MenuRDRAW);
}


void
Undo()

{
    extern char *MenuUNDO;

    MenuSelect(MenuUNDO);
    ShowPrompt("Sorry, but it's too late.");
    MenuDeselect(MenuUNDO);
}


void
DoSet45()

{
    extern char *Menu45S;

    if (Parameters.kp45s) {
        MenuDeselect(Menu45S);
        Parameters.kp45s = False;
    }
    else {
        MenuSelect(Menu45S);
        Parameters.kp45s = True;
    }
}


void
AbortKIC()

{
    char *TypeIn;
    extern char *MenuEXIT;

    MenuSelect(MenuEXIT);

    if (Parameters.kpModified) {
        sprintf(TypeOut,"Cell %s has been modified. Save it? (y) ",
            Parameters.kpCellName);
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) goto quit;
        if (*TypeIn != 'n' && *TypeIn != 'N') {
            if (CDUpdate(Parameters.kpCellDesc,(char *)NULL)) {
                Parameters.kpModified = False;
                if (Not CDReflect(Parameters.kpCellDesc))
                    MallocFailed();
            }
            else {
                /* shouldn't happen */
                sprintf(TypeOut,"Can't save %s.",Parameters.kpCellName);
                ShowPromptAndWait(TypeOut);
                goto quit;
            }
        }
    }
    if (!CheckModified()) {
        FBEnd();
        exit(0);
    }

quit:
    MenuDeselect(MenuEXIT);
    ErasePrompt();
}


void
ShowFull()

{
    extern char *MenuVIEW;

    MenuSelect(MenuVIEW);
    SaveLastView();
    CenterFullView();
    RedisplayViewports();
    ShowParameters();
    MenuDeselect(MenuVIEW);
}


void
Snap()

{
    char *TypeIn;
    double d;
    extern char *MenuSNAP;

    MenuSelect(MenuSNAP);
    ShowPrompt("Number of cursor snap points in grid interval?");
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL And sscanf(TypeIn,"%le",&d) == 1) {
        if (d > 10 Or d < 1) d = 1.0;
        Parameters.kpPixToLambdaSnapping = d;

        sprintf(TypeOut,"Snap point spacing set to %g",
            (double)Parameters.kpGrid/
            (Parameters.kpPixToLambdaSnapping*RESOLUTION));
        ShowPrompt(TypeOut);
    }
    else
        ErasePrompt();
    MenuDeselect(MenuSNAP);
}


void
Edit(Ready,Center,Modified)

int Ready,Center,Modified;
{
    extern char *MenuEDIT;
    /*
     * Ready == True if the name of the cell to be edited is
     * already in Parameters.kpCellName.
     * Center == True if the cell should be automatically centered.
     * Pop does NOT want it to be--it wants it to be exactly where it was
     * when it was pushed into.
     */
    char *TypeIn,*CellName;
    int CloseOldCell = False;
    struct s *TempCellDesc = 0;

    MenuSelect(MenuEDIT);

    if (Parameters.kpModified And Parameters.kpCellName[0] != EOS){
        ShowPrompt("You've modified this cell.  Do you want to save it (y)?");
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) {
            ErasePrompt();
            MenuDeselect(MenuEDIT);
            return;
        }
        if (TypeIn[0] == 'n' Or TypeIn[0] == 'N') {
            /* don't want to close old cell until after new one is opened */
            TempCellDesc = Parameters.kpCellDesc;
            CloseOldCell = True;
        }
        else
            Save();
    }
    ClearContext();

    if (Not Ready){
        ShowPrompt("Cell?");
        CellName = FBEdit(NULL);
        if (CellName == NULL) {
            ErasePrompt();
            MenuDeselect(MenuEDIT);
            return;
        }
        while (isspace(*CellName)) CellName++;
        if (*CellName == '\0') {
            /* get next cellname from argument list */
            CellName = NextCellName();
            if (CellName == NULL) {
                if (Parameters.kpCellName == NULL 
                    Or *Parameters.kpCellName == '\0')
                    CellName = DEFAULT_EDIT_FILE;
                else
                    CellName = Parameters.kpCellName;
            }
        }
        if (!strcmp(Parameters.kpCellName,CellName)) {
            if (CloseOldCell) {
                if (Not CDClose(TempCellDesc))
                    MallocFailed();
                CloseOldCell = False;
            }
        }
        else
            strcpy(Parameters.kpCellName,CellName);
    }

#ifdef WIN32
    /* In Win32, use forward slashes internally for dir sep */
    for (TypeIn = Parameters.kpCellName; *TypeIn; TypeIn++)
        if (*TypeIn == '\\')
            *TypeIn = '/';
#endif

    /* strip off any path prefix */
    if (FixCellName(Parameters.kpCellName)) {
        ShowPromptAndWait("Warning: internal path overflow");
    }

    SQClear();
    ShowParameters();
    ShowPrompt("Building database.  Please wait.");
    if (Not CDOpen(Parameters.kpCellName,&Parameters.kpCellDesc,'w')) {
        CDClose(Parameters.kpCellDesc);
        Parameters.kpCellDesc = NULL;
        DefaultWindows();
        RedisplayViewports();
        sprintf(TypeOut,"Can't edit cell %s. MORE",Parameters.kpCellName); 
        ShowPrompt(TypeOut);
        (void)FBGetchar(ERASE);
        sprintf(TypeOut,"%s",CDStatusString);
        ShowPrompt(TypeOut);
        Parameters.kpCellName[0] = '\0';
    }
    else {
        if (CloseOldCell)
            if (Not CDClose(TempCellDesc))
                MallocFailed();
        strcpy(Parameters.kpTopName,Parameters.kpCellName);
        Parameters.kpTopDesc = Parameters.kpCellDesc;

        if (CDStatusInt == CDNEWSYMBOL)
            DefaultWindows();
        elif (Center)
            CenterFullView();
        /*
         * CDUnmark is expensive and has two purposes!
         * First, we make sure that all objects begin with a 0 info field.
         * We will therefore see EVERYTHING in the symbol during redisplay.
         * Secondly, the bounding boxes are recomputed and therefore
         * propogated correctly through the hierarchy.
         */
        CDUnmark(Parameters.kpCellDesc);
        RedisplayViewports();
        if (CDStatusInt == CDNEWSYMBOL)
            ShowPrompt("New cell is ready to be edited.");
        else
            ShowPrompt("Cell is ready to be edited.");
        Parameters.kpModified = Modified;
        TitleWindow();
    }
    ShowParameters();
    SaveLastView();
    MenuDeselect(MenuEDIT);
}


void
TitleWindow()

{
    static char Title[80];

    sprintf(Title,"KIC-%s    (%s)",VersionString,Parameters.kpCellName);
    FBSetName(Title,"kic");
}


void
Save()

{
    ShowPrompt("Saving cell.    Please wait.");
    if (CDUpdate(Parameters.kpCellDesc,(char *)NULL)) {
        ShowPrompt("Current cell has been saved.");
        Parameters.kpModified = False;
        if (Not CDReflect(Parameters.kpCellDesc))
            MallocFailed();
    }
    else
        ShowPrompt("Can't save cell.");
}


void
WriteCell()

{
    char *TypeIn,NewName[64];
    extern char *MenuSAVE;

    MenuSelect(MenuSAVE);
    if (Parameters.kpCellName[0] == '\0') {
        ShowPrompt("There isn't anything to save.");
        MenuDeselect(MenuSAVE);
        return;
    }
    ShowPrompt("Cell name?");
    TypeIn = FBEdit(Parameters.kpCellName);
    if (TypeIn == NULL Or *TypeIn == '\0' Or *TypeIn == '\n') {
        MenuDeselect(MenuSAVE);
        ErasePrompt();
        return;
    }
    strcpy(NewName,TypeIn);
    ShowPrompt("Writing cell.    Please wait.");

    if (strcmp(Parameters.kpCellName,NewName)) {
        /* new name given */

        TypeIn = Parameters.kpCellDesc->sName;
        Parameters.kpCellDesc->sName = NewName;
        if (Not CDUpdate(Parameters.kpCellDesc,NewName)) {
            ShowPrompt("Can't save cell.");
            MenuDeselect(MenuSAVE); 
            Parameters.kpCellDesc->sName = TypeIn;
            return;
        }
        ShowPrompt("New cell has been saved.");
        Parameters.kpCellDesc->sName = TypeIn;
        UpdateParent(NewName);
    }
    else {
        if (Not CDUpdate(Parameters.kpCellDesc,(char *)NULL)) {
            ShowPrompt("Can't save cell.");
            MenuDeselect(MenuSAVE); 
            return;
        }
        ShowPrompt("Current cell has been saved.");
        Parameters.kpModified = False;
        if (Not CDReflect(Parameters.kpCellDesc))
            MallocFailed();
    }
    MenuDeselect(MenuSAVE);
}


void
Peek()

{
    extern char *MenuEXPND;
    extern char *MenuPEEK;

    if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY)
        ShowPrompt("Peek mode isn't required.");
    elif (Parameters.kpExpandFineViewportOnly) {
        MenuDeselect(MenuPEEK);
        Parameters.kpExpandFineViewportOnly = False;
        ShowFineViewport();
    }
    else {
        Parameters.kpExpandFineViewportOnly = True;
        MenuSelect(MenuPEEK);
        if (Parameters.kpExpandInstances) {
            MenuDeselect(MenuEXPND);
            Parameters.kpExpandInstances = False;
            RedisplayViewports();
        }
        else
            ShowFineViewport();
    }
}


void
Expand()

{
    extern char *MenuEXPND;
    extern char *MenuPEEK;

    if (Parameters.kpExpandInstances) {
        MenuDeselect(MenuEXPND);
        Parameters.kpExpandInstances = False;
    }
    else {
        MenuSelect(MenuEXPND);
        Parameters.kpExpandInstances = True;
        MenuDeselect(MenuPEEK);
        Parameters.kpExpandFineViewportOnly = False;
    }
    RedisplayViewports();
}


void
CenterFullView()

{
    int L,B,R,T;
    int X,Y,Width,Height;
    double Cratio,Vratio;

    if (Not CDBB(Parameters.kpCellDesc,(struct o *)NULL,&L,&B,&R,&T))
        MallocFailed();
    /*
     * Window cell automatically.  How slick.
     * Leave some white space around the perimeter.
     * Check for a null (empty) cell.
     */
    if (L == R Or B == T Or (L == CDINFINITY And B == CDINFINITY)){
        DefaultWindows();
        return;
    }
    Width = R - L;
    Height = T - B;
    X = L + Width/2;
    Y = B + Height/2;
    if (Width < 0) Width = -Width;
    if (Height < 0) Height = -Height;
    Vratio = View->kvCoarseViewport->kaWidth/View->kvCoarseViewport->kaHeight;
    Cratio = (double) Width/Height;

    if (Cratio > Vratio)
        Width *= 1.1;
    else
        Width = 1.1*Height*Vratio;

    InitCoarseWindow(X,Y,Width);
    InitFineWindow(X,Y);
    SetPositioning();
}


static void
newtok(p,t)

char **p, *t;
{
    char *s = *p;

    while (isspace(*s)) s++;
    while (*s && !isspace(*s)) *t++ = *s++;
    *t = '\0';
    while (isspace(*s)) s++;
    *p = s;
}


int
FixCellName(cname)

/* Strip off any path prefix and add to search path if not already
 * there.
 */
char *cname;
{
    char *c, *p;
    char tok[81];
    int i, len, ret = 0;

    c = strrchr(cname,DIRC);
    if (!c)
        return (0);
    *c = '\0';

    p = PGetPath();
    len = strlen(p);
    while (*p) {
        newtok(&p,tok);
#ifdef MSDOS
        if (!stricmp(tok,cname))
#else
        if (!strcmp(tok,cname))
#endif
            goto done;
    }
    /* length of path string from cd/paths.c */
    if (len + strlen(cname) + 1 < 512)
        strcpy(p,cname);
    else
        ret = 1;

done:
    for (i = 0, c++; *c; i++, c++)
        cname[i] = *c;
    cname[i] = '\0';
    return (ret);
}

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
 * Zoom, pan, window, etc.
 * 
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuPAN;
extern char *MenuZOOM;
extern char *MenuWINDO;
extern char *MenuLAST;
extern char *MenuEXPND;
extern char *MenuPEEK;


void
Pan(LookedAhead)

int *LookedAhead;
{
    MenuSelect(MenuPAN);
    ShowPrompt("Point to center of new window.");
    loop {
        switch (PointLoopSafe(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            MenuDeselect(MenuPAN);
            ErasePrompt();
            return;
        case PL_PCW:
            SaveLastView();
            InitCoarseWindow(KicCursor.kcX,KicCursor.kcY,
                (int)View->kvCoarseWindow->kaWidth);
            InitFineWindow(KicCursor.kcX, KicCursor.kcY);
            RedisplayViewports();
            FBTransfer();
        }
    }
}


void
Zoom(LookedAhead)

int *LookedAhead;
{
    char *TypeIn;
    int NewWindowWidth;
    int X,Y;

    MenuSelect(MenuZOOM);
    ShowPrompt("New window width? (RETURN toggles full screen display) ");
    TypeIn = FBEdit(NULL);
    if (TypeIn == NULL) { /* esc entered */
        MenuDeselect(MenuZOOM);
        ErasePrompt();
        return;
    }
    SaveLastView();
    if ((sscanf(TypeIn,"%d",&NewWindowWidth) < 1) Or NewWindowWidth <= 0) {
        if (View->kvControl != FULLSCREEN) {
            View->kvControl = FULLSCREEN;
            Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
            NewWindowWidth = View->kvFineWindow->kaWidth;
            View->kvCoarseWindow->kaX = View->kvFineWindow->kaX;
            View->kvCoarseWindow->kaY = View->kvFineWindow->kaY;
            InitCoarseWindow(View->kvCoarseWindow->kaX,
                View->kvCoarseWindow->kaY,NewWindowWidth);
            SetPositioning();
        }
        else {
            Parameters.kpRedisplayControl = SPLITSCREEN;
            View->kvControl = SPLITSCREEN;
            X = View->kvCoarseWindow->kaX;
            Y = View->kvCoarseWindow->kaY;
            NewWindowWidth = View->kvCoarseWindow->kaWidth;
            SetPositioning();
            CenterFullView();
            if (NewWindowWidth > View->kvCoarseWindow->kaWidth)
                NewWindowWidth = 3*View->kvCoarseWindow->kaWidth/4;
            View->kvFineWindow->kaWidth = NewWindowWidth;
            InitFineWindow(X,Y);
        }
    }
    else {
        NewWindowWidth *= RESOLUTION;
        if (View->kvControl == FULLSCREEN)
            InitCoarseWindow(View->kvCoarseWindow->kaX,
                View->kvCoarseWindow->kaY,NewWindowWidth);
        else
            /* zoom from fine window position */
            InitCoarseWindow(View->kvFineWindow->kaX,
                View->kvFineWindow->kaY,NewWindowWidth);
        SetPositioning();
    }
    ShowParameters();
    RedisplayViewports();
    MenuDeselect(MenuZOOM);
    *LookedAhead = False;
    ErasePrompt();
    FBTransfer();
    return;
}


void
Windo(LookedAhead)

int *LookedAhead;
{
    int NewWindowWidth, Width, Height, Tmp;
    MenuSelect(MenuWINDO);
    loop {
        ShowPrompt("Point to endpoints of diagonal.");
        switch (PointLoopSafe(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_PCW:
            break;
        }
        FBSetRubberBanding('R');
        switch (PointLoopSafe(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            FBSetRubberBanding(0);
            goto quit;
        case PL_PCW:
            FBSetRubberBanding(0);
            break;
        }
        SaveLastView();
        /*
         * Coarse window height is smaller than its width.
         * When user points to endpoints of diagonal to define new window,
         * he expects to see everything in this window after redisplay.
         * To make this happen, we have to EXPAND the width enough so that
         * the height that InitCoarseWindow computes is equal to the height
         * the user wants.
         * The following lines compute the multiplier that expands the
         * width by the right amount.
         */

        Width = KicCursor.kcX - KicCursor.kcPredX;
        Height = KicCursor.kcY - KicCursor.kcPredY;
        if (Width < 0) Width = -Width;
        if (Height < 0) Height = -Height;

        Tmp = Height * View->kvCoarseViewport->kaWidth/
            View->kvCoarseViewport->kaHeight;
        NewWindowWidth = max(Width,Tmp);

        Height /= 2;
        Width /= 2;

        if (NewWindowWidth <= 0)    NewWindowWidth = RESOLUTION;
        InitCoarseWindow( min(KicCursor.kcX,KicCursor.kcPredX)+Width,
            min(KicCursor.kcY,KicCursor.kcPredY)+Height,NewWindowWidth);

        InitFineWindow(min(KicCursor.kcX,KicCursor.kcPredX)+Width,
            min(KicCursor.kcY,KicCursor.kcPredY)+Height);

        SetPositioning();
        ShowParameters();
        RedisplayViewports();
        FBTransfer();
    }
quit:
    MenuDeselect(MenuWINDO);
    ErasePrompt();
}

/* The three routines that follow have been modified to keep
 * track of the the display status of the last view (^T), and the fine
 * viewport magnification.  The last view is updated after every change
 * of either window.  Also, the saved window name is assigned a
 * letter instead of prompting the user for a name.
 */

void
LastView()

{
    int  i;
    struct kw Last;
    struct kw *Tmp;
    char oldMenu;
    int  EscWasReturned = 0;

    Last.kwLastWindowX           = View->kvCoarseWindow->kaX;
    Last.kwLastWindowY           = View->kvCoarseWindow->kaY;
    Last.kwLastFineWindowX       = View->kvFineWindow->kaX;
    Last.kwLastFineWindowY       = View->kvFineWindow->kaY;
    Last.kwLastWindowWidth       = View->kvCoarseWindow->kaWidth;
    Last.kwLastFineWindowWidth   = View->kvFineWindow->kaWidth;
    Last.kwExpand                = Parameters.kpExpandInstances;
    Last.kwExpandFineOnly        = Parameters.kpExpandFineViewportOnly;
    (Last.kwName)[5]             = View->kvFineViewportOnBottom;
    (Last.kwName)[6]             = View->kvControl;

    i = 0;
    for (Tmp = Parameters.kpWindowStack; Tmp; Tmp = Tmp->kwNext)
        AmbiguityMenu[i++].mEntry = Tmp->kwName;
    AmbiguityMenu[i].mEntry = NULL;
    FixMenuPrefix(AmbiguityMenu);

    if (i > 1) {
        /* Show ambiguity menu. */
        oldMenu = Parameters.kpMenu;
        Parameters.kpMenu = AMBIGUITYMENU;
        ShowMenu(AmbiguityMenu);

        /* Which viewport is user interested in? */
        ShowPrompt("Point to the name of the desired view.");
        loop {
            int dummy;
            switch (PointLoopSafe(&dummy)) {
            case PL_ESC:
                EscWasReturned = 1;
                break;
            case PL_CMD:
                break;
            case PL_PCW:
                ShowPrompt("You aren't pointing at the menu.");
                continue;
            }
            break;
        }
        Parameters.kpMenu = oldMenu;
        ShowCommandMenu();
        if (EscWasReturned) goto quit;

        /* find new view */
        for (Tmp = Parameters.kpWindowStack; Tmp; Tmp = Tmp->kwNext)
            if (strncmp(Tmp->kwName,Parameters.kpCommand,
                MenuViewport.kaRight-MenuViewport.kaLeft+1) == 0){
                break;
            }
    }
    else
        Tmp = Parameters.kpWindowStack;

    /* The status of the fine window positioning is stored in
     * the name field, as is full screen mode flag.
     */
    View->kvFineViewportOnBottom = (Tmp->kwName)[5];
    View->kvControl = (Tmp->kwName)[6];

    InitViewport(); 

    /* change to new viewport */
    InitCoarseWindow(Tmp->kwLastWindowX,Tmp->kwLastWindowY,
        Tmp->kwLastWindowWidth);

    SetPositioning();

    InitFineWindow(Tmp->kwLastFineWindowX,Tmp->kwLastFineWindowY);

    if (Tmp->kwExpand) {
        MenuSelect(MenuEXPND);
        Parameters.kpExpandInstances = True;
    }
    else {
        MenuDeselect(MenuEXPND);
        Parameters.kpExpandInstances = False;
    }
    if (Tmp->kwExpandFineOnly) {
        MenuSelect(MenuPEEK);
        Parameters.kpExpandFineViewportOnly = True;
    }
    else {
        MenuDeselect(MenuPEEK);
        Parameters.kpExpandFineViewportOnly = False;
    }

    Parameters.kpWindowStack->kwLastWindowX         = Last.kwLastWindowX;        
    Parameters.kpWindowStack->kwLastWindowY         = Last.kwLastWindowY;        
    Parameters.kpWindowStack->kwLastFineWindowX     = Last.kwLastFineWindowX;    
    Parameters.kpWindowStack->kwLastFineWindowY     = Last.kwLastFineWindowY;    
    Parameters.kpWindowStack->kwLastWindowWidth     = Last.kwLastWindowWidth;    
    Parameters.kpWindowStack->kwLastFineWindowWidth = Last.kwLastFineWindowWidth;
    Parameters.kpWindowStack->kwExpand              = Last.kwExpand;
    Parameters.kpWindowStack->kwExpandFineOnly      = Last.kwExpandFineOnly;
    (Parameters.kpWindowStack->kwName)[5]           = (Last.kwName)[5];          
    (Parameters.kpWindowStack->kwName)[6]           = (Last.kwName)[6];

    ShowParameters();
    RedisplayViewports();
    FBTransfer();
quit:
    MenuDeselect(MenuLAST);
    ErasePrompt();
}


void
RestoreLastView()

{
    struct kw *Tmp;

    Tmp = Parameters.kpWindowStack;

    /* The status of the fine window positioning is stored in
     * the name field, as is full screen mode flag.
     */
    View->kvFineViewportOnBottom = (Tmp->kwName)[5];
    View->kvControl = (Tmp->kwName)[6];

    InitViewport(); 

    /* change to new viewport */
    InitCoarseWindow(Tmp->kwLastWindowX,Tmp->kwLastWindowY,
        Tmp->kwLastWindowWidth);

    SetPositioning();

    InitFineWindow(Tmp->kwLastFineWindowX,Tmp->kwLastFineWindowY);

    if (Tmp->kwExpand) {
        MenuSelect(MenuEXPND);
        Parameters.kpExpandInstances = True;
    }
    else {
        MenuDeselect(MenuEXPND);
        Parameters.kpExpandInstances = False;
    }
    if (Tmp->kwExpandFineOnly) {
        MenuSelect(MenuPEEK);
        Parameters.kpExpandFineViewportOnly = True;
    }
    else {
        MenuDeselect(MenuPEEK);
        Parameters.kpExpandFineViewportOnly = False;
    }
    ShowParameters();
    RedisplayViewports();
}


void
SaveLastView()

{
    /*
     * Parameters.kpWindowStack is always the last view.
     */
    if (Parameters.kpWindowStack == NULL) {
        if ((Parameters.kpWindowStack = alloc(kw)) == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        strcpy(Parameters.kpWindowStack->kwName,"prev");
        Parameters.kpWindowStack->kwNext = NULL;
    }
    Parameters.kpWindowStack->kwLastWindowX     = View->kvCoarseWindow->kaX;
    Parameters.kpWindowStack->kwLastWindowY     = View->kvCoarseWindow->kaY;
    Parameters.kpWindowStack->kwLastFineWindowX = View->kvFineWindow->kaX;
    Parameters.kpWindowStack->kwLastFineWindowY = View->kvFineWindow->kaY;
    Parameters.kpWindowStack->kwLastWindowWidth = 
        View->kvCoarseWindow->kaWidth;
    Parameters.kpWindowStack->kwLastFineWindowWidth =
        View->kvFineWindow->kaWidth;
    Parameters.kpWindowStack->kwExpand  = Parameters.kpExpandInstances;
    Parameters.kpWindowStack->kwExpandFineOnly =
        Parameters.kpExpandFineViewportOnly;
    (Parameters.kpWindowStack->kwName)[5] = View->kvFineViewportOnBottom;
    (Parameters.kpWindowStack->kwName)[6] = View->kvControl;
}


void
SaveViewOnStack()

{
    int i = 0;
    char buf[32];
    struct kw *New, *Tmp;

    New = alloc(kw);
    if (New == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }

    New->kwLastWindowX         = View->kvCoarseWindow->kaX;
    New->kwLastWindowY         = View->kvCoarseWindow->kaY;
    New->kwLastFineWindowX     = View->kvFineWindow->kaX;
    New->kwLastFineWindowY     = View->kvFineWindow->kaY;
    New->kwLastWindowWidth     = View->kvCoarseWindow->kaWidth;
    New->kwLastFineWindowWidth = View->kvFineWindow->kaWidth;
    New->kwExpand              = Parameters.kpExpandInstances;
    New->kwExpandFineOnly      = Parameters.kpExpandFineViewportOnly;
    (New->kwName)[5]           = View->kvFineViewportOnBottom;
    (New->kwName)[6]           = View->kvControl;

    if (Parameters.kpWindowStack == NULL)
        SaveLastView();

    New->kwNext = NULL;
    for (Tmp = Parameters.kpWindowStack; Tmp->kwNext; i++,Tmp = Tmp->kwNext) ;
    Tmp->kwNext = New;
    sprintf(buf,"Current view assigned to: %c",'A'+i);
    ShowPrompt(buf);
    New->kwName[0] = 'a'+ i;
    New->kwName[1] = '\0';
}

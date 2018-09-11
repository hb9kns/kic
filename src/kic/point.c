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
 * Graphical and keyboard input.
 */

#include "prefix.h"
#include "kic.h"

#define DEBOUNCETIME 100

/* button1: basic point operation
 * button2: pan operation
 * button3: window operation
 * button4: return coords, no operation
 */

/* Point() command characters */
#define ESC 27
#define BSP '\b'
#define DEL 127
#define NEWL 13
#define SPA  32
#define CTRL_A 1
#define CTRL_C 3
#define CTRL_E 5
#define CTRL_F 6
#define CTRL_G 7
#define CTRL_L 12
#define CTRL_N 14
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24

static int OldButton3;  /* remember last button 3 */
static int OldButton3X,OldButton3Y;

#define Matching(string) !strcmp(Parameters.kpCommand,string)

/* we keep track of the time between pointing events to debounce the cursor */
static unsigned int LastPointTime = 0;

/* SRW ** disable screen modifying functions such as scaling, pan. */
int LockOut;

/* SRW ** return from Point() if esc entered */
int EscReturn;

#ifdef __STDC__
static void show_sq_info(void);
static int  button_press(int,int,int);
static void new_fine_window(int,int,int,int);
static void type_coordinate(void);
static void where_am_i(int,int,int);
static int  ctrl_at(int,int);
static int  is_pcw(int,int);
#else
static void show_sq_info();
static int  button_press();
static void new_fine_window();
static void type_coordinate();
static void where_am_i();
static int  ctrl_at();
static int  is_pcw();
#endif

extern char *VersionString;

void
Point()

{
    /*
     * When user has typed Condition, do Action.
     *
     * Condition
     *     shortest unique prefix of Menu[i]
     * Action
     *     Return with command selected stored in Parameters.kpCommand.
     *
     * Condition
     *     ESC
     * Action
     *     Forget remembered type in.
     *
     * Condition
     *     ctrl-a
     * Action
     *     Execute abort routine.
     *
     * Condition
     *     point key or tablet stylus button Z 
     * Action
     *     If user is pointing at a layer in the layer table viewport,
     *     change the current layer.
     *     If user is pointing at a command menu selection, return with
     *     command selected stored as Parameters.kpCommand.
     *     If user is pointing inside a layout viewport--coarse or fine--
     *     return with Parameters.kpCommand[0] == EOS and cursor
     *     descriptor up-to-date.
     *
     * Condition
     *     ctrl-c or ctrl-e
     * Action
     *     Prompt user for a coordinate.
     *     Return with Parameters.kpCommand[0] == EOS and cursor
     *     descriptor up-to-date.
     *
     * Condition
     *     ctrl-f or tablet stylus button 1
     * Action
     *     Wait for user to point.
     *     Redisplay in fine viewport around where he pointed
     *     if fine positioning is enabled, or pan otherwise.
     *
     * Condition
     *     ctrl-g
     * Action
     *     Change scale of magnifying glass, or otherwise window, using
     *     next two point actions.
     *
     * Condition
     *     ctrl-l
     * Action
     *     Select current layer from keyboard
     *
     * Condition
     *     ctrl-n
     * Action
     *     Save the present view context in a list.
     *
     * Condition
     *     ctrl-t or ctrl-v
     * Action
     *     Toggle position of magnifying glass, bottom or right.
     *
     * Condition
     *     ctrl-w or tablet stylus button 2
     * Action
     *     Wait for user to point.
     *     Show location, but perform no action.
     */

    char *TypeIn;
    MENU *Menu;
    unsigned int newtime;
    int NumCommand,Buttons,Int1,Int2,Int3;
    int Layer;
    int X,Y;
    int Key;
    extern char *MenuEXIT;

    show_sq_info();

    /*
     * The best way to handle interrupts reliably is to 
     * initialize the service routines as frequently as
     * possible.  Therefore, . . .
     */
    InitSignals();
    Menu = GetCurrentMenu();
    Parameters.kpCommand[NumCommand = 0] = EOS;
    Parameters.kpPointLayerTable = False;
    Parameters.kpPointCoarseWindow = False;
    if (OldButton3) {
        OldButton3 = False;
        FBSetRubberBanding(0);
    }
    LastPointTime = FBTime();

    loop {
        loop {
            FBPoint(&X,&Y,&Key,&Buttons);
            if (Key != 0) {
                /* convert to lower case */
                if (isupper(Key)) Key = tolower(Key);
                break;
            }
            if (FB.fButtons) {
                if ((Buttons == FB.fButtonMask[0] Or
                    Buttons == FB.fButtonMask[1] Or
                    Buttons == FB.fButtonMask[2] Or
                    Buttons == FB.fButtonMask[3]) And
                    (X < FB.fMaxX And X > 0 And Y < FB.fMaxY And Y > 0))
                    break;
            }
        }

        if (Xcheck()) {
            /* debouncing is done in msdos graphis library, otherwise do it
             * here
             */
            newtime = FBTime();
            if (newtime > LastPointTime &&
                    newtime - LastPointTime < DEBOUNCETIME)
                continue;
            LastPointTime = newtime;
        }

        switch (Key) { 

        case 0:
            break;

        case BSP:
        case DEL:
            if (NumCommand) NumCommand--;
            Parameters.kpCommand[NumCommand] = EOS;
            continue;

        case ESC:
            Parameters.kpCommand[NumCommand = 0] = EOS;
            /* SRW ** so we know if esc was entered */
            Parameters.kpCommand[1] = ESC;
            if (EscReturn) return;
            continue;

        case NEWL:
            Parameters.kpCommand[NumCommand = 0] = EOS;
            if (ctrl_at(X,Y)) return;
            continue;

        case '!':
            /* shell command */
            if (LockOut) continue; /* ignore */
            ShowPrompt("! ");
            TypeIn = FBEdit(NULL);
            if (TypeIn != NULL)
                ShowProcess(TypeIn);
            Parameters.kpCommand[NumCommand = 0] = EOS;
            continue;

        case CTRL_A:
            /* SRW ** abort gracefully */
            strcpy(Parameters.kpCommand,MenuEXIT);
            NumCommand = 0;
            return;

        case CTRL_C:
        case CTRL_E:
            type_coordinate();
            Parameters.kpCommand[NumCommand = 0] = EOS;
            return;

        case CTRL_F:
            if (LockOut) continue;  /* ignore ^F */
            Parameters.kpCommand[NumCommand = 0] = EOS;
            SaveLastView();
            /* SRW ** pan if coarse viewport only */
            if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY) {
                if (!InBox(X,Y,View->kvCoarseViewport)) {
                    NotPointingAtLayout();
                    continue;
                }
                PToL(View->kvCoarseWindow,&X,&Y);
                InitCoarseWindow(X,Y,(int)View->kvCoarseWindow->kaWidth);
                InitFineWindow(X,Y);
                RedisplayViewports();
            }
            else
                FinePosition(X,Y,Key);
            /*
             * This is necessary for debouncing.
             * It takes time to redisplay, so we set the time of the last
             * pointing event when the redisplay is finished.
             */
            LastPointTime = FBTime();
            continue;

        case CTRL_G:
            if (LockOut) continue; /* ignore ^G */
            Parameters.kpCommand[NumCommand = 0] = EOS;
            if (Parameters.kpCellName[0] == EOS)
                return;
            loop {
                ShowPrompt("Point to diagonal of area to be magnified.");
                FBPoint(&X,&Y,&Key,&Buttons);
                if (Key == ESC) goto skip;
                if (FB.fButtons And Buttons == FB.fButtonMask[0]) {
                    if (InBox(X,Y,View->kvCoarseViewport))
                        PToL(View->kvCoarseWindow,&X,&Y);
                    elif (InBox(X,Y,View->kvFineViewport))
                        PToL(View->kvFineWindow,&X,&Y);
                    else {
                        NotPointingAtLayout();
                        continue;
                    }
                    OldButton3X = X;
                    OldButton3Y = Y;
                    FBSetRubberBanding('R');
                    break;
                }
            }
            loop {
                ShowPrompt("Point to second endpoint.");
                FBPoint(&X,&Y,&Key,&Buttons);
                if (Key == ESC) {
                    FBSetRubberBanding(0);
                    goto skip;
                }
                if (FB.fButtons And Buttons == FB.fButtonMask[0]) {
                    if (InBox(X,Y,View->kvCoarseViewport))
                        PToL(View->kvCoarseWindow,&X,&Y);
                    elif (InBox(X,Y,View->kvFineViewport))
                        PToL(View->kvFineWindow,&X,&Y);
                    else {
                        NotPointingAtLayout();
                        continue;
                    }
                    FBSetRubberBanding(0);
                    break;
                }
            }
            new_fine_window(OldButton3X,OldButton3Y,X,Y);
skip:
            ErasePrompt();
            continue;

        case CTRL_L:
            ShowPrompt("Layer #?");
            Layer = 1;
            TypeIn = FBEdit(NULL);
            if (TypeIn != NULL)
                sscanf(TypeIn,"%d",&Layer);
            PointLayerTable(LayerTableViewport.kaBottom,(Layer-1)*6+1);
            Parameters.kpCommand[NumCommand = 0] = EOS;
            ErasePrompt();
            return;

        case CTRL_N:
            Parameters.kpCommand[NumCommand = 0] = EOS;
            SaveViewOnStack();
            continue;

        case CTRL_U:
        case CTRL_X:
            NumCommand = 0;
            Parameters.kpCommand[0] = EOS;
            continue;

        case CTRL_V:
            sprintf(TypeOut,
                "KIC-%s  distrib. by Whiteley Research Inc. www.wrcad.com",
                VersionString);
            ShowPrompt(TypeOut);
            continue;
        case CTRL_T:
            if (LockOut) continue;  /* ignore */
            Parameters.kpCommand[NumCommand = 0] = EOS;
            View->kvFineViewportOnBottom ^= 1;
            InitViewport();
            if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY)
                continue;
            SetPositioning();
            ShowParameters();
            RedisplayViewports();
            continue;

        case CTRL_W:
            where_am_i(X,Y,Key);
            Parameters.kpCommand[NumCommand = 0] = EOS;
            continue;

        case SPA:
            where_am_i(X,Y,(char)0);
            Parameters.kpCommand[NumCommand = 0] = EOS;
            continue;

        default:
            Parameters.kpCommand[NumCommand++] = Key; 
            if (NumCommand > 80) NumCommand = 80;
            Parameters.kpCommand[NumCommand] = EOS;
            /*
             * Test for shortest unique prefix, or prefix matching upper
             * case part of menu.  Stupid search is plenty fast. 
             */
            Int3 = -1;
            Int2 = 0;
            for (Int1 = 0; Menu[Int1].mEntry != NULL; Int1++) {
                for (Int2 = 0; Int2 < NumCommand; Int2++) {
                    char c = Menu[Int1].mPrefix[Int2];
                    if (isupper(c)) c = tolower(c);
                    if (Parameters.kpCommand[Int2] != c) break;
                }
                if (Parameters.kpCommand[Int2] == EOS And Int2 > 0) {
                    if (!Menu[Int1].mPrefix[Int2]) {
                        /* found a match */
                        if (Int3 >= 0) {
                            /* oops, more than 1 match */
                            Int2 = -1;
                            break;
                        }
                        Int3 = Int1;
                    }
                }
            }
            if (Int3 >= 0 && Int2 >= 0) {
                strcpy(Parameters.kpCommand,Menu[Int3].mEntry);
                return;
            }
            continue;
        }
        NumCommand = 0;
        if (button_press(Buttons,X,Y))
            return;
    }
}


static void
show_sq_info()
{
    struct ks *s, *ss;
    int Row,Col,cnt;

    /* put message in properties line, flushed right */
    Row = FB.fNumRows - 1;
    Col = FB.fNumColumns - 10;
    OutlineText(Col,Row,FB.fNumColumns,Row,FILL,ERASE,0);
    if (SelectQHead) {

        for (cnt = 0, s = SelectQHead; s; s = s->ksSucc)
            if (s->ksPointer->oInfo == SQ_OLDSEL ||
                s->ksPointer->oInfo == SQ_NEWSEL) {
                cnt++;
                /* check for dups */
                for (ss = s->ksSucc; ss; ss = ss->ksSucc) {
                    if (s->ksPointer == ss->ksPointer) {
                        cnt--;
                        break;
                    }
                }
            }

        if (cnt) {
            FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
            FBText(ROW_COLUMN,Row,Col,"select:");
            sprintf(TypeOut,"%d",cnt);
            FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
            FBText(ROW_COLUMN,Row,Col + 7,TypeOut);
        }
    }
}


static int
button_press(Buttons,X,Y)
int Buttons;
int X,Y;
{

    Parameters.kpCommand[0] = EOS;
    if (Buttons == 0) {
        /* shouldn't get here unless null from keyboard */
        return (False);
    } 
    if (Buttons == FB.fButtonMask[0]) {
        if (ctrl_at(X,Y)) return (True);
        return (False);
    }
    if (Buttons == FB.fButtonMask[1]) {
        if (LockOut) {              /* treat like button 0 */
            Buttons = FB.fButtonMask[0];
            if (ctrl_at(X,Y)) return (True);
            return (False);
        }
        SaveLastView();
        /* pan if coarse viewport only */
        if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY) {
            if (!InBox(X,Y,View->kvCoarseViewport)) {
                NotPointingAtLayout();
                return (False);
            }
            PToL(View->kvCoarseWindow,&X,&Y);
            InitCoarseWindow(X,Y,(int)View->kvCoarseWindow->kaWidth);
            InitFineWindow(X,Y);
            RedisplayViewports();
        }
        else
            FinePosition(X,Y,(char)0);
        LastPointTime = FBTime();
        return (False);
    }
    if (Buttons == FB.fButtonMask[2]) {
        where_am_i(X,Y,(char)0);
        return (False);
    }
    if (Buttons == FB.fButtonMask[3]) {
        if (Parameters.kpCellName[0] == EOS) return (False);
        if (LockOut) {  /* treat like button 0 */
            Buttons = FB.fButtonMask[0];
            if (ctrl_at(X,Y)) return (True);
            return (False);
        }
        if (Not OldButton3) {
            if (InBox(X,Y,View->kvCoarseViewport))
                PToL(View->kvCoarseWindow,&X,&Y);
            elif (InBox(X,Y,View->kvFineViewport))
                PToL(View->kvFineWindow,&X,&Y);
            else {
                NotPointingAtLayout();
                return (False);
            }
            OldButton3X = KicCursor.kcRawX;
            OldButton3Y = KicCursor.kcRawY;
            KicCursor.kcRawX = X;
            KicCursor.kcRawY = Y;
            FBSetRubberBanding('R');
            KicCursor.kcRawX = OldButton3X;
            KicCursor.kcRawY = OldButton3Y;
            OldButton3X = X;
            OldButton3Y = Y;
            OldButton3 = True;
        }
        else {
            if (InBox(X,Y,View->kvCoarseViewport))
                PToL(View->kvCoarseWindow,&X,&Y);
            elif (InBox(X,Y,View->kvFineViewport))
                PToL(View->kvFineWindow,&X,&Y);
            else {
                NotPointingAtLayout();
                return (False);
            }
            FBSetRubberBanding(0);
            OldButton3 = False;
            new_fine_window(OldButton3X,OldButton3Y,X,Y);
        }
        return (False);
    }
    if (Buttons == FB.fButtonMask[4]) {
        return (False);
    }
    return (False);
}



int
PointLoop(LookedAhead)

/* Loop until UNDO, a "non-safe" command, point to coarse window, or
 * ESC is entered.  Return value used for dispatching in non-safe (i.e.,
 * cell modifying) commands.
 */
int *LookedAhead;
{
    extern char *MenuUNDO;

    loop {
        if (*LookedAhead == False) {
            EscReturn = True;
            Point();
            EscReturn = False;
        }
        else
            *LookedAhead = False;
        if (Parameters.kpCommand[1] == ESC)
            return (PL_ESC);
        if (Parameters.kpCommand[0] != EOS) {
            if (SafeCmds(LookedAhead))
                continue;
            if (Matching(MenuUNDO)) return (PL_UND);
            *LookedAhead = True;
            return (PL_CMD);
        }
        if (Parameters.kpPointLayerTable)
            continue;
        if (Parameters.kpPointCoarseWindow)
            return (PL_PCW);
        NotPointingAtLayout();
    }
}


int
PointLoopCreate(LookedAhead)

/* Same as PointLoop(), but returns PL_PLT when the layer table is
 * pointed to.  This enables routines that are creating geometry to
 * redraw with the correct layer during object creation.
 */
int *LookedAhead;
{
    extern char *MenuUNDO;

    loop {
        if (*LookedAhead == False) {
            EscReturn = True;
            Point();
            EscReturn = False;
        }
        else
            *LookedAhead = False;
        if (Parameters.kpCommand[1] == ESC)
            return (PL_ESC);
        if (Parameters.kpCommand[0] != EOS) {
            if (SafeCmds(LookedAhead))
                continue;
            if (Matching(MenuUNDO)) return (PL_UND);
            *LookedAhead = True;
            return (PL_CMD);
        }
        if (Parameters.kpPointLayerTable)
            return (PL_PLT);
        if (Parameters.kpPointCoarseWindow)
            return (PL_PCW);
        NotPointingAtLayout();
    }
}


int
PointLoopSafe(LookedAhead)

/* Loop until UNDO, any  command, point to coarse window, or
 * ESC is entered.  Return value used for dispatching in safe
 * (i.e. non cell modifying) commands.
 */
int *LookedAhead;
{

    loop {
        EscReturn = True;
        Point();
        EscReturn = False;
        if (Parameters.kpCommand[1] == ESC)
            return (PL_ESC);
        if (Parameters.kpCommand[0] != EOS) {
            *LookedAhead = True;
            return (PL_CMD);
        }
        if (Parameters.kpPointLayerTable)
            continue;
        if (Parameters.kpPointCoarseWindow)
            return (PL_PCW);
        NotPointingAtLayout();
    }
}


int
PointLoopLayer(LookedAhead)

/* Loop until any command, point to layer menu, or
 * ESC is entered.  Return value used for dispatching in
 * layer selection commands.
 */
int *LookedAhead;
{

    loop {
        EscReturn = True;
        Point();
        EscReturn = False;
        if (Parameters.kpCommand[1] == ESC)
            return (PL_ESC);
        if (Parameters.kpCommand[0] != EOS) {
            *LookedAhead = True;
            return (PL_CMD);
        }
        if (Parameters.kpPointLayerTable)
            return (PL_PLT);
        if (Parameters.kpPointCoarseWindow)
            return (PL_PCW);
    }
}


void
NotPointingAtLayout()

{
    ShowPromptAndWait("You aren't pointing in the layout viewport.");
}


void
RedisplayKIC()

/* Redisplay what is on the screen. */
{
    if (RepaintFILL())
        return;
    if (RepaintMore())
        return;
    FullRedisplay();
}


void
FullRedisplay()

/* This is called after a resizing, or after the screen has been
 * seriously messed with.  We reinititialize everything, as the
 * parameters may have changed.  RedrawPrompt should take care of
 * anything on the prompt line, including a return from FBGetchar()
 * in the DISPLAY mode.  FBKbRepaint() redraws the editing input
 * if there was any.
 */
{
    FBBegin(FB.fDisplay);
    InitViewport();
    InitCoarseWindow(View->kvCoarseWindow->kaX,View->kvCoarseWindow->kaY,
        (int)View->kvCoarseWindow->kaWidth);
    SetCurrentAOI(View->kvCoarseWindow);
    InitFineWindow(View->kvFineWindow->kaX,View->kvFineWindow->kaY);
    InitVLT();
    FBForeground(ERASE,0);
    FBFlood();
    ShowCommandMenu();
    ShowLayerTable();
    ShowParameters();
    if (Parameters.kpCellName[0] != EOS) {
        Redisplay(View->kvCoarseWindow);
        if (Parameters.kpRedisplayControl == SPLITSCREEN)
            XORfineViewport();
    }
    RedrawPrompt();
    FBKbRepaint((1+FB.fLastCursorColumn)*FB.fFontWidth,
        FB.fMaxY - FB.fFontHeight*(FB.fNumRows-2));
}


void
FinePosition(X,Y,Key)

int X,Y;
int Key;
{
    int Buttons;

    if (Parameters.kpCellName[0] == EOS)
        return;
    if (Not FB.fButtons Or Key != EOS) {
        ShowPrompt("Point to center of area you want magnified.");    
        loop {
            FBPoint(&X,&Y,&Key,&Buttons);
            if (Key == EOS Or Key == NEWL)
                break;
            elif (Key == CTRL_C Or Key == CTRL_E) {
                type_coordinate();
                CoarseLToP(KicCursor.kcX,KicCursor.kcY,X,Y);
                ClipVP(View->kvCoarseViewport,X,Y);
                break;
            }
        }
    }
    if (InBox(X,Y,View->kvCoarseViewport))
        PToL(View->kvCoarseWindow,&X,&Y);
    elif (InBox(X,Y,View->kvFineViewport))
        PToL(View->kvFineWindow,&X,&Y);
    else {
        NotPointingAtLayout();
        return;
    }
    XORfineViewport();
    InitFineWindow(X,Y);
    XORfineViewport();
    ShowFineViewport();
}


static void
new_fine_window(OldX,OldY,X,Y)

int OldX,OldY,X,Y;
{
    int NewWindowWidth, Hei, Wid, Tmp, CenterX, CenterY;

    if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY) {

        SaveLastView();
        Wid = X - OldX;
        Hei = Y - OldY;
        if (Wid < 0) Wid = -Wid;
        if (Hei < 0) Hei = -Hei;
        Tmp = Hei*
            (View->kvCoarseViewport->kaWidth/
            View->kvCoarseViewport->kaHeight);
        NewWindowWidth = max(Wid,Tmp);
        if (NewWindowWidth <= 0)
            NewWindowWidth = RESOLUTION;
        Wid /= 2;
        Hei /= 2;
        InitCoarseWindow(min(X,OldX)+Wid,
            min(Y,OldY)+Hei,NewWindowWidth);
        InitFineWindow(min(X,OldX)+Wid,
            min(Y,OldY)+Hei);
        RedisplayViewports();
        ShowParameters();
    }
    else {
        if (X > OldX)
            SwapInts(X,OldX);
        if ((OldX - X) < 2) { /* two lambda minimum width */
            ShowPrompt("Magnifying glass width too small.");
            return;
        }
        SaveLastView();
        if (Y > OldY)
            SwapInts(Y,OldY);
        CenterX = (OldX - X)/2 + X;
        CenterY = (OldY - Y)/2 + Y;
        View->kvFineWindow->kaWidth = OldX - X;
        View->kvFineWindow->kaHeight =
            View->kvFineWindow->kaWidth*
            (View->kvFineViewport->kaHeight/
            View->kvFineViewport->kaWidth);
        XORfineViewport();
        InitFineWindow(CenterX,CenterY);
        XORfineViewport();
        ShowFineViewport();
    }
}


static void
type_coordinate()

{
    char *TypeIn;
    int i;
    float x, y;

    ShowPrompt("x y? ");
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL) {
        i = sscanf(TypeIn,"%f%f",&x,&y);
        if (i >= 1) {
            KicCursor.kcPredX = KicCursor.kcX;
            KicCursor.kcX  = (int)(RESOLUTION*x);
            KicCursor.kcDX = KicCursor.kcX-KicCursor.kcPredX;
        }
        if (i == 2) {
            KicCursor.kcPredY = KicCursor.kcY;
            KicCursor.kcY = (int)(RESOLUTION*y);
            KicCursor.kcDY = KicCursor.kcY-KicCursor.kcPredY;
        }
        if (i >= 1)
            ShowParameters();
    }
    Parameters.kpPointCoarseWindow = True;
    ErasePrompt();
}


static void
where_am_i(X,Y,Key)

int X,Y;
int Key;
{
    int Buttons;

    if (Not FB.fButtons Or Key != 0) {
        ShowPrompt("Point to see where you are.");
        loop {
            FBPoint(&X,&Y,&Key,&Buttons);
            if (Not FB.fButtons And Key == NEWL)
                break;
            if (FB.fButtons And Buttons == FB.fButtonMask[0] And
                X < FB.fMaxX And X > 0 And Y < FB.fMaxY And Y > 0)
                break;
        }
    }
    if (is_pcw(X,Y))
        return;
    NotPointingAtLayout();
    Parameters.kpCommand[0] = '\0';
    
}


static int
ctrl_at(X,Y)

int X,Y;
{
    int i, Row, Column;
    MENU *Menu;

    if (is_pcw(X,Y)) {
        Parameters.kpPointCoarseWindow = True;
        return (True);
    }
    Menu = GetCurrentMenu();
    Row = (FB.fMaxY-Y-3)/FB.fFontHeight+1;
    Column = X/FB.fFontWidth+1;
    if (InBox(Column,Row,&MenuViewport)) {
        for (i = 0; ; i++)
            if (Menu[i].mEntry == NULL) break;
        if (Column > 5)
            Row += MenuViewport.kaY;
        if (i > Row - 1) {
            strcpy(Parameters.kpCommand,Menu[Row-1].mEntry);
            return (True);
        }
        return (False);
    }
    if (InBox(Column,Row,&LayerTableViewport)) {
        Parameters.kpCommand[0] = '\0';
        return (PointLayerTable(Row,Column));
    }
    return (False);
}


static int
is_pcw(X,Y)

int X,Y;
{
    if (InBox(X,Y,View->kvCoarseViewport)) {
        KicCursor.kcInFine = False;
        KicCursor.kcPredX = KicCursor.kcX;
        KicCursor.kcPredY = KicCursor.kcY;
        KicCursor.kcX = X; 
        KicCursor.kcY = Y; 
        KicCursor.kcRawX = X; 
        KicCursor.kcRawY = Y; 
        PToL(View->kvCoarseWindow,&KicCursor.kcX,&KicCursor.kcY);
        PToL(View->kvCoarseWindow,&KicCursor.kcRawX,&KicCursor.kcRawY);
        ClipToGridPoint(&KicCursor.kcX,&KicCursor.kcY);
        KicCursor.kcDX = KicCursor.kcX-KicCursor.kcPredX;
        KicCursor.kcDY = KicCursor.kcY-KicCursor.kcPredY;
        ShowParameters();
        Parameters.kpCommand[0] = '\0';
        return (True);
    }
    if (InBox(X,Y,View->kvFineViewport)) {
        KicCursor.kcInFine = True;
        KicCursor.kcPredX = KicCursor.kcX;
        KicCursor.kcPredY = KicCursor.kcY;
        KicCursor.kcX = X; 
        KicCursor.kcY = Y; 
        KicCursor.kcRawX = X; 
        KicCursor.kcRawY = Y; 
        PToL(View->kvFineWindow,&KicCursor.kcX,&KicCursor.kcY);
        PToL(View->kvFineWindow,&KicCursor.kcRawX,&KicCursor.kcRawY);
        ClipToGridPoint(&KicCursor.kcX,&KicCursor.kcY);
        KicCursor.kcDX = KicCursor.kcX-KicCursor.kcPredX;
        KicCursor.kcDY = KicCursor.kcY-KicCursor.kcPredY;
        ShowParameters();
        Parameters.kpCommand[0] = '\0';
        return (True);
    }
    return (False);
}

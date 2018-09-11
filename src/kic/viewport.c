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
 * Management of PARAMETER, LAYER TABLE, and MENU viewports.
 * 
 */

#ifdef WIN32
#include <windows.h>
#endif
#include "prefix.h"
#include "kic.h"
#include "sline.h"
#ifndef vms
#ifndef MSDOS
#include <signal.h>
#endif
#endif
#ifndef WIN32
#include <sys/wait.h>
#include <pwd.h>
#endif

#ifdef __STDC__
static void set_active(MENU*);
static void display_entry(MENU*,int);
static void display_selected(MENU*,int);
#if !defined(WIN32) && !defined(MSDOS) && !defined(vms)
static FILE *propen(char*,char*);
static int  prclose(FILE*);
#endif
#else
static void set_active();
static void display_entry();
static void display_selected();
#if !defined(WIN32) && !defined(MSDOS) && !defined(vms)
static FILE *propen();
static int  prclose();
#endif
#endif

#if __NDPC__
extern void sleep();
#endif

/***********************************************************************
 *
 * Routines used to process cursor data.
 * 
 *
 ***********************************************************************/

/* also see macros in coords.h */


void
PToL(Window,X,Y)

/* convert viewport to window coordinates */
struct ka *Window;
int *X, *Y;
{
    if (Window == View->kvCoarseWindow) {
        *X = .5+(*X - View->kvCoarseViewport->kaLeft)/
            View->kvCoarseRatio;
        *X += Window->kaLeft;
        *Y = .5+(*Y - View->kvCoarseViewport->kaBottom)/
            View->kvCoarseRatio;
        *Y += Window->kaBottom;
    }
    elif (Window == View->kvFineWindow) {
        *X = .5+(*X - View->kvFineViewport->kaLeft)/
            View->kvFineRatio;
        *X += Window->kaLeft;
        *Y = .5+(*Y - View->kvFineViewport->kaBottom)/
            View->kvFineRatio;
        *Y += Window->kaBottom;
    }
    else {
        *X = *Y = 0;
        return;
    }
}


void
ClipToGridPoint(x,y)

/* Clip to nearest window grid coordinate */
int *x,*y;
{

    int j,k,l;

    k = Parameters.kpGrid/Parameters.kpPixToLambdaSnapping;

    j = (*x/Parameters.kpGrid)*Parameters.kpGrid;
    if (*x >= 0) {
        l = *x - j;
        l = ((l + k/2)/k)*k;
        *x = j + l;
    }
    else {
        l = j - *x;
        l = ((l + k/2)/k)*k;
        *x = j - l;
    }

    j = (*y/Parameters.kpGrid)*Parameters.kpGrid;
    if (*y >= 0) {
        l = *y - j;
        l = ((l + k/2)/k)*k;
        *y = j + l;
    }
    else {
        l = j - *y;
        l = ((l + k/2)/k)*k;
        *y = j - l;
    }
}


/***********************************************************************
 *
 * Routines to dislay parameter data.
 * 
 *
 ***********************************************************************/

static int RelX, RelY;
static int  Rel;


void
ShowParameters()

{
    int Row,Col;

    Row = FB.fNumRows - 1;
    OutlineText(1,Row,FB.fNumColumns,Row,FILL,ERASE,0);
    FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
    Col = 1;
    if (!Rel) {
        FBText(ROW_COLUMN,Row,Col,"x=");
        sprintf(TypeOut,"%g",(double)KicCursor.kcX/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 2,TypeOut);
        FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
        FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"y=");
        sprintf(TypeOut,"%g",(double)KicCursor.kcY/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 2,TypeOut);
        FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
        FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"dx=");
        sprintf(TypeOut,"%g",(double)KicCursor.kcDX/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 3,TypeOut);
        FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
        FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"dy=");
        sprintf(TypeOut,"%g",(double)KicCursor.kcDY/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 3,TypeOut);
    }
    else {
        FBText(ROW_COLUMN,Row,Col,"x=");
        sprintf(TypeOut,"%g",(double)KicCursor.kcX/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 2,TypeOut);
        FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
        FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"y=");
        sprintf(TypeOut,"%g",(double)KicCursor.kcY/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 2,TypeOut);
        FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
        FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"DX=");
        sprintf(TypeOut,"%g",(double)(KicCursor.kcX-RelX)/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 3,TypeOut);
        FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
        FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"DY=");
        sprintf(TypeOut,"%g",(double)(KicCursor.kcY-RelY)/RESOLUTION);
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 3,TypeOut);
    }

    FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
    FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"width=");
    sprintf(TypeOut,"%g",View->kvCoarseWindow->kaWidth/RESOLUTION);
    FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
    FBText(ROW_COLUMN,Row,Col += 6,TypeOut);
    FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
    FBText(ROW_COLUMN,Row,Col += strlen(TypeOut) + 1,"cell:");
    if (FB.fNumColumns > 52) {
        strncpy(TypeOut,Parameters.kpCellName,FB.fNumColumns-52);
        TypeOut[FB.fNumColumns-52] = EOS;
        FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
        FBText(ROW_COLUMN,Row,Col += 5,TypeOut);
    }
    ShowElectrical();
    FBTransfer();
}


void
SetRelative(X,Y,Flag)

int X,Y;
int Flag;
{
    RelX = X;
    RelY = Y;
    Rel = Flag;
}


void
ShowElectrical()

/* Print some electrical characteristics when making boxes.  Layers in the
 * tech file can have the following entries:
 *
 * resis[tance] <ohm_per_square>
 *
 * cap[acitance] <units_per_square_micron>
 *
 * tran[smission] <line_thick> <line_penet> <grnd_thick> <grnd_penet> \
 *     <diel_thick> <diel_epsilon_rel>
 *
 * Each of the <...> is a number.  Tran parameters are in microns.
 * After each box is made (or the get coordinate button is pressed after the
 * initial corner is located) the appropriate results are printed.
 */

{
    struct eparms *ee;
    char buf[128];
    double Wid, Hei, o[4];
    char c;

    if (!MakingBoxes) return;
    if (!Rel) return;
    if ((ee = LayerTable[Parameters.kpLayer].klElectrical) == NULL) return;
    Wid = fabs((double)(KicCursor.kcX - RelX)/RESOLUTION);
    Hei = fabs((double)(KicCursor.kcY - RelY)/RESOLUTION);

    switch (ee->e_type) {
    case ERESIS:
        if (Wid == 0 || Hei == 0) {
            ErasePrompt();
            break;
        }
        sprintf(buf,"      Resistance: %g (L to R), %g (B to T)",
            *ee->e_parms*Wid/Hei,*ee->e_parms*Hei/Wid);
        ShowPrompt(buf);
        break;
    case ECAP:
        sprintf(buf,"      Capacitance: %g",*ee->e_parms*Wid*Hei);
        ShowPrompt(buf);
        break;
    case ETRANS:
        if (Wid == 0 || Hei == 0) {
            ErasePrompt();
            break;
        }
        if (Wid > Hei) {
            ee->e_parms[6] = Hei;
            ee->e_parms[7] = Wid;
            c = 'X';
        }
        else {
            ee->e_parms[6] = Wid;
            ee->e_parms[7] = Hei;
            c = 'Y';
        }
        sline((struct params*)ee->e_parms,(struct output*)o);
        sprintf(buf,"      Along %c: L=%g  C=%g  Z=%g  T=%g",
            c,o[0],o[1],o[2],o[3]);
        ShowPrompt(buf);
        break;
    }
}


/***********************************************************************
 *
 * Routines to manage layer table.
 * 
 *
 ***********************************************************************/


void
ShowLayerTable()

{
    int Row,Column,Layer,j;
    char LayerName[5];

    LayerName[4] = EOS;

    OutlineText(1,FB.fNumRows,FB.fNumColumns,FB.fNumRows,FILL,ERASE,0);
    for(j = 1; j <= Parameters.kpLayersPerMenuRow; ++j){
        Layer = Parameters.kpVisibleLayerMenuRow *
        Parameters.kpLayersPerMenuRow + j;

        if(Layer <= NumLayerTable){
            Row = FB.fNumRows;
            Column = (j - 1) * 6 + 1;
            FBForeground(DISPLAY,Layer);
            LayerName[0] = LayerTable[Layer].klTechnology;
            LayerName[1] = LayerTable[Layer].klMask[0];
            LayerName[2] = LayerTable[Layer].klMask[1];
            LayerName[3] = LayerTable[Layer].klMask[2];
            FBText(ROW_COLUMN,Row,Column+2,LayerName);
            if (LayerTable[Layer].klAttributes & VISIBLE)
                LtBox(Column,Row,DISPLAY,Layer);
            if (Layer == Parameters.kpLayer)
                OutlineText(Column+2,Row,Column+5,Row,OUTLINE,DISPLAY,
                ColorTable[HighlightingColor].Ent);
        }
    }
    if (Parameters.kpNumLayerMenuRows > 1)
        LtMore();
    FBTransfer();
}


void
LtMore()

{
    int Row,Col;

    FBForeground(DISPLAY,ColorTable[MoreTextColor].Ent);
    Row = FB.fNumRows;
    Col = Parameters.kpLayersPerMenuRow * 6 + 3;
    FBText(ROW_COLUMN,Row,Col,"MORE");
    OutlineText(Col,Row,Col+3,Row,OUTLINE,DISPLAY,
        ColorTable[MenuPromptColor].Ent);
}


void
LtBox(Left,Bottom,DisplayOrErase,Layer)

int Left,Bottom,Layer;
int DisplayOrErase;
{
    /*
     * Draw the layer designator for the layer table
     */
    int Right, Top;

    Bottom = FB.fMaxY + 1 - Bottom*FB.fFontHeight;
    Top    = Bottom + FB.fFontHeight - 1;
    Left   = (Left-1) * FB.fFontWidth;
    Right  = Left + 2*FB.fFontWidth;
    Left  += 2;
    Right -= 2;

    if (DisplayOrErase == ERASE) {
        FBFilledBox(Layer,ERASE,0,Left,Bottom,Right,Top);
        return;
    }
    if (!(LayerTable[Layer].klAttributes & FILLED) ||
        (LayerTable[Layer].klAttributes & OUTLINED))
        FBEmptyBox(Layer,DISPLAY,0,Left,Bottom,Right,Top);
    if (LayerTable[Layer].klAttributes & FILLED)
        FBFilledBox(Layer,DISPLAY,LayerTable[Layer].klStyleID,
            Left,Bottom,Right,Top);
}


int
PointLayerTable(CharRow,CharColumn)

int CharRow, CharColumn;
{
    int Row,Column,Layer,Lmin;
    char LayerName[5];

    LayerName[4] = EOS;
    Column = (CharColumn - 1)/6 + 1;
    if (Column == Parameters.kpLayersPerMenuRow+1){
        Parameters.kpVisibleLayerMenuRow++;
        if (Parameters.kpVisibleLayerMenuRow ==
            Parameters.kpNumLayerMenuRows)
            Parameters.kpVisibleLayerMenuRow = 0;
        ShowLayerTable();
        return False;
    }

    Layer = Parameters.kpVisibleLayerMenuRow *
        Parameters.kpLayersPerMenuRow + Column;
    if(Layer <= NumLayerTable And Column <= Parameters.kpLayersPerMenuRow){
        Parameters.kpPointLayerTable = True;
        LayerName[0] = LayerTable[Parameters.kpLayer].klTechnology;
        LayerName[1] = LayerTable[Parameters.kpLayer].klMask[0];
        LayerName[2] = LayerTable[Parameters.kpLayer].klMask[1];
        LayerName[3] = LayerTable[Parameters.kpLayer].klMask[2];


        Lmin = Parameters.kpVisibleLayerMenuRow *
            Parameters.kpLayersPerMenuRow + 1;
        Row = FB.fNumRows;
        if (Parameters.kpLayer  >= Lmin && Parameters.kpLayer <
            Lmin + Parameters.kpLayersPerMenuRow) {

            Column = ((Parameters.kpLayer-1) %
                Parameters.kpLayersPerMenuRow) * 6;

            OutlineText(Column+3,Row,Column+6,Row,OUTLINE,ERASE,0);
            FBForeground(DISPLAY,Parameters.kpLayer);
            FBText(ROW_COLUMN,Row,Column+3,LayerName);
        }
        Parameters.kpLayer = Layer;

        Column = ((Parameters.kpLayer-1) %
            Parameters.kpLayersPerMenuRow) * 6;

        OutlineText(Column+3,CharRow,Column+6,CharRow,OUTLINE,DISPLAY,
            ColorTable[HighlightingColor].Ent);
        /*
         * A user might have done a redisplay with this layer chosen to
         * be invisible.  If so, make this layer visible.  If this
         * wasn't done, he might create a geometry, but not see it.
         * This would be very annoying!
         */
        if(Not NoMakeVisible &&
            !(LayerTable[Parameters.kpLayer].klAttributes & VISIBLE))
            MakeLayerVisible(Parameters.kpLayer);
    }
    return True;
}


/***********************************************************************
 *
 * Routines to dislay and manipulate menus.
 * 
 *
 ***********************************************************************/


extern char *MenuCNTXT;
extern char *MenuMX;
extern char *MenuMY;
extern char *MenuLAYER;
extern char *MenuEXPND;
extern char *MenuPEEK;
extern char *Menu0;
extern char *Menu90;
extern char *Menu180;
extern char *Menu270;

void
ShowCommandMenu()

{
    if (Parameters.kpMenu == ATTRIBUTESMENU) {
        ShowMenu(AttributeMenu);
        return;
    }
    if (Parameters.kpMenu == BASICMENU) {
        ShowMenu(BasicMenu);
        return;
    }
    if (Parameters.kpMenu == DEBUGMENU) {
        ShowMenu(DebugMenu);
        return;
    }
    if (Parameters.kpMenu == PROPERTYMENU) {
        ShowMenu(PropertyMenu);
        return;
    }
    if (Parameters.kpMenu == AMBIGUITYMENU) {
        ShowMenu(AmbiguityMenu);
        return;
    }
}


void
ShowMenu(Menu)

MENU *Menu;
{
    int Int1;

    /*
     * Erase the entire menu viewport. 
     */
    OutlineText(MenuViewport.kaLeft,MenuViewport.kaBottom,
    MenuViewport.kaRight,MenuViewport.kaTop,FILL,ERASE,0);
    /*
     * Display Menu Table
     */
    set_active(Menu);
    for (Int1 = 0; ; Int1++) {
        if (Menu[Int1].mEntry == NULL) break;
        if (Menu[Int1].mActive)
            display_selected(Menu,Int1);
        else
            display_entry(Menu,Int1);
    }
    FBTransfer();
}


static void
set_active(Menu)

MENU *Menu;
{
    int i;

    i = GetMenuIndex(Menu,MenuCNTXT);
    if (i >= 0) {
        if (Parameters.kpShowContext)
            Menu[i].mActive = True;
        else
            Menu[i].mActive = False;
    }
    i = GetMenuIndex(Menu,MenuMX);
    if (i >= 0) {
        if (Parameters.kpMX)
            Menu[i].mActive = True;
        else
            Menu[i].mActive = False;
    }
    i = GetMenuIndex(Menu,MenuMY);
    if (i >= 0) {
        if (Parameters.kpMY)
            Menu[i].mActive = True;
        else
            Menu[i].mActive = False;
    }
    i = GetMenuIndex(Menu,MenuLAYER);
    if (i >= 0) {
        if (Parameters.kpLayerSpecificSelection)
            Menu[i].mActive = True;
        else
            Menu[i].mActive = False;
    }
    i = GetMenuIndex(Menu,MenuEXPND);
    if (i >= 0) {
        if (Parameters.kpExpandInstances)
            Menu[i].mActive = True;
        else
            Menu[i].mActive = False;
    }
    i = GetMenuIndex(Menu,MenuPEEK);
    if (i >= 0) {
        if (Parameters.kpExpandFineViewportOnly)
            Menu[i].mActive = True;
        else
            Menu[i].mActive = False;
    }
    i = GetMenuIndex(Menu,Menu0);
    if (i >= 0)
        Menu[i].mActive = False;
    i = GetMenuIndex(Menu,Menu90);
    if (i >= 0)
        Menu[i].mActive = True;
    i = GetMenuIndex(Menu,Menu180);
    if (i >= 0)
        Menu[i].mActive = True;
    i = GetMenuIndex(Menu,Menu270);
    if (i >= 0)
        Menu[i].mActive = True;
}


int
GetMenuIndex(Menu,String)

MENU *Menu;
char *String;
{
    int i = 0;

    while (Menu[i].mEntry != NULL) {
        if (!strcmp(Menu[i].mEntry,String)) return i;
        i++;
    }
    return -1;
}


MENU *
GetCurrentMenu()

{
    if (Parameters.kpMenu == AMBIGUITYMENU)
        return AmbiguityMenu;
    if (Parameters.kpMenu == ATTRIBUTESMENU)
        return AttributeMenu;
    if (Parameters.kpMenu == BASICMENU)
        return BasicMenu;
    if (Parameters.kpMenu == DEBUGMENU)
        return DebugMenu;
    if (Parameters.kpMenu == PROPERTYMENU)
        return PropertyMenu;
    return NULL;
}


void
AlterMenuEntries(Word1,Word2)

/* Change all the entries of Word1 in the menus to Word2. */
char *Word1,*Word2;
{
    int i;

    i = GetMenuIndex(AttributeMenu,Word1);
    if (i >= 0) {
        AttributeMenu[i].mEntry = Word2;
        FixMenuPrefix(AttributeMenu);
        if (Parameters.kpMenu == ATTRIBUTESMENU)
            ShowMenu(AttributeMenu);
    }
    i = GetMenuIndex(BasicMenu,Word1);
    if (i >= 0) {
        BasicMenu[i].mEntry = Word2;
        FixMenuPrefix(BasicMenu);
        if (Parameters.kpMenu == BASICMENU)
            ShowMenu(BasicMenu);
    }
    i = GetMenuIndex(DebugMenu,Word1);
    if (i >= 0) {
        DebugMenu[i].mEntry = Word2;
        FixMenuPrefix(DebugMenu);
        if (Parameters.kpMenu == DEBUGMENU)
            ShowMenu(DebugMenu);
    }
    i = GetMenuIndex(PropertyMenu,Word1);
    if (i >= 0) {
        PropertyMenu[i].mEntry = Word2;
        FixMenuPrefix(PropertyMenu);
        if (Parameters.kpMenu == PROPERTYMENU)
            ShowMenu(PropertyMenu);
    }
}


void
MenuSelect(Selection)

char *Selection; 
{
    int Int1;
    MENU *Menu;

    if ((Menu = GetCurrentMenu()) == NULL) return;
    Int1 = GetMenuIndex(Menu,Selection);
    if (Int1 < 0) return;
    Menu[Int1].mActive = True;
    display_selected(Menu,Int1);
    FBTransfer();
}


void
MenuDeselect(Selection)

char *Selection; 
{
    int Int1;
    MENU *Menu;

    if ((Menu = GetCurrentMenu()) == NULL) return;
    Int1 = GetMenuIndex(Menu,Selection);
    if (Int1 < 0) return;
    Menu[Int1].mActive = False;
    display_entry(Menu,Int1);
    FBTransfer();
}


void
FixMenuPrefix(Menu)

MENU *Menu;
{
    int i;

    for (i = 0; Menu[i].mEntry; i++)
        FixMenuEntryPrefix(Menu,i);
}


void
FixMenuEntryPrefix(Menu,Index)

/* Find a unique prefix for the menu entry and save it
 * in the mPrefix field.
 */
MENU *Menu;
int Index;
{
    int Count;
    char m1[32], m2[32];
    int j,k;

    memset(Menu[Index].mPrefix,0,6);

    Count = 1;
    strcpy(m1,Menu[Index].mEntry);
    for (j = 0; Menu[j].mEntry; j++) {
        if (Index == j) continue;
        strcpy(m2,Menu[j].mEntry);
        if (!strcasecmp(m1,m2)) {
            if (*Menu[Index].mEntry != ' ')
                for (k = j; Menu[k].mEntry; k++) {
                    strcpy(Menu[k].mPrefix,Menu[k+1].mPrefix);
                    Menu[k].mEntry = Menu[k+1].mEntry;
                    Menu[k].mActive = Menu[k+1].mActive;
                }
            continue;
        }
        while (!strncasecmp(m1,m2,Count))
            Count++;
    }
    if (Count > 5) Count = 5;
    while (Count--)
        Menu[Index].mPrefix[Count] = Menu[Index].mEntry[Count];
}


static void
display_entry(Menu,Index)

MENU *Menu;
int Index;
{
    char MenuSelection[8],*prefix;
    int i;
    int Left;

    strncpy(MenuSelection,Menu[Index].mEntry,MenuViewport.kaX);
    MenuSelection[MenuViewport.kaX] = '\0';
    prefix = Menu[Index].mPrefix;

    Index += MenuViewport.kaTop;
    Left = MenuViewport.kaLeft;
    if (Index > MenuViewport.kaY) {
        Left += 6;
        Index -= MenuViewport.kaY;
    }
    /*
     * Erase highlight box.
     */
    OutlineText(Left,Index,Left+4,Index,FILL,ERASE,0);
    /*
     * Redisplay menu selection.
     */

    i = strlen(prefix);
    while (i--)
        if (islower(MenuSelection[i]))
            MenuSelection[i] = toupper(MenuSelection[i]);

    FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
    FBText(ROW_COLUMN,Index,Left,MenuSelection);
}


static void
display_selected(Menu,Index)

MENU *Menu;
int Index;
{
    char MenuSelection[8], *prefix, Fill;
    int i;
    int Left;

    strncpy(MenuSelection,Menu[Index].mEntry,MenuViewport.kaX);
    MenuSelection[MenuViewport.kaX] = '\0';
    prefix = Menu[Index].mPrefix;

    Index += MenuViewport.kaTop;
    Left = MenuViewport.kaLeft;
    if (Index > MenuViewport.kaY) {
        Left += 6;
        Index -= MenuViewport.kaY;
    }
    /*
     * Erase Menu command
     */
    OutlineText(Left,Index,Left+4,Index,FILL,ERASE,0);
    if (FB.fNonDestructiveText)
        Fill = FILL;
    else
        Fill = OUTLINE;

    /*
     * Display highlight box.
     */
    OutlineText(Left,Index,Left+4,Index,Fill,DISPLAY,
        ColorTable[MenuHighlightingColor].Ent);

    /*
     * Redisplay menu selection.
     */

    i = strlen(prefix);
    while (i--)
        if (islower(MenuSelection[i]))
            MenuSelection[i] = toupper(MenuSelection[i]);

    FBForeground(DISPLAY,ColorTable[MenuSelectColor].Ent);
    FBText(ROW_COLUMN,Index,Left,MenuSelection);
}


/***********************************************************************
 *
 * Routines to dislay message text.
 * 
 *
 ***********************************************************************/

static char BackPrompt[200];
static char BackColor;


void
ShowPrompt(Prompt)

char *Prompt;
{
    ShowPromptWithColor(Prompt,ColorTable[MenuPromptColor].Ent);
}


void
ShowPromptAndWait(cp)

char *cp;
{
    putchar('\007');
    fflush(stdout);
    ShowPrompt(cp);
#ifdef WIN32
    Sleep(2000);
#else
    sleep(2);
#endif
}


void
ShowPromptWithColor(Prompt,Color)

char *Prompt;
int Color;
{
    /*
     * Implements a basic scroller in a prompt window.
     * Displays MORE when window is full and continues when
     * user hits and key.
     */
    char *cp;
    char buffer[200];
    int Int1;
    int Row;

    Row = FB.fNumRows - 2;
    ErasePrompt();
    FBForeground(DISPLAY,Color);
    BackColor = Color;
    cp = Prompt;
    Int1 = 0;
    while (*cp != EOS) {
        buffer[Int1++] = *cp++;
        if (*cp == EOS) {
            buffer[Int1++] = EOS;
            strcpy(BackPrompt,buffer);
            FBText(ROW_COLUMN,Row,1,buffer);
            FBTransfer();
            FB.fLastCursorColumn = strlen(buffer) + 1;
        }
        elif ((Int1 > FB.fNumColumns - 8) Or (Int1 > 190)) {
            sprintf(&buffer[Int1]," MORE");
            strcpy(BackPrompt,buffer);
            FBText(ROW_COLUMN,Row,1,buffer);
            FBTransfer();
            FB.fLastCursorColumn = strlen(buffer);
            (void)FBGetchar(ERASE);
            ErasePrompt();
            FBForeground(DISPLAY,Color);
            Int1 = 0;
        }
    }
}


void
RedrawPrompt()

{
    if (BackPrompt[0] == '\0') return;
    FBForeground(DISPLAY,BackColor);
    FBText(ROW_COLUMN,FB.fNumRows-2,1,BackPrompt);
    FB.fLastCursorColumn = strlen(BackPrompt);
}


void
AppendToOldPrompt(c)

int c;
{
    char s[4];

    s[0] = ' ';
    s[1] = s[0];
    s[2] = (char)c;
    s[3] = '\0';
    strcat(BackPrompt,s);
}


void
ErasePrompt()

{
    OutlineText(1,FB.fNumRows-2,FB.fNumColumns,
        FB.fNumRows-2,FILL,ERASE,0);
    FB.fLastCursorColumn = 1;
    BackPrompt[0] = '\0';
    FBTransfer();
}


void
OutlineText(Left,Bottom,Right,Top,Type,DisplayOrErase,Pixel)

int Left,Bottom,Right,Top,Pixel;
int Type,DisplayOrErase;
{
    /*
     * Outline the box defined by LeftColumn, BottomRow, RightColumn, and
     * TopRow in the color associated with Pixel.
     */
    Bottom = FB.fMaxY-Bottom*FB.fFontHeight;
    Top = FB.fMaxY - (Top-1)*FB.fFontHeight;
    if (Top > FB.fMaxY) Top = FB.fMaxY;
    Left = (Left-1) * FB.fFontWidth;
    Right = (Right * FB.fFontWidth) + 1;
    if (Bottom < 0)
        Bottom = 0;
    else
        Bottom++;
    FBBox(Pixel,DisplayOrErase,Type,0,Left,Bottom,Right,Top);
}


/***********************************************************************
 *
 * Routines to dislay and erase viewports.
 * 
 *
 ***********************************************************************/


void
EraseLargeCoarseViewport()

{
    /*
     * Why -1?  Major grid points that are centered on the left or bottom
     * edges of the viewport wouldn't be totally erased without -1.
     */
    FBFilledBox(0,ERASE,0,View->kvLargeCoarseViewport->kaLeft-1,
    View->kvLargeCoarseViewport->kaBottom-1,
    View->kvLargeCoarseViewport->kaRight,
    View->kvLargeCoarseViewport->kaTop);
}


void
RedisplayViewports()

{
    EraseLargeCoarseViewport();
    if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY)
        Redisplay(View->kvCoarseWindow);
    else {
        Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
        Redisplay(View->kvCoarseWindow);
        XORfineViewport();
        Parameters.kpRedisplayControl = FINEVIEWPORTONLY;
        Redisplay(View->kvFineWindow);
        Parameters.kpRedisplayControl = SPLITSCREEN;
    }
}


void
ShowFineViewport()

{
    if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY) {
        EraseLargeCoarseViewport();
        Redisplay(View->kvCoarseWindow);
    } 
    else {
        FBFilledBox(0,ERASE,0,View->kvFineViewport->kaLeft-1,
            View->kvFineViewport->kaBottom-1,
            View->kvFineViewport->kaRight,
            View->kvFineViewport->kaTop);
        /* Redisplay in magnifying glass. */
        Parameters.kpRedisplayControl = FINEVIEWPORTONLY;
        Redisplay(View->kvFineWindow);
        Parameters.kpRedisplayControl = SPLITSCREEN;
    }
    FBTransfer();
}


/***********************************************************************
 *
 * Routines to dislay and erase marked objects and points.
 * 
 *
 ***********************************************************************/

/* for point marks */
struct mark {
    int X;
    int Y;
    int Delta;
    struct mark *next;
};


void
ShowCurrentObject(Pointer,DisplayOrErase)

struct o *Pointer;
int DisplayOrErase;
{
    struct ka AOI;
    /*
     * Show the object with an X through the bounding box.
     */
    CDBB(Parameters.kpCellDesc,Pointer,&AOI.kaLeft,&AOI.kaBottom,
        &AOI.kaRight,&AOI.kaTop);
    if (DisplayOrErase == DISPLAY) {
        ShowLine(ColorTable[HighlightingColor].Ent,AOI.kaLeft,AOI.kaBottom,
            AOI.kaRight,AOI.kaTop);
        ShowLine(ColorTable[HighlightingColor].Ent,AOI.kaLeft,AOI.kaTop,
            AOI.kaRight,AOI.kaBottom);
        ShowManhattanLine(ColorTable[HighlightingColor].Ent,AOI.kaLeft,
            AOI.kaBottom,AOI.kaRight,AOI.kaBottom);
        ShowManhattanLine(ColorTable[HighlightingColor].Ent,AOI.kaRight,
            AOI.kaBottom,AOI.kaRight,AOI.kaTop);
        ShowManhattanLine(ColorTable[HighlightingColor].Ent,AOI.kaRight,
            AOI.kaTop,AOI.kaLeft,AOI.kaTop);
        ShowManhattanLine(ColorTable[HighlightingColor].Ent,AOI.kaLeft,
            AOI.kaTop,AOI.kaLeft,AOI.kaBottom);
    }
    else {
        EraseBox(&AOI);
        Redisplay(&AOI);
    }
    FBTransfer();
}


void
ShowInstanceMarker(DisplayOrErase,Layer,Pointer)

int Layer;
int DisplayOrErase;
struct o *Pointer;
{
    struct t *TGen;
    char Type;
    int DX,DY,X,Y;

    if(Not Parameters.kpShowInstanceMarkers)
        return;
    /*Mark reference point of symbol call */
    X = Y = 0;
    CDInitTGen(Pointer,&TGen);
    loop {
        CDTGen(&TGen,&Type,&DX,&DY);
        if(TGen == NULL)
            break;
        elif(Type == CDTRANSLATE){
            X = DX;
            Y = DY;
        }
    }
    ShowMarker(DisplayOrErase,Layer,X,Y,200);
}


void
ShowMarker(DisplayOrErase,Layer,X,Y,Delta)

int DisplayOrErase;
int Layer;
int X,Y;
int Delta;    /* width of marker in RESOLUTION*Lambda */
{
    struct ka BB;

    /* SRW ** save markers for redraw after screen alteration */
    static struct mark *mlist;
    struct mark *mm, *mtmp;

    /* SRW ** display stored marks */
    if (!Delta && DisplayOrErase == DISPLAY) {
        for (mm = mlist; mm; mm = mm->next) {
            X = mm->X;
            Y = mm->Y;
            Delta = mm->Delta;

            /* transform for PUSH */
            TPoint(&X,&Y);

            ShowLine(Layer,X-Delta,Y,X+Delta,Y);
            ShowLine(Layer,X,Y-Delta,X,Y+Delta);
        }
        FBTransfer();
        return;
    }

    /* SRW ** constant size on screen    */
    Delta /= RESOLUTION;
    if (Parameters.kpRedisplayControl == COARSEVIEWPORTONLY)
        Delta *= View->kvCoarseWindow->kaHeight/100;
    else
        Delta *= View->kvFineWindow->kaHeight/50;

    if (DisplayOrErase == DISPLAY) {
        FBForeground(DISPLAY,Layer);

        /* add marker to list */
        mm = alloc(mark);
        if (!mm) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        mm->X = X;
        mm->Y = Y;
        mm->Delta = Delta;
        mm->next = mlist;
        mlist = mm;

        /* transform for PUSH */
        TPoint(&X,&Y);

        ShowLine(Layer,X-Delta,Y,X+Delta,Y);
        ShowLine(Layer,X,Y-Delta,X,Y+Delta);
    }
    else {
        /* remove from list */
        for (mtmp = NULL,mm = mlist; mm; mtmp = mm,mm = mm->next) {
            if (mm->X == X && mm->Y == Y) {
                if (mm == mlist) mlist = mm->next;
                else mtmp->next = mm->next;
                afree(mm,mark);
                break;
            }
        }
        BB.kaLeft = X - Delta;
        BB.kaRight = X + Delta;
        BB.kaBottom = Y - Delta;
        BB.kaTop = Y + Delta;
        EraseBox(&BB);
        Redisplay(&BB);
    }
    FBTransfer();
}


/***********************************************************************
 *
 * Etc.
 * 
 *
 ***********************************************************************/

#ifdef WIN32

// Exec cmdline as a new sub-process
//
static HANDLE
msw_NewProcess(DWORD *pid, char *cmdline, int with_console, HANDLE hin,
    HANDLE hout, HANDLE herr)
{
    PROCESS_INFORMATION info;
    DWORD flags = 0;
    STARTUPINFO startup = { 0 };

    startup.cb = sizeof(STARTUPINFO);
    if (hin || hout || herr) {
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = hin ? hin : GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput = hout ? hout : GetStdHandle(STD_OUTPUT_HANDLE);
        startup.hStdError = herr ? herr : GetStdHandle(STD_ERROR_HANDLE);
    }

    if (with_console)
        flags |= CREATE_NEW_CONSOLE;

    if (!CreateProcess(0, cmdline, 0, 0, True, flags,
            0, 0, &startup, &info))
        return (0);

    CloseHandle(info.hThread);
    if (pid)
        *pid = info.dwProcessId;
    return (info.hProcess);
}

#endif


void
ShowProcess(cp)

char *cp;
{
#ifdef vms
    FILE *list;
    char *bp;
    char *tmp;
    char *cmd;
    char buffer[80];
    char tmpname[80];
    char tf[32];
    int i;

    cmd = cp;
    FBForeground(DISPLAY,ColorTable[MoreTextColor].Ent);
    /* convert to upper case */
    bp = cp;
    while(*bp != NULL){
        if(*bp >= 'a' And *bp <= 'z')
            *bp -= 32;
        ++bp;
    }

    /* substitute tmp filename for "/OUTPUT=KIC" */
    bp = cp;
    strcpy(tf,"SYSXXXXXX");
    tmp = mktemp(tf);
    sprintf(tmpname,"%s.LIS",tmp);
    tmp = tmpname;
    while(*bp != NULL){
        if(*bp == 'O' And *(bp+1) == 'U' And *(bp+2) == 'T'){
            bp += 3;
            /* scan ahead for '=' */
            while(*bp != NULL And *bp != '=') ++bp;
            if(*bp != NULL){
                i = bp - cp + 1;
                if(cp[i] == 'K' And cp[i+1] == 'I' And cp[i+2] == 'C'){
                    cp[i] = NULL;
                    bp = &cp[i + 3];
                    sprintf(buffer,"%s%s %s",cp,tmp,bp);
                    cmd = buffer;
                }
            }
            break;
        }
        ++bp;
    }
    system(cmd);
    if((list = fopen(tmp,"r")) != NULL){
        FBMore(View->kvFineViewport->kaLeft-1,
            View->kvFineViewport->kaBottom-1,
            View->kvFineViewport->kaRight,
            View->kvFineViewport->kaTop,list);
        fclose(list);
        sprintf(buffer,"DELETE %s;*",tmp);
        system(buffer);
    }
#endif
#ifdef MSDOS
    FILE *list;
    char buffer[80];

    FBForeground(DISPLAY,ColorTable[MoreTextColor].Ent);
    if (!cp || !strlen(cp)) {
        cp = "command";
        FBEnd();
        (void)system(cp);
        FB.fInitialized = False;
        FBBegin(FB.fDisplay);
        FullRedisplay();
        ErasePrompt();
        return;
    }
    sprintf(buffer,"%s > tmp__kic.__t",cp);
    (void)system(buffer);
    list = fopen("tmp__kic.__t","r");
    if (list == NULL) {
        ShowPrompt("Can't open process.");
        return;
    }
    EnableMore(True);
    while (fgets(TypeOut,200,list) != NULL) {
        if (MoreLine(TypeOut))
            goto quit;
    }
    if (MorePageDisplay()) {
        ShowPrompt("Hit any key to continue.");
        (void)FBGetchar(ERASE);
    }
quit:
    EnableMore(False);
    fclose(list);
    unlink("tmp__kic.__t");

#else

    FBForeground(DISPLAY,ColorTable[MoreTextColor].Ent);
    if (!cp || !strlen(cp)) {
#ifdef WIN32
        char *shellpath = getenv("SHELL");
        if (!shellpath)
            shellpath = getenv("COMSPEC");
        if (!shellpath)
            shellpath = "/bin/sh";
        msw_NewProcess(NULL, shellpath, True, 0, 0, 0);
#else
        system("xterm &");
#endif
        ErasePrompt();
        return;
    }

    /* intercept 'cd' */
    if (!strncmp(cp, "cd", 2)) {
        char *dir = cp + 2;
        if (!*dir || isspace(*dir)) {
            while (isspace(*dir))
                dir++;
            if (!*dir) {
#ifndef WIN32
                struct passwd *pw = getpwuid(getuid());
                if (pw)
                    dir = pw->pw_dir;
                else
#endif
                    dir = getenv("HOME");
                if (!dir)
                    dir = "/";
            }
            if (!chdir(dir))
                sprintf(TypeOut, "Current working directory now %s", dir);
            else
                sprintf(TypeOut, "Directory change to %s failed", dir);
            ShowPrompt(TypeOut);
            return;
        }
    }


#ifdef WIN32
    {
        FILE *list;
        char buffer[256];

        sprintf(buffer,"%s > tmp__kic.__t",cp);
        (void)system(buffer);
        list = fopen("tmp__kic.__t","r");
        if (list == NULL) {
            ShowPrompt("Can't open process.");
            return;
        }
        EnableMore(True);
        while (fgets(TypeOut,200,list) != NULL) {
            if (MoreLine(TypeOut))
                goto quit;
        }
        if (MorePageDisplay()) {
            ShowPrompt("Hit any key to continue.");
            (void)FBGetchar(ERASE);
        }
quit:
        EnableMore(False);
        fclose(list);
        unlink("tmp__kic.__t");
    }

#else
    {
        FILE *list;
        if ((list = propen(cp,"r")) == NULL) {
            ShowPrompt("Can't open process.");
            return;
        }
        EnableMore(True);
        while (fgets(TypeOut,200,list) != NULL) {
            strcat(TypeOut,"\n");
            if (MoreLine(TypeOut))
                goto quit;
        }
        if (MorePageDisplay()) {
            ShowPrompt("Hit any key to continue.");
            (void)FBGetchar(ERASE);
        }
quit:
        EnableMore(False);
        prclose(list);
    }
#endif

#endif

    FullRedisplay();
    ErasePrompt();
}


#if !defined(WIN32) && !defined(MSDOS) && !defined(vms)

#define  tst(a,b)   (*mode == 'r'? (b) : (a))
#define  RDR    0
#define  WTR    1

static int propen_pid[20];


static FILE *
propen(cmd,mode)

char *cmd;
char *mode;
{
    int p[2];
    int myside, hisside, pid;

    if(pipe(p) < 0)
        return(NULL);
    myside = tst(p[WTR], p[RDR]);
    hisside = tst(p[RDR], p[WTR]);
    if((pid = fork()) == 0){
        /* myside and hisside reverse roles in child */
        close(myside);
        if(dup2(hisside, tst(0, 1)) == -1)
            exit(1);
        close(hisside);
        execl("/bin/sh", "sh", "-c", cmd, (char*)0);
        _exit(1);
    }
    if(pid == -1)
        return(NULL);
    propen_pid[myside] = pid;
    close(hisside);
    return(fdopen(myside, mode));
}


static int
prclose(ptr)

FILE *ptr;
{
    int f, r;
    void (*hstat)(), (*istat)(), (*qstat)();
    int status;

    f = fileno(ptr);
    fclose(ptr);
    istat = signal(SIGINT, SIG_IGN);
#ifdef SIGQUIT
    qstat = signal(SIGQUIT, SIG_IGN);
#endif
#ifdef SIGHUP
    hstat = signal(SIGHUP, SIG_IGN);
#endif
    while((r = wait((int *)&status)) != propen_pid[f] && r != -1)
        ;
    if(r == -1)
        status = -1;
    signal(SIGINT, istat);
#ifdef SIGQUIT
    signal(SIGQUIT, qstat);
#endif
#ifdef SIGHUP
    signal(SIGHUP, hstat);
#endif
    return(status);
}

#endif

#if __NDPC__

/* sleep() delays for i seconds
 * MicroWay NDP-C clock() returns hundredths of a second
 */

void
sleep(i)

int i;
{
    int t;

    if (i <= 0) return;
    t = clock();
    while (100*i > clock() - t) ;
}

#endif

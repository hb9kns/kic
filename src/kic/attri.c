/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- Giles C. Billingsley
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
 * Layer attribute menu code.
 */

#include "prefix.h"
#include "kic.h"

extern int LockOut;       /* SRW ** disables ^F, etc. in Point()          */

int NoMakeVisible;        /* SRW ** don't make invisible layers visible   */
                          /* in PointLayerTable()                         */

#define Matching(string) !strcmp(Parameters.kpCommand,string)

static int DefaultFillPatterns[][8] = {
    {0x89, 0x18, 0x24, 0xc2, 0x43, 0x24, 0x18, 0x91},
    {0x88, 0x48, 0x24, 0x13, 0xc8, 0x24, 0x12, 0x11},
    {0x42, 0x81, 0x3c, 0x42, 0x42, 0x3c, 0x81, 0x42},
    {0x11, 0x0e, 0x02, 0x62, 0x91, 0x28, 0x48, 0x90},
    {0x81, 0x42, 0x3c, 0x00, 0x24, 0x24, 0x42, 0x81},
    {0x91, 0x52, 0x64, 0x08, 0x10, 0x26, 0x4a, 0x89},
    {0x44, 0x28, 0x93, 0x82, 0x82, 0x93, 0x28, 0x44},
    {0x81, 0x42, 0x24, 0x99, 0x42, 0x24, 0x42, 0x81},
    {0x54, 0x94, 0x23, 0xc0, 0x03, 0xc4, 0x29, 0x2a},
    {0x08, 0x08, 0x08, 0xcf, 0x40, 0x40, 0x78, 0x08},
    {0x00, 0x00, 0x42, 0x00, 0x24, 0x00, 0x42, 0x00},
    {0x11, 0x22, 0x40, 0x88, 0x11, 0x02, 0x44, 0x88},
    {0x01, 0x9c, 0xa2, 0xaa, 0xa2, 0x9c, 0x40, 0x3e},
    {0x01, 0x39, 0x27, 0x20, 0x20, 0x27, 0x39, 0x01},
    {0x00, 0x55, 0x00, 0x82, 0x10, 0x82, 0x00, 0x55},
    {0x00, 0x32, 0x14, 0x14, 0x15, 0x3e, 0x40, 0x00}
};

/* XKIC fill patterns
static int DefaultFillPatterns[][8] = {
    {0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11},
    {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88},
    {0x66, 0x11, 0x88, 0x66, 0x66, 0x11, 0x88, 0x66},
    {0x66, 0x88, 0x11, 0x66, 0x66, 0x88, 0x11, 0x66},
    {0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55},
    {0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0},
    {0x55, 0x00, 0x55, 0x00, 0x55, 0x00, 0x55, 0x00},
    {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA},
    {0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC},
    {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA},
    {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00},
    {0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33},
    {0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F},
    {0xFE, 0x82, 0xBA, 0xAA, 0xA2, 0xBE, 0x80, 0xFF},
    {0x02, 0x7A, 0x4A, 0x4A, 0x42, 0x42, 0x7E, 0x00},
    {0x2C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x2C},
    {0x18, 0x24, 0x42, 0x81, 0x81, 0x42, 0x24, 0x18}
};
*/


#ifdef __STDC__
static void color(int*,int);
static int  get_layer(int*);
static int  point_color(int*,int);
static void alter_color(int,int,int);
static void show_choices(void);
static void show_pixel_map(void);
static void flip_pixel(void);
#else
static void color();
static int  get_layer();
static int  point_color();
static void alter_color();
static void show_choices();
static void show_pixel_map();
static void flip_pixel();
#endif


void
Attri(LookedAhead)

int *LookedAhead;
{
    *LookedAhead = False;
    Parameters.kpMenu = ATTRIBUTESMENU;
    FixMenuPrefix(AttributeMenu);
    ShowCommandMenu();
}


void
Updat()

{
    extern char *MenuUPDAT;

    MenuSelect(MenuUPDAT);
    SaveTechFile();
    MenuDeselect(MenuUPDAT);
}


void
DisplayLabels()

{
    extern char *MenuLABLS;

    if (Parameters.kpDisplayAllLabels) {
        MenuDeselect(MenuLABLS);
        ShowPrompt("Labels will not be shown in large windows.");
        Parameters.kpDisplayAllLabels = False;
    }
    else {
        MenuSelect(MenuLABLS);
        ShowPrompt("Labels will always be displayed.");
        Parameters.kpDisplayAllLabels = True;
    }
}


void
LabelInstances()

{
    extern char *MenuCNAMS;

    if (Parameters.kpLabelAllInstances) {
        MenuDeselect(MenuCNAMS);
        ShowPrompt("Instances will not be labeled in large windows.");
        Parameters.kpLabelAllInstances = False;
    }
    else {
        MenuSelect(MenuCNAMS);
        ShowPrompt("Instances will always be labeled.");
        Parameters.kpLabelAllInstances = True;
    }
}


void
Mark()

{
    extern char *MenuMARK;

    if (Parameters.kpShowInstanceMarkers) {
        MenuDeselect(MenuMARK);
        ShowPrompt("Instances will not be marked when selected.");
        Parameters.kpShowInstanceMarkers = False;
    }
    else {
        MenuSelect(MenuMARK);
        ShowPrompt("Instances will always be marked when selected.");
        Parameters.kpShowInstanceMarkers = True;
    }
}


void
Sides()

{
    char Buf[80];
    int i;
    char *TypeIn;
    extern char *MenuSIDES;

    MenuSelect(MenuSIDES);
    loop {
        sprintf(Buf,"Enter number of sides for a round flash (8 to 90, now %d).",
            Parameters.kpNumRoundFlashSides);
        ShowPrompt(Buf);
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) break;
        if (!strlen(TypeIn)) break;
        if (sscanf(TypeIn,"%d",&i) != 1) continue;;
        if (i > 7 And i < 91) {
            Parameters.kpNumRoundFlashSides = i;
            break;
        }
    }
    sprintf(Buf,"Clip polygon vertices to grid points (%c)?",
        Parameters.kpClipVerticesToGrid ? 'Y' : 'N');
    ShowPrompt(Buf);
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL) {
        if (*TypeIn == 'y' Or *TypeIn == 'Y')
            Parameters.kpClipVerticesToGrid = True;
        elif (*TypeIn == 'n' Or *TypeIn == 'N')
            Parameters.kpClipVerticesToGrid = False;
    }
    ErasePrompt();
    MenuDeselect(MenuSIDES);
}


void
ShowRGB()

{
    extern char *MenuRGB;

    MenuSelect(MenuRGB);
    sprintf(TypeOut,"<%d,%d,%d>",
        LayerTable[Parameters.kpLayer].klR,
        LayerTable[Parameters.kpLayer].klG,
        LayerTable[Parameters.kpLayer].klB);
    ShowPrompt(TypeOut);
    MenuDeselect(MenuRGB);
}


void
SetColor(RGBpix,PlusOrMinus)

int RGBpix;
int PlusOrMinus;
{
    alter_color(-1,RGBpix,PlusOrMinus);
}


char *MenuBGRND = "bgrnd";
char *MenuHLITE = "hlite";
char *MenuPRMPT = "prmpt";
char *MenuMTEXT = "mtext";
char *MenuMHLTE = "mhlte";
char *MenuMSEL  = "msel";
char *MenuMORE  = "more ";
char *MenuBBINS = "bbins";
char *MenuNMINS = "nmins";
char *MenuSZINS = "szins";
char *MenuCGRID = "cgrid";
char *MenuFGRID = "fgrid";


void
AttribColor(LookedAhead)

int *LookedAhead;
{
    int i = 0, OldMenu;
    extern char *MenuPLUSR,*MenuMINSR;
    extern char *MenuPLUSG,*MenuMINSG;
    extern char *MenuPLUSB,*MenuMINSB;

    OldMenu = Parameters.kpMenu;
    Parameters.kpMenu = AMBIGUITYMENU;

    AmbiguityMenu[0].mEntry  = MenuHLITE;
    AmbiguityMenu[1].mEntry  = MenuBGRND;
    AmbiguityMenu[2].mEntry  = "     ";
    AmbiguityMenu[3].mEntry  = MenuPRMPT;
    AmbiguityMenu[4].mEntry  = MenuMTEXT;
    AmbiguityMenu[5].mEntry  = MenuMHLTE;
    AmbiguityMenu[6].mEntry  = MenuMSEL;
    AmbiguityMenu[7].mEntry  = MenuMORE;
    AmbiguityMenu[8].mEntry  = "     ";
    AmbiguityMenu[9].mEntry  = MenuFGRID;
    AmbiguityMenu[10].mEntry = MenuCGRID;
    AmbiguityMenu[11].mEntry = "     ";
    AmbiguityMenu[12].mEntry = MenuBBINS;
    AmbiguityMenu[13].mEntry = MenuNMINS;
    AmbiguityMenu[14].mEntry = MenuSZINS;
    AmbiguityMenu[15].mEntry = "     ";
    AmbiguityMenu[16].mEntry = MenuPLUSR;
    AmbiguityMenu[17].mEntry = MenuMINSR;
    AmbiguityMenu[18].mEntry = MenuPLUSG;
    AmbiguityMenu[19].mEntry = MenuMINSG;
    AmbiguityMenu[20].mEntry = MenuPLUSB;
    AmbiguityMenu[21].mEntry = MenuMINSB;
    AmbiguityMenu[22].mEntry = NULL;
    FixMenuPrefix(AmbiguityMenu);
    ShowMenu(AmbiguityMenu);

    loop {
        if (*LookedAhead == False)
            i = PointLoopSafe(LookedAhead);
        else
            *LookedAhead = False;

        switch (i) {
        case PL_ESC:
            goto quit;
        case PL_PCW:
            continue;
        case PL_CMD:

            if (Matching(MenuHLITE))
                {color(LookedAhead,HighlightingColor); continue;}
            if (Matching(MenuBGRND))
                {color(LookedAhead,0); continue;}
            if (Matching(MenuPRMPT))
                {color(LookedAhead,MenuPromptColor); continue;}
            if (Matching(MenuMTEXT))
                {color(LookedAhead,MenuTextColor); continue;}
            if (Matching(MenuMHLTE))
                {color(LookedAhead,MenuHighlightingColor); continue;}
            if (Matching(MenuMSEL))
                {color(LookedAhead,MenuSelectColor); continue;}
            if (Matching(MenuMORE))
                {color(LookedAhead,MoreTextColor); continue;}
            if (Matching(MenuFGRID))
                {color(LookedAhead,FineGridColor); continue;}
            if (Matching(MenuCGRID))
                {color(LookedAhead,CoarseGridColor); continue;}
            if (Matching(MenuBBINS))
                {color(LookedAhead,InstanceBBColor); continue;}
            if (Matching(MenuNMINS))
                {color(LookedAhead,InstanceNameColor); continue;}
            if (Matching(MenuSZINS))
                {color(LookedAhead,InstanceSizeColor); continue;}
            if (Matching(MenuMINSB))
                {SetColor('b','-'); continue;}
            if (Matching(MenuMINSG))
                {SetColor('g','-'); continue;}
            if (Matching(MenuMINSR))
                {SetColor('r','-'); continue;}
            if (Matching(MenuPLUSB))
                {SetColor('b','+'); continue;}
            if (Matching(MenuPLUSG))
                {SetColor('g','+'); continue;}
            if (Matching(MenuPLUSR))
                {SetColor('r','+'); continue;}
            continue;
        }
        Parameters.kpMenu = OldMenu;
        break;
    }
quit:
    Parameters.kpMenu = OldMenu;
    ShowCommandMenu();
}

static void
color(LookedAhead,which)

int *LookedAhead;
int which;
{
    char *menu;

    if (Parameters.kpMergeColors)
        strcpy(TypeOut,"Point to color for ");
    else
        strcpy(TypeOut,"Set RGB for ");

    switch (which) {

    case 0:
        MenuSelect(MenuBGRND);
        ShowPrompt("Set RGB of background color.");
        (void)point_color(LookedAhead,0);
        ErasePrompt();
        MenuDeselect(MenuBGRND);
        return;
    case HighlightingColor:
        MenuSelect(MenuHLITE);
        ShowPrompt("Set RGB of highlighting color.");
        (void)point_color(LookedAhead,HighlightingColor);
        ErasePrompt();
        MenuDeselect(MenuHLITE);
        return;
    case MenuTextColor:
        menu = MenuMTEXT;
        strcat(TypeOut,"menu text.");
        break;
    case MenuHighlightingColor:
        menu = MenuMHLTE;
        strcat(TypeOut,"menu highlighting.");
        break;
    case MenuSelectColor:
        menu = MenuMSEL;
        strcat(TypeOut,"selected menu text.");
        break;
    case MoreTextColor:
        menu = MenuMORE;
        strcat(TypeOut,"\"more\" mode text.");
        break;
    case MenuPromptColor:
        menu = MenuPRMPT;
        strcat(TypeOut,"prompt message text.");
        break;
    case FineGridColor:
        menu = MenuFGRID;
        strcat(TypeOut,"fine grid lines.");
        break;
    case CoarseGridColor:
        menu = MenuCGRID;
        strcat(TypeOut,"coarse grid lines.");
        break;
    case InstanceBBColor:
        menu = MenuBBINS;
        strcat(TypeOut,"unexpanded instance bounding boxes.");
        break;
    case InstanceNameColor:
        menu = MenuNMINS;
        strcat(TypeOut,"unexpanded instance names.");
        break;
    case InstanceSizeColor:
        menu = MenuSZINS;
        strcat(TypeOut,"unexpanded instance sizes.");
        break;
    default:
        return;
    }

    MenuSelect(menu);
    ShowPrompt(TypeOut);

    if (Parameters.kpMergeColors) {
        if (get_layer(LookedAhead)) {
            ColorTable[which].Ent = Parameters.kpLayer;
            switch (which) {

            case MenuTextColor:
            case MenuHighlightingColor:
            case MenuSelectColor:
                ShowCommandMenu();
                break;
            case MoreTextColor:
                ShowPrompt("New \"more\" color set.");
                break;
            case MenuPromptColor:
                ShowPrompt("New prompt color set.");
                break;
            case FineGridColor:
            case CoarseGridColor:
                ShowPrompt(
                    "New grid color will be visible after redraw.");
                break;
            case InstanceBBColor:
            case InstanceNameColor:
            case InstanceSizeColor:
                ShowPrompt(
                    "New instance color will be visible after redraw.");
                break;
            }
        }
        else {
            ErasePrompt();
        }
    }
    else {
        (void)point_color(LookedAhead,which);
        ErasePrompt();
    }
    MenuDeselect(menu);
}


void
SetGrid(LookedAhead)

int *LookedAhead;
{
    char *TypeIn;
    double d;
    int OldParameter = Parameters.kpShowGridInLargeViewport;
    int i;
    extern char *MenuGRID;

    MenuSelect(MenuGRID);

    if (Parameters.kpDoingHardcopy) {
        sprintf(TypeOut,"Show grid in hard copy? (%c) ",
            Parameters.kpGridDisplayed ? 'y' : 'n');
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) goto quit;
        if (*TypeIn == 'y' || *TypeIn == 'Y') {
            Parameters.kpGridDisplayed = True;
            Parameters.kpHardcopyGrid = True;
        }
        if (*TypeIn == 'n' || *TypeIn == 'N') {
            Parameters.kpGridDisplayed = False;
            Parameters.kpHardcopyGrid = False;
        }
        goto quit;
    }
    if (Parameters.kpMenu == ATTRIBUTESMENU) {
        sprintf(TypeOut,"Hex code for grid line style, 0 for point grid (%x)?",
            Parameters.kpGridLineStyle);
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) goto quit;
        if (sscanf(TypeIn,"%x",&i) == 1)
            if (i >= 0 && i < 256)
                Parameters.kpGridLineStyle = i;

        sprintf(TypeOut,"Show grid above layout geometries (%c)?",
            Parameters.kpGridOnTop ? 'Y' : 'N');
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) goto quit;
        if (*TypeIn == 'n' Or *TypeIn == 'N')
            Parameters.kpGridOnTop = False;
        elif (*TypeIn == 'y' Or *TypeIn == 'Y')
            Parameters.kpGridOnTop = True;

        sprintf(TypeOut,"Show grid in coarse viewport of a split screen (%c)?",
            Parameters.kpShowGridInLargeViewport ? 'Y' : 'N');
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) goto quit;
        if (*TypeIn == 'n' Or *TypeIn == 'N')
            Parameters.kpShowGridInLargeViewport = False;
        elif (*TypeIn == 'y' Or *TypeIn == 'Y')
            Parameters.kpShowGridInLargeViewport = True;

    }
    sprintf(TypeOut,
        "Enter new grid interval (currently %g, CR to turn grid %s): ",
        (double)Parameters.kpGrid/RESOLUTION,
        Parameters.kpGridDisplayed ? "off" : "on" );
    ShowPrompt(TypeOut);
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL){
        if (!strlen(TypeIn)) {
            Parameters.kpGridDisplayed ^= 1;
            ErasePrompt();
        }
        elif (sscanf(TypeIn,"%lg",&d) == 1 And d > 0) {
            Parameters.kpGrid = RESOLUTION*d;
            if (Parameters.kpGrid == 0)
                Parameters.kpGrid = 1;
            sprintf(TypeOut,"Grid/Snap point spacing now %g/%g.",
                (double)Parameters.kpGrid/RESOLUTION,
                (double)Parameters.kpGrid/
                (Parameters.kpPixToLambdaSnapping*RESOLUTION));
            ShowPrompt(TypeOut);
        }
        else
            ErasePrompt();
    }
quit:
    if (Parameters.kpShowGridInLargeViewport Or OldParameter)
        FullRedisplay();
    else
        ShowFineViewport();
    ErasePrompt();
    MenuDeselect(MenuGRID);
}


void
Visib(LookedAhead)

int *LookedAhead;
{
    extern char *MenuVISIB;

    MenuSelect(MenuVISIB);
    ShowPrompt("Point at layers to toggle visibility.");
    NoMakeVisible = True;
    loop {
        switch (PointLoopLayer(LookedAhead)) {
        case PL_PLT:
            if (LayerTable[Parameters.kpLayer].klAttributes & VISIBLE)
                MakeLayerInvisible(Parameters.kpLayer);
            else
                MakeLayerVisible(Parameters.kpLayer);
            break;
        case PL_ESC:
        case PL_CMD:
            MenuDeselect(MenuVISIB);
            NoMakeVisible = False;
            ErasePrompt();
            return;
        }
    }
}


void
MakeLayerVisible(Layer)

int Layer;
{
    int Row,Column,Lmin;

    if (Layer <= NumLayerTable) {

        Lmin = Parameters.kpVisibleLayerMenuRow *
            Parameters.kpLayersPerMenuRow + 1;

        LayerTable[Layer].klAttributes |= VISIBLE;
        if (Layer >= Lmin &&  Layer < Lmin + Parameters.kpLayersPerMenuRow) {

            Row = FB.fNumRows;
            Column = (Layer - 1 - (((Layer-1)/Parameters.kpLayersPerMenuRow) * 
                Parameters.kpLayersPerMenuRow)) * 6 + 1;
            LtBox(Column,Row,DISPLAY,Layer);
        }
    }
}


void
MakeLayerInvisible(Layer)

int Layer;
{
    int Row,Column,Lmin;

    if (Layer <= NumLayerTable) {
        Lmin = Parameters.kpVisibleLayerMenuRow *
            Parameters.kpLayersPerMenuRow + 1;

        LayerTable[Layer].klAttributes &= ~VISIBLE;
        if (Layer >= Lmin &&  Layer < Lmin + Parameters.kpLayersPerMenuRow) {

            Row = FB.fNumRows;
            Column = (Layer - 1 - (((Layer-1)/Parameters.kpLayersPerMenuRow) * 
                Parameters.kpLayersPerMenuRow)) * 6 + 1;
            LtBox(Column,Row,ERASE,Layer);
        }
    }
}


void
Blink(LookedAhead)

int *LookedAhead;
{
    int Layer;
    extern char *MenuBLINK;

    MenuSelect(MenuBLINK);

    if (! FB.fBlinkers) {
        ShowPrompt("Sorry, blinking layers are not available.");
        *LookedAhead = False;
        MenuDeselect(MenuBLINK);
        return;
    }
    ShowPrompt("Point to EACH layer you want to be blinking.");
    for (Layer = 1; Layer <= NumLayerTable; Layer++) {
        LayerTable[Layer].klAttributes &= ~BLINK;
        FBBlink(Layer,0,0,0,0);
    }
    loop {
        Point();
        if (Parameters.kpPointLayerTable)
            LayerTable[Parameters.kpLayer].klAttributes |= BLINK;
        else {
            *LookedAhead = True;
            MenuDeselect(MenuBLINK);
            return;
        }
    }
}


void
Dimen(LookedAhead)

int *LookedAhead;
{
    char *TypeIn;
    double d;
    extern char *MenuDIMEN;

    MenuSelect(MenuDIMEN);
    ShowPrompt("Point to layer on which to define minimum feature size.");
    Point();
    if (Parameters.kpPointLayerTable) {
        sprintf(TypeOut,"Minimum dimension? (currently %g): ",(double)
            LayerTable[Parameters.kpLayer].klMinDimensions/RESOLUTION);
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        *LookedAhead = False;
        if (TypeIn != NULL && *TypeIn != '\0'  && *TypeIn != '\n') {
            if (sscanf(TypeIn,"%lg",&d) == 1 And d >= 0) {
                LayerTable[Parameters.kpLayer].klMinDimensions =
                    d*RESOLUTION;
                sprintf(TypeOut,
                    "Minimum dimension on selected layer set to %g.",d);
                ShowPrompt(TypeOut);
                if (LayerTable[Parameters.kpLayer].klWireWidth <
                        LayerTable[Parameters.kpLayer].klMinDimensions)
                    LayerTable[Parameters.kpLayer].klWireWidth =
                        LayerTable[Parameters.kpLayer].klMinDimensions;
            }
            else
                ShowPrompt("Bad input, parameter not changed.");
        }
        else
            ErasePrompt();
    }
    else {
        *LookedAhead = True;
        ErasePrompt();
    }
    MenuDeselect(MenuDIMEN);
    return;
}


void
RemoveLayer(LookedAhead)

int *LookedAhead;
{
    int i,Layer;
    extern char *MenuRMOVE;

    ShowPrompt("Point to the layers which you want removed.");
    loop {
        MenuSelect(MenuRMOVE);
        if (get_layer(LookedAhead)) {
            Layer = Parameters.kpLayer;
            for (i = Layer; i < NumLayerTable; i++)
                LayerTable[i] = LayerTable[i+1];
            --NumLayerTable;
            InitVLT();
            ShowLayerTable();
        }
        else
            if (Parameters.kpCommand[0] != '\0' Or
                    Parameters.kpCommand[1] == ESCAPE)
                break;
    }
    MenuDeselect(MenuRMOVE);
    ErasePrompt();
}


void
AddLayer()

{
    int i,n = 0;
    char c,*TypeIn;
    extern char *MenuADLYR;

    MenuSelect(MenuADLYR);
    sprintf(TypeOut,"Enter new layer number (1 for bottom layer, %d for top).",
        (NumLayerTable + 1));
    ShowPrompt(TypeOut);
    TypeIn = FBEdit(NULL);
    if (TypeIn == NULL) goto quit;
    sscanf(TypeIn,"%d",&n);
    if (n <= 0 Or n > (NumLayerTable + 1)) {
        ShowPrompt("Sorry, bad layer number.");
        goto quit;
    }

    ShowPrompt("Enter layer name.");
    TypeIn = FBEdit(NULL);
    if (TypeIn == NULL Or *TypeIn == '\0' Or *TypeIn == '\n') goto quit;

    ++NumLayerTable;

    for (i = 1; i < n; ++i)
        CDSetLayer(i,LayerTable[i].klTechnology,LayerTable[i].klMask);
    for (i = NumLayerTable; i > n; i--) {
        LayerTable[i] = LayerTable[i - 1];
        CDSetLayer(i,LayerTable[i].klTechnology,LayerTable[i].klMask);
    }

    LayerTable[n].klTechnology = LayerTable[n].klMask[0] = 
        LayerTable[n].klMask[1] = LayerTable[n].klMask[2] = ' ';

    LayerTable[n].klTechnology = *TypeIn++;
    for (i = 0; i < 3; i++) {
        if ((c = *TypeIn++) == '\0')
            break;
        else
            LayerTable[n].klMask[i] = c;
    }
    LayerTable[n].klAttributes = VISIBLE | FILLED;
    LayerTable[n].klStyleID       = 0;
    LayerTable[n].klMinDimensions = 0;
    LayerTable[n].klWireWidth     = 0;
    LayerTable[n].klR = LayerTable[n].klG = LayerTable[n].klB = 127;
    for (i = 0; i < 8; i++)
        LayerTable[n].klStyle[i] = 0;

    ShowPrompt("Is this layer symbolic (N)?");
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL And (*TypeIn == 'y' Or *TypeIn == 'Y'))
        LayerTable[n].klAttributes |= SYMBOLIC;

    ShowPrompt("Enter RGB of this layer.");
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL) {
        int r, g, b;
        int nn = sscanf(TypeIn,"%d %d %d",&r,&g,&b);
        if (nn > 2)
            LayerTable[n].klB = min(max(b,0),255);
        if (nn > 1)
            LayerTable[n].klG = min(max(g,0),255);
        if (nn > 0)
            LayerTable[n].klR = min(max(r,0),255);
    }
    InitVLT();
    ShowLayerTable();
quit:
    MenuDeselect(MenuADLYR);
    ErasePrompt();
}


static int
get_layer(LookedAhead)

/* if successful, Parameters.kpLayer contains choice */
int *LookedAhead;
{
    switch (PointLoopLayer(LookedAhead)) {
    case PL_ESC:
    case PL_CMD:
    case PL_PCW:
        break;
    case PL_PLT:
        return (True);
    }     
    return (False);
}


static int
point_color(LookedAhead,Color)

/* Loop until UNDO, a non color setting command, point to coarse window, or
 * ESC is entered.
 */
int *LookedAhead;
int Color;
{
    extern char *MenuPLUSR,*MenuMINSR;
    extern char *MenuPLUSG,*MenuMINSG;
    extern char *MenuPLUSB,*MenuMINSB;
    extern int EscReturn;

    loop {
        if (*LookedAhead == False) {
            EscReturn = True;
            Point();
            EscReturn = False;
        }
        else
            *LookedAhead = False;
        if (Parameters.kpCommand[1] == ESCAPE)
            return (PL_ESC);
        if (Parameters.kpCommand[0] != EOS) {

            if (Matching(MenuMINSB)) { alter_color(Color,'b','-'); continue; }
            if (Matching(MenuMINSG)) { alter_color(Color,'g','-'); continue; }
            if (Matching(MenuMINSR)) { alter_color(Color,'r','-'); continue; }
            if (Matching(MenuPLUSB)) { alter_color(Color,'b','+'); continue; }
            if (Matching(MenuPLUSG)) { alter_color(Color,'g','+'); continue; }
            if (Matching(MenuPLUSR)) { alter_color(Color,'r','+'); continue; }

            *LookedAhead = True;
            return (PL_CMD);
        }
        if (Parameters.kpPointCoarseWindow)
            return (PL_PCW);
    }
}


static void
alter_color(Color,RGBpix,PlusOrMinus)

int Color;
int RGBpix;
int PlusOrMinus;
{
    extern char *MenuPLUSR,*MenuMINSR;
    extern char *MenuPLUSG,*MenuMINSG;
    extern char *MenuPLUSB,*MenuMINSB;
    char *menu;
    short *which;

    switch (RGBpix) {
    case 'r':
    case 'R':
        if (PlusOrMinus == '+') 
            menu = MenuPLUSR;
        else
            menu = MenuMINSR;
        if (Color >= 0)
            which = &ColorTable[Color].R;
        else
            which = &LayerTable[Parameters.kpLayer].klR;
        break;
    case 'g':
    case 'G':
        if (PlusOrMinus == '+') 
            menu = MenuPLUSG;
        else
            menu = MenuMINSG;
        if (Color >= 0)
            which = &ColorTable[Color].G;
        else
            which = &LayerTable[Parameters.kpLayer].klG;
        break;
    case 'b':
    case 'B':
        if (PlusOrMinus == '+') 
            menu = MenuPLUSB;
        else
            menu = MenuMINSB;
        if (Color >= 0)
            which = &ColorTable[Color].B;
        else
            which = &LayerTable[Parameters.kpLayer].klB;
        break;
    default:
        return;
    }
    MenuSelect(menu);
    if (PlusOrMinus == '+') {
        if (*which == FB.fMaxIntensity) *which = 0;
        else if (((*which) += 10) > FB.fMaxIntensity)
            *which = FB.fMaxIntensity;
    }
    else {
        if (*which == 0) *which = FB.fMaxIntensity;
        else if (((*which) -= 10) < 0) *which = 0;
    }
    if (Color >= 0) {
        FBVLT(ColorTable[Color].Ent,ColorTable[Color].R,
            ColorTable[Color].G,ColorTable[Color].B);
        sprintf(TypeOut,"<%d,%d,%d>",ColorTable[Color].R,
            ColorTable[Color].G,ColorTable[Color].B);
    }
    else {
        FBVLT(Parameters.kpLayer,
            LayerTable[Parameters.kpLayer].klR,
            LayerTable[Parameters.kpLayer].klG,
            LayerTable[Parameters.kpLayer].klB);

        sprintf(TypeOut,"<%d,%d,%d>",
            LayerTable[Parameters.kpLayer].klR,
            LayerTable[Parameters.kpLayer].klG,
            LayerTable[Parameters.kpLayer].klB);
    }
    ShowLayerTable();
    ShowPrompt(TypeOut);
    MenuDeselect(menu);
}


/*********************************************************************
 *
 * Fill pattern editing
 *
 * Pointing to the box in location 0 toggles pixels.  Point at layer
 * to load pixel editor (first box), starts with DefaultFillPattern[0].
 *
 *********************************************************************/

static struct {
    int fi_rc;      /* temporary redisplay control */
    int fi_vc;      /* temporary view control */
    int fi_on;      /* True if doing FILL command */
    int fi_px[8];   /* edited pixel storage */
    int fi_eboxbot; /* bottom of edit box */
    int fi_spa;     /* spacing of edit box big pixels */
    int fi_del;     /* width of edit box big pixel */
    int fi_incr;    /* spacing of pattern menu boxes */
    int fi_sep;     /* separation of pattern menu boxes */
} fi;


void
Fill(LookedAhead)

int *LookedAhead;
{
    int i,j=0;
    char *ol;
    extern char *MenuFILL;

    MenuSelect(MenuFILL);
    *LookedAhead = False;

    if (Not FB.fDefinableFillPatterns) {
        ShowPrompt("Point at layers to switch between filled and outlined.");
        loop {
            switch (PointLoopLayer(LookedAhead)) {
            case PL_ESC:
            case PL_PCW:
            case PL_CMD:
                MenuDeselect(MenuFILL);
                ErasePrompt();
                return;
            case PL_PLT:
                LayerTable[Parameters.kpLayer].klAttributes ^= FILLED;
                ShowLayerTable();
                continue;
            }
        }
    }

    fi.fi_on = True;
    fi.fi_rc = Parameters.kpRedisplayControl;
    fi.fi_vc = View->kvControl;
    View->kvControl = SPLITSCREEN;
    SetPositioning();
    Parameters.kpRedisplayControl = FINEVIEWPORTONLY;

    LockOut = True;

    /* compute spacing between fill pattern menu */
    i = 10;
    if (!View->kvFineViewportOnBottom) i = 6;
    fi.fi_incr = View->kvFineWindow->kaWidth/i;
    fi.fi_sep = fi.fi_incr >> 3;

    for (i = 0; i < 8; i++)
        fi.fi_px[i] = DefaultFillPatterns[0][i];
    show_choices();

    loop {
        ShowPrompt(
            "Point at layer to load fillpattern, or fillpattern to assign.");
        loop {
            switch (PointLoopLayer(LookedAhead)) {
            case PL_PLT:
                if (!(LayerTable[Parameters.kpLayer].klAttributes
                    & FILLED)) {
                    ShowPrompt(
    "Empty fill, can't edit.  Select another layer or fillpattern.");
                    continue;
                }
                for (i = 0; i < 8; i++)
                    if (LayerTable[Parameters.kpLayer].klStyle[i]) break;
                if (i == 8) {
                    ShowPrompt(
    "Solid fill, can't edit.  Select another layer or fillpattern.");
                    continue;
                }

                ShowPrompt(
    "Use edit box to change pixels, point at fillpattern to end editing.");

                for (i = 0; i < 8; i++)
                    fi.fi_px[i] =
                        LayerTable[Parameters.kpLayer].klStyle[i];
                show_pixel_map();
                continue;

            case PL_PCW:
                if (InBox(KicCursor.kcRawX,KicCursor.kcRawY,
                    View->kvFineWindow)) {
                    j = 1 + (KicCursor.kcRawX -
                        View->kvFineWindow->kaLeft)/fi.fi_incr;
                    if (j > 1) break;
                    if (KicCursor.kcRawY < fi.fi_eboxbot) {
                        if (KicCursor.kcRawY >
                            (View->kvFineWindow->kaBottom +
                            fi.fi_eboxbot)/2)
                            j = 0;
                        break;
                    }
                    flip_pixel();
                    continue;
                }

            case PL_ESC:
            case PL_CMD:
                goto quit;
            }
            break;
        }

        ShowPrompt("Point at layer to have this fill pattern.");
        switch (PointLoopLayer(LookedAhead)) {
        case PL_ESC:
        case PL_PCW:
        case PL_CMD:
            goto quit;
        }
        switch (j) {
        case 0:
            LayerTable[Parameters.kpLayer].klAttributes
                &= ~(FILLED|OUTLINED);
            break;

        case 1:
            LayerTable[Parameters.kpLayer].klAttributes |= FILLED;
            LayerTable[Parameters.kpLayer].klAttributes &= ~OUTLINED;
            for (i = 0; i < 8; i++)
                LayerTable[Parameters.kpLayer].klStyle[i] = 0;
            break;

        case 2:
            for (i = 0; i < 8; i++)
                LayerTable[Parameters.kpLayer].klStyle[i] = fi.fi_px[i];
            break;

        default:
            if (View->kvFineViewportOnBottom) {
                if (KicCursor.kcRawY <
                    (View->kvFineWindow->kaTop +
                    View->kvFineWindow->kaBottom)/2)
                    j += 8;
            }
            else {
                int delta = 
                    (View->kvFineWindow->kaTop -
                    View->kvFineWindow->kaBottom)/4;
                if (KicCursor.kcRawY <
                    View->kvFineWindow->kaTop - delta)
                    j += 4;
                if (KicCursor.kcRawY <
                    View->kvFineWindow->kaTop - 2*delta)
                    j += 4;
                if (KicCursor.kcRawY <
                    View->kvFineWindow->kaTop - 3*delta)
                    j += 4;
            }
            for (i = 0; i < 8; i++)
                LayerTable[Parameters.kpLayer].klStyle[i] =
                    DefaultFillPatterns[j-3][i];
            break;
        }
        
        if (j > 1) {
            LayerTable[Parameters.kpLayer].klAttributes |= FILLED;
            ShowPrompt("Outline fill pattern? [n]");
            ol = FBEdit(NULL);
            if (ol != NULL And (strchr(ol,'y') || strchr(ol,'Y')))
                LayerTable[Parameters.kpLayer].klAttributes |= OUTLINED;
            else
                LayerTable[Parameters.kpLayer].klAttributes &= ~OUTLINED;
        }
        InitVLT();
        MakeLayerInvisible(Parameters.kpLayer);
        MakeLayerVisible(Parameters.kpLayer);
    }
quit:
    InitVLT();
    MenuDeselect(MenuFILL);
    ErasePrompt();
    Parameters.kpRedisplayControl = fi.fi_rc;
    View->kvControl = fi.fi_vc;
    SetPositioning();
    ShowLayerTable();
    RedisplayViewports();

    if (!strcmp(Parameters.kpCommand,MenuFILL)) {
        Parameters.kpCommand[0] = EOS;
        *LookedAhead = False;
    }            
    LockOut = False;
    fi.fi_on = False;
    FBTransfer();
    return;
}


static void
show_choices()

/* show fill pattern menu in magnifying glass */
{
    int j,EboxW,tmpat,tmpst;
    struct ka BOX;

    EraseBox(View->kvFineWindow);

    tmpat = LayerTable[1].klAttributes;
    tmpst = LayerTable[1].klStyleID;

    LayerTable[1].klAttributes = VISIBLE;
    LayerTable[1].klStyleID = 0;

    EboxW        = fi.fi_incr - fi.fi_sep;
    BOX.kaLeft   = View->kvFineWindow->kaLeft;
    BOX.kaRight  = View->kvFineWindow->kaLeft + EboxW;
    BOX.kaTop    = View->kvFineWindow->kaTop - EboxW;
    fi.fi_eboxbot= BOX.kaTop;
    BOX.kaBottom = (View->kvFineWindow->kaBottom + BOX.kaTop)/2;

    /* Filled = False */
    ShowBox(1,&BOX);

    /* Filled = True */
    LayerTable[1].klAttributes |= FILLED;
    BOX.kaTop    = BOX.kaBottom;
    BOX.kaBottom = View->kvFineWindow->kaBottom;
    ShowBox(1,&BOX);

    BOX.kaTop    = View->kvFineWindow->kaTop;
    BOX.kaLeft  += fi.fi_incr;
    BOX.kaRight += fi.fi_incr;

    LayerTable[1].klStyleID = 1;
    LayerTable[1].klAttributes |=
        (FINE_FILL | COARSE_FILL);

    if (View->kvFineViewportOnBottom) {
        BOX.kaBottom = (BOX.kaTop + BOX.kaBottom + fi.fi_sep)/2;
        for  (j = 0; j < 8; j++) {
            if (j)
                FBDefineFillPattern(1,DefaultFillPatterns[j]);
            else 
                FBDefineFillPattern(1,fi.fi_px);
            BOX.kaLeft  += fi.fi_incr;
            BOX.kaRight += fi.fi_incr;
            ShowBox(1,&BOX);
        }
        BOX.kaTop = BOX.kaBottom - fi.fi_sep;
        BOX.kaBottom = View->kvFineWindow->kaBottom;
        BOX.kaLeft = View->kvFineWindow->kaLeft + fi.fi_incr;
        BOX.kaRight = BOX.kaLeft + EboxW;
        for  (j = 8; j < 16; j++) {
            FBDefineFillPattern(1,DefaultFillPatterns[j]);
            BOX.kaLeft  += fi.fi_incr;
            BOX.kaRight += fi.fi_incr;
            ShowBox(1,&BOX);
        }
    }
    else {
        BOX.kaBottom += 3*(BOX.kaTop - BOX.kaBottom)/4 + fi.fi_sep/2;
        for  (j = 0; j < 4; j++) {
            if (j)
                FBDefineFillPattern(1,DefaultFillPatterns[j]);
            else 
                FBDefineFillPattern(1,fi.fi_px);
            BOX.kaLeft  += fi.fi_incr;
            BOX.kaRight += fi.fi_incr;
            ShowBox(1,&BOX);
        }
        BOX.kaTop = BOX.kaBottom - fi.fi_sep;
        BOX.kaBottom -=
            (View->kvFineWindow->kaTop - View->kvFineWindow->kaBottom)/4;
        BOX.kaLeft = View->kvFineWindow->kaLeft + fi.fi_incr;
        BOX.kaRight = BOX.kaLeft + EboxW;
        for  (j = 4; j < 8; j++) {
            FBDefineFillPattern(1,DefaultFillPatterns[j]);
            BOX.kaLeft  += fi.fi_incr;
            BOX.kaRight += fi.fi_incr;
            ShowBox(1,&BOX);
        }
        BOX.kaTop = BOX.kaBottom - fi.fi_sep;
        BOX.kaBottom -=
            (View->kvFineWindow->kaTop - View->kvFineWindow->kaBottom)/4;
        BOX.kaLeft = View->kvFineWindow->kaLeft + fi.fi_incr;
        BOX.kaRight = BOX.kaLeft + EboxW;
        for  (j = 8; j < 12; j++) {
            FBDefineFillPattern(1,DefaultFillPatterns[j]);
            BOX.kaLeft  += fi.fi_incr;
            BOX.kaRight += fi.fi_incr;
            ShowBox(1,&BOX);
        }
        BOX.kaTop = BOX.kaBottom - fi.fi_sep;
        BOX.kaBottom = View->kvFineWindow->kaBottom;
        BOX.kaLeft = View->kvFineWindow->kaLeft + fi.fi_incr;
        BOX.kaRight = BOX.kaLeft + EboxW;
        for  (j = 12; j < 16; j++) {
            FBDefineFillPattern(1,DefaultFillPatterns[j]);
            BOX.kaLeft  += fi.fi_incr;
            BOX.kaRight += fi.fi_incr;
            ShowBox(1,&BOX);
        }
    }

    fi.fi_del = EboxW/8;
    fi.fi_spa = EboxW/40;
    show_pixel_map();

    LayerTable[1].klAttributes = tmpat;
    LayerTable[1].klStyleID = tmpst;
}


static void
show_pixel_map()

/* display the pixel unit cell map and fill preview box */
{
    int i,j,tmpat,tmpst;
    struct ka BOX, PBOX;

    tmpat = LayerTable[1].klAttributes;
    tmpst = LayerTable[1].klStyleID;

    LayerTable[1].klAttributes = VISIBLE | FILLED;
    LayerTable[1].klStyleID = 0;

    BOX.kaLeft   = View->kvFineWindow->kaLeft;
    BOX.kaRight  = View->kvFineWindow->kaLeft + fi.fi_incr - fi.fi_sep;
    BOX.kaBottom = fi.fi_eboxbot;
    BOX.kaTop    = View->kvFineWindow->kaTop;

    EraseBox(&BOX);
    ShowEmptyBox(1,&BOX);

    /* Pixel map */
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++) {
            if (!(fi.fi_px[i] & (1 << j))) continue;
            PBOX.kaLeft   = View->kvFineWindow->kaLeft +
                j*fi.fi_del + fi.fi_spa;
            PBOX.kaRight  = View->kvFineWindow->kaLeft +
                (j+1)*fi.fi_del - fi.fi_spa;
            PBOX.kaBottom = fi.fi_eboxbot + i*fi.fi_del + fi.fi_spa;
            PBOX.kaTop    = fi.fi_eboxbot + (i+1)*fi.fi_del - fi.fi_spa;
            ShowBox(1,&PBOX);
        }

    LayerTable[1].klAttributes |=
        (OUTLINED | FINE_FILL | COARSE_FILL);
    LayerTable[1].klStyleID = 1;

    FBDefineFillPattern(1,fi.fi_px);

    BOX.kaLeft  += fi.fi_incr;
    BOX.kaRight += fi.fi_incr;
    BOX.kaBottom = View->kvFineWindow->kaBottom;
    EraseBox(&BOX);
    ShowBox(1,&BOX);
    FBTransfer();

    LayerTable[1].klAttributes = tmpat;
    LayerTable[1].klStyleID = tmpst;
}


static void
flip_pixel()

/* toggle pixels in the pixel map and redisplay preview */
{
    int i,j,tmpat,tmpst;
    struct ka BOX, PBOX;

    j = (8*(KicCursor.kcRawX - View->kvFineWindow->kaLeft))/
        (fi.fi_incr - fi.fi_sep);
    i = (8*(KicCursor.kcRawY - fi.fi_eboxbot))/(fi.fi_incr - fi.fi_sep);
    fi.fi_px[i] ^= (1 << j);

    PBOX.kaLeft   = View->kvFineWindow->kaLeft + j*fi.fi_del + fi.fi_spa;
    PBOX.kaRight  = View->kvFineWindow->kaLeft + (j+1)*fi.fi_del - fi.fi_spa;
    PBOX.kaBottom = fi.fi_eboxbot + i*fi.fi_del + fi.fi_spa;
    PBOX.kaTop    = fi.fi_eboxbot + (i+1)*fi.fi_del - fi.fi_spa;

    tmpat = LayerTable[1].klAttributes;
    tmpst = LayerTable[1].klStyleID;
    LayerTable[1].klAttributes = VISIBLE | FILLED;
    LayerTable[1].klStyleID = 0;

    if (fi.fi_px[i] & (1 << j)) {
        SetCurrentAOI(View->kvFineWindow);
        ShowBox(1,&PBOX);
    }
    else
        EraseBox(&PBOX);

    LayerTable[1].klAttributes |=
        (OUTLINED | FINE_FILL | COARSE_FILL);
    LayerTable[1].klStyleID = 1;

    FBDefineFillPattern(1,fi.fi_px);

    BOX.kaLeft    = View->kvFineWindow->kaLeft + fi.fi_incr;
    BOX.kaRight   = View->kvFineWindow->kaLeft + 2*fi.fi_incr - fi.fi_sep;
    BOX.kaBottom  = View->kvFineWindow->kaBottom;
    BOX.kaTop     = View->kvFineWindow->kaTop;
    EraseBox(&BOX);
    ShowBox(1,&BOX);
    FBTransfer();

    LayerTable[1].klAttributes = tmpat;
    LayerTable[1].klStyleID = tmpst;
}


int
RepaintFILL()

/* Repaint screen while in fill pattern editor.  Does the complete
 * job.
 */
{
    int i;

    if (!fi.fi_on)
        return (False);

    Parameters.kpRedisplayControl = fi.fi_rc;
    View->kvControl = fi.fi_vc;
    FullRedisplay();

    View->kvControl = SPLITSCREEN;
    SetPositioning();
    Parameters.kpRedisplayControl = FINEVIEWPORTONLY;

    /* compute spacing between fill pattern menu */
    i = 10;
    if (!View->kvFineViewportOnBottom) i = 6;
    fi.fi_incr = View->kvFineWindow->kaWidth/i;
    fi.fi_sep = fi.fi_incr >> 3;

    show_choices();
    return (True);
}

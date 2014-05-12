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
 * Initialize KIC. 
 */

#include "prefix.h"
#include "kic.h"

struct kv *View;
struct ka MenuViewport;
struct ka ParameterViewport;
struct ka LayerTableViewport;
struct kp Parameters;
struct kl LayerTable[CDNUMLAYERS+1];
struct kc KicCursor;
struct a  CurrentAOI;
struct cl ColorTable[12];
int NumLayerTable;
char TypeOut[200];


#ifdef __STDC__
static void init_ColorTable1(void);
static void init_ColorTable2(void);
static void match(int);
#else
static void init_ColorTable1();
static void init_ColorTable2();
static void match();
#endif


/* If the ratio of window to viewport width is less than this, use the
 * stippled fill patterns.  Otherwise, use a solid fill.
 */
/* I don't like this "feature"
#define STIPPLERATIO 100
*/
#define STIPPLERATIO 0x7fffffffL


void
Init()

{
    /*
     * Read in .KIC file and initialize CD package.
     */
    init_ColorTable1();
    ReadTechFile();
    FBBegin(FB.fDisplay);
    init_ColorTable2();
    InitParameters();
    InitSignals();
}


static void
init_ColorTable1()

{
    ColorTable[0].Ent = -1;
    ColorTable[0].R = 0;
    ColorTable[0].G = 0;
    ColorTable[0].B = 0;

    ColorTable[HighlightingColor].Ent = -1;
    ColorTable[HighlightingColor].R = 240;
    ColorTable[HighlightingColor].G = 240;
    ColorTable[HighlightingColor].B = 240;

    ColorTable[MenuPromptColor].Ent = -1;
    ColorTable[MenuPromptColor].R = 255;
    ColorTable[MenuPromptColor].G = 235;
    ColorTable[MenuPromptColor].B = 110;

    ColorTable[MenuTextColor].Ent = -1;
    ColorTable[MenuTextColor].R = 120;
    ColorTable[MenuTextColor].G = 240;
    ColorTable[MenuTextColor].B = 0;

    ColorTable[MenuHighlightingColor].Ent = -1;
    ColorTable[MenuHighlightingColor].R = 110;
    ColorTable[MenuHighlightingColor].G = 160;
    ColorTable[MenuHighlightingColor].B = 255;

    ColorTable[MenuSelectColor].Ent = -1;
    ColorTable[MenuSelectColor].R = 180;
    ColorTable[MenuSelectColor].G = 250;
    ColorTable[MenuSelectColor].B = 150;

    ColorTable[MoreTextColor].Ent = -1;
    ColorTable[MoreTextColor].R = 170;
    ColorTable[MoreTextColor].G = 210;
    ColorTable[MoreTextColor].B = 200;

    ColorTable[InstanceBBColor].Ent = -1;
    ColorTable[InstanceBBColor].R = 150;
    ColorTable[InstanceBBColor].G = 225;
    ColorTable[InstanceBBColor].B = 195;

    ColorTable[InstanceNameColor].Ent = -1;
    ColorTable[InstanceNameColor].R = 180;
    ColorTable[InstanceNameColor].G = 60;
    ColorTable[InstanceNameColor].B = 0;

    ColorTable[InstanceSizeColor].Ent = -1;
    ColorTable[InstanceSizeColor].R = 220;
    ColorTable[InstanceSizeColor].G = 85;
    ColorTable[InstanceSizeColor].B = 180;

    ColorTable[CoarseGridColor].Ent = -1;
    ColorTable[CoarseGridColor].R = 150;
    ColorTable[CoarseGridColor].G = 170;
    ColorTable[CoarseGridColor].B = 255;

    ColorTable[FineGridColor].Ent = -1;
    ColorTable[FineGridColor].R = 60;
    ColorTable[FineGridColor].G = 20;
    ColorTable[FineGridColor].B = 200;
}


static void
init_ColorTable2()

{
    int i, ind = FB.fNumColors;

    ColorTable[0].Ent = 0;
    ColorTable[HighlightingColor].Ent = ind--;

    if (!Parameters.kpMergeColors) {

        if (ColorTable[MenuPromptColor].Ent >= 0) {
            i = ColorTable[MenuPromptColor].Ent;
            ColorTable[MenuPromptColor].R = LayerTable[i].klR;
            ColorTable[MenuPromptColor].G = LayerTable[i].klG;
            ColorTable[MenuPromptColor].B = LayerTable[i].klB;
        }
        ColorTable[MenuPromptColor].Ent = ind--;

        if (ColorTable[MenuTextColor].Ent >= 0) {
            i = ColorTable[MenuTextColor].Ent;
            ColorTable[MenuTextColor].R = LayerTable[i].klR;
            ColorTable[MenuTextColor].G = LayerTable[i].klG;
            ColorTable[MenuTextColor].B = LayerTable[i].klB;
        }
        ColorTable[MenuTextColor].Ent = ind--;

        if (ColorTable[MenuHighlightingColor].Ent >= 0) {
            i = ColorTable[MenuHighlightingColor].Ent;
            ColorTable[MenuHighlightingColor].R = LayerTable[i].klR;
            ColorTable[MenuHighlightingColor].G = LayerTable[i].klG;
            ColorTable[MenuHighlightingColor].B = LayerTable[i].klB;
        }
        ColorTable[MenuHighlightingColor].Ent = ind--;

        if (ColorTable[MenuSelectColor].Ent >= 0) {
            i = ColorTable[MenuSelectColor].Ent;
            ColorTable[MenuSelectColor].R = LayerTable[i].klR;
            ColorTable[MenuSelectColor].G = LayerTable[i].klG;
            ColorTable[MenuSelectColor].B = LayerTable[i].klB;
        }
        ColorTable[MenuSelectColor].Ent = ind--;

        if (ColorTable[MoreTextColor].Ent >= 0) {
            i = ColorTable[MoreTextColor].Ent;
            ColorTable[MoreTextColor].R = LayerTable[i].klR;
            ColorTable[MoreTextColor].G = LayerTable[i].klG;
            ColorTable[MoreTextColor].B = LayerTable[i].klB;
        }
        ColorTable[MoreTextColor].Ent = ind--;

        if (ColorTable[InstanceBBColor].Ent >= 0) {
            i = ColorTable[InstanceBBColor].Ent;
            ColorTable[InstanceBBColor].R = LayerTable[i].klR;
            ColorTable[InstanceBBColor].G = LayerTable[i].klG;
            ColorTable[InstanceBBColor].B = LayerTable[i].klB;
        }
        ColorTable[InstanceBBColor].Ent = ind--;

        if (ColorTable[InstanceNameColor].Ent >= 0) {
            i = ColorTable[InstanceNameColor].Ent;
            ColorTable[InstanceNameColor].R = LayerTable[i].klR;
            ColorTable[InstanceNameColor].G = LayerTable[i].klG;
            ColorTable[InstanceNameColor].B = LayerTable[i].klB;
        }
        ColorTable[InstanceNameColor].Ent = ind--;

        if (ColorTable[InstanceSizeColor].Ent >= 0) {
            i = ColorTable[InstanceSizeColor].Ent;
            ColorTable[InstanceSizeColor].R = LayerTable[i].klR;
            ColorTable[InstanceSizeColor].G = LayerTable[i].klG;
            ColorTable[InstanceSizeColor].B = LayerTable[i].klB;
        }
        ColorTable[InstanceSizeColor].Ent = ind--;

        if (ColorTable[CoarseGridColor].Ent >= 0) {
            i = ColorTable[CoarseGridColor].Ent;
            ColorTable[CoarseGridColor].R = LayerTable[i].klR;
            ColorTable[CoarseGridColor].G = LayerTable[i].klG;
            ColorTable[CoarseGridColor].B = LayerTable[i].klB;
        }
        ColorTable[CoarseGridColor].Ent = ind--;

        if (ColorTable[FineGridColor].Ent >= 0) {
            i = ColorTable[FineGridColor].Ent;
            ColorTable[FineGridColor].R = LayerTable[i].klR;
            ColorTable[FineGridColor].G = LayerTable[i].klG;
            ColorTable[FineGridColor].B = LayerTable[i].klB;
        }
        ColorTable[FineGridColor].Ent = ind--;

    }
    else {

        if (ColorTable[MenuPromptColor].Ent < 0)
            match(MenuPromptColor);

        if (ColorTable[MenuTextColor].Ent < 0)
            match(MenuTextColor);

        if (ColorTable[MenuHighlightingColor].Ent < 0)
            match(MenuHighlightingColor);

        if (ColorTable[MenuSelectColor].Ent < 0)
            match(MenuSelectColor);

        if (ColorTable[MoreTextColor].Ent < 0)
            match(MoreTextColor);

        if (ColorTable[InstanceBBColor].Ent < 0)
            match(InstanceBBColor);

        if (ColorTable[InstanceNameColor].Ent < 0)
            match(InstanceNameColor);

        if (ColorTable[InstanceSizeColor].Ent < 0)
            match(InstanceSizeColor);

        if (ColorTable[CoarseGridColor].Ent < 0)
            match(CoarseGridColor);

        if (ColorTable[FineGridColor].Ent < 0)
            match(FineGridColor);
    }
}


static void
match(what)

/* find the layer with the closest color match */
int what;
{
    int Layer;
    int dd, ddmax;
    int m = 0;
    int r, g, b;

    ddmax = 3*255*255+1;
    r = ColorTable[what].R;
    g = ColorTable[what].G;
    b = ColorTable[what].B;

    for (Layer = 1; Layer <= NumLayerTable; Layer++) {
        dd =
            (r - LayerTable[Layer].klR)*(r - LayerTable[Layer].klR) +
            (g - LayerTable[Layer].klG)*(g - LayerTable[Layer].klG) +
            (b - LayerTable[Layer].klB)*(b - LayerTable[Layer].klB);
        if (dd < ddmax) {
            m = Layer;
            ddmax = dd;
        }
    }
    ColorTable[what].Ent = m;
}


void
InitVLT()

{
    int Layer,styleID;
    int TFine,TCoarse;
    int fp[8];

    FBSetCursorColor(ColorTable[HighlightingColor].Ent);
    /*
     * Background color is zero.
     */
    FBVLT(0,ColorTable[0].R,ColorTable[0].G,ColorTable[0].B);
    FBVLT(ColorTable[HighlightingColor].Ent,ColorTable[HighlightingColor].R,
        ColorTable[HighlightingColor].G,ColorTable[HighlightingColor].B);

    if (!Parameters.kpMergeColors) {

        FBVLT(ColorTable[MenuPromptColor].Ent,
            ColorTable[MenuPromptColor].R,
            ColorTable[MenuPromptColor].G,
            ColorTable[MenuPromptColor].B);

        FBVLT(ColorTable[MenuTextColor].Ent,
            ColorTable[MenuTextColor].R,
            ColorTable[MenuTextColor].G,
            ColorTable[MenuTextColor].B);

        FBVLT(ColorTable[MenuHighlightingColor].Ent,
            ColorTable[MenuHighlightingColor].R,
            ColorTable[MenuHighlightingColor].G,
            ColorTable[MenuHighlightingColor].B);

        FBVLT(ColorTable[MenuSelectColor].Ent,
            ColorTable[MenuSelectColor].R,
            ColorTable[MenuSelectColor].G,
            ColorTable[MenuSelectColor].B);

        FBVLT(ColorTable[MoreTextColor].Ent,
            ColorTable[MoreTextColor].R,
            ColorTable[MoreTextColor].G,
            ColorTable[MoreTextColor].B);

        FBVLT(ColorTable[InstanceBBColor].Ent,
            ColorTable[InstanceBBColor].R,
            ColorTable[InstanceBBColor].G,
            ColorTable[InstanceBBColor].B);

        FBVLT(ColorTable[InstanceNameColor].Ent,
            ColorTable[InstanceNameColor].R,
            ColorTable[InstanceNameColor].G,
            ColorTable[InstanceNameColor].B);

        FBVLT(ColorTable[InstanceSizeColor].Ent,
            ColorTable[InstanceSizeColor].R,
            ColorTable[InstanceSizeColor].G,
            ColorTable[InstanceSizeColor].B);

        FBVLT(ColorTable[CoarseGridColor].Ent,
            ColorTable[CoarseGridColor].R,
            ColorTable[CoarseGridColor].G,
            ColorTable[CoarseGridColor].B);

        FBVLT(ColorTable[FineGridColor].Ent,
            ColorTable[FineGridColor].R,
            ColorTable[FineGridColor].G,
            ColorTable[FineGridColor].B);
    }

    /*
     * Layer colors.
     */
    styleID = 1; /* 0 is solid */
    for (Layer = 1; Layer <= NumLayerTable; Layer++) {
        FBVLT(Layer,LayerTable[Layer].klR,LayerTable[Layer].klG,
            LayerTable[Layer].klB);
        if (LayerTable[Layer].klAttributes & BLINK)
            FBBlink(Layer,1000,1000,1000,1);
        if (!FB.fDefinableFillPatterns) {
            LayerTable[Layer].klStyleID = 0;
            continue;
        }
        if (!(LayerTable[Layer].klAttributes & FILLED)) {
            LayerTable[Layer].klStyleID = 0;
            continue;
        }
        if ((LayerTable[Layer].klAttributes & FILLED) And
            LayerTable[Layer].klStyle[0] == 0 And
            LayerTable[Layer].klStyle[1] == 0 And
            LayerTable[Layer].klStyle[2] == 0 And
            LayerTable[Layer].klStyle[3] == 0 And
            LayerTable[Layer].klStyle[4] == 0 And
            LayerTable[Layer].klStyle[5] == 0 And
            LayerTable[Layer].klStyle[6] == 0 And
            LayerTable[Layer].klStyle[7] == 0) {
            LayerTable[Layer].klStyleID = 0;
            continue;
        }
        FBForeground(DISPLAY,Layer);
        LayerTable[Layer].klStyleID = styleID;
        fp[0] = LayerTable[Layer].klStyle[0];
        fp[1] = LayerTable[Layer].klStyle[1];
        fp[2] = LayerTable[Layer].klStyle[2];
        fp[3] = LayerTable[Layer].klStyle[3];
        fp[4] = LayerTable[Layer].klStyle[4];
        fp[5] = LayerTable[Layer].klStyle[5];
        fp[6] = LayerTable[Layer].klStyle[6];
        fp[7] = LayerTable[Layer].klStyle[7];
        
        FBDefineFillPattern(styleID,fp);
        styleID++;
    }

    TFine = TCoarse = False;
    if (1/View->kvFineRatio < STIPPLERATIO)
        TFine = True;
    if (1/View->kvCoarseRatio < STIPPLERATIO)
        TCoarse = True;

    for (Layer = 1; Layer <= NumLayerTable; Layer++) {
        if(TFine)
            LayerTable[Layer].klAttributes |= FINE_FILL;
        else
            LayerTable[Layer].klAttributes &= ~FINE_FILL;
        if(TCoarse)
            LayerTable[Layer].klAttributes |= COARSE_FILL;
        else
            LayerTable[Layer].klAttributes &= ~COARSE_FILL;
    }
}


void
InitParameters()

{
    /* set in techfile.c */
    extern int FineVPonBottom;
    extern char InitScreenMode;

    Parameters.kpCellName = malloc(80);
    if (Parameters.kpCellName == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    Parameters.kpTopName = malloc(80);
    if (Parameters.kpTopName == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    Parameters.kpCommand = malloc(80);
    if (Parameters.kpCommand == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    Parameters.kpTopDesc                  = NULL;
    Parameters.kpCellDesc                 = NULL;
    Parameters.kpShowContext              = True;
    Parameters.kpLayer                    = 1;
    Parameters.kp45s                      = False;
    Parameters.kpStretchType              = STR_TBRL;
    Parameters.kpEnableSelectQRedisplay   = True;
    Parameters.kpLayerSpecificSelection   = False;
    Parameters.kpClipVerticesToGrid       = False;
    Parameters.kpExpandInstances          = False;
    Parameters.kpShowInstanceMarkers      = False;
    Parameters.kpRotationAngle            = 0;
    Parameters.kpWindowStack              = NULL;
    Parameters.kpSelectTypes[0]           = CDSYMBOLCALL;
    Parameters.kpSelectTypes[1]           = CDPOLYGON;
    Parameters.kpSelectTypes[2]           = CDROUNDFLASH;
    Parameters.kpSelectTypes[3]           = CDLABEL;
    Parameters.kpSelectTypes[4]           = CDWIRE;
    Parameters.kpSelectTypes[5]           = CDBOX;
    Parameters.kpSelectTypes[6]           = EOS;
    Parameters.kpNumX                     = 1;
    Parameters.kpNumY                     = 1;
    Parameters.kpDX                       = 0;
    Parameters.kpDY                       = 0;

    /*
     * The fine window has a FIXED width and height. 
     * What should it be?
     * PointingThreshold is the minimum value of ViewportWidth/WindowWidth
     * such that it is still comfortable to point with lambda precision.
     * If PointingThreshold == 7 and FineViewportWidth ~ 467 pixels, 
     * FineWindowWidth ~ 60 lambda.
     * Most people have prefered 6 or 7.
     */
    Parameters.kpPointingThreshold = 6;

    Parameters.kpShowBandwidth = False;

    KicCursor.kcX = KicCursor.kcPredX = 0;
    KicCursor.kcY = KicCursor.kcPredY = 0;

    Parameters.kpModified = False;

    View = alloc(kv);
    View->kvFineWindow = alloc(ka);
    View->kvFineViewport = alloc(ka);
    View->kvLargeCoarseViewport = alloc(ka);
    View->kvSmallCoarseViewport = alloc(ka);
    View->kvCoarseWindow = alloc(ka);
    View->kvFineViewportOnBottom = FineVPonBottom;

    /* can start out in FULLSCREEN or SPLITSCREEN mode */
    View->kvControl = InitScreenMode;

    TInit();
    TStore();
    InitViewport();
    DefaultWindows();
    SQInit();
}


void
DefaultWindows()

{
    int DefWidth;

    View->kvCoarseViewport = View->kvLargeCoarseViewport;
    if (View->kvControl == FULLSCREEN)
        DefWidth = 100;
    else
        DefWidth = 1000;
    InitCoarseWindow(0,0,DefWidth*RESOLUTION);

    /* default fine window */
    View->kvFineWindow->kaWidth = View->kvFineViewport->kaWidth/
        Parameters.kpPointingThreshold*RESOLUTION;
 
    View->kvFineWindow->kaHeight = View->kvFineWindow->kaWidth*
        View->kvFineViewport->kaHeight/View->kvFineViewport->kaWidth;
 
    SetPositioning();
    SaveLastView();
}


void
InitCoarseWindow(X,Y,Width)

int X,Y,Width;
{
    int diff;
    int Layer;
    double Wid2;
    struct ka *Cw;

    Wid2 = Width/2;
    diff = (int)((Wid2*View->kvCoarseViewport->kaHeight)/
        View->kvCoarseViewport->kaWidth);

    Cw = View->kvCoarseWindow;
    Cw->kaX      = X;
    Cw->kaY      = Y; 
    Cw->kaLeft   = Cw->kaX - Wid2; 
    Cw->kaRight  = Cw->kaX + Wid2; 
    Cw->kaBottom = Cw->kaY - diff;
    Cw->kaTop    = Cw->kaY + diff;
    Cw->kaWidth  = Cw->kaRight - Cw->kaLeft;
    Cw->kaHeight = Cw->kaTop - Cw->kaBottom;

    View->kvCoarseRatio = View->kvCoarseViewport->kaWidth/
        Cw->kaWidth;

    if (1/View->kvCoarseRatio < STIPPLERATIO)
        for (Layer = 1; Layer <= NumLayerTable; Layer++)
            LayerTable[Layer].klAttributes |= COARSE_FILL;
    else
        for (Layer = 1; Layer <= NumLayerTable; Layer++)
            LayerTable[Layer].klAttributes &= ~COARSE_FILL;
}


void
InitFineWindow(X,Y)

int X,Y;
{
    /* Width and height are fixed. */
    int diff;
    int Layer;
    double Wid2;
    struct ka *Fw;

    Wid2 = View->kvFineWindow->kaWidth/2;
    diff = (int)((Wid2*View->kvFineViewport->kaHeight)/
        View->kvFineViewport->kaWidth);

    Fw = View->kvFineWindow;
    Fw->kaX      = X;
    Fw->kaY      = Y;
    Fw->kaLeft   = Fw->kaX - Wid2; 
    Fw->kaRight  = Fw->kaX + Wid2; 
    Fw->kaBottom = Fw->kaY - diff;
    Fw->kaTop    = Fw->kaY + diff; 
    Fw->kaWidth  = Fw->kaRight - Fw->kaLeft;
    Fw->kaHeight = Fw->kaTop - Fw->kaBottom;

    View->kvFineRatio = View->kvFineViewport->kaWidth/Fw->kaWidth;

    if (1/View->kvFineRatio < STIPPLERATIO)
        for(Layer = 1;Layer <= NumLayerTable;++Layer)
            LayerTable[Layer].klAttributes |= FINE_FILL;
    else
        for(Layer = 1;Layer <= NumLayerTable;++Layer)
            LayerTable[Layer].klAttributes &= ~FINE_FILL;
}


void
SetPositioning()

{
    int X,Y,Width;

    if (View->kvControl != FULLSCREEN And
        View->kvCoarseRatio < Parameters.kpPointingThreshold) {
        /*
         * Split--enable fine positioning.
         */
        Parameters.kpRedisplayControl = SPLITSCREEN;
        View->kvCoarseViewport = View->kvSmallCoarseViewport;
    }
    else {
        /*
         * Don't split--don't enable fine positioning.
         */
        Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
        View->kvCoarseViewport = View->kvLargeCoarseViewport;
    }
    Width = View->kvCoarseWindow->kaWidth;
    X     = View->kvCoarseWindow->kaX;
    Y     = View->kvCoarseWindow->kaY;
    InitCoarseWindow(X,Y,Width);

    /* it is enough to just check the width */
    if (Width <= View->kvFineWindow->kaWidth) {
        View->kvFineWindow->kaWidth = Width - Width/4;
        View->kvFineWindow->kaHeight = View->kvFineWindow->kaWidth*
            (View->kvFineViewport->kaHeight/View->kvFineWindow->kaWidth);
    }
    InitFineWindow(X,Y);
}


void
InitViewport()

{
    struct ka *Vl, *Vs, *Vf;
    extern int NumBasicMenu;

    Parameters.kpLayersPerMenuRow = FB.fNumColumns/6;
    Parameters.kpNumLayerMenuRows =
        (NumLayerTable - 1)/Parameters.kpLayersPerMenuRow + 1;

    if (Parameters.kpNumLayerMenuRows > 1) {
        Parameters.kpLayersPerMenuRow--;
        Parameters.kpNumLayerMenuRows =
            (NumLayerTable - 1)/Parameters.kpLayersPerMenuRow + 1;
    }

    MenuViewport.kaLeft         = 1;
    MenuViewport.kaBottom       = FB.fNumRows - 3;
    MenuViewport.kaTop          = 1;
    if (NumBasicMenu < MenuViewport.kaBottom)
        MenuViewport.kaRight    = 5;
    else
        MenuViewport.kaRight    = 11;

    /* save column width and height in X,Y integer fields */
    MenuViewport.kaX            = 5;
    MenuViewport.kaY            = MenuViewport.kaBottom -
                                      MenuViewport.kaTop + 1;

    LayerTableViewport.kaLeft   = 1;
    LayerTableViewport.kaBottom = FB.fNumRows;
    LayerTableViewport.kaRight  = FB.fNumColumns;
    LayerTableViewport.kaTop    = FB.fNumRows;

    ParameterViewport.kaLeft    = 1;
    ParameterViewport.kaBottom  = FB.fNumRows - 1;
    ParameterViewport.kaRight   = FB.fNumColumns;
    ParameterViewport.kaTop     = FB.fNumRows - 1;

    /*
     * The COARSE viewport is LARGE if fine-positioning isn't enabled.
     * If it is, the LARGE COARSE viewport is split into a SMALL COARSE 
     * viewport and the FINE viewport.
     */
    if (View->kvFineViewportOnBottom) {

        Vl = View->kvLargeCoarseViewport;
        Vl->kaLeft   = FB.fFontWidth*MenuViewport.kaRight + FB.fFontWidth/2;
        Vl->kaBottom = FB.fFontHeight*4;
        Vl->kaRight  = FB.fMaxX;
        Vl->kaTop    = FB.fMaxY;
        Vl->kaWidth  = Vl->kaRight - Vl->kaLeft;
        Vl->kaHeight = Vl->kaTop - Vl->kaBottom;

        Vs = View->kvSmallCoarseViewport;
        Vs->kaLeft   = Vl->kaLeft; 
        Vs->kaBottom = Vl->kaBottom + ((Vl->kaTop - Vl->kaBottom)*5)/12;
        Vs->kaRight  = Vl->kaRight; 
        Vs->kaTop    = Vl->kaTop;
        Vs->kaWidth  = Vs->kaRight - Vs->kaLeft;
        Vs->kaHeight = Vs->kaTop - Vs->kaBottom;
    
        Vf = View->kvFineViewport;
        Vf->kaLeft   = Vs->kaLeft; 
        Vf->kaBottom = Vl->kaBottom; 
        Vf->kaRight  = Vs->kaRight; 
        Vf->kaTop    = Vs->kaBottom - 1;
        Vf->kaWidth  = Vf->kaRight - Vf->kaLeft;
        Vf->kaHeight = Vf->kaTop - Vf->kaBottom;
    }
    else {

        Vl = View->kvLargeCoarseViewport;
        Vl->kaLeft   = FB.fFontWidth*MenuViewport.kaRight + FB.fFontWidth/2;
        Vl->kaBottom = FB.fFontHeight*4;
        Vl->kaRight  = FB.fMaxX;
        Vl->kaTop    = FB.fMaxY;
        Vl->kaWidth  = Vl->kaRight - Vl->kaLeft;
        Vl->kaHeight = Vl->kaTop - Vl->kaBottom;

        Vs = View->kvSmallCoarseViewport;
        Vs->kaLeft   = Vl->kaLeft; 
        Vs->kaBottom = Vl->kaBottom;
        Vs->kaRight  = (FB.fMaxX - Vl->kaLeft)/2 + Vl->kaLeft;
        Vs->kaTop    = Vl->kaTop; 
        Vs->kaWidth  = Vs->kaRight - Vs->kaLeft;
        Vs->kaHeight = Vs->kaTop - Vs->kaBottom;
      
        Vf = View->kvFineViewport;
        Vf->kaLeft   = Vs->kaRight + 1; 
        Vf->kaBottom = Vl->kaBottom; 
        Vf->kaRight  = Vl->kaRight; 
        Vf->kaTop    = Vs->kaTop;
        Vf->kaWidth  = Vf->kaRight - Vf->kaLeft;
        Vf->kaHeight = Vf->kaTop - Vf->kaBottom;
    }
    /* Inform X of the size and position of the InputOnly window
     * used to solicit pointer motion and window enter/leave events.
     */
    FBResizeDrawingWindow(Vl->kaLeft,Vl->kaBottom,
        Vl->kaRight,Vl->kaTop);
}


void
SetCurrentAOI(Window)

struct ka *Window;
{
    CoarseLToP(Window->kaLeft,Window->kaBottom,CurrentAOI.aLC,CurrentAOI.aBC);
    CoarseLToP(Window->kaRight,Window->kaTop,CurrentAOI.aRC,CurrentAOI.aTC);
    if (CurrentAOI.aLC <= View->kvCoarseViewport->kaRight And
        CurrentAOI.aRC >= View->kvCoarseViewport->kaLeft And
        CurrentAOI.aBC <= View->kvCoarseViewport->kaTop And
        CurrentAOI.aTC >= View->kvCoarseViewport->kaBottom) {
        ClipVP(View->kvCoarseViewport,CurrentAOI.aLC,CurrentAOI.aBC);
        ClipVP(View->kvCoarseViewport,CurrentAOI.aRC,CurrentAOI.aTC);
        CurrentAOI.aInCoarse = True;
    }
    else
        CurrentAOI.aInCoarse = False;

    FineLToP(Window->kaLeft,Window->kaBottom,CurrentAOI.aLF,CurrentAOI.aBF);
    FineLToP(Window->kaRight,Window->kaTop,CurrentAOI.aRF,CurrentAOI.aTF);
    if (CurrentAOI.aLF <= View->kvFineViewport->kaRight And
        CurrentAOI.aRF >= View->kvFineViewport->kaLeft And
        CurrentAOI.aBF <= View->kvFineViewport->kaTop And
        CurrentAOI.aTF >= View->kvFineViewport->kaBottom) {
        ClipVP(View->kvFineViewport,CurrentAOI.aLF,CurrentAOI.aBF);
        ClipVP(View->kvFineViewport,CurrentAOI.aRF,CurrentAOI.aTF);
        CurrentAOI.aInFine = True;
    }
    else
        CurrentAOI.aInFine = False;
}

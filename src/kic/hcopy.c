/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1994
 *
 *************************************************************************/
/*************************************************************************
 *
 * Hcopy command.
 *
 *************************************************************************/

#include "prefix.h"
#include "kic.h"

#include <sys/types.h>
#include <time.h>

#define Matching(string) !strcmp(Parameters.kpCommand,string)

#ifdef __STDC__
static void display();
static int  legend(int);
static int frame(int*);
#else
static void display();
static int  legend();
static int frame();
#endif

extern char *MenuHCOPY;
extern char *MenuHELP;
extern char *MenuFILL;
extern char *MenuVISIB;
extern char *MenuGRID;
extern char *MenuRDRAW;
char *MenuRESOL = "resol";
char *MenuPRN   = "prn  ";
char *MenuGO    = "go   ";
char *MenuFRAME = "frame";
char *MenuHCTYP = "hctyp";

static int FrameOn;
static struct ka FrameBox;

void
Hcopy()

{

    int Int1,dummy;
    int didcopy = False;
    int tmpgrid;
    char *outname = NULL,*TypeIn;
    char OldMenu;
    char *tmpDsp;
    struct { short attr; unsigned char pattrn[8];} Temp[CDNUMLAYERS+1];
#ifndef MSDOS
    char pname[256];
    char tf[32];
#endif

    MenuSelect(MenuHCOPY);
    Parameters.kpDoingHardcopy = True;
    tmpgrid = Parameters.kpGridDisplayed;
    Parameters.kpGridDisplayed = Parameters.kpHardcopyGrid;
    for (Int1 = 1; Int1 <= NumLayerTable; Int1++) {
        Temp[Int1].attr = LayerTable[Int1].klAttributes;
        memcpy(Temp[Int1].pattrn,LayerTable[Int1].klStyle,8);
        memcpy(LayerTable[Int1].klStyle,LayerTable[Int1].klAltStyle,8);
        if (LayerTable[Int1].klAttributes & ALT_FILLED)
            LayerTable[Int1].klAttributes |= FILLED;
        else
            LayerTable[Int1].klAttributes &= ~FILLED;
        if (LayerTable[Int1].klAttributes & ALT_OUTLINED)
            LayerTable[Int1].klAttributes |= OUTLINED;
        else
            LayerTable[Int1].klAttributes &= ~OUTLINED;
        if (LayerTable[Int1].klAttributes & ALT_VISIBLE)
            LayerTable[Int1].klAttributes |= VISIBLE;
        else
            LayerTable[Int1].klAttributes &= ~VISIBLE;
    }
    InitVLT();
    RedisplayViewports();
    ErasePrompt();

    ShowLayerTable();
    OldMenu = Parameters.kpMenu;
    Parameters.kpMenu = AMBIGUITYMENU;

    AmbiguityMenu[0].mEntry = MenuHELP;
    AmbiguityMenu[1].mEntry = MenuFRAME;
    AmbiguityMenu[2].mEntry = MenuGO;
    AmbiguityMenu[3].mEntry = "     ";
    AmbiguityMenu[4].mEntry = MenuFILL;
    AmbiguityMenu[5].mEntry = MenuVISIB;
    AmbiguityMenu[6].mEntry = MenuGRID;
    AmbiguityMenu[7].mEntry = MenuRDRAW;
    AmbiguityMenu[8].mEntry = "     ";
    AmbiguityMenu[9].mEntry = MenuRESOL;
    AmbiguityMenu[10].mEntry = MenuPRN;
    AmbiguityMenu[11].mEntry = MenuHCTYP;
    AmbiguityMenu[12].mEntry = NULL;
    FixMenuPrefix(AmbiguityMenu);
    ShowMenu(AmbiguityMenu);

    loop {
        switch (PointLoopSafe(&dummy)) {
        case PL_ESC:
            goto quit;
        case PL_PCW:
            continue;
        case PL_CMD:

            if (Matching(MenuHELP))  {Help();          continue;}
            if (Matching(MenuFILL))  {Fill(&dummy);    continue;}
            if (Matching(MenuVISIB)) {Visib(&dummy);   continue;}
            if (Matching(MenuGRID))  {SetGrid(&dummy); continue;}
            if (Matching(MenuRDRAW)) {Rdraw();         continue;}
            if (Matching(MenuFRAME)) {
                if (frame(&dummy))
                    goto quit;
                continue;
            }
            if (Matching(MenuRESOL)) {
                MenuSelect(MenuRESOL);
                sprintf(TypeOut,
"Resolution is %d dpi.  Enter new resolution (75 100 150 300): ",
Parameters.kpHardcopyResolution);
                ShowPrompt(TypeOut);
                for (;;) {
                    if ((TypeIn = FBEdit(NULL)) == NULL) break;
                    if (*TypeIn == '\0' || *TypeIn == '\n') break;
                    Int1 = atoi(TypeIn);
                    if (Int1 == 75 || Int1 == 100 ||
                        Int1 == 150 || Int1 == 300) {
                        Parameters.kpHardcopyResolution = Int1;
                        break;
                    }
                    ShowPrompt("Enter resolution (75 100 150 300): ");
                }
                MenuDeselect(MenuRESOL);
                ErasePrompt();
                continue;
            }
            if (Matching(MenuPRN)) {
                MenuSelect(MenuPRN);
                ShowPrompt("Enter file or device for output: ");
                outname = FBEdit(Parameters.kpHardcopyDevice);
                if (outname != NULL) {
                    Parameters.kpHardcopyDevice =
                        tmalloc(strlen(outname)+1);
                    strcpy(Parameters.kpHardcopyDevice,outname);
                }
                MenuDeselect(MenuPRN);
                ErasePrompt();
                continue;
            }
            if (Matching(MenuHCTYP)) {
                MenuSelect(MenuHCTYP);
                ShowPrompt("Enter h (HP laser) or p (postscript): ");
                outname = FBEdit(Parameters.kpHardcopyFormat);
                if (outname &&
                        (*outname == HPLASER || *outname == POSTSC))
                    *Parameters.kpHardcopyFormat = *outname;
                MenuDeselect(MenuHCTYP);
                ErasePrompt();
                continue;
            }
            if (Matching(MenuGO)) {
                MenuSelect(MenuGO);
                break;
            }
            *Parameters.kpCommand = '\0';
            continue;
        }
        break;
    }

    outname = Parameters.kpHardcopyDevice;

#ifdef MSDOS
    Int1 = -1;
    if      (!strcasecmp(outname,"prn"))  Int1 = 0;
    else if (!strcasecmp(outname,"lpt1")) Int1 = 0;
    else if (!strcasecmp(outname,"lpt2")) Int1 = 1;
    else if (!strcasecmp(outname,"lpt3")) Int1 = 2;

    if (Int1 >= 0) {
        while (PrinterPortStatus(Int1)) {
            putc('\007',stdout);
            fflush(stdout);
            ShowPrompt("Error: Printer is not accessible.  MORE");
            (void)FBGetchar(ERASE);
            ShowPrompt("Hit any key to continue, ESC to abort: ");
            if (FBGetchar(DISPLAY) == ESCAPE)
                goto quit;
        }
    }
#else
    *pname = '\0';
    if (!strncmp(outname,"lpr",3)) {
        strcpy(pname,outname);
        strcpy(tf,"/tmp/hcXXXXXX");
        outname = mktemp(tf);
    }

#endif
    SaveLastView();
    FB.fInitialized = False;
    FB.fInterface = 1;
    tmpDsp = FB.fDisplay;
    FBBegin(outname);
    display();
    FBEnd();
    FB.fDisplay = tmpDsp;
    FB.fInterface = 0;
    didcopy = True;
quit:
    Parameters.kpGridDisplayed = tmpgrid;
    for (Int1 = 1; Int1 <= NumLayerTable; Int1++) {
        memcpy(LayerTable[Int1].klAltStyle,LayerTable[Int1].klStyle,8);
        memcpy(LayerTable[Int1].klStyle,Temp[Int1].pattrn,8);
        if (LayerTable[Int1].klAttributes & FILLED)
            Temp[Int1].attr |= ALT_FILLED;
        else
            Temp[Int1].attr &= ~ALT_FILLED;
        if (LayerTable[Int1].klAttributes & OUTLINED)
            Temp[Int1].attr |= ALT_OUTLINED;
        else
            Temp[Int1].attr &= ~ALT_OUTLINED;
        if (LayerTable[Int1].klAttributes & VISIBLE)
            Temp[Int1].attr |= ALT_VISIBLE;
        else
            Temp[Int1].attr &= ~ALT_VISIBLE;
        LayerTable[Int1].klAttributes = Temp[Int1].attr;
    }
    Parameters.kpDoingHardcopy = False;
    MenuDeselect(MenuGO);
    MenuDeselect(MenuFRAME);
    FrameOn = False;
    Parameters.kpMenu = OldMenu;
    if (didcopy) {
        FB.fInitialized = True;
        FBBegin(FB.fDisplay);
        InitViewport();
        InitVLT();
        FBForeground(ERASE,0);
        FBFlood();
        RestoreLastView();
#ifndef MSDOS
        /* send the output to the printer */
        if (*pname) {
            sprintf(TypeOut,"%s %s; unlink %s &",pname,outname,outname);
            system(TypeOut);
        }
#endif
    }
    else {
        InitVLT();
        RedisplayViewports();
    }
    ShowLayerTable();
    ShowCommandMenu();
    ErasePrompt();
    MenuDeselect(MenuHCOPY);
}


static void
display()

{
    int L,B,R,T;
    int W,H,X,Y;
    double RY,RX;
    int Margin;
    int Leg;
    char OldControl;


    OldControl = Parameters.kpRedisplayControl;
    Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;

    InitVLT();
    Margin = (FB.fMaxX+1) * .25/8; /* 1/4 inch */
    Leg = legend(1);

    View->kvCoarseViewport->kaLeft   = Margin;
    View->kvCoarseViewport->kaBottom = Leg;
    View->kvCoarseViewport->kaRight  = FB.fMaxX - Margin;
    View->kvCoarseViewport->kaTop    = FB.fMaxY - Margin;
    View->kvCoarseViewport->kaWidth  =
        View->kvCoarseViewport->kaRight - View->kvCoarseViewport->kaLeft;
    View->kvCoarseViewport->kaHeight =
        View->kvCoarseViewport->kaTop - View->kvCoarseViewport->kaBottom;

    if (FrameOn) {
        L = FrameBox.kaLeft;
        R = FrameBox.kaRight;
        B = FrameBox.kaBottom;
        T = FrameBox.kaTop;
    }
    else {
        if (Not CDBB(Parameters.kpCellDesc,(struct o *)NULL,&L,&B,&R,&T))
            MallocFailed();
    }

    TPush();
    TIdentity();
    if ((R-L) > (T-B)) {
        TRotate(0L,1L);
        TPoint(&L,&B);
        TPoint(&R,&T);
        if (L > R) SwapInts(L,R);
        if (B > T) SwapInts(T,B);
    }
    else TIdentity();
    TPremultiply();

    X = (L+R)/2;
    Y = (B+T)/2;
    RY = (double)(T - B)/View->kvCoarseViewport->kaHeight;
    RX = (double)(R - L)/View->kvCoarseViewport->kaWidth;
    if (RX > RY) {
        W = R - L;
        H = W*View->kvCoarseViewport->kaHeight/
            View->kvCoarseViewport->kaWidth;
    }
    else {
        H = T - B;
        W = H*View->kvCoarseViewport->kaWidth/
            View->kvCoarseViewport->kaHeight;
    }

    View->kvCoarseWindow->kaX = X;
    View->kvCoarseWindow->kaY = Y;
    View->kvCoarseWindow->kaLeft = X - W/2;
    View->kvCoarseWindow->kaRight = X + W/2;
    View->kvCoarseWindow->kaBottom = Y - H/2;
    View->kvCoarseWindow->kaTop = Y + H/2;
    View->kvCoarseWindow->kaWidth = W;
    View->kvCoarseWindow->kaHeight = H;
    View->kvCoarseRatio =
        View->kvCoarseViewport->kaWidth/View->kvCoarseWindow->kaWidth;

    Redisplay(View->kvCoarseWindow);
    legend(0);
    TPop();
    Parameters.kpRedisplayControl = OldControl;
}


static int
legend(mode)

/* mode == 0:  add the legend to the plot
 * mode != 0:  simply return the height of the legend
 */
int mode;
{
    struct tm *t;
    int xpos, ypos, xu, yu;
    int entry_wd, xspace, entry_ht, yspace, ncols, nrows;
    int cwidth, i, j;
    int Layer;
    int Margin;
    int NumVisible;
    char buf[120], buf1[80], *s, *strrchr();
    char LayerName[8];
    time_t secs;
        
    FBSetTextClip(0,0,FB.fMaxX,FB.fMaxY);
    LayerName[4] = '\0';

    Margin = (FB.fMaxX + 1) * .65/8;
    entry_ht = (FB.fMaxX + 1) * .25/8;
    cwidth = FB.fFontWidth;
    xspace = entry_ht >> 1;
    entry_wd = entry_ht + (cwidth << 2);
    yspace = entry_ht >> 2;

    NumVisible = 0;
    for (i = 1; i <= NumLayerTable; i++)
        if (LayerTable[i].klAttributes & VISIBLE) NumVisible++;

    ncols = (FB.fMaxX + xspace - (Margin << 1))/(entry_wd+xspace);
    nrows = (NumVisible - 1)/ncols + 1;

    ypos = nrows*(entry_ht + yspace) + Margin/2;
    if (mode)
        return (ypos + entry_ht + yspace);

    time(&secs);
    t = localtime(&secs);
    sprintf(buf1,"%02d-%02d-%02d %02d:%02d",t->tm_mon+1,t->tm_mday,
        t->tm_year,t->tm_hour,t->tm_min);
    
    s = strrchr(Parameters.kpCellDesc->sName,DIRC);
    if (s) s++;
    else   s = Parameters.kpCellDesc->sName;
    sprintf(buf,"KIC-%s  Cell: %s ",VersionString,s);

    FBSetColor(ColorTable[HighlightingColor].Ent);
    FBScaledText(buf1,FB.fMaxX - Margin - strlen(buf1)*cwidth,ypos,0,
        Parameters.kpHardcopyTextScale);
    FBScaledText(buf,Margin,ypos,0,Parameters.kpHardcopyTextScale);

    Layer = 0;
    ypos -= (entry_ht + yspace - 1);
    for (i = 0; i < nrows; i++) {
        for (j = 0; j < ncols; j++) {
            Layer++;
            while (!(LayerTable[Layer].klAttributes & VISIBLE)) {
                Layer++;
                if (Layer > NumLayerTable) return (0);
            }
            if (Layer > NumLayerTable) return (0);
            FBForeground(DISPLAY,Layer);

            xpos = Margin + j*(entry_wd+xspace);

            xu = xpos + entry_ht;
            yu = ypos + entry_ht;

            if (!(LayerTable[Layer].klAttributes & FILLED) ||
                (LayerTable[Layer].klAttributes & OUTLINED))
                FBEmptyBox(Layer,DISPLAY,0,xpos,ypos,xu,yu);

            if (LayerTable[Layer].klAttributes & FILLED)
                FBFilledBox(Layer,DISPLAY,LayerTable[Layer].klStyleID,
                    xpos,ypos,xu,yu);
            LayerName[0] = LayerTable[Layer].klTechnology;
            LayerName[1] = LayerTable[Layer].klMask[0];
            LayerName[2] = LayerTable[Layer].klMask[1];
            LayerName[3] = LayerTable[Layer].klMask[2];
            LayerName[4] = '\0';
            xu += cwidth/3;
            FBScaledText(LayerName,xu,ypos,0,Parameters.kpHardcopyTextScale);
        }
        ypos -= entry_ht+yspace;
    }
    return (0);
}


static int
frame(LookedAhead)

int *LookedAhead;
{
    int X, Y, OldRawX = 0, OldRawY = 0;

    *LookedAhead = False;
    if (FrameOn) {
        MenuDeselect(MenuFRAME);
        FrameOn = False;
        return (0);
    }
    MenuSelect(MenuFRAME);
    FrameOn = True;

    ShowPrompt("Point to endpoints of diagonal.");
    switch (PointLoop(LookedAhead)) {
    case PL_UND:
    case PL_ESC:
    case PL_CMD:
        MenuDeselect(MenuFRAME);
        FrameOn = False;
        return (1);
    case PL_PCW:
        FBSetRubberBanding('R');
        OldRawX = KicCursor.kcRawX;
        OldRawY = KicCursor.kcRawY;
    }
    ShowPrompt("Point to second endpoint.");
    switch (PointLoop(LookedAhead)) {
    case PL_ESC:
    case PL_CMD:
    case PL_UND:
        FBSetRubberBanding(0);
        MenuDeselect(MenuFRAME);
        FrameOn = False;
        return (1);
    case PL_PCW:
        FBSetRubberBanding(0);
        X = KicCursor.kcRawX;
        Y = KicCursor.kcRawY;
        FrameBox.kaLeft = min(X,OldRawX);
        FrameBox.kaRight  = max(X,OldRawX);
        FrameBox.kaBottom = min(Y,OldRawY);
        FrameBox.kaTop = max(Y,OldRawY);
        sprintf(TypeOut,"Frame entered: %d,%d  %d,%d",
            FrameBox.kaLeft/RESOLUTION,
            FrameBox.kaBottom/RESOLUTION,
            FrameBox.kaRight/RESOLUTION,
            FrameBox.kaTop/RESOLUTION);
        ShowPrompt(TypeOut);
        break;
    }
    return (0);
}

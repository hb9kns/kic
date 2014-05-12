/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1994
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
#include "driver.h"
#include "mfbext.h"
#include <ctype.h>

#ifndef WIN32

#ifdef __STDC__
static int  MY_MFBOpen(char*);
static void ghost_line(int,int,int,int);
static void ghost_line_snap(int,int,int,int);
static void ghost_box(int,int,int,int);
static void ghost_box_snap(int,int,int,int);
static void ghost_stretch(int,int,int,int);
static void ghost_move(int,int,int,int);
static void ghost_place(int,int,int,int);
static char *kbedit(char*,int,int,int,int,int);
static void textcursor(int,int);
#else
static int  MY_MFBOpen();
static void ghost_line();
static void ghost_line_snap();
static void ghost_box();
static void ghost_box_snap();
static void ghost_stretch();
static void ghost_move();
static void ghost_place();
static char *kbedit();
static void textcursor();
#endif

struct f FB = { 0 };


struct sGinterface GR[FBNUMINTERFACE] = {
{   MFBInfo,
    MY_MFBOpen,
    MFBClose,
    MFBUpdate,
    MFBPixel,
    MFBLine,
    MFBBox,
    (void(*)(Poly*))MFBPolygon,
    MFBDefineLineStyle,
    MFBSetLineStyle,
    MFBDefineFillPattern,
    MFBSetFillPattern,
    MFBDefineColor,
    MFBSetColor,
    MFBText,
    MFBScaledText,

    MFBFlood,
    MFBPoint,
    MFBBeep,
    MFBSetFullScreenCursor,
    MFBTextBB,
    MFBMore,
    MFBSetName,
    MFBSetBlinker,
    MFBSelectFont,
    MFBSelectCursor,
    MFBSetTextClip,
    MFBResizeDrawingWindow
    },

{   GR_info,
    GR_open,
    GR_close,
    GR_update,
    GR_pixel,
    GR_line,
    GR_box,
    GR_polygon,
    GR_defLs,
    GR_setLs,
    GR_defFp,
    GR_setFp,
    GR_defColor,
    GR_setColor,
    GR_text,
    GR_scText,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0 }
};


static int
MY_MFBOpen(name)

char *name;
{
    int error;

    if (MFBOpen(name,name,&error) == NULL) {
        fprintf(stderr,"%s\n",MFBError(error));
        return (1);
    }
    return (0);
}


void
FBBegin(Dspl)

char *Dspl;
{
    FB.fDisplay = Dspl;
    if (!(FB.fInitialized)) {
        if ((*GR[FB.fInterface].gOpen)(FB.fDisplay))
            fatal_error("Unable to open display.");
        else {
            if (Parameters.kpFontName[0] != '\0')
                MFBSetFont(Parameters.kpFontName);
            if (Parameters.kpCursorShape >= 0)
                MFBSetCursor(Parameters.kpCursorShape);
            if (Parameters.kpFullScreenCursor)
                MFBSetFullScreenCursor(True);
        }
    }

    FBSetLineStyle(0);
    FB.fMaxX                    = FBInfo(MAXX);
    FB.fMaxY                    = FBInfo(MAXY);
    FB.fMaxP                    = 1000;
    FB.fMaxIntensity            = 255;
    FB.fNumColors               = FBInfo(MAXCOLORS) - 1;
    if (FB.fNumColors >= 32)
        Parameters.kpMergeColors = False;
    else
        Parameters.kpMergeColors = True;
    FB.fNumFillPatterns         = FBInfo(MAXFILLPATTERNS) - 1;
    FB.fButtonMask              = SetButtonMask();
    FB.fButtons                 = FBInfo(POINTINGBUTTONS);
    FB.fNumButtons              = FBInfo(NUMBUTTONS);
    /*
     * Font width and height is henceforth assumed to
     * include the X and Y offsets.
     */
    FB.fFontWidth               = FBInfo(FONTWIDTH);
    FB.fFontHeight              = FBInfo(FONTHEIGHT);
    FB.fFontWidth              += FBInfo(FONTXOFFSET);
    FB.fFontHeight             += FBInfo(FONTYOFFSET);
    FB.fNumRows                 = FB.fMaxY/FB.fFontHeight;
    FB.fNumColumns              = FB.fMaxX/FB.fFontWidth;
    FB.fFilledPolygons          = FBInfo(FILLEDPOLYGONS);
    FB.fDefinableFillPatterns   = FBInfo(DEFFILLPATTERNS);
    FB.fNonDestructiveText      = FBInfo(OVERSTRIKETEXT);
    FB.fLastCursorColumn        = 0;
    FB.fBlinkers                = FBInfo(BLINKERS);
    FB.fInitialized             = True;
    MFBSetTextMode(False);
    FBSetTextClip(0,0,FB.fMaxX,FB.fMaxY);
}


void
FBForeground(DisplayOrErase,Pixel)

int DisplayOrErase;
int Pixel;
{
    if (DisplayOrErase == ERASE) Pixel = 0;
    Pixel = min(FB.fNumColors,Pixel);
    (*GR[FB.fInterface].gSetColor)(Pixel);
}


void
FBVLT(Pixel,R,G,B)

int Pixel,R,G,B;
{
    if (FBInfo(VLT)) {
        R = ((int)R * FB.fMaxP)/FB.fMaxIntensity;
        G = ((int)G * FB.fMaxP)/FB.fMaxIntensity;
        B = ((int)B * FB.fMaxP)/FB.fMaxIntensity;
        (*GR[FB.fInterface].gDefColor)(Pixel,R,G,B);
    }
}


void
FBText(Mode,RowOrX,ColumnOrY,Text)

int Mode;
int RowOrX,ColumnOrY;
char *Text;
{
    int i;

    if (Mode == ROW_COLUMN) {
        i = FB.fMaxY - RowOrX * FB.fFontHeight;
        RowOrX = (ColumnOrY - 1) * FB.fFontWidth;
        ColumnOrY = i;
    }
    (*GR[FB.fInterface].gText)(Text,RowOrX,ColumnOrY,0);
}


int
FBGetchar(DisplayOrErase)

int DisplayOrErase;
{
    int x,y;
    short c;
    char s[2];

    if (DisplayOrErase == ERASE)
        return (MFBGetchar());

    y = FB.fMaxY - FB.fNumRows*FB.fFontHeight;
    x = FB.fLastCursorColumn*FB.fFontWidth;
    FBForeground(DISPLAY,ColorTable[HighlightingColor].Ent);
    textcursor(x,y);
    c = MFBGetchar();
    s[0] = (char)(c & 0xff);
    s[1] = '\0';
    FBForeground(DISPLAY,ColorTable[MenuPromptColor].Ent);
    (*GR[FB.fInterface].gText)(s,x,y,0);
    AppendToOldPrompt(s[0]);
    FBForeground(ERASE,0);
    textcursor(x,y);
    return (c);
}


char *
FBEdit(string)

char *string;
{
    int x,y;

    y = FB.fMaxY - (FB.fNumRows - 2)*FB.fFontHeight;
    x = FB.fLastCursorColumn*FB.fFontWidth;
    return (kbedit(string,x,y,0,ColorTable[MenuPromptColor].Ent,
        ColorTable[HighlightingColor].Ent));
}


void
FBSetCursorColor(colorId)

int colorId;
{
    MFBSetCursorColor(colorId,colorId);  /* no blinking */
}


void
FBPolygon(Pixel,Type,StyleId,xy,n)

int Pixel,Type,StyleId,n;
int *xy;
{
    /*
     * Type == FILL means fill
     * Type == OUTLINE means outline
     * StyleId defines stipple pattern (0 = SOLID)
     */
    int j,n2,X,Y,Z,T,Xo,Yo;
    Poly poly;
    FBForeground(DISPLAY,Pixel);
    (*GR[FB.fInterface].gSetFp)(StyleId);

    poly.xy = xy;

    while (NewPolygon(&poly)) {
        n = poly.nvertices;

        if (Type == OUTLINE) {
            n2 = n + n;
            j = 2;
            Xo = X = xy[0];
            Yo = Y = xy[1];
            while (j < n2) {
                Z = xy[j++];
                T = xy[j++];
                (*GR[FB.fInterface].gLine)(X,Y,Z,T);
                X = Z;
                Y = T;
            }
            (*GR[FB.fInterface].gLine)(X,Y,Xo,Yo);
        }
        else {
            if (n < 2) return;
            (*GR[FB.fInterface].gPolygon)(&poly);
        }
    }
}


void
FBPolygonClip(coord,n,window)

int *coord;
int *n;
struct ka *window;
{
    Poly poly;

    poly.nvertices = *n;
    if (*n < 3) return;
    poly.xy = coord;
    PolygonClip(&poly,window->kaLeft,window->kaBottom,
        window->kaRight,window->kaTop);
}

/* Warning: 16 bit ints will not work below */
void
FBSetRubberBanding(mode)

/* if mode is lower case will clip to grid point */
int mode;
{
    /* need some hokum for button 3 box */
#ifdef __STDC__
    static void (*func)(int,int,int,int);
#else
    static void (*func)();
#endif
    static int X, Y;
    static int lastR;

    switch (mode) {
    case 'l':
        func = ghost_line_snap;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        MFBSetGhost(func,X,Y);
        return;

    case 'L':
        func = ghost_line;
        X = (int)KicCursor.kcRawX;
        Y = (int)KicCursor.kcRawY;
        MFBSetGhost(func,X,Y);
        return;

    case 'r':
        func = ghost_box_snap;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        MFBSetGhost(func,X,Y);
        return;

    case 'R':
        if (func) {
            lastR = True;
            MFBSetGhost(ghost_box,
                (int)KicCursor.kcRawX,(int)KicCursor.kcRawY);
            return;
        }
        func = ghost_box;
        X = (int)KicCursor.kcRawX;
        Y = (int)KicCursor.kcRawY;
        MFBSetGhost(func,X,Y);
        return;

    case 's':
        func = ghost_stretch;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        MFBSetGhost(func,X,Y);
        return;

    case 'm':
    case 'M':
        func = ghost_move;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        MFBSetGhost(func,X,Y);
        return;

    case 'p':
    case 'P':
        func = ghost_place;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        MFBSetGhost(func,X,Y);
        return;

    case 0:
        if (lastR) {
            MFBSetGhost(func,X,Y);
        }
        else {
            MFBSetGhost(NULL,0,0);
#ifdef __STDC__
            func = (void(*)(int,int,int,int))NULL;
#else
            func = (void(*)())NULL;
#endif
        }
        lastR = False;
            return;
    }
}


static void
ghost_line(x,y,refx,refy)

int x, y, refx, refy;
{
    if (!GetWindowCoords((int*)&x,(int*)&y,False))
        return;
    if (Parameters.kpRotationAngle == 0 ||
            Parameters.kpRotationAngle == 180)
        ShowLine(ColorTable[HighlightingColor].Ent,x,
            View->kvCoarseWindow->kaBottom,
            x,View->kvCoarseWindow->kaTop);
    else
        ShowLine(ColorTable[HighlightingColor].Ent,
            View->kvCoarseWindow->kaLeft,
            y,View->kvCoarseWindow->kaRight,y);
    MFBSetColor(Parameters.kpLayer);
}


static void
ghost_line_snap(x,y,refx,refy)

int x, y, refx, refy;
{
    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    if (Parameters.kpRotationAngle == 0 ||
            Parameters.kpRotationAngle == 180)
        ShowLine(ColorTable[HighlightingColor].Ent,x,
            View->kvCoarseWindow->kaBottom,
            x,View->kvCoarseWindow->kaTop);
    else
        ShowLine(ColorTable[HighlightingColor].Ent,
            View->kvCoarseWindow->kaLeft,
            y,View->kvCoarseWindow->kaRight,y);
    MFBSetColor(Parameters.kpLayer);
}


static void
ghost_box(x,y,refx,refy)

int x, y, refx, refy;
{
    struct ka BB;

    if (!GetWindowCoords((int*)&x,(int*)&y,False))
        return;
    BB.kaLeft = refx;
    BB.kaBottom = refy;
    BB.kaRight = x;
    BB.kaTop = y;
    ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
    MFBSetColor(Parameters.kpLayer);
}


static void
ghost_box_snap(x,y,refx,refy)

int x, y, refx, refy;
{
    struct ka BB;

    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    BB.kaLeft = refx;
    BB.kaBottom = refy;
    BB.kaRight = x;
    BB.kaTop = y;
    ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
    MFBSetColor(Parameters.kpLayer);
}


static void
ghost_stretch(x,y,refx,refy)

int x, y, refx, refy;
{
    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    ShowStretch(x,y,refx,refy);
    MFBSetColor(Parameters.kpLayer);
}


static void
ghost_move(x,y,refx,refy)

int x, y, refx, refy;
{
    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    ShowMove(refx,refy,x,y);
    MFBSetColor(Parameters.kpLayer);
}


static void
ghost_place(x,y,refx,refy)

int x, y, refx, refy;
{
    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    ShowNewInstance(x,y,refx,refy);
    MFBSetColor(Parameters.kpLayer);
}


/* cursor height - 1 */
#define CURHT 0

/* What a pain - have to be able to redraw during editing
 * when a resize event occurs.
 */
static int doing_edit;
static int editX, editY, editFg, editCc;
static char editBuf[128], *editC;


static char *
kbedit(s,x,y,bg,fg,cc)

/*
 * s      Initial string to edit.
 * x,y    Lower left coordinates.
 * bg,fg  Background and foreground colors.
 * cc     Background color at cursor location.
 * Returns edited string (static!).
 */
char *s;
int x, y, bg, fg, cc;
{
    char tbuf[128];
    int k;
    char *end, ctmp[2];

    doing_edit = True;
    editFg = fg;
    editCc = cc;
    editX = x;
    editY = y;

    MFBSetColor(fg);
    *editBuf = '\0';
    if (s) {
        /* s can be NULL */
        MFBText(s, editX, editY, 0);
        strcpy(editBuf,s);
    }
    ctmp[1] = '\0';
    editC = editBuf;
    end = strchr(editBuf,'\0');
    *(end+1) = '\0';
    ctmp[0] = *editC;
    MFBSetColor(cc);
    textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
    MFBSetColor(fg);
    MFBText(ctmp, editX + (int)(editC-editBuf)*FB.fFontWidth, editY, 0);
    for (;;) {
        k = MFBGetchar();
        if ((char) k == '\r') break;
        switch (k) {

        case 333:    /* right arrow */
            if (editC >= end) continue;
            ctmp[0] = *editC;
            MFBSetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBSetColor(fg);
            MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC++;
            ctmp[0] = *editC;
            MFBSetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBSetColor(fg);
            MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case 331:    /* left arrow */
            if (editC <= editBuf) continue;
            ctmp[0] = *editC;
            MFBSetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBSetColor(fg);
            MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC--;
            ctmp[0] = *editC;
            MFBSetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBSetColor(fg);
            MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case '\b':
            if (editC == editBuf) continue;
            MFBSetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            editC--;
            MFBText(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            MFBSetColor(fg);
            *editC = '\0';
            strcat(editBuf,++editC);
            MFBText(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC--;
            if (end > editBuf) end--;
            ctmp[0] = *editC;
            MFBSetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBSetColor(fg);
            MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case 339: /* DEL, in DOS */
        case 127:
            if (editC == end) continue;
            MFBSetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBText(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            MFBSetColor(fg);
            *editC = '\0';
            strcat(editBuf,++editC);
            MFBText(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC--;
            if (end > editBuf) end--;
            ctmp[0] = *editC;
            MFBSetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBSetColor(fg);
            MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case '\025': /* ^U */
        case '\030': /* ^X */
        case '\033': /* ESC */
            MFBSetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBText(editBuf,editX,editY,0);
            editC = end = editBuf;
            *editC = '\0';
            if ((char) k == '\033') {
                doing_edit = False;
                return (NULL);
            }
            MFBSetColor(cc);
            textcursor(editX,editY);
            MFBSetColor(fg);
            continue;
        default:
            if (k > 255) continue;
            if (((char) k < ' ') || ((char) k > '~'))  continue;
            MFBSetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBText(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            strcpy(tbuf,editC);
            *editC = (char) k;
            sprintf(++editC,tbuf);
            ctmp[0] = *editC;
            MFBSetColor(fg);
            MFBText(editC-1,
                editX+(int)(editC-1-editBuf)*FB.fFontWidth,editY,0);
            MFBSetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            MFBSetColor(fg);
            MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            end++;
            *(end+1) = '\0';

        }
    }
    MFBSetColor(bg);
    textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
    MFBSetColor(fg);
    ctmp[0] = *editC;
    MFBText(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
    doing_edit = False;
    return (editBuf);
}


void
FBKbRepaint(x,y)

int x,y;
{
    if (!doing_edit) return;
    editX = x;
    editY = y;
    MFBSetColor(editCc);
    textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
    MFBSetColor(editFg);
    MFBText(editBuf,editX,editY,0);
}


static void
textcursor(x,y)

int x,y;
{
    MFBBox(x,y-1,x+FB.fFontWidth,y+CURHT);
}

#endif

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

#ifdef WIN32
 
#include <windows.h>
#include <windowsx.h>
#ifndef _WINDOWS
#define _WINDOWS
#endif
#include "prefix.h"
#include "kic.h"
#include "driver.h"
#include <ctype.h>
#include <time.h>

#define IDR_MAINFRAME 1
#define DEF_FWID 6
#define DEF_FHEI 12

extern void out_printf(char*, ...);

static int  w_Info(int);
static int  w_Open(char*);
static int  w_Close(void);
static int  w_Update(void);
static void w_Pixel(int,int);
static void w_Line(int,int,int,int);
static void w_Box(int,int,int,int);
static void w_Polygon(Poly*);
static int  w_DefLs(int,int);
static int  w_SetLs(int);
static int  w_DefFp(int,int*);
static int  w_SetFp(int);
static int  w_DefColor(int,int,int,int);
static int  w_SetColor(int);
static void w_Text(char*,int,int,int);
static void w_ScText(char*,int,int,int,int);

static void w_Clear(void);
static int  w_Point(int*, int*, int*, int*);
static void w_Beep(int);
static int  w_SetFullScreenCursor(int);
static void w_TextBB(char*, int*, int*);
static void w_More(int, int, int, int, FILE*);
static void w_SetName(char*, char*);
static int  w_Blink(int, int, int, int, int);
static int  w_SelectFont(int, int, int, int);
static void w_SelectCursor(int, int, int, int);
static void w_SetTextClip(int, int, int, int);
static void w_Resize(int, int, int, int);

typedef ATOM (WINAPI* REGISTERCLASSEXPROC)(const LPWNDCLASSEX);
static ATOM msw_RegisterClass(const LPWNDCLASSEX);
static LRESULT CALLBACK mainFrameWndProc(HWND, UINT, WPARAM, LPARAM);
static HFONT msw_GetFixedFont(int, int);

static void draw_fs_cursor(int, int);
static int  w_SetGhost(void(*)(int, int, int, int), int, int);
static void ghost_line(int,int,int,int);
static void ghost_line_snap(int,int,int,int);
static void ghost_box(int,int,int,int);
static void ghost_box_snap(int,int,int,int);
static void ghost_stretch(int,int,int,int);
static void ghost_move(int,int,int,int);
static void ghost_place(int,int,int,int);
static char *kbedit(char*,int,int,int,int,int);
static void textcursor(int,int);
static int  textwidth(char*, int);
static int  w_Getchar(void);

#define INV(y) (FB.fMaxY - (y))
#define MAXCHARLINE 128

struct f FB = { 0 };

struct sGinterface GR[FBNUMINTERFACE] = {
{   w_Info,
    w_Open,
    w_Close,
    w_Update,
    w_Pixel,
    w_Line,
    w_Box,
    w_Polygon,
    w_DefLs,
    w_SetLs,
    w_DefFp,
    w_SetFp,
    w_DefColor,
    w_SetColor,
    w_Text,
    w_ScText,

    w_Clear,
    w_Point,
    w_Beep,
    w_SetFullScreenCursor,
    w_TextBB,
    w_More,
    w_SetName,
    w_Blink,
    w_SelectFont,
    w_SelectCursor,
    w_SetTextClip,
    w_Resize },

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
    0 },
};

static HWND w_hwnd;                                 /* main window handle */
static HDC w_dc;                                    /* window DC */
static int w_fg_indx;                               /* current color index */
static COLORREF w_fg;                               /* color pixel value */
static COLORREF w_colors[64];                       /* color pixel table */
static int w_linestyle;                             /* cur linestyle (0/1) */
static int w_lsmask;                                /* linestyle mask */
static void(*w_drawghost)(int, int, int, int);      /* ghost draw function */
static int w_lastx, w_lasty, w_refx, w_refy;        /* ghost draw values */
static int w_firstghost;                            /* ghost draw flag */
static int w_fs_cursor;                             /* using fs curser */
static int w_fs_first;                              /* fs cursor flag */
static int w_curfp;                                 /* fill pattern index */
static HBITMAP w_fp[64];                            /* fill pattern table */

static int w_cmap_max_size;
static int w_cmap_size;
static HPALETTE w_cmap;


/*=========================================================================
 *
 * Global Exports
 *
 *========================================================================*/

void
FBBegin(char *Dspl)
{
    FB.fDisplay = Dspl;
    if (!(FB.fInitialized)) {
        if ((*GR[FB.fInterface].gOpen)(Dspl))
            fatal_error("Unable to open display.");
        else {
        /*
            if (Parameters.kpFontName[0] != '\0')
                MFBSetFont(Parameters.kpFontName);
            if (Parameters.kpCursorShape >= 0)
                MFBSetCursor(Parameters.kpCursorShape);
        */
            if (Parameters.kpFullScreenCursor)
                w_SetFullScreenCursor(True);
        }
        FB.fInitialized = 1;
    }

    FBSetLineStyle(0);

    FB.fMaxX                    = FBInfo(MAXX);
    FB.fMaxY                    = FBInfo(MAXY);
    FB.fMaxP                    = 255;
    FB.fMaxIntensity            = 255;
    FB.fNumColors               = FBInfo(MAXCOLORS);
    if (FB.fNumColors >= 32)
        Parameters.kpMergeColors = False;
    else
        Parameters.kpMergeColors = True;
    FB.fNumFillPatterns         = FBInfo(MAXFILLPATTERNS);
    FB.fButtonMask              = SetButtonMask();
    FB.fButtons                 = 1;
    FB.fNumButtons              = 4;

    FB.fFontWidth               = FBInfo(FONTWIDTH);
    FB.fFontHeight              = FBInfo(FONTHEIGHT);
    FB.fNumRows                 = FB.fMaxY/FB.fFontHeight;
    FB.fNumColumns              = FB.fMaxX/FB.fFontWidth;
    FB.fFilledPolygons          = 1;
    FB.fDefinableFillPatterns   = 1;
    FB.fNonDestructiveText      = 1;
    FB.fLastCursorColumn        = 0;
    FB.fBlinkers                = 0;
    FB.fInitialized             = True;

    {
    HBRUSH brush = CreateSolidBrush(w_colors[0]);
    brush = (HBRUSH)SetClassLong(w_hwnd, GCL_HBRBACKGROUND, (DWORD)brush);
    if (brush)
        DeleteBrush(brush);
    }
}


void
FBForeground(int DisplayOrErase, int Pixel)
{
    if (DisplayOrErase == ERASE) Pixel = 0;
    Pixel = min(FB.fNumColors,Pixel);
    (*GR[FB.fInterface].gSetColor)(Pixel);
}


void
FBVLT(int Pixel, int R, int G, int B)
{
    R = ((int)R * FB.fMaxP)/FB.fMaxIntensity;
    G = ((int)G * FB.fMaxP)/FB.fMaxIntensity;
    B = ((int)B * FB.fMaxP)/FB.fMaxIntensity;
    (*GR[FB.fInterface].gDefColor)(Pixel,R,G,B);
}


void
FBText(int Mode, int RowOrX, int ColumnOrY, char *Text)
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
FBGetchar(int DisplayOrErase)
{
    int x,y;
    short c;
    char s[2];

    if (DisplayOrErase == ERASE)
        return (w_Getchar());

    y = FB.fMaxY - FB.fNumRows*FB.fFontHeight;
    x = FB.fLastCursorColumn*FB.fFontWidth;
    FBForeground(DISPLAY,ColorTable[HighlightingColor].Ent);
    textcursor(x,y);
    c = w_Getchar();
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
FBEdit(char *string)
{
    int x,y;

    y = FB.fMaxY - (FB.fNumRows - 2)*FB.fFontHeight;
    x = FB.fLastCursorColumn*FB.fFontWidth;
    return (kbedit(string,x,y,0,ColorTable[MenuPromptColor].Ent,
        ColorTable[HighlightingColor].Ent));
}


void
FBSetCursorColor(int colorId)
{
/*    MFBSetCursorColor(colorId,colorId); */  /* no blinking */
}


void
FBPolygon(int Pixel, int Type, int StyleId, int *xy, int n)
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
FBPolygonClip(int *coord, int *n, struct ka *window)
{
    Poly poly;

    poly.nvertices = *n;
    if (*n < 3) return;
    poly.xy = coord;
    PolygonClip(&poly,window->kaLeft,window->kaBottom,
        window->kaRight,window->kaTop);
}


void
FBSetRubberBanding(int mode)
/* if mode is lower case will clip to grid point */
{
    /* need some hokum for button 3 box */
    static void (*func)(int,int,int,int);
    static int X, Y;
    static int lastR;

    switch (mode) {
    case 'l':
        func = ghost_line_snap;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        w_SetGhost(func,X,Y);
        return;

    case 'L':
        func = ghost_line;
        X = (int)KicCursor.kcRawX;
        Y = (int)KicCursor.kcRawY;
        w_SetGhost(func,X,Y);
        return;

    case 'r':
        func = ghost_box_snap;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        w_SetGhost(func,X,Y);
        return;

    case 'R':
        if (func) {
            lastR = True;
            w_SetGhost(ghost_box,
                (int)KicCursor.kcRawX,(int)KicCursor.kcRawY);
            return;
        }
        func = ghost_box;
        X = (int)KicCursor.kcRawX;
        Y = (int)KicCursor.kcRawY;
        w_SetGhost(func,X,Y);
        return;

    case 's':
        func = ghost_stretch;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        w_SetGhost(func,X,Y);
        return;

    case 'm':
    case 'M':
        func = ghost_move;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        w_SetGhost(func,X,Y);
        return;

    case 'p':
    case 'P':
        func = ghost_place;
        X = (int)KicCursor.kcX;
        Y = (int)KicCursor.kcY;
        w_SetGhost(func,X,Y);
        return;

    case 0:
        if (lastR) {
            w_SetGhost(func,X,Y);
        }
        else {
            w_SetGhost(NULL,0,0);
            func = (void(*)(int,int,int,int))NULL;
        }
        lastR = False;
        return;
    }
}


/* cursor height - 1 */
#define CURHT 0

/* What a pain - have to be able to redraw during editing
 * when a resize event occurs.
 */
static int doing_edit;
static int editX, editY, editFg, editCc;
static char editBuf[128], *editC;


void
FBKbRepaint(int x, int y)
{
    if (!doing_edit) return;
    editX = x;
    editY = y;
    w_SetColor(editCc);
    textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
    w_SetColor(editFg);
    w_Text(editBuf,editX,editY,0);
}


/* Return a time stamp
 */
unsigned int
FBTime()
{
    return (time(0));
}


/* Used only in DOS
 */
void
FBFuncKeys(int y, int dx)
{
}


/* Erase or draw the fine positioning window in coarse window.
 */
void
XORfineViewport(void)
{
    char tmp = Parameters.kpRedisplayControl;
    Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
    SetROP2(w_dc, R2_XORPEN);
    ShowEmptyBox(ColorTable[HighlightingColor].Ent,View->kvFineWindow);
    SetROP2(w_dc, R2_COPYPEN);
    Parameters.kpRedisplayControl = tmp;
}


/* replace X,Y (in pixel coords) with the window coords if pointing
 * in the Coarse or Fine viewports, and return True.  Otherwise
 * return False.  Clip to grid if Clip is True.
 */
int
GetWindowCoords(int *X, int *Y, int Clip)
{
    int X1,Y1;
    RECT r;

    GetClientRect(w_hwnd, &r);
    X1 = *X;
    Y1 = (r.bottom - r.top) - *Y;
    if (InBox(X1,Y1,View->kvCoarseViewport)) {
        PToL(View->kvCoarseWindow,&X1,&Y1);
        if (Clip)
            ClipToGridPoint(&X1,&Y1);
        *X = X1;
        *Y = Y1;
        return (True);
    }
    if (InBox(X1,Y1,View->kvFineViewport)) {
        PToL(View->kvFineWindow,&X1,&Y1);
        if (Clip)
            ClipToGridPoint(&X1,&Y1);
        *X = X1;
        *Y = Y1;
        return (True);
    }
    return (False);
}


int *
SetButtonMask(void)

{
    static int mask[4];
    mask[0] = 1;
    mask[1] = 2;
    mask[2] = 4;
    mask[3] = 3;
    return (mask);
}

 
/* Menu command to select font
 */
void
SelectKicFont()
{
    extern char *MenuFONT;
    char *in, buf[80];
    int w, h;

    sprintf(buf, "Enter char size WxH (currently %dx%d)",
        FB.fFontWidth, FB.fFontHeight);
    ShowPrompt(buf);
    in = FBEdit(0);
    if (in && sscanf(in, "%dx%d", &w, &h) == 2) {
        HFONT f = msw_GetFixedFont(w, h);
        if (f) {
            f = SelectFont(w_dc, f);
            DeleteFont(f);
            FullRedisplay();
            sprintf(Parameters.kpFontName, "%dx%d",
                FB.fFontWidth, FB.fFontHeight);
        }
        else {
            ShowPrompt("Failed to set font.");
            MenuDeselect(MenuFONT);
            return;
        }
    }
    MenuDeselect(MenuFONT);
    ErasePrompt();
}


/* Menu command to set/unset full-screen cursor
 */
void
SelectKicCursor()
{
    extern char *MenuCURSR;

    if (w_fs_cursor) {
        w_SetFullScreenCursor(False);
        MenuDeselect(MenuCURSR);
    }
    else {
        w_SetFullScreenCursor(True);
        MenuSelect(MenuCURSR);
    }
}
 

/* Return True if using X
 */
int
Xcheck()
{
    return (0);
}


/* color print in DOS
 */
void
cprint(int i, char *s)
{
    out_printf(s);
}


/*=========================================================================
 *
 * Local Interface Functions
 *
 *========================================================================*/

static int
w_Info(int num)
{
    RECT r;
    SIZE size;

    GetClientRect(w_hwnd, &r);
    GetTextExtentPoint32(w_dc, "x", 1, &size);

    switch (num) {
    case MAXX:             return (r.right - r.left - 1);
    case MAXY:             return (r.bottom - r.top - 1);
    case MAXCOLORS:        return (32);
    case MAXINTENSITY:     return (255);
    case MAXFILLPATTERNS:  return (16);
    case MAXLINESTYLES:    return (1);
    case MAXBLINKERS:      return (0);
    case POINTINGDEVICE:   return (0);
    case POINTINGBUTTONS:  return (0);
    case NUMBUTTONS:       return (0);
    case BUTTON1:          return (1);
    case BUTTON2:          return (2);
    case BUTTON3:          return (3);
    case BUTTON4:          return (4);
    case BUTTON5:          return (0);
    case BUTTON6:          return (0);
    case BUTTON7:          return (0);
    case BUTTON8:          return (0);
    case BUTTON9:          return (0);
    case BUTTON10:         return (0);
    case BUTTON11:         return (0);
    case BUTTON12:         return (0);
    case TEXTPOSITIONABLE: return (1);
    case TEXTROTATABLE:    return (1);
    case FONTHEIGHT:       return (size.cy);
    case FONTWIDTH:        return (size.cx);
    case FONTXOFFSET:      return (0);
    case FONTYOFFSET:      return (0);
    case DESTRUCTIVETEXT:  return (0);
    case OVERSTRIKETEXT:   return (1);
    case VLT:              return (0);
    case BLINKERS:         return (0);
    case FILLEDPOLYGONS:   return (1);
    case DEFFILLPATTERNS:  return (1);
    case DEFLINEPATTERN:   return (1);
    case CURFGCOLOR:       return (1);
    case CURFILLPATTERN:   return (w_curfp);
    case CURLINESTYLE:     return (w_linestyle);
    case NUMBITPLANES:     return (1);

    default:               return (-1);
    }
}


static int
w_Open(char *name)
{
    HINSTANCE instance = GetModuleHandle(0);
    WNDCLASSEX wcex;
    char *title = "Kic Layout Editor";
    int rcaps;
    int fw, fh;

    // The application's main frame window
    //
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc   = mainFrameWndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = instance;
    wcex.hIcon         = LoadIcon(instance, MAKEINTRESOURCE(IDR_MAINFRAME));
    wcex.hCursor       = LoadCursor(0, IDC_CROSS);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINFRAME);
    wcex.lpszClassName = MAKEINTRESOURCE(IDR_MAINFRAME);
    wcex.hIconSm       = (HICON)LoadImage(instance,
        MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_SHARED);

    // Register the window class and return success/failure code.
    if (!msw_RegisterClass(&wcex))
        return (1);

    // Create the main frame window
    w_hwnd = CreateWindowEx(
        0,                  // Extended window styles
        MAKEINTRESOURCE(IDR_MAINFRAME), // registered class name
        title,              // Address of window name
        WS_OVERLAPPEDWINDOW,// Window style
        CW_USEDEFAULT,      // Horizontal position of window
        0,                  // Vertical position of window
        800,                // Window width
        600,                // Window height
        0,                  // Handle of parent or owner window
        0,                  // Handle of menu for this window
        instance,           // Handle of application instance
        0);                 // Address of window-creation data

    if (!w_hwnd)
       return (1);

    w_dc = GetDC(w_hwnd);

#define CNUM 2

    if (sscanf(Parameters.kpFontName, "%dx%d", &fw, &fh) == 2 &&
            fw > 2 && fw < 15 && fh > 4 && fh < 30)
        SelectFont(w_dc, msw_GetFixedFont(fw, fh));
    else
        SelectFont(w_dc, msw_GetFixedFont(DEF_FWID, DEF_FHEI));
    SetBkMode(w_dc, TRANSPARENT);
    SetTextAlign(w_dc, TA_LEFT | TA_BOTTOM | TA_NOUPDATECP);

    rcaps = GetDeviceCaps(w_dc, RASTERCAPS);
    if (rcaps & RC_PALETTE) {
        char logPalBuf[sizeof(LOGPALETTE) + CNUM*sizeof(PALETTEENTRY)];
        LOGPALETTE *logPalPtr = (LOGPALETTE*)logPalBuf;
        PALETTEENTRY *p = logPalPtr->palPalEntry;
        int i;

        logPalPtr->palVersion = 0x300;
        logPalPtr->palNumEntries = CNUM;

        for (i = 0; i < CNUM; i++) {
            if (i == 0) {
                p->peRed = 0;
                p->peGreen = 0;
                p->peBlue = 0;
                p->peFlags = 0;
            }
            else {
                p->peRed = 255;
                p->peGreen = 255;
                p->peBlue = 255;
                p->peFlags = 0;
            }
            p++;
            w_colors[i] = PALETTEINDEX(i);
        }

        w_cmap_max_size = GetDeviceCaps(w_dc, SIZEPALETTE);
        w_cmap_size = logPalPtr->palNumEntries; 
        w_cmap = CreatePalette(logPalPtr);

        SelectPalette(w_dc, w_cmap, False);
        RealizePalette(w_dc);
        SetBkColor(w_dc, PALETTEINDEX(0));
        SetTextColor(w_dc, PALETTEINDEX(1));
    }
    else {
        SetBkColor(w_dc, RGB(0, 0, 0));
        SetTextColor(w_dc, RGB(255, 255, 255));
    }

    DragAcceptFiles(w_hwnd, True);
    ShowWindow(w_hwnd, SW_SHOW);

    return (0);
}


static int
w_Close(void)
{
    return (0);
}


static int
w_Update(void)
{
    UpdateWindow(w_hwnd);
    return (0);
}


static void
w_Pixel(int x, int y)
{
    SetPixelV(w_dc, x, INV(y), w_fg);
}


static void
w_Line(int x1, int y1, int x2, int y2)
{
    MoveToEx(w_dc, x1, INV(y1), 0);
    LineTo(w_dc, x2, INV(y2));
}


// Some ROP3's
#define COPYFG 0x00ca0749  // dst = (pat & src) | (!pat & dst)
#define COPYBG 0x00ac0744  // dst = (!pat & src) | (pat & dst)

static void
w_Box(int x1, int y1, int x2, int y2)
{
    y1 = INV(y1);
    y2 = INV(y2);

    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    x2++;
    y2++;

    if (w_fp[w_curfp]) {
        HDC dcMem;
        HBITMAP bm;
        RECT r;
        r.left = r.top = 0;
        r.right = x2-x1;
        r.bottom = y2-y1;

        dcMem = CreateCompatibleDC(w_dc);
        if (w_cmap)
            SelectPalette(dcMem, w_cmap, False);
        bm = CreateCompatibleBitmap(w_dc, r.right, r.bottom);
        bm = (HBITMAP)SelectObject(dcMem, bm);
        SetBkColor(dcMem, w_fg);
        ExtTextOut(dcMem, 0, 0, ETO_OPAQUE, &r, 0, 0, 0);
        SetTextColor(w_dc, RGB(255,255,255));
        BitBlt(w_dc, x1, y1, r.right, r.bottom, dcMem, 0, 0, COPYFG);
        SetTextColor(w_dc, w_fg);
        bm = (HBITMAP)SelectObject(dcMem, bm);
        DeleteBitmap(bm);
        DeleteDC(dcMem);
    }
    else {
        COLORREF oldc;
        RECT r;
        r.left = x1;
        r.top = y1;
        r.right = x2;
        r.bottom = y2;
        // faster?  Ousterhout claims so in tk
        oldc = SetBkColor(w_dc, w_fg);
        SetBkMode(w_dc, OPAQUE);
        ExtTextOut(w_dc, 0, 0, ETO_OPAQUE, &r, 0, 0, 0);
        SetBkColor(w_dc, oldc);
        SetBkMode(w_dc, TRANSPARENT);
        /*
        FillRect(w_dc, &r,
            (HBRUSH)GetCurrentObject(w_dc, OBJ_BRUSH));
        */
    }
}


static void
w_Polygon(Poly *poly)
{
    POINT *pl = (POINT*)malloc(poly->nvertices * sizeof(POINT));
    POINT *data = (POINT*)poly->xy;
    RECT r;
    int i;

    r.left = r.right = data[0].x;
    r.top = r.bottom = data[0].y;
    for (i = 0; i < poly->nvertices; i++) {
       pl[i].x = data[i].x;
       pl[i].y = INV(data[i].y);
       if (r.left > pl[i].x)
           r.left = pl[i].x;
       if (r.top > pl[i].y)
           r.top = pl[i].y;
       if (r.right < pl[i].x)
           r.right = pl[i].x;
       if (r.bottom < pl[i].y)
           r.bottom = pl[i].y;
    }
    if (w_fp[w_curfp]) {
        int i, x1, y1;
        HDC dcMem;
        HBITMAP bm;
        HBRUSH brush;

        for (i = 0; i < poly->nvertices; i++) {
           pl[i].x -= r.left;
           pl[i].y -= r.top;
        }
        x1 = r.left;
        r.right -= r.left;
        r.left = 0;
        y1 = r.top;
        r.bottom -= r.top;
        r.top = 0;

        dcMem = CreateCompatibleDC(w_dc);
        if (w_cmap)
            SelectPalette(dcMem, w_cmap, False);
        bm = CreateCompatibleBitmap(w_dc, r.right, r.bottom);
        bm = (HBITMAP)SelectObject(dcMem, bm);
        BitBlt(dcMem, 0, 0, r.right, r.bottom, w_dc, x1, y1, SRCCOPY);
        brush = CreateSolidBrush(w_fg);
        brush = SelectBrush(dcMem, brush);
        Polygon(dcMem, pl, poly->nvertices);
        brush = SelectBrush(dcMem, brush);
        DeleteBrush(brush);
        SetTextColor(w_dc, RGB(255,255,255));
        BitBlt(w_dc, x1, y1, r.right, r.bottom, dcMem, 0, 0, COPYFG);
        SetTextColor(w_dc, w_fg);
        bm = (HBITMAP)SelectObject(dcMem, bm);
        DeleteBitmap(bm);
        DeleteDC(dcMem);
    }
    else
        Polygon(w_dc, pl, poly->nvertices);
    free(pl);
}


static HPEN
new_pen(COLORREF cref, int mask)
{
    HPEN pen;
    if (mask == 0 || mask == 255)
        pen = CreatePen(PS_SOLID, 0, cref);
    else {
        DWORD cnt, st[8];
        int i, len, lst;
        LOGBRUSH lb;
        lb.lbStyle = BS_SOLID;
        lb.lbColor = cref;

        len = 1;
        cnt = 0;
        lst = mask & 1;
        for (i = 1; i < 8; i++) {
            if (mask & (1 << i)) {
                if (!lst) {
                    st[cnt++] = len;
                    len = 1;
                    lst = 1;
                }
                else
                    len++;
            }
            else {
                if (lst) {
                    st[cnt++] = len;
                    len = 1;
                    lst = 0;
                }
                else
                    len++;
            }
        }
        st[cnt++] = len;
        pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 0, &lb, cnt, st);
    }
    return (pen);
}


static int
w_DefLs(int id, int mask)
{
    if (id) {
        w_lsmask = mask;
        w_SetLs(id);
    }
    return (0);
}


static int
w_SetLs(int id)
{
    HPEN pen = new_pen(w_fg, (id && w_lsmask) ? w_lsmask : 0);
    w_linestyle = id ? 1 : 0;
    pen = SelectPen(w_dc, pen);
    if (pen)
        DeletePen(pen);
    return (0);
}


// Reverse bit order and complement
//
unsigned char
revnotbits(unsigned char c)
{
    unsigned char out = 0;
    int i;

    for (i = 0;;) {
        if (!(c & 1))
            out |= 1;
        i++;
        if (i == 8)
            break;
        c >>= 1;
        out <<= 1;
    }
    return (out);
}


static int
w_DefFp(int id, int *map)
{
    if (id < 0 || id >= 64)
        return (1);
    if (w_fp[id]) {
        DeleteBitmap(w_fp[id]);
        w_fp[id] = 0;
    }
    if (map) {
        unsigned short invmap[16];
        int nx = 8;
        int ny = 8;
        int sx = nx/8;
        int i, j = 0;

        // 0 -> text fg color, 1 -> text bg color, so invert map pixels
        for (i = 0; i < ny; i++) {
            if (sx == 1) {
                invmap[i] = revnotbits(map[j]);
                j++;
            }
            else {
                invmap[i] = (revnotbits(map[j]) << 8) |
                    revnotbits(map[j+1]);
                j += 2;
            }
        }
        w_fp[id] = CreateBitmap(nx, ny, 1, 1, invmap);
    }
    return (0);
}


static int
w_SetFp(int id)
{
    HBRUSH brush;
    if (id < 0 || id >= 64)
        return (1);
    w_curfp = id;
    if (!w_fp[w_curfp])
        brush = CreateSolidBrush(w_fg);
    else
        brush = CreatePatternBrush(w_fp[w_curfp]);
    brush = SelectBrush(w_dc, brush);
    if (brush)
        DeleteBrush(brush);
    return (0);
}


static int
w_DefColor(int pix, int r, int g, int b)
{
    if (!w_cmap)
        w_colors[pix] = RGB(r, g, b);
    else {
        if (w_colors[pix] & 0xff000000) {
            /* already have an allocated entry for pix, reset the color */
            PALETTEENTRY p;
            p.peRed = r;
            p.peGreen = g;
            p.peBlue = b;
            /* This doesn't work reliably.  How to find out how many unused
             * system entries are available?
             */
            /*
            if (pix && pix < CNUM) {
            */
            if (0) {
                p.peFlags = PC_RESERVED;
                AnimatePalette(w_cmap, pix, 1, &p);
            }
            else {
                p.peFlags = 0;
                SetPaletteEntries(w_cmap, w_colors[pix], 1, &p);
                RealizePalette(w_dc);
            }
        }
        else {
            /* allocate a new color table entry */
            PALETTEENTRY entry;
            entry.peRed = r;
            entry.peGreen = g;
            entry.peBlue = b;
            entry.peFlags = 0;
            w_cmap_size++;
            ResizePalette(w_cmap, w_cmap_size);
            SetPaletteEntries(w_cmap, w_cmap_size - 1, 1, &entry);
            w_colors[pix] = PALETTEINDEX(w_cmap_size - 1);
            RealizePalette(w_dc);
        }
    }
    w_SetColor(pix);
    return (0);
}


static int
w_SetColor(int pix)
{
    HPEN pen;

    w_fg = w_colors[pix];
    w_fg_indx = pix;
    SetTextColor(w_dc, w_fg);
    pen = new_pen(w_fg, (w_linestyle && w_lsmask) ? w_lsmask : 0);
    pen = SelectPen(w_dc, pen);
    if (pen)
        DeletePen(pen);
    if (!w_fp[w_curfp]) {
        HBRUSH brush = CreateSolidBrush(w_fg);
        brush = SelectBrush(w_dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    return (0);
}


static void
w_Text(char *text, int x, int y, int notused)
{
    if (!text)
        return;
    TextOut(w_dc, x, INV(y), text, strlen(text));
}


static void
w_ScText(char *text, int x, int y, int notused1, int notused2)
{
    if (!text)
        return;
    TextOut(w_dc, x, INV(y), text, strlen(text));
}


void
w_Clear()
{
    RECT r;
    HBRUSH brush;

    GetClientRect(w_hwnd, &r);
    r.right++;
    r.bottom++;
    brush = (HBRUSH)GetClassLong(w_hwnd, GCL_HBRBACKGROUND);
    if (!brush) {
        brush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
        FillRect(w_dc, &r, brush);
        DeleteBrush(brush);
    }
    else
        FillRect(w_dc, &r, brush);
}


int
w_Point(int *xret, int *yret, int *keyret, int *butret)
{
    MSG msg;
    int done = 0;

    *keyret = *butret = 0;
    *xret = *yret = 0;
    while (!done && GetMessage(&msg, 0, 0, 0)) {
        switch (msg.message) {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            if (msg.message == WM_LBUTTONDOWN)
                *butret = 1;
            else if (msg.message == WM_MBUTTONDOWN)
                *butret = 2;
            else
                *butret = 3;
            *xret = msg.lParam & 0xffff;
            *yret = INV(msg.lParam >> 16);
            done = 1;
            break;

        case WM_CHAR:
            *keyret = msg.wParam & 0xff;
            *xret = 0;
            *yret = INV(0);
            *butret = -1;
            done = 1;
            break;

        default:
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_LEFT) {
                *keyret = 331;
                *xret = 0;
                *yret = INV(0);
                *butret = -1;
                done = 1;
                break;
            }
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_RIGHT) {
                *keyret = 333;
                *xret = 0;
                *yret = INV(0);
                *butret = -1;
                done = 1;
                break;
            }
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_DELETE) {
                *keyret = 127;
                *xret = 0;
                *yret = INV(0);
                *butret = -1;
                done = 1;
                break;
            }
            if (!TranslateAccelerator(msg.hwnd, 0, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    if (w_drawghost && w_firstghost == False) {
        int tmpfg;
        SetROP2(w_dc, R2_XORPEN);
        tmpfg = w_fg_indx;
        w_SetColor(ColorTable[HighlightingColor].Ent);
        (*w_drawghost)(w_lastx, w_lasty, w_refx, w_refy);
        w_SetColor(tmpfg);
        SetROP2(w_dc, R2_COPYPEN);
        w_firstghost = True;
    }
    if (w_fs_cursor && w_fs_first == False) {
        int tmpfg;
        SetROP2(w_dc, R2_XORPEN);
        tmpfg = w_fg_indx;
        w_SetColor(ColorTable[HighlightingColor].Ent);
        draw_fs_cursor(w_lastx, w_lasty);
        w_SetColor(tmpfg);
        SetROP2(w_dc, R2_COPYPEN);
        w_fs_first = True;
    }
    return (0);
}


static void
w_Beep(int i)
{
    putchar('\007');
}


static int
w_SetFullScreenCursor(int onflag)
{
    w_fs_cursor = onflag;
    if (onflag) {
        SetCursor(0);
        w_fs_first = True;
    }
    return (0);
}


static void
w_TextBB(char *t, int *x, int *y)
{
    int hei = 0, wid = 0, cnt = 0;
    char *s, *s0;

    if (!t)
        t = "x";
    s0 = t;
    for (s = t; *s; s++) {
        if (*s == '\n') {
            int w = textwidth(s0, cnt);
            if (w > wid)
                wid = w;
            hei += FB.fFontHeight;
            s0 = s+1;
            cnt = 0;
            continue;
        }
        cnt++;
        if (*(s+1) == 0) {
            int w = textwidth(s0, cnt);
            if (w > wid)
                wid = w;
        }
    }
    if (wid == 0)
        wid = FB.fFontWidth;
    *x = wid;
    if (hei == 0)
        hei = FB.fFontHeight;
    *y = hei;
}


void
w_More(int Left, int Bottom, int Right, int Top, FILE *Textfile)
{
    char cbuf[MAXCHARLINE+9];    /* add extra space for tab expansion */
    int key;
    int i,j,x,y,c,button,dy;
    int done;
    int oldfillpattern;
    int oldforeground;
    int newbackground;
    int nlines;
    int pagecount;
    int controlchar;
    int width_screen;       /* width of the box */
    int width_string;       /* width of the current string */
    int width_space;        /* width of a space character */
    int variable_width;    /* True if current font is variable width */

    /* test to be sure of window area */
    if (Top < Bottom) {
        int tmp = Top;
        Top = Bottom;
        Bottom = tmp;
    }
    if (Right < Left) {
        int tmp = Right;
        Right = Left;
        Left = tmp;
    }

    /* calculate parameters */
    dy = FB.fFontHeight;
    nlines = (Top - Bottom) / dy;
    if (nlines <= 0)
        return;
    width_screen = Right - Left;
    width_space = textwidth(" ", 1);
        variable_width = False;
    if (textwidth("x", 1) != textwidth("M", 1)) {
        variable_width = True;
        width_screen -= 3;
    }

    /* save old style ID's */
    oldforeground = w_fg_indx;
    oldfillpattern = w_curfp;

    w_SetFp(0);
    if (oldforeground == 0)
        newbackground = ColorTable[HighlightingColor].Ent;
    else
        newbackground = 0;

    pagecount = 0;
    done = 0;

    while (!done) {
        /* Display one page of text from the file */
        w_SetColor(newbackground);
        w_Box(Left, Bottom, Right, Top);   /* Erase the screen */
        w_SetColor(oldforeground);
        for (j = 1; j < nlines; ++j) {
            /* Loop for each line of page */
            i = 0;
            width_string = 0;
            controlchar = 0;
            while ((c = getc(Textfile)) != '\n' && c != EOF) {
                /* Get a char */
                if (c == '\t') {
                    /* tab */
                    cbuf[i++] = ' ';
                    if ((width_string += width_space) > width_screen) {
                        ungetc(c,Textfile);
                        break;
                    }
                    while (i % 8 != 0) {
                        cbuf[i++] = ' ';
                        width_string += width_space;
                    }
                    if (width_string > width_screen)
                        break;
                }
                else if (c < ' ') {
                    /* control character */
                    if (controlchar == 0) {
                        cbuf[i] = '^';
                        ungetc(c,Textfile);
                        if ((width_string += (variable_width) ?
                                textwidth(&cbuf[i], 1) :
                                width_space) > width_screen)
                            break;
                        controlchar = 1;
                    }
                    else {
                        cbuf[i] = c + '@';
                        controlchar = 0;
                        if ((width_string += (variable_width) ?
                                textwidth(&cbuf[i], 1) :
                                width_space) > width_screen) {
                            ungetc(c,Textfile);
                            --i;
                            break;
                        }
                    }
                    ++i;
                }
                else if (c <= '~') {
                    /* < DEL character? */
                    if ((width_string += (variable_width) ?
                            textwidth((char *) &c, 1) :
                            width_space) > width_screen) {
                        ungetc(c,Textfile);
                        break;
                    }
                    cbuf[i++] = c;
                }
                if (i >= MAXCHARLINE)
                    break;
            }
            cbuf[i] = '\0';
            w_Text(cbuf, Left, Top - j * dy, 0);
            if (c == EOF) {
                done = 1;
                break;
            }
        }
        if (done)
            strcpy(cbuf, "-DONE- (^U to scroll up, ? for help)");
        else
            strcpy(cbuf, "-MORE- (^U to scroll up, ^D to exit, ? for help)");
        w_Text(cbuf, Left, Bottom, 0);

        i = Left + textwidth(cbuf, strlen(cbuf));
        sprintf(cbuf, "Page %d", ++pagecount);
        j = Right - textwidth(cbuf, strlen(cbuf)) - 3;
        if (j < i)
            j = i;
        w_Text(cbuf, j, Bottom, 0);


        /* Wait for user interaction and perform requested function */
        for (;;) {
            w_Point(&x, &y, &key, &button);
            if (button == -1) {
                /* Keyboard key pressed */
                if (key == 4)   /* ^D */
                    done = 1;
            }
            else {
                /* Mouse button pressed */
                if (x < Left || x > Right || y < Bottom || y > Top)
                    done = 1;
                else if (button == FB.fButtonMask[1]) {
                    /* MB2 - down */
                    if (done)
                        continue;
                }
                else if (button != FB.fButtonMask[0])
                    continue;
            }
            break;
        }
    }
    w_SetFp(oldfillpattern);
}


static void
w_SetName(char *wname, char *iname)
{
    SetWindowText(w_hwnd, wname);
}


static int
w_Blink(int colorId, int r,  int g, int b, int onflag)
{
    return (1);
}


static int
w_SelectFont(int l, int b, int r, int t)
{
    return (1);
}


static void
w_SelectCursor(int l, int b, int r, int t)
{
}


static void
w_SetTextClip(int l, int b, int r, int t)
{
}


static void
w_Resize(int l, int b, int r, int t)
{
}


/*=========================================================================
 *
 * Local MS Windows Functions
 *
 *========================================================================*/


// Register the window class using RegisterClassEx if it is available.
// If not, registers the class using RegisterClass.
//
static ATOM
msw_RegisterClass(const LPWNDCLASSEX lpwcex)
{
    // Get the module handle of the 32-bit USER DLL
    HANDLE hModule = GetModuleHandle (TEXT("USER32"));
    WNDCLASS wc;
    if (hModule != 0) {

        // If we're running on a Win32 version supporting RegisterClassEx
        //  get the address of the function so we can call it

        REGISTERCLASSEXPROC proc =
            (REGISTERCLASSEXPROC)GetProcAddress((HINSTANCE)hModule, 
            "RegisterClassExA");

        if (proc != 0)
            // RegisterClassEx exists...
            // return RegisterClassEx(&wcex);
            return (*proc)(lpwcex);
    }

    /* Convert the WNDCLASSEX structure to a WNDCLASS structure */
    wc.style         = lpwcex->style;
    wc.lpfnWndProc   = lpwcex->lpfnWndProc;
    wc.cbClsExtra    = lpwcex->cbClsExtra;
    wc.cbWndExtra    = lpwcex->cbWndExtra;
    wc.hInstance     = lpwcex->hInstance;
    wc.hIcon         = lpwcex->hIcon;
    wc.hCursor       = lpwcex->hCursor;
    wc.hbrBackground = lpwcex->hbrBackground;
    wc.lpszMenuName  = lpwcex->lpszMenuName;
    wc.lpszClassName = lpwcex->lpszClassName;

    return (RegisterClass(&wc));
}


// Process messages for the main (shell) window and the subwindow shells
//
static LRESULT CALLBACK
mainFrameWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    char buf[256];
    int x, y;

    switch (message) {
    case WM_PAINT:
        BeginPaint(w_hwnd, &ps);
        EndPaint(w_hwnd, &ps);
        if (FB.fInitialized)
            RedisplayKIC();
        return (0);

    case WM_MOUSEMOVE:
        x = LOWORD(lParam);
        y = HIWORD(lParam);
        if (w_drawghost) {
            if (!w_firstghost) {
                SetROP2(w_dc, R2_XORPEN);
                (*w_drawghost)(w_lastx, w_lasty, w_refx, w_refy);
                SetROP2(w_dc, R2_COPYPEN);
            }

            SetROP2(w_dc, R2_XORPEN);
            (*w_drawghost)(x, y, w_refx, w_refy);
            SetROP2(w_dc, R2_COPYPEN);
            w_firstghost = False;
        }
        if (w_fs_cursor) {
            if (!w_fs_first) {
                SetROP2(w_dc, R2_XORPEN);
                draw_fs_cursor(w_lastx, w_lasty);
                SetROP2(w_dc, R2_COPYPEN);
            }
            SetROP2(w_dc, R2_XORPEN);
            draw_fs_cursor(x, y);
            SetROP2(w_dc, R2_COPYPEN);
            w_fs_first = False;
        }
        w_lastx = x;
        w_lasty = y;
        return (0);

    case WM_DROPFILES:
        if (DragQueryFile((HDROP)wParam, 0, buf, 255) > 0) {
            strcpy(Parameters.kpCellName, buf);
            Edit(True, True, False);
        }
        DragFinish((HDROP)wParam);
        return (0);

    case WM_CLOSE:
        AbortKIC();
        return (0);
    }

    return (DefWindowProc(hwnd, message, wParam, lParam));
}


#define MSW_FIXED_FONT "Lucida Console"
//#define MSW_FIXED_FONT "Andale Mono"

// Return a "suitable" fixed font
//
static HFONT
msw_GetFixedFont(int width, int height)
{
    HFONT fixed;
    LOGFONT lf;
    lf.lfHeight = height;
    lf.lfWidth = width;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_DONTCARE;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = 0;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    strcpy(lf.lfFaceName, MSW_FIXED_FONT);
    fixed = CreateFontIndirect(&lf);
    return (fixed);
}


/*=========================================================================
 *
 * Misc. Local Functions
 *
 *========================================================================*/

static void
draw_fs_cursor(int x, int y)
{
    int xmin = MenuViewport.kaRight * FB.fFontWidth;
    int xmax = FB.fMaxX;
    int ymin = INV(FB.fMaxY);
    int ymax = INV(ParameterViewport.kaTop);
    int tmpfg = w_fg_indx;

    SetCursor(0); /* hide the regular cursor */
    w_SetColor(ColorTable[HighlightingColor].Ent);
    if (x < xmin || x > xmax || y < ymin || y > ymax) {
        w_Line(x-5, INV(y), x+6, INV(y));
        w_Line(x, INV(y-5), x, INV(y+6));
    }
    else {
        w_Line(xmin, INV(y), xmax, INV(y));
        w_Line(x, INV(ymin), x, INV(ymax));
    }
    w_SetColor(tmpfg);
}


static int
w_SetGhost(void(*callback)(int, int, int, int), int x, int y)
{
    if (callback) {
        w_refx = x;
        w_refy = y;
        w_drawghost = callback;
        w_firstghost = True;
        return (0);
    }
    w_drawghost = NULL;
    return (0);
}


static void
ghost_line(int x, int y, int refx, int refy)
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
    w_SetColor(Parameters.kpLayer);
}


static void
ghost_line_snap(int x, int y, int refx, int refy)
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
    w_SetColor(Parameters.kpLayer);
}


static void
ghost_box(int x, int y, int refx, int refy)
{
    struct ka BB;

    if (!GetWindowCoords((int*)&x,(int*)&y,False))
        return;
    BB.kaLeft = refx;
    BB.kaBottom = refy;
    BB.kaRight = x;
    BB.kaTop = y;
    ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
    w_SetColor(Parameters.kpLayer);
}


static void
ghost_box_snap(int x, int y, int refx, int refy)
{
    struct ka BB;

    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    BB.kaLeft = refx;
    BB.kaBottom = refy;
    BB.kaRight = x;
    BB.kaTop = y;
    ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
    w_SetColor(Parameters.kpLayer);
}


static void
ghost_stretch(int x, int y, int refx, int refy)
{
    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    ShowStretch(x,y,refx,refy);
    w_SetColor(Parameters.kpLayer);
}


static void
ghost_move(int x, int y, int refx, int refy)
{
    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    ShowMove(refx,refy,x,y);
    w_SetColor(Parameters.kpLayer);
}


static void
ghost_place(int x, int y, int refx, int refy)
{
    if (!GetWindowCoords((int*)&x,(int*)&y,True))
        return;
    ShowNewInstance(x,y,refx,refy);
    w_SetColor(Parameters.kpLayer);
}


static char *
kbedit(char *s, int x, int y, int bg, int fg, int cc)
/*
 * s      Initial string to edit.
 * x,y    Lower left coordinates.
 * bg,fg  Background and foreground colors.
 * cc     Background color at cursor location.
 * Returns edited string (static!).
 */
{
    char tbuf[128];
    int k;
    char *end, ctmp[2];

    doing_edit = True;
    editFg = fg;
    editCc = cc;
    editX = x;
    editY = y;

    w_SetColor(fg);
    *editBuf = '\0';
    if (s) {
        /* s can be NULL */
        w_Text(s, editX, editY, 0);
        strcpy(editBuf,s);
    }
    ctmp[1] = '\0';
    editC = editBuf;
    end = strchr(editBuf,'\0');
    *(end+1) = '\0';
    ctmp[0] = *editC;
    w_SetColor(cc);
    textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
    w_SetColor(fg);
    w_Text(ctmp, editX + (int)(editC-editBuf)*FB.fFontWidth, editY, 0);
    for (;;) {
        k = w_Getchar();
        if ((char) k == '\r') break;
        switch (k) {

        case 333:    /* right arrow */
            if (editC >= end) continue;
            ctmp[0] = *editC;
            w_SetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_SetColor(fg);
            w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC++;
            ctmp[0] = *editC;
            w_SetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_SetColor(fg);
            w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case 331:    /* left arrow */
            if (editC <= editBuf) continue;
            ctmp[0] = *editC;
            w_SetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_SetColor(fg);
            w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC--;
            ctmp[0] = *editC;
            w_SetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_SetColor(fg);
            w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case '\b':
            if (editC == editBuf) continue;
            w_SetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            editC--;
            w_Text(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            w_SetColor(fg);
            *editC = '\0';
            strcat(editBuf,++editC);
            w_Text(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC--;
            if (end > editBuf) end--;
            ctmp[0] = *editC;
            w_SetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_SetColor(fg);
            w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case 339: /* DEL, in DOS */
        case 127:
            if (editC == end) continue;
            w_SetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_Text(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            w_SetColor(fg);
            *editC = '\0';
            strcat(editBuf,++editC);
            w_Text(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            editC--;
            if (end > editBuf) end--;
            ctmp[0] = *editC;
            w_SetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_SetColor(fg);
            w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            continue;

        case '\025': /* ^U */
        case '\030': /* ^X */
        case '\033': /* ESC */
            w_SetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_Text(editBuf,editX,editY,0);
            editC = end = editBuf;
            *editC = '\0';
            if ((char) k == '\033') {
                doing_edit = False;
                return (NULL);
            }
            w_SetColor(cc);
            textcursor(editX,editY);
            w_SetColor(fg);
            continue;
        default:
            if (k > 255) continue;
            if (((char) k < ' ') || ((char) k > '~'))  continue;
            w_SetColor(bg);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_Text(editC,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            strcpy(tbuf,editC);
            *editC = (char) k;
            sprintf(++editC,tbuf);
            ctmp[0] = *editC;
            w_SetColor(fg);
            w_Text(editC-1,
                editX+(int)(editC-1-editBuf)*FB.fFontWidth,editY,0);
            w_SetColor(cc);
            textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
            w_SetColor(fg);
            w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
            end++;
            *(end+1) = '\0';

        }
    }
    w_SetColor(bg);
    textcursor(editX+(int)(editC-editBuf)*FB.fFontWidth,editY);
    w_SetColor(fg);
    ctmp[0] = *editC;
    w_Text(ctmp,editX + (int)(editC-editBuf)*FB.fFontWidth,editY,0);
    doing_edit = False;
    return (editBuf);
}


static void
textcursor(int x, int y)
{
    w_Box(x,y-1,x+FB.fFontWidth,y+CURHT);
}


static int
textwidth(char *t, int len)
{
    SIZE sz;
    if (!t) {
        t = "x";
        len = 1;
    }
    else if (len < 0)
        len = strlen(t);
    else if (len == 0)
        return (0);
    GetTextExtentPoint32(w_dc, t, len, &sz);
    return (sz.cx);
}


static int
w_Getchar(void)
{
    int x, y, key, button;
    int fs_save = w_fs_cursor;
    w_SetFullScreenCursor(False);

    for (;;) {
        w_Point(&x, &y, &key, &button);
        if (button == -1) {
            w_SetFullScreenCursor(fs_save);
            break;
        }
    }
    return (key);
}

#endif

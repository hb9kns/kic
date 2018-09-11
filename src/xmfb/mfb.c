/*************************************************************************
 XMFB X-Window MFB emulation package
 Authors: Peter C. Vernam, C. S. Draper Laboratory, 1989
          Stephen R. Whiteley, 1992
 *************************************************************************/

/*
 * This is a hard-coded version of MFB written to work with the X
 * window management package developed at MIT (the Athena Project).
 * It supports one variable sized window.
 * It should be possible to use this library without 
 * recompiling the source program.
 *
 * Some odd quirks:
 *   MFB assumes the origin is at the lower left.  X assumes the
 *   origin is at the upper left.  All Y coordinates must be
 *   translated.  This is done using the macro TRAN.
 *
 * David Harrison, May 1985:
 *   Revised significantly June 1986.
 *   This version of MFB is valid for version 10 release 3 of
 *   the X window system.  It should work on both the XDEV_QVSS and
 *   XDEV_QDSS displays.
 *
 * Peter C. Vernam, Charles Stark Draper Laboratory, October 1989:
 *   Revised completely for use with X Window System version 11.3 (in
 *   particular, VAX/VMS DECwindows version 5.1).  Only Xlib is required
 *   (i.e., none of the higher-level DECwindows toolkit routines are used),
 *   since the intention was to preserve the look and feel of KIC, not make
 *   it conform to some other user interface style.  The enhancements include:
 *   1) All event handling has been centralized in MFBPoint.  MFBGetchar and
 *      MFBKeyboard both call MFBPoint to get keyboard characters.
 *   2) The text display routines MFBKeyboard, MFBMore, and MFBScroll have
 *      been revised considerably to correctly handle variable-width text
 *      fonts.
 *   3) Considerable code has been added to provide support for rubber-banded
 *      lines and rectangles, and an optional (dragged) full-screen cursor.
 *   4) Several new routines have been created, which are in mfbcsdl.c.
 *
 *   This version is tailored specifically for KIC, the only application we
 *   have that makes calls to MFB routines.  If it is to be used with any
 *   other applications, the following hard-coded details will have to be
 *   dealt with:
 *   1) The window name and icon name are both set to "XKIC" in the call
 *      to XSetStandardProperties within MFBOpen.
 *   2) Exposure events are handled (in MFBPoint) by calling XClearWindow
 *      and FullRedisplay (a KIC routine) at the end of the series of events.
 *      No attempt has been made to do partial redraws when a small portion
 *      of the top-level window becomes exposed (FullRedisplay was fast enough
 *      on a VS3200 that it was deemed to be not worth the effort to implement
 *      this).
 *   3) The number of color cells allocated is 40.  This is based on the fact
 *      that KIC requires 37 (35 layers + background + highlighting), leaving
 *      a few to spare.  If MFB tries to allocate too many color cells, it
 *      may fail or not leave enough for other applications.  No attempt has
 *      been made to gracefully recover from failure to allocate color cells.
 *
 *   This version is also tailored specifically to run on a VAXstation 3200,
 *   running DECwindows version 1 (VMS 5.1).  Although it should run just fine
 *   on any other X-server, the default size of the top-level window is set
 *   such that it nearly fills the entire VS3200 screen, after allowing for
 *   the decoration that is added by the DECwindows window manager.
 *
 * Modified by S. R. Whiteley, 1992.
 * 1. Rubber banding improved. The function
 *    MFBSetGhost(void(*callback)(int x, int y,int refx, int refy),
 *        int refx,int refy)
 *    registers the function callback() to be called for ghosting,
 *    with the saved refx and refy (can be window or viewport coords),
 *    and x,y as the current viewport coords.
 *
 * 2. The XMFB defines were added to simplify window initialization.
 *
 * 3. MFB now calls a routine RepaintWindow() instead of FullRedisplay().
 *
 * 4. A stack of frames was added, so that multiple windows can be opened.
 *
 * 5. NUM_XMFB_COLORS (default 40) colors are allocated.
 *
 * 6. Callback
 *    SetDisplayWindow(int screen_width, int screen_height, int *x, int *y,
 *        int *width, int *height)
 *    was added so that window size and placement can be predefined through
 *    the pointers.
 *
 * 7. The default cursor was changed.
 *
 * Modified by S. R. Whiteley 1998
 * 1. Support added for TrueColor visuals
 * 
 */

#ifndef XMFB_START_FONT
#define XMFB_START_FONT "9x15"
#endif
#ifndef XMFB_WIN_TITLE
#define XMFB_WIN_TITLE  "KIC"
#endif
#ifndef XMFB_ICON_TITLE
#define XMFB_ICON_TITLE "KIC"
#endif

/* Choose one of these for mapping the white pixel */
#define WHITE_INDEX  mfb_maxColors-1
/*
#define WHITE_INDEX  1
*/

/* this is the number of colors allocated */
int NUM_XMFB_COLORS = 40;

#define MFBALLOCATE
#include "mfb.h"            /* standard MFB header file */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <X11/cursorfont.h>


MFB *MFBCurrent;

#if __STDC__
extern void fatal_error(const char*);
extern int MFBXError(Display*, XErrorEvent*);
static int fixup(int*);
static void draw_full_screen_cursor(void);
#else
extern void fatal_error();
extern int MFBXError();
static int fixup();
static void draw_full_screen_cursor();
#endif

/* Small cross-hair cursor */
#define cursor_width 16
#define cursor_height 16
#define cursor_x_hot 8
#define cursor_y_hot 7


static char cursorCross_bits[] = {
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x7e, 0xfc,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00 };
static char cursorCross_mask[] = {
        0x80, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03,
        0x80, 0x03, 0x80, 0x03, 0x7e, 0xfc, 0x7e, 0xfc,
        0x7e, 0xfc, 0x80, 0x03, 0x80, 0x03, 0x80, 0x03,
        0x80, 0x03, 0x80, 0x03, 0x80, 0x03, 0x00, 0x00 };

/* Tiny dot cursor (to display at the center of the full-screen cursor) */
static char cursorDot_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* Border is a tight hashing of black and white */
#define gray_width 16
#define gray_height 16
static char gray_bits[] = {
        0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
        0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
        0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
        0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa };

static char mfbInitMessage[81] =
        "X-Windows initialization error for display \"";

typedef unsigned long Pixel;

#ifdef MFBFRAMES

struct frame {
    MFB *f_mfb;
    struct frame *f_next;
};
static struct frame *Frames;

#endif


// Return information about the user's graphics mode
//
static XVisualInfo *
find_visual(Display *display, int screen)
{
    XVisualInfo vt, *vl;
    int i, vmatch;
    vt.screen = screen;
    /* This sucks, there must be an easier way to get info on the
       default visual */
    vl = XGetVisualInfo(display, VisualScreenMask, &vt, &vmatch);
    for (i = 0; i < vmatch; i++)
        if (vl[i].visual == DefaultVisual(display, screen))
            break;
    if (vl[i].class == PseudoColor && vl[i].depth >= 8)
        return (vl + i);
    if (vl[i].class == TrueColor)
        return (vl + i);
    return (0);
}


MFB *
MFBOpen(TerminalName, DeviceName, errCode)

char *TerminalName, *DeviceName;
int *errCode;
/*
 * This routine binds a terminal to an MFB driver and fills
 * information in the MFB data structure.  Under X, DeviceName
 * is a null terminated string representing the host name and display
 * number.  These two items are seperated by a colon.  If the display
 * number is not provided, it defaults to '0'.  X does not use the
 * TerminalName field.
 */
{
    Display           *mydisplay;
    Window            mywindow;
    int               myscreen;
    GC                mygc;
    Pixmap            myborder, mycursor_data, mycursor_mask;
    XWindowAttributes winInfo;
    XSizeHints        sizehint;
    XFontStruct       *myfont;
    XColor            mybackg, myforeg;
    int               index, fontheight;
    int               ht,wd,dw_x,dw_y,dw_w,dw_h;
    unsigned long black_pixel, white_pixel;

    if (!DeviceName)
    DeviceName = ":0";

    /* Open the connection to the X server */
    if ((mydisplay = XOpenDisplay(DeviceName)) == NULL) {
        strcat(mfbInitMessage, DeviceName ? DeviceName : "");
        strcat(mfbInitMessage, "\"");
        *errCode = MFBBADENT;
        return NULL;
    }
    else {
        strcat(mfbInitMessage, DisplayString(mydisplay));
        strcat(mfbInitMessage, "\"");
    }

    /* Set up the error handler */
    XSetErrorHandler(MFBXError);

    /* Load the starting font */
    myfont = XLoadQueryFont(mydisplay,XMFB_START_FONT);
    if (myfont==NULL)
        myfont = XLoadQueryFont(mydisplay,"fixed");
    if (myfont==NULL)
        fatal_error("Can't open font.");
    fontheight = myfont->ascent + myfont->descent;

    /* Open a new X window.  The estimated size of the window is large:
     * nearly the entire root window.  However, the user can resize it to
     * suit his taste.
     * (Note: The margins subtracted from the display width and height
     * below are to allow for the decoration that is added by the DECwindows
     * window manager.)
     */

    myscreen = DefaultScreen(mydisplay);
    ht = DisplayHeight(mydisplay,myscreen);
    wd = DisplayWidth(mydisplay,myscreen);
    dw_x = 0;
    dw_y = 0;
    dw_w = wd - 18;
    dw_h = ht - 40;
    SetDisplayWindow(wd,ht,&dw_x,&dw_y,&dw_w,&dw_h);
    sizehint.width  = dw_w;
    sizehint.height = dw_h;
    sizehint.x = dw_x;
    sizehint.y = dw_y;
    sizehint.min_width  = sizehint.width / 4;
    sizehint.min_height = sizehint.height / 4;
    sizehint.flags = PPosition | PSize | PMinSize;

    /* Create window with border width 5, border white, background black. */
    black_pixel  = BlackPixel(mydisplay, myscreen);
    white_pixel  = WhitePixel(mydisplay, myscreen);
    mywindow = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay),
        sizehint.x, sizehint.y, sizehint.width, sizehint.height,
        5, white_pixel, black_pixel);
    if (mywindow == (Window)0L) {
        *errCode = MFBBADENT;
        return NULL;
    }

    /* Create a GC to be used for line objects */
    mygc = XCreateGC(mydisplay, mywindow, 0, 0);
    if (mygc == NULL) {
        *errCode = MFBBADENT;
        return NULL;
    }
    XSetFont(mydisplay, mygc, myfont->fid);
    XSetFunction(mydisplay, mygc, GXcopy);        /* this is the default */
    XSetArcMode(mydisplay, mygc, ArcPieSlice);    /* this is the default */
    XSetFillRule(mydisplay, mygc, WindingRule);

#if defined(MFBMALLOC) || defined(MFBFRAMES)
    /* Create and fill the MFB record */
    MFBCurrent = (MFB *) malloc(sizeof(MFB));
    if (MFBCurrent == NULL) {
        *errCode =  MFBBADMEM;
        return NULL;
    }
    memset(MFBCurrent,(char)0,sizeof(MFB));
#ifdef MFBFRAMES
    { struct frame *f = (struct frame *) malloc(sizeof(struct frame));
    if (f == NULL) {
        *errCode = MFBBADMEM;
        return NULL;
    }
    f->f_mfb = MFBCurrent;
    f->f_next = Frames;
    Frames = f; }
#endif
#else
    MFBCurrent = &MFBData;
#endif
    MFBSetFont("MFB8X16");
    mfb_display    = mydisplay;
    mfb_window     = mywindow;
    mfb_font       = myfont;
    mfb_curALUMode = MFBALUJAM;
    mfb_lineGC     = mygc;

    /* Allocate color cells */
    mfb_cmap = DefaultColormap(mydisplay, myscreen);
    if (DisplayPlanes(mydisplay, myscreen) < 2)
        /* On B/W, we make black color 0 and white color 1 */
        mfb_maxColors = 2;
    else
        /* On a color display,  we attempt to get NUM_XMFB_COLORS
         * color cells for read/write.  Other color applications
         * may fail while MFB is running. */
        mfb_maxColors = MFBmin(DisplayCells(mydisplay, myscreen),
        NUM_XMFB_COLORS);
    mfb_colors = (Pixel *) malloc((unsigned) (mfb_maxColors * sizeof(Pixel)));

{
    XVisualInfo *vl = find_visual(mydisplay, myscreen);
    if (!vl)
        fatal_error("Graphics mode unsupported.");
    if (vl->class == PseudoColor) {
        MFBCurrent->privateColors = True;
        if (!XAllocColorCells(mydisplay, mfb_cmap, 0, NULL, 0,
            mfb_colors, mfb_maxColors)) {
            fprintf(stderr,
                "Unable to allocate %d color cells.\n", mfb_maxColors);
            MFBClose();
            *errCode = MFBBADENT;
            return NULL;
        }
        XStoreNamedColor(mydisplay, mfb_cmap, "black", mfb_colors[0], -1);
        XStoreNamedColor(mydisplay, mfb_cmap, "white",
            mfb_colors[WHITE_INDEX], -1);
    }
    else {
        MFBCurrent->privateColors = False;
        for (index = 0; index < mfb_maxColors; ++index)
            mfb_colors[index] = -1;
        mfb_colors[0] = black_pixel;
        mfb_colors[WHITE_INDEX] = white_pixel;
    }
}

    XSetWindowBackground(mydisplay, mywindow, mfb_colors[0]);
    XSetBackground(mydisplay, mygc, mfb_colors[0]);
    XSetForeground(mydisplay, mygc, mfb_colors[WHITE_INDEX]);

    /* Create a GC to use for filled objects */
    mfb_fillGC = XCreateGC(mydisplay, mywindow, 0, 0);
    XCopyGC(mydisplay, mygc, -1, mfb_fillGC);

    /* Create a GC to use for rubber-banded and dragged objects */
    mfb_dragGC = XCreateGC(mydisplay, mywindow, 0, 0);
    XCopyGC(mydisplay, mygc, -1, mfb_dragGC);

    XSetForeground(mydisplay, mfb_dragGC,
        mfb_colors[0] ^ mfb_colors[WHITE_INDEX]);
    XSetBackground(mydisplay, mfb_dragGC, mfb_colors[0]);
    XSetFunction(mydisplay, mfb_dragGC, GXxor);

    /* Define a cursor.  The cursor is a 15x15 vertical cross
     * defined by cursor_bitmap. */
    mybackg.pixel = mfb_colors[0];
    myforeg.pixel = mfb_colors[WHITE_INDEX];
    XQueryColor(mfb_display, mfb_cmap, &mybackg);
    XQueryColor(mfb_display, mfb_cmap, &myforeg);
    mycursor_data = XCreateBitmapFromData(mydisplay, mywindow,
        cursorCross_bits, cursor_width, cursor_height);
    mycursor_mask = XCreateBitmapFromData(mydisplay, mywindow,
        cursorCross_mask, cursor_width, cursor_height);
    mfb_cursorCross = XCreatePixmapCursor(mydisplay, mycursor_data,
        mycursor_mask, &myforeg, &mybackg,
        cursor_x_hot, cursor_y_hot);
    XDefineCursor(mydisplay, mywindow, mfb_cursorCross);
    mfb_cursor = mfb_cursorCross;
    mfb_cursorShape = -1;
    XFreePixmap(mydisplay, mycursor_data);
    XFreePixmap(mydisplay, mycursor_mask);
    mycursor_data = XCreateBitmapFromData(mydisplay, mywindow,
        cursorDot_bits, cursor_width, cursor_height);
    mfb_cursorDot = XCreatePixmapCursor(mydisplay, mycursor_data,
        mycursor_data, &myforeg, &mybackg,
        cursor_x_hot, cursor_y_hot);
    XFreePixmap(mydisplay, mycursor_data);
    /* Create an Image stucture for display of the the cross cursor by
     * MFBScrollFont. */
    mfb_cursorImage = XCreateImage(mydisplay, DefaultVisual(mydisplay,myscreen),
        1, XYBitmap, 0, cursorCross_bits,
        cursor_width, cursor_height, 16, 0);
    mfb_cursorImage->byte_order = LSBFirst;

    myborder = XCreatePixmapFromBitmapData(mydisplay, mywindow, gray_bits,
        gray_width, gray_height, white_pixel, black_pixel,
        DefaultDepth(mydisplay, myscreen));
    XSetWindowBorderPixmap(mydisplay, mywindow, myborder);
    XSetStandardProperties(mydisplay, mywindow, XMFB_WIN_TITLE,
        XMFB_ICON_TITLE, None, NULL, 0, &sizehint);
    XFreePixmap(mydisplay, myborder);

    /* Specify the events we are interested in.
     * These include keyboard pushes and button presses. */
    mfb_eventMask =     KeyPressMask |
                        ButtonPressMask |
                        ExposureMask |
                        OwnerGrabButtonMask |
                     /* VisibilityChangeMask | */
                        StructureNotifyMask;
    mfb_visState = VisibilityUnobscured;
    XSelectInput(mydisplay, mywindow, mfb_eventMask);

    /* Get window width and height */
    XGetWindowAttributes(mydisplay, mywindow, &winInfo);
    mfb_maxX = winInfo.width;
    mfb_maxY = winInfo.height;

    /* Create an input-only window for the drawing area, to be used for
     * handling the full-screen cursor.  It will be moved and resized by a
     * call to MFBResizeDrawingWindow from within InitViewports(). */
    mfb_dwgwin = XCreateWindow(mydisplay, mywindow, 0, 0, mfb_maxX, mfb_maxY,
        0, 0, InputOnly, CopyFromParent, 0, NULL);

    mfb_dwgwinX                = 0;
    mfb_dwgwinY                = 0;
    mfb_dwgwinXmax             = mfb_maxX;
    mfb_dwgwinYmax             = mfb_maxY;
    mfb_inDwgwin               = false;   /* cursor not in dwg window */
    mfb_fullScreenCursor       = false;   /* F-S cursor not active */

    XMapWindow(mydisplay, mywindow);
    XMapSubwindows(mydisplay, mywindow);

    mfb_name                   = DisplayString(mydisplay);
    mfb_memoryBlock            = NULL;

    MFBInitialize();

    mfb_deviceType             = TTY;
    mfb_vltBool = (DisplayPlanes(mydisplay, myscreen) > 1) ? true : false;
    mfb_vltUseHLSBool          = false;
    mfb_channelMaskBool        = false;   /* no channel mask req. */
    mfb_readMaskBool           = false;
    mfb_pointingDeviceBool     = true;
    mfb_buttonsBool            = true;
    mfb_readImmediateBool      = true;
    mfb_keyboardBool           = true;
    mfb_linePatternDefineBool  = true;
    mfb_reissueLineStyleBool   = true;    /* Can change line style */
    mfb_filledPlygnBool        = true;    /* Can draw filled polys */
    mfb_textPositionableBool   = true;
    mfb_textRotateBool         = false;   /* Can't rotate text */
    mfb_replaceTextBool        = true;    /* Can replace and overstr */
    mfb_overstrikeTextBool     = true;
    mfb_blinkersBool           = false;
    mfb_rastCopyBool           = true;    /* Can copy rasters */
    mfb_rastRSCSFBool          = false;   /* No funny raster ordering */
    mfb_fillPtrnDefineBool     = true;    /* Full fill-pattern support */
    mfb_fillDefineRowMajorBool = true;
        
    /* Numerics */

    mfb_hPixelsPerInch         = DisplayWidth(mydisplay, myscreen) * 254 /
                                 DisplayWidthMM(mydisplay, myscreen) / 10;
    mfb_vPixelsPerInch         = DisplayHeight(mydisplay, myscreen) * 254 /
                                 DisplayHeightMM(mydisplay, myscreen) / 10;

    /* No off screen memory */
    mfb_minOffScreenX          = 0;
    mfb_minOffScreenY          = 0;
    mfb_offScreenDX            = 0;
    mfb_offScreenDY            = 0;

    mfb_maxIntensity = (DisplayPlanes(mydisplay, myscreen) > 0) ?
                                        MFBntox(2,8) : 0;
    mfb_lengthOfVLT            = 7;
    mfb_keyboardYOffset        = 0;       /* No keyboard positioning */
    mfb_keyboardXOffset        = 0;

    mfb_lineDefineLength       = 1;       /* One byte line styles */
    mfb_maxLineStyles          = MAXLSTYLE;

    mfb_fontWidth              = myfont->max_bounds.width;
    mfb_fontHeight             = fontheight;
    mfb_fontXOffset            = 0;
    mfb_fontYOffset            = myfont->descent;
    mfb_fontSize               = myfont->max_char_or_byte2 + 1;
    mfb_fontName[0]            = '\0';

    mfb_maxBlinkers            = 0;
    mfb_fillDefineHeight       = 8;       /* 8-bits down */
    mfb_fillDefineWidth        = 1;       /* 8-bits across */
    mfb_maxFillPatterns        = MAXFPATT;
    mfb_cursorColor1Id         = 0;
    mfb_cursorColor2Id         = 0;
    mfb_drawghost              = NULL;

    mfb_numberOfButtons        = 5;

    mfb_textMode               = 1;
    mfb_fgColorId              = -1;
    mfb_lineStyle              = -1;
    mfb_fillPattern            = -1;
    mfb_buttonMask[0]          = 1<<0;
    mfb_buttonMask[1]          = 1<<1;
    mfb_buttonMask[2]          = 1<<2;
    mfb_buttonMask[3]          = 1<<3;
    mfb_buttonMask[4]          = 1<<4;
    mfb_buttonMask[5]          = 1<<5;
    mfb_buttonMask[6]          = 1<<6;
    mfb_buttonMask[7]          = 1<<7;
    mfb_buttonMask[8]          = 1<<8;
    mfb_buttonMask[9]          = 1<<9;
    mfb_buttonMask[10]         = 1<<10;
    mfb_buttonMask[11]         = 1<<11;

    for (index = 0; index < MAXLSTYLE; ++index)
        mfb_lStyles[index][1] = 0;
    for (index = 0; index < MAXFPATT; ++index)
        mfb_fillMap[index] = None;

    mfb_initializedBool        = true;
    *errCode = MFBOK;
    MFBSetLineStyle(0);
    MFBSetFillPattern(0);
    for (;;) {
        /* wait for first Expose event */
        XEvent event;
        XNextEvent(mydisplay,&event);
        if (event.type == Expose) break;
    }
    if ((fcntl(ConnectionNumber(mfb_display),F_SETFD,1)) == -1)
        fprintf(stderr,"bad bad bad\n");

    return MFBCurrent;
}


void
SetCurrentMFB(mfb)

MFB *mfb;
/* 
 * Sets current mfb device.  Sets MFBCurrent.
 */
{
    MFBCurrent = mfb;
}


int
MFBClose()

/*
 * Wraps up the graphics display.  Closes the X display and its window.
 */
{
    XFlush(mfb_display);
    XCloseDisplay(mfb_display);
    free((char *) mfb_colors);
#ifdef MFBFRAMES
    { struct frame *f = Frames;
    Frames = Frames->f_next;
    free((char *)f); }
    if (Frames != NULL)
        MFBCurrent = Frames->f_mfb;
    else
        MFBCurrent = NULL;
#else
#ifdef MFBMALLOC
    free((char *) MFBCurrent);
#endif
    MFBCurrent = NULL;
#endif
    return MFBOK;
}


int
MFBInitialize()

/*
 * Sets up graphics display.  All this does under X is flush the buffer.
 */
{
    MFBUpdate();
    return MFBOK;
}


int
MFBUpdate()

/*
 * Flushes the output buffer to the graphics device.  Using X,  this
 * routine flushes the current TCP/IP output stream buffer.  This routine
 * DOES NOT return the number of characters sent through the stream.
 * It returns MFBOK.
 */
{
    XFlush(mfb_display);
    return MFBOK;
}


int
MFBHalt()

/*
 * Temporarily releases the graphics device for pausing.  In X,
 * MFB will be running as a seperate process in a seperate window.
 * It should not be necessary to call this routine.  It returns
 * a bad code.
 */
{
    return MFBBADOPT;
}


int
MFBntox(n, x)
int n, x;
/*
 * Raises n to the x power
 */
{
    int temp;

    if (x == 0)
        return 1;
    temp = MFBntox(n, x >> 1);
    if ((x & 1) == 0)
        return (temp * temp);
    else
        return (temp * temp * n);
}


char *
MFBError(errnum)

int errnum;
/*
 * Returns a text message describing 'errnum'.
 */
{
    char *retValue = NULL;

    switch (errnum) {
    case MFBOK:
        retValue = "Successful return ";
        break;
    case MFBGENERR:
        retValue = "General error     ";
        break;
    case MFBBADENT:
        retValue = mfbInitMessage;
        break;
    case MFBBADMCF:
        retValue = "Can't open mfbcap file ";
        break;
    case MFBMCELNG:
        retValue = "MFBCAP entry too long ";
        break;
    case MFBBADMCE:
        retValue = "Bad mfbcap entry ";
        break;
    case MFBINFMCE:
        retValue = "Infinite mfbcap entry ";
        break;
    case MFBBADTTY:
        retValue = "stdout not in /dev ";
        break;
    case MFBBADLST:
        retValue = "Illegal line style ";
        break;
    case MFBBADFST:
        retValue = "Illegal fill style ";
        break;
    case MFBBADCST:
        retValue = "Illegal color style ";
        break;
    case MFBBADTM1:
        retValue = "No destructive text ";
        break;
    case MFBBADTM2:
        retValue = "No overstriking text ";
        break;
    case MFBNODFLP:
        retValue = "No definable line styles ";
        break;
    case MFBNODFFP:
        retValue = "No definable fill styles ";
        break;
    case MFBNODFCO:
        retValue = "No definable colors ";
        break;
    case MFBNOBLNK:
        retValue = "No blinkers ";
        break;
    case MFBTMBLNK:
        retValue = "Too many blinkers ";
        break;
    case MFBBADDEV:
        retValue = "Can't open or close device ";
        break;
    case MFBBADOPT:
        retValue = "Can't access or set device stat ";
        break;
    case MFBNOMASK:
        retValue = "No definable read or write mask ";
        break;
    case MFBBADWRT:
        retValue = "Error in write ";
        break;
    case MFBPNTERR:
        retValue = "Error in pointing device ";
        break;
    case MFBNOPTFT:
        retValue = "No format for pointing device ";
        break;
    case MFBNOPNT:
        retValue = "No pointing device ";
        break;
    case MFBNORBND:
        retValue = "No Rubberbanding ";
        break;
    case MFBBADALU:
        retValue = "Cannot set ALU mode ";
        break;
    case MFBBADMEM:
        retValue = "Memory allocation error ";
        break;
    }
    return retValue;
}


static int getchar_flag = false;

int
MFBPoint(X,Y,key,button)

int *X,*Y,*key,*button;
/*
 * Enables the graphics pointing device and waits for user
 * input.  If the user presses a key,  the routine returns
 * the ASCII code for that key in 'key'.  If the user releases
 * a mouse button, the routine returns the X,Y coordinates of
 * the device along with the button mask.  Evidently, MFB has
 * no concept of pushing and releasing mouse buttons.  Thus,
 * button pressed events are not caught.  The current drawing
 * position is NOT changed.
 *
 * Note: The other MFB input routines (MFBGetchar and MFBKeyboard) are
 * now implemented by calling MFBPoint and ignoring the returned button
 * events.  This centralizes the handling of X-windows events within
 * this routine.
 */
{
    XEvent newEvent, nextEvent;
    KeySym mykey;
    int nbytes, mybutton;
    int done = 0;
    char text[20];
#ifdef MFBFRAMES
    int stk;
    struct frame *f;
#endif
    GC tmpGC;

    if (mfb_drawghost) {
        /* draw any rubber-banding */
        tmpGC = mfb_lineGC;
        mfb_lineGC = mfb_dragGC;
        (*mfb_drawghost)(mfb_lastX,mfb_lastY,mfb_bandX,mfb_bandY);
        mfb_lineGC = tmpGC;
    }
    if (mfb_fullScreenCursor)
        draw_full_screen_cursor();     /* draw full-screen cursor */

    while (!done) {    /* Wait for new event */
#ifdef MFBFRAMES
        if (XPending(mfb_display) || !Frames->f_next) {
#else
        if (1) {
#endif
            XNextEvent(mfb_display, &newEvent);
            switch (newEvent.type) {
            case ButtonPress:
                if (mfb_drawghost) {
                    /* undraw any rubber-banding */
                    tmpGC = mfb_lineGC;
                    mfb_lineGC = mfb_dragGC;
                    (*mfb_drawghost)(mfb_lastX,mfb_lastY,mfb_bandX,
                        mfb_bandY);
                    mfb_lineGC = tmpGC;
                }
                if (newEvent.xbutton.x >= mfb_dwgwinX &&
                    newEvent.xbutton.x <= mfb_dwgwinXmax &&
                    newEvent.xbutton.y >= mfb_dwgwinY &&
                    newEvent.xbutton.y <= mfb_dwgwinYmax) {
                    mfb_lastX = newEvent.xbutton.x;
                    mfb_lastY = newEvent.xbutton.y;
                    mfb_inDwgwin = True;
                }
                else
                    mfb_inDwgwin = False;
                mfb_actionTime = newEvent.xbutton.time;
                mybutton = newEvent.xbutton.button;

                if (newEvent.xbutton.state & ShiftMask)
                    mybutton += 3;
                /*
                if (newEvent.xbutton.state & ControlMask)
                    mybutton += 6;
                */
                if (mybutton == 4) mybutton = 3;
                else if (mybutton == 3) mybutton = 4;

                *X = newEvent.xbutton.x;
                *Y = TRAN(newEvent.xbutton.y);
                *key = 0;
                *button = mfb_buttonMask[mybutton - 1];
                done = 1;
                break;
            case KeyPress:
                nbytes = XLookupString((XKeyEvent *)&newEvent, text, 20,
                    &mykey,(XComposeStatus *) 0);
                if (getchar_flag) {
                    if (nbytes <= 0)
                        *key = mykey;
                    else
                        *key = text[0];
                    if (fixup(key)) break;
                    *button = -1;
                    done = 1;
                    break;
                }
                if (nbytes <= 0)
                    break;
                if (mfb_drawghost) {
                    /* undraw any rubber-banding */
                    tmpGC = mfb_lineGC;
                    mfb_lineGC = mfb_dragGC;
                    (*mfb_drawghost)(mfb_lastX,mfb_lastY,mfb_bandX,
                        mfb_bandY);
                    mfb_lineGC = tmpGC;
                }
                if (newEvent.xkey.x >= mfb_dwgwinX &&
                    newEvent.xkey.x <= mfb_dwgwinXmax &&
                    newEvent.xkey.y >= mfb_dwgwinY &&
                    newEvent.xkey.y <= mfb_dwgwinYmax) {
                    mfb_lastX = newEvent.xkey.x;
                    mfb_lastY = newEvent.xkey.y;
                    mfb_inDwgwin = True;
                }
                else
                    mfb_inDwgwin = False;
                mfb_actionTime = newEvent.xkey.time;
                *key = text[0];
                *button = -1;
                done = 1;
                break;
            case MotionNotify:
                /* draw rubber-band line or rectangle */
                while (XEventsQueued(newEvent.xmotion.display,
                    QueuedAfterReading) > 0) {
                    XPeekEvent(newEvent.xmotion.display, &nextEvent);
                    if (nextEvent.type != MotionNotify)
                        break;
                    XNextEvent(newEvent.xmotion.display, &newEvent);
                }
                if (mfb_fullScreenCursor)
                    draw_full_screen_cursor();  /* undraw full-screen cursor */
                if (mfb_drawghost) {
                    /* undraw any rubber-banding */
                    tmpGC = mfb_lineGC;
                    mfb_lineGC = mfb_dragGC;
                    (*mfb_drawghost)(mfb_lastX,mfb_lastY,mfb_bandX,
                        mfb_bandY);
                    mfb_lineGC = tmpGC;
                }
                mfb_lastX = mfb_dwgwinX + newEvent.xmotion.x;
                mfb_lastY = mfb_dwgwinY + newEvent.xmotion.y;
                mfb_inDwgwin = True;
                if (mfb_drawghost) {
                    /* redraw any rubber-banding */
                    tmpGC = mfb_lineGC;
                    mfb_lineGC = mfb_dragGC;
                    (*mfb_drawghost)(mfb_lastX,mfb_lastY,mfb_bandX,
                        mfb_bandY);
                    mfb_lineGC = tmpGC;
                }
                if (mfb_fullScreenCursor)
                    draw_full_screen_cursor();  /* redraw full-screen cursor */
                break;
            case EnterNotify:
                /* pointer entered the drawing window */
                if (newEvent.xcrossing.mode != NotifyNormal)
                    break;
                if (mfb_drawghost) {
                    /* undraw any rubber-banding */
                    tmpGC = mfb_lineGC;
                    mfb_lineGC = mfb_dragGC;
                    (*mfb_drawghost)(mfb_lastX,mfb_lastY,mfb_bandX,
                        mfb_bandY);
                    mfb_lineGC = tmpGC;
                }
                mfb_lastX = mfb_dwgwinX + newEvent.xcrossing.x;
                mfb_lastY = mfb_dwgwinY + newEvent.xcrossing.y;
                mfb_inDwgwin = True;
                if (mfb_drawghost) {
                    /* redraw any rubber-banding */
                    tmpGC = mfb_lineGC;
                    mfb_lineGC = mfb_dragGC;
                    (*mfb_drawghost)(mfb_lastX,mfb_lastY,mfb_bandX,
                        mfb_bandY);
                    mfb_lineGC = tmpGC;
                }
                if (mfb_fullScreenCursor)
                    draw_full_screen_cursor();  /* redraw full-screen cursor */
                break;
            case LeaveNotify:
                /* pointer left the drawing window */
                if (newEvent.xcrossing.mode != NotifyNormal)
                    break;
                if (mfb_fullScreenCursor)
                    draw_full_screen_cursor();  /* undraw full-screen cursor */
                mfb_inDwgwin = False;
                break;
            case ConfigureNotify:
                /* save width and height on window re-size */
                if (newEvent.xconfigure.window == mfb_window) {
                    mfb_maxX = newEvent.xconfigure.width;
                    mfb_maxY = newEvent.xconfigure.height;
                }
                break;
            case VisibilityNotify:
                /* save visibility state */
                if (newEvent.xvisibility.window == mfb_window) {
                    mfb_visState = newEvent.xvisibility.state;
                    printf("VisibilityState = %d\n", mfb_visState);
                }
                break;
            case Expose:
                /* repaint window on expose events */
                if (newEvent.xexpose.count == 0 &&
                    newEvent.xexpose.window == mfb_window &&
                    mfb_visState == VisibilityUnobscured) {
                    XClearWindow(newEvent.xexpose.display,
                        newEvent.xexpose.window);
                    RepaintWindow(0);
                    /* Eat any other expose events for
                     * this window (Motif hack)
                     */
                    while (XCheckWindowEvent(mfb_display, mfb_window,
                        (long) ExposureMask, &newEvent)) ;
                }
                if (mfb_drawghost) {
                    /* draw any rubber-banding */
                    tmpGC = mfb_lineGC;
                    mfb_lineGC = mfb_dragGC;
                    (*mfb_drawghost)(mfb_lastX,mfb_lastY,
                        mfb_bandX,mfb_bandY);
                    mfb_lineGC = tmpGC;
                }
                if (mfb_fullScreenCursor)
                    draw_full_screen_cursor();

                break;
            default:
                /* No action.  All other events are ignored */
                break;
            }
        }
#ifdef MFBFRAMES
        /* Now look for exposure events in other windows */
        for (stk = 1, f = Frames->f_next; f; stk++, f = f->f_next) {
            if (XPending(f->f_mfb->display)) {
                MFBCurrent = f->f_mfb;
                while (XPending(mfb_display)) {
                    XNextEvent(mfb_display,&newEvent);
                    if (newEvent.type == ConfigureNotify) {
                        mfb_maxX = newEvent.xconfigure.width;
                        mfb_maxY = newEvent.xconfigure.height;
                    }
                    else if (newEvent.type == Expose) {
                        if (newEvent.xexpose.count == 0 &&
                            newEvent.xexpose.window == mfb_window &&
                            mfb_visState == VisibilityUnobscured) {

                            XClearWindow(newEvent.xexpose.display,
                                newEvent.xexpose.window);
                            RepaintWindow(stk);
                            /* Eat any other expose events for
                             * this window (Motif hack)
                             */
                            while (XCheckWindowEvent(mfb_display, mfb_window,
                                (long) ExposureMask, &newEvent)) ;
                        }
                    }    
                }    
            }    
        }
        MFBCurrent = Frames->f_mfb;
        /* The -1 argument specifies a reinitialization to the top
         * level frame of any parameters set by the application, without
         * repainting anything.
         */
        RepaintWindow(-1);
#endif
    }

    if (mfb_fullScreenCursor)
        draw_full_screen_cursor(); /* undraw the full-screen cursor */
    return MFBOK;
}


static int
fixup(k)

int *k;
{
    if      (*k == 0xff51) *k = 331;
    else if (*k == 0xff52) *k = 328;
    else if (*k == 0xff53) *k = 333;
    else if (*k == 0xff54) *k = 336;
    if (*k >= 0xff00) return true;
    return false;
}


#ifdef MFBFRAMES

void
MFBNewContext()

/* This is a hack.  After forking, we can't have two processes
 * sharing the same display(s), as events get "lost".  We also
 * can't close the display in one process, as this destroys
 * resources in the X server which causes errors and core
 * dumps.  The solution is to call this routine, which simply
 * causes open windows to be ignored in the process from which
 * this is called.
 * The process MUST EXIT before any references to previously
 * open displays/windows are encountered.
 */
{
    Frames = NULL;
    MFBCurrent = NULL;
}

#endif


int
MFBGetchar()

/*
 * Returns a single character from the graphics input device.
 *
 * This routine is implemented by calling MFBPoint and ignoring
 * any returned button events, permitting the handling of all
 * X-windows events to be coded in one place.
 */
{
    int  x, y, button;
    int key;
    Bool fullScreenCursorSave;

    getchar_flag = true;
    fullScreenCursorSave = mfb_fullScreenCursor;
    MFBSetFullScreenCursor(false);
    for (;;) {        /* Infinite loop */
        MFBPoint(&x, &y, &key, &button);
        if (button == -1) {
            MFBSetFullScreenCursor(fullScreenCursorSave);
            break;
        }
    }
    getchar_flag = false;
    return key;
}


char *
MFBKeyboard(X, Y, background, foreground)

int X, Y;                        /* Position of input */
int background, foreground;      /* Foreground and background colors */
/*
 * Enables the graphics keyboard and waits for user input.
 * Returns a pointer to a null terminated string of user input.  This
 * space is static - old input is erased each time MFBKeyboard is called.
 * The routine echoes characters and does character and line editing.
 * ^H and DEL are considered the erase characters and ^U and ^X are
 * considered the kill characters.  Carriage return terminates input.
 *
 * This routine is implemented by calling MFBPoint and ignoring
 * any returned button events, permitting the handling of all
 * X-windows events to be coded in one place.
 */
{
    static char InputBuffer[200];
    int key;
    int curX, curY, curLen, ytop, width, x, y, button;
    int cursorwidth;
    Bool fullScreenCursorSave;

    fullScreenCursorSave = mfb_fullScreenCursor;
    MFBSetFullScreenCursor(false);
    X += mfb_fontXOffset;
    Y = TRAN(Y) - mfb_fontYOffset;
    curX = X;
    curY = Y;
    ytop = curY - mfb_font->ascent;
    curLen = 0;
    cursorwidth = XTextWidth(mfb_font, "X", 1);
    XFillRectangle(mfb_display, mfb_window, mfb_dragGC,
                        curX, ytop, cursorwidth, mfb_fontHeight);
    for (;;) {        /* Infinite loop */
        MFBPoint(&x, &y, &key, &button);
        if (button == -1) {
            XFillRectangle(mfb_display, mfb_window, mfb_dragGC,
                        curX, ytop, cursorwidth, mfb_fontHeight);
            if (key == CR || key == LF) {
                InputBuffer[curLen] = '\0';
                MFBSetFullScreenCursor(fullScreenCursorSave);
                return InputBuffer;
            } 
            else if ((key == BS || key == DEL) && curLen > 0) {
                /* Delete a character */
                width = XTextWidth(mfb_font, &InputBuffer[--curLen], 1);
                curX -= width;
                XClearArea(mfb_display, mfb_window, curX, ytop,
                                width, mfb_fontHeight, False);
            }
            else if ((key == CTRLU || key == CTRLX) && curLen > 0) {
                /* Delete the line */
                width = curX - X;
                curX = X;
                XClearArea(mfb_display, mfb_window, curX, ytop,
                                width, mfb_fontHeight, False);
                curLen = 0;
            }
            else if (key >= ' ' && key < DEL) {
                /* Add the character to the string and display */
                char c = key;
                InputBuffer[curLen++] = c;
                XDrawImageString(mfb_display, mfb_window,
                                mfb_lineGC, curX, curY, &c, 1);
                curX += XTextWidth(mfb_font, &c, 1);
            }
            XFillRectangle(mfb_display, mfb_window, mfb_dragGC,
                        curX, ytop, cursorwidth, mfb_fontHeight);
        }
    }
}


int
MFBDefineColor(colorID, red, green, blue)

int colorID;
int red, green, blue;
/*
 * Defines the VLT entry 'colorID' to 'red','green', and 'blue'.
 * Color intensities are normalized to 1000.  If the display
 * has color,  this function will work according to spec.  It
 * sets the current color to the newly defined color.
 */
{
    XColor mycolor;

    if (!mfb_vltBool)
        return MFBNODFCO;
    if (colorID > mfb_maxColors || colorID < 0)
        return MFBBADCST;
    if (red > 1000 || green > 1000 || blue > 1000)
        return MFBBADCST;

    mycolor.red   = (red   * 65535) / 1000;
    mycolor.green = (green * 65535) / 1000;
    mycolor.blue  = (blue  * 65535) / 1000;
    if (MFBCurrent->privateColors) {
        mycolor.flags = -1;
        mycolor.pixel = mfb_colors[colorID];
        XStoreColor(mfb_display, mfb_cmap, &mycolor);
    }
    else {
        if (mfb_colors[colorID] > -1)
            XFreeColors(mfb_display, mfb_cmap, &mfb_colors[colorID], 1, 0);
        if (!XAllocColor(mfb_display, mfb_cmap, &mycolor))
            return MFBBADCST;
        mfb_colors[colorID] = mycolor.pixel;
    }
    if (mfb_lineGC == mfb_dragGC)
        return (MFBOK);
    MFBSetColor(colorID);
    if (colorID == 0) {
        XSetWindowBackground(mfb_display, mfb_window, mycolor.pixel);
        XSetBackground(mfb_display, mfb_lineGC, mycolor.pixel);
        XSetBackground(mfb_display, mfb_fillGC, mycolor.pixel);
        XSetBackground(mfb_display, mfb_dragGC, mycolor.pixel);
        XSetForeground(mfb_display, mfb_dragGC,
            mycolor.pixel ^ WhitePixel(mfb_display,DefaultScreen(mfb_display)));
    }
    return MFBOK;
}


int
MFBSetColor(colorID)

int colorID;
/*
 * Sets the current foreground color to 'colorID'.  Must be
 * between 0 and the maximum number of colors for the device.
 */
{
    if (colorID >= mfb_maxColors || colorID < 0)
        return MFBBADCST;

    if (mfb_lineGC != mfb_dragGC) {
        XSetForeground(mfb_display, mfb_lineGC, mfb_colors[colorID]);
        XSetForeground(mfb_display, mfb_fillGC, mfb_colors[colorID]);
        mfb_fgColorId = colorID;
    }
    return MFBOK;
}


int
MFBSetCursorColor(colorID1,colorID2)

int colorID1,colorID2;
/*
 * Sets up cursor to blink between colorID1 and colorID2.
 *
 * Cursor blinking is not supported in X, so we'll just
 * set the cursor foreground color to colorID1 and its
 * background color to 0, ignoring colorID2 (which is
 * always identical to colorID1 when called from KIC).
 */
{
    XColor myforeg, mybackg;

    if (colorID1 >= mfb_maxColors || colorID1 < 0)
        return MFBBADCST;
    if (colorID1 == 0 || (int)mfb_colors[colorID1] < 0)
        colorID1 = 1;
    myforeg.pixel = mfb_colors[colorID1];
    mybackg.pixel = mfb_colors[0];
    XQueryColor(mfb_display, mfb_cmap, &myforeg);
    XQueryColor(mfb_display, mfb_cmap, &mybackg);
    XRecolorCursor(mfb_display, mfb_cursor, &myforeg, &mybackg);
    return MFBOK;
}


int
MFBDefineFillPattern(styleID,BitArray)

int styleID;
int *BitArray;
/*
 * Redefines fill pattern 'styleID' to that specified by
 * 'BitArray'.  'BitArray' is a pointer to an array of
 * eight integers of which the lower 8-bits are significant.
 * Fill pattern zero is always solid and may not be redefined.
 * The routine sets the current fill pattern to the one
 * defined.
 */
{
    int i, j, index;
    char pattern[8];

    if (styleID > mfb_maxFillPatterns || styleID <= 0)
        return MFBBADFST;

    for (i = 0, j = 7;  i < 8; ++i, --j)
        pattern[j] = BitArray[i] & 0xFF;

    index = styleID - 1;
    if (mfb_fillMap[index] != None)
        XFreePixmap(mfb_display, mfb_fillMap[index]);
    mfb_fillMap[index] = XCreateBitmapFromData(mfb_display,
        mfb_window, pattern, 8, 8);
    MFBSetFillPattern(styleID);
    return MFBOK;
}


int
MFBSetFillPattern(styleID)

int styleID;
/*
 * Sets the fill pattern to the one specified by 'styleID'.  With
 * X,  definable fill patterns are fully supported.  The maximum
 * number of patterns is (arbitrarily) MAXFPATT.
 */
{
    if (styleID > mfb_maxFillPatterns || styleID < 0)
        return MFBBADFST;

    if (styleID > 0 && mfb_fillMap[styleID - 1] != None) {
        XSetFillStyle(mfb_display, mfb_fillGC, FillStippled);
        XSetStipple(mfb_display, mfb_fillGC, mfb_fillMap[styleID - 1]);
    }
    else
        XSetFillStyle(mfb_display, mfb_fillGC, FillSolid);
    mfb_fillPattern = styleID;

    return MFBOK;
}


int
MFBDefineLineStyle(styleID,mask)

int styleID;
int mask;
/*
 * Defines line style 'styleID' to be the lowest eight bits
 * of 'mask'.  This must be converted into an X11 dash_list,
 * which is stored in the lStyles array of the MFB structure.
 */
{
    int i, j, n, index, shift, thisbit, lastbit, offset, count[9];

    if (styleID > mfb_maxLineStyles || styleID <= 0)
        return MFBBADLST;
    mask &= 0xFF;
    count[0] = n = i = 0;
    lastbit = mask & 1;
    for (shift = 0; shift < 8; ++shift) {
        thisbit = mask & 1;
        if (thisbit == lastbit)
            count[n] += (thisbit) ? 2 : -2;
        else
            count[++n] = (thisbit) ? 2 : -2;
        lastbit = thisbit;
        mask >>= 1;
    }
    if (n & 1) {
        ++n;
        offset = 0;
        if (count[0] < 0) {
            count[n++] = count[i++];
            for (j = 1; j < n; ++j)
                offset += MFBabs(count[j]);
        }
    }
    else {
        if (count[0] > 0) {
            offset = count[n];
            count[0] += count[n];
        }
        else {
            offset = 0;
            for (j = 0; j < n; ++j)
                offset += MFBabs(count[j]);
            count[n] += count[i++];
        }
    }
    index = styleID - 1;
    mfb_lStyles[index][0] = offset;  /* set dash_offset */
    mfb_lStyles[index][1] = n - i;   /* set count */
    for (j = 2; i < n; ++i, ++j)
        mfb_lStyles[index][j] = MFBabs(count[i]);
    MFBSetLineStyle(styleID);
    return MFBOK;
}


int
MFBSetLineStyle(styleID)

int styleID;
/*
 * Sets the current line style to 'styleID'.  Implemented
 * here by setting the appropriate field in the
 * current mfb status record defined above.  Now fully
 * implemented.
 */
{
    if (styleID > mfb_maxLineStyles || styleID < 0)
        return MFBBADLST;

    if (styleID > 0 && mfb_lStyles[styleID-1][1] > 0) {
        int index = styleID - 1;
        XSetLineAttributes(mfb_display, mfb_lineGC,
            0, LineOnOffDash, CapButt, JoinMiter);
        XSetDashes(mfb_display, mfb_lineGC,
            (int) mfb_lStyles[index][0],        /* dash_offset*/
                 &mfb_lStyles[index][2],        /* dash_list */
            (int) mfb_lStyles[index][1]);       /* n */
    }
    else
        XSetLineAttributes(mfb_display, mfb_lineGC,
            0, LineSolid, CapButt, JoinMiter);
    mfb_lineStyle = styleID;
    return MFBOK;
}


int
MFBSetTextMode(destructiveBool)

Bool destructiveBool;
/*
 * Sets the current text mode flag in the MFB structure.  If destructive,
 * text will replace what is under it according to the current
 * ALU mode.  Otherwise,  it overwrites using OR.
 */
{
    mfb_textMode = (int) destructiveBool;
    return MFBOK;
}


int
MFBSetChannelMask(channelMask)

int channelMask;
/*
 * This sets the current plane mask for writing.  It is not supported
 * under this version of XMFB.
 */
{
    return MFBNOMASK;
}


int
MFBSetReadMask(readmask)

int readmask;
/*
 * Sets the current plane mask for reading.  Not supported.
 */
{
    return MFBNOMASK;
}


void
MFBPixel(X,Y)

int X,Y;
/*
 * Sets the pixel at X,Y to the current color.
 */
{
    XDrawPoint(mfb_display, mfb_window, mfb_fillGC, X, TRAN(Y));
}


void
MFBCircle(X, Y, rad, nsides)

int X, Y, rad, nsides;
/*
 * Draw the outline of a circle.
 */
{
    XDrawArc(mfb_display, mfb_window, mfb_lineGC,
        X-rad, TRAN(Y)-rad, rad, rad, 0, 23040);
}


void
MFBFlash(X, Y, rad, nsides)

int X, Y, rad, nsides;
/*
 * Draw a filled circle.
 */
{
    XFillArc(mfb_display, mfb_window, mfb_fillGC,
        X-rad, TRAN(Y)-rad, rad, rad, 0, 23040);
}


void
MFBArc(X, Y, rad, angle1, angle2, nsides)

int X, Y, rad, angle1, angle2, nsides;
/*
 * Draw the outline of an arc.
 */
{
    XDrawArc(mfb_display, mfb_window, mfb_lineGC,
        X-rad, TRAN(Y)-rad, rad, rad, angle1*64, angle2*64);
}


void
MFBBox(left,bottom,right,top)

int left,bottom,right,top;
/*
 * Displays a box with the given dimensions using the current ALU mode,
 * fill pattern, and color.  Leaves the "pen" at right, top.
 */
{
    XFillRectangle(mfb_display, mfb_window, mfb_fillGC,
        left, TRAN(top), right-left+1, top-bottom+1);
    mfb_X = right;
    mfb_Y = top;
}


void
MFBNaiveBoxFill(left,bottom,right,top)

int left,bottom,right,top;
/*
 * Used to draw filled boxes on devices which do not support
 * filled regions.   Since X supports filled regions directly,
 * this routine is exactly the same as MFBBox.
 */
{
    MFBBox(left, bottom, right, top);
}


void
MFBRasterCopy(X,Y,DX,DY,DestX,DestY)

int X,Y,DX,DY,DestX,DestY;
/*
 * Copies a rectangular region specified by X,Y,DX,DY to
 * DestX DestY.  This is directly implementable under
 * X.  Note the routine uses the current ALU
 * mode to determine the function used for output.  Leaves 
 * the pen at DestX, DestY.
 */
{
    mfb_X = DestX;
    mfb_Y = DestY;
    XCopyArea(mfb_display, mfb_window, mfb_window, mfb_fillGC,
        X, TRAN(Y)-DY, DX, DY, DestX, TRAN(DestY)-DY);
}


void
MFBFlood()

/*
 * Draws a solid box over the entire screen.
 */
{
    if (mfb_fgColorId != 0) {
        XSetWindowBackground(mfb_display, mfb_window,
            mfb_colors[mfb_fgColorId]);
        XClearWindow(mfb_display, mfb_window);
        XSetWindowBackground(mfb_display, mfb_window, mfb_colors[0]);
    }
    else
        XClearWindow(mfb_display, mfb_window);
}


void
MFBLine(X1,Y1,X2,Y2)

int X1,Y1,X2,Y2;
/*
 * Draws a line in the current line style from (X1,Y1) to
 * (X2,Y2).  Leaves the "pen" positioned at (X2, Y2).
 */
{
    XDrawLine(mfb_display, mfb_window, mfb_lineGC,
        X1, TRAN(Y1), X2, TRAN(Y2));
    mfb_X = X2;
    mfb_Y = Y2;
}


void
MFBMoveTo(X1,Y1)

int X1,Y1;
/*
 * Sets the current graphics position to X1,Y1.  Directly
 * supported.
 */
{
    mfb_X = X1;
    mfb_Y = Y1;
}


void
MFBDrawLineTo(X1,Y1)

int X1,Y1;
/*
 * Draws a line from the current graphics position to X1,Y1.
 * Supported using the current position and the MFBLine routine.
 */
{
    MFBLine(mfb_X, mfb_Y, X1, Y1);
    mfb_X = X1;
    mfb_Y = Y1;
}


void
MFBDrawPath(path)

MFBPATH *path;
/*
 * Draws a path of vectors in the current line style and color.  Fully
 * supported under X.
 */
{
    int index;
    XPoint *allPoints;

    allPoints = (XPoint *) malloc( (unsigned) (sizeof(XPoint) *
        (path->nvertices+1)) );
        
    for (index = 0;  index < path->nvertices;  index++) {
        allPoints[index].x = path->xy[index*2];
        allPoints[index].y = TRAN(path->xy[index*2+1]);
    }

    XDrawLines(mfb_display, mfb_window, mfb_lineGC,
        allPoints, index, CoordModeOrigin);

    free( (char *) allPoints );
    mfb_X = path->xy[0];
    mfb_Y = path->xy[1];
}


void
MFBPolygon(poly)

MFBPOLYGON *poly;
/*
 * Draws a filled polygon specified in 'poly'.  The polygon must
 * be translated and copied into the XPoint structure before
 * handing it off to XFillPolygon.
 */
{
    int index;
    XPoint *allPoints;

    allPoints = (XPoint *) malloc( (unsigned) (sizeof(XPoint) *
        (poly->nvertices+1)) );
        
    for (index = 0;  index < poly->nvertices;  index++) {
        allPoints[index].x = poly->xy[index*2];
        allPoints[index].y = TRAN(poly->xy[index*2+1]);
    }

    XFillPolygon(mfb_display, mfb_window, mfb_fillGC,
        allPoints, index, Complex, CoordModeOrigin);

    free( (char *) allPoints );
    mfb_X = poly->xy[0];
    mfb_Y = poly->xy[1];
}


MFBPATH *
MFBArcPath(x,y,r,astart,astop,s)

int x;       /* x coordinate of center */
int y;       /* y coordinate of center */
int r;       /* radius of arc */
int astart;  /* initial angle ( +x axis = 0 degrees ) */
int astop;   /* final angle ( +x axis = 0 degrees ) */
int s;       /* number of segments in a 360 degree arc */
/*
 * Notes:
 *    Returns MFBPATH containing an arc.
 */
{
    static MFBPATH pth;
    static int xy[400];
    int *ip, i, j;
    double d;

    pth.xy = ip = xy;
    while (astart >= astop)
        astop += 360;
    if (s < 2 || s > 180)
        s = 20;
    pth.nvertices = 2;
    j = MFBmax(1, (astop - astart)/s);
    d = astart / RADTODEG;
    *ip++ = x + (int)(r * cos(d));
    *ip++ = y + (int)(r * sin(d));
    for (i = astart + j; i <= astop; i += j) {
        d = i / RADTODEG;
        *ip++ = x + (int)(r * cos(d));
        *ip++ = y + (int)(r * sin(d));
        ++pth.nvertices;
    }
    d = astop / RADTODEG;
    *ip++ = x + (int)(r * cos(d));
    *ip   = y + (int)(r * sin(d));
    return &pth;
}


MFBPOLYGON *
MFBEllipse(x,y,rx,ry,s)

int x;       /* x coordinate of center */
int y;       /* y coordinate of center */
int rx;      /* radius of ellipse in x direction */
int ry;      /* radius of ellipse in y direction */
int s;       /* number of line segments in circle (default = 20) */
/*
 * Notes:
 *    Returns MFBPOLYGON containing an ellipse with no more than 200 coords.
 */
{
    static MFBPOLYGON poly;
    static int xy[400];
    int i=0;
    int j;
    int *ip;
    double d=0;

    poly.xy = ip = xy;
    *ip++ = x + rx;
    *ip++ = y;
    if (s > 2 && s < 181)
        j = 360 / s;
    else {
        j = 18;
        s = 20;
    }
    poly.nvertices = s + 1;
    while (--s) {
        i += j;
        d = i / RADTODEG;
        *ip++ = x + (int)(rx * cos(d));
        *ip++ = y + (int)(ry * sin(d));
    }
    *ip++ = x + rx;
    *ip++ = y;
    return &poly;
}


void
MFBText(text,X,Y,phi)

char *text;
int X,Y,phi;
/*
 * Displays a null terminated string pointed to by 'text' with
 * the lower corner at X,Y rotated 'phi' degrees.  Currently
 * does not support rotation.  The routine adheres to the mode
 * set by MFBSetTextMode.  
 */
{
    char *buf;

    if (text == NULL || strlen(text) == 0)
	return;
    if (phi != 0) {
        fprintf(stderr,
            "Call to MFBText(\"%s\", %d, %d, %d) with non-zero phi\n",
            text, X, Y, phi);
        return;
    }

    /* convert tabs to space */
    buf = malloc(strlen(text)+1);
    if (buf != NULL)
        strcpy(buf,text);
    else
        return;
    /* don't print anything after \n */
    for (text = buf; *text; text++) {
        if (*text == '\t') {*text = ' '; continue;}
        if (*text == '\n') {*text = '\0'; break;}
    }
    text = buf;

    if (mfb_textMode) {
        /* Destructive mode */
        XDrawImageString(mfb_display, mfb_window, mfb_lineGC,
            X + mfb_fontXOffset, TRAN(Y) - mfb_fontYOffset,
            text, strlen(text));
    }
    else {
        /* Non-destructive mode */
        XDrawString(mfb_display, mfb_window, mfb_lineGC,
            X + mfb_fontXOffset, TRAN(Y) - mfb_fontYOffset,
            text, strlen(text));
    }
    free(text);
}


int
MFBSetALUMode(alumode)

int alumode;
/*
 * Changes the way graphics display is changed when an overwrite
 * occurs.  Can be MFBALUJAM,  MFBALUOR,  MFBALUNOR,  MFBALUEOR.
 */
{
    int func;

    switch (alumode) {
    case MFBALUJAM:
        func = GXcopy;
        break;
    case MFBALUOR:
        func = GXor;
        break;
    case MFBALUNOR:
        func = GXnor;
        break;
    case MFBALUEOR:
        func = GXxor;
        break;
    default:
        return MFBBADALU;
        break;
    }
    XSetFunction(mfb_display, mfb_lineGC, func);
    XSetFunction(mfb_display, mfb_fillGC, func);
    mfb_curALUMode = alumode;
    return MFBOK;
}


int
MFBSetGhost(callback,X,Y)

#if __STDC__
void (*callback)(int,int,int,int);
#else
void (*callback)();
#endif
int X,Y;
{
    if (callback) {
        mfb_bandX = X;
        mfb_bandY = Y;
        mfb_drawghost = callback;
        XSelectInput(mfb_display, mfb_dwgwin,
            PointerMotionMask | ButtonMotionMask |
            EnterWindowMask | LeaveWindowMask);
        return (MFBOK);
    }
    else if (!mfb_fullScreenCursor)
        XSelectInput(mfb_display, mfb_dwgwin, 0);
    mfb_drawghost = NULL;
    return (MFBOK);
}


int
MFBSetFullScreenCursor(onFlag)

int onFlag;
{
    mfb_fullScreenCursor = onFlag;
    if (onFlag) {
        XDefineCursor(mfb_display, mfb_dwgwin, mfb_cursorDot);
        XSelectInput(mfb_display, mfb_dwgwin,
            PointerMotionMask | ButtonMotionMask |
            EnterWindowMask | LeaveWindowMask);
    }
    else {
        XUndefineCursor(mfb_display, mfb_dwgwin);
        if (!mfb_drawghost)
            XSelectInput(mfb_display, mfb_dwgwin, 0);
    }
    return MFBOK;
}


static void
draw_full_screen_cursor()

{
    if (mfb_inDwgwin) {
         XDrawLine(mfb_display, mfb_window, mfb_dragGC,
            mfb_dwgwinX, mfb_lastY, mfb_dwgwinXmax, mfb_lastY);
         XDrawLine(mfb_display, mfb_window, mfb_dragGC,
            mfb_lastX, mfb_dwgwinY, mfb_lastX, mfb_dwgwinYmax);
    }
}


void
MFBResizeDrawingWindow(left,bottom,right,top)

int left,bottom,right,top;
{
    mfb_dwgwinX = left;
    mfb_dwgwinY = TRAN(top);
    mfb_dwgwinXmax = right;
    mfb_dwgwinYmax = TRAN(bottom);
    XMoveResizeWindow(mfb_display, mfb_dwgwin,
         mfb_dwgwinX, mfb_dwgwinY, right - left, top - bottom);
}


int
MFBSetBlinker(colorID, red, green, blue, onFlag)

int colorID;
int red, green, blue;
int onFlag;
/*
 * MFBSetBlinker - Enables the color identified by colorID to blink between
 *                 its currently defined color and the color defined
 *                 by red, green, and blue.  If onFlag is zero,  blinking
 *                 is disabled.  Under X,  this feature is not supported.
 */
{
    return MFBNOBLNK;
}


int
MFBPutchar(c)

char c;
/*
 * Places character 'c' on the output buffer.  Under X,  this
 * is equiv. to output to stdout.  NOTE:  This means this
 * type of output will go to the invoking window NOT the
 * MFB window!
 */
{
    putc(c, stdout);
    return (0);
}


int
MFBPutstr(cp,nchars)

char *cp;
int nchars;
{
    while (nchars-- > 0)
        putc(*cp++, stdout);
    return (0);
}


void
MFBAudio()

/*
 * Rings the terminal's bell.  Uses XBell.
 */
{
    XBell(mfb_display, 0);
}


long
MFBInfo(int Info)
{
    /*
     * Notes:
     *    MFBInfo is the only routine for acquiring device
     *    specific information.  The valid arguments to
     *    MFBInfo are defined in mfb.h.  If an invalid argument
     *    is used, MFBInfo returns -1.
     */
    switch(Info) {
    case MAXX:
        return mfb_maxX;
    case MAXY:
        return mfb_maxY;
    case MAXCOLORS:
        return mfb_maxColors;
    case MAXINTENSITY:
        return mfb_maxIntensity;
    case MAXFILLPATTERNS:
        return mfb_maxFillPatterns;
    case MAXLINESTYLES:
        return mfb_maxLineStyles;
    case MAXBLINKERS:
        return mfb_maxBlinkers;
    case POINTINGDEVICE:
        return (int)mfb_pointingDeviceBool;
    case POINTINGBUTTONS:
        return (int)mfb_buttonsBool;
    case NUMBUTTONS:
        return mfb_numberOfButtons;
    case BUTTON1:
        return mfb_buttonMask[0];
    case BUTTON2:
        return mfb_buttonMask[1];
    case BUTTON3:
        return mfb_buttonMask[2];
    case BUTTON4:
        return mfb_buttonMask[3];
    case BUTTON5:
        return mfb_buttonMask[4];
    case BUTTON6:
        return mfb_buttonMask[5];
    case BUTTON7:
        return mfb_buttonMask[6];
    case BUTTON8:
        return mfb_buttonMask[7];
    case BUTTON9:
        return mfb_buttonMask[8];
    case BUTTON10:
        return mfb_buttonMask[9];
    case BUTTON11:
        return mfb_buttonMask[10];
    case BUTTON12:
        return mfb_buttonMask[11];
    case TEXTPOSITIONABLE:
        return (int)mfb_textPositionableBool;
    case TEXTROTATABLE:
        return (int)mfb_textRotateBool;
    case FONTHEIGHT:
        return mfb_fontHeight;
    case FONTWIDTH:
        return mfb_fontWidth;
    case FONTXOFFSET:
        return 0;
/*      return mfb_fontXOffset;*/
    case FONTYOFFSET:
        return 0;
/*      return mfb_fontYOffset;*/
    case DESTRUCTIVETEXT:
        return (int)mfb_replaceTextBool;
    case OVERSTRIKETEXT:
        return (int)mfb_overstrikeTextBool;
    case VLT:
        return (int)mfb_vltBool;
    case BLINKERS:
        return (int)mfb_blinkersBool;
    case FILLEDPOLYGONS:
        return (int)mfb_filledPlygnBool;
    case DEFFILLPATTERNS:
        return (int)mfb_fillPtrnDefineBool;
    case DEFREADMASK:
        return (int)mfb_readMaskBool;
    case DEFCHANNELMASK:
        return (int)mfb_channelMaskBool;
    case DEFLINEPATTERN:
        return (int)mfb_linePatternDefineBool;
    case CURFGCOLOR:
        return mfb_fgColorId;
    case CURFILLPATTERN:
        return mfb_fillPattern;
    case CURLINESTYLE:
        return mfb_lineStyle;
    case CURCHANNELMASK:
        return mfb_channelMask;
    case CURREADMASK:
        return mfb_readMask;
    case NUMBITPLANES:
        return mfb_lengthOfVLT;
    case RASTERCOPY:
        return (int)mfb_rastCopyBool;
    case OFFSCREENX:
        return mfb_minOffScreenX;
    case OFFSCREENY:
        return mfb_minOffScreenY;
    case OFFSCREENDX:
        return mfb_offScreenDX;
    case OFFSCREENDY:
        return mfb_offScreenDY;
    case CURFONTSIZE:
        return mfb_fontWidth;
    case HORTPIXPERINCH:
        return mfb_hPixelsPerInch;
    case VERTPIXPERINCH:
        return mfb_vPixelsPerInch;
    case ACTIONTIME:
        return mfb_actionTime;
    case FONTNAME:
        return (long)mfb_fontName;
    case CURSORSHAPE:
        return mfb_cursorShape;
    case FULLSCREENCURSOR:
        return mfb_fullScreenCursor;
    default:
        return -1;
    }
}


/* Some new get and set routines */


char *
MFBGetStorage()

/*
 * Returns the user hook information in the MFB record
 */
{
    return mfb_memoryBlock;
}


int
MFBSetStorage(ptr)

char *ptr;
/*
 * Sets the user hook information in the MFB record
 */
{
    mfb_memoryBlock = ptr;
    return (0);
}


/* Very simple error handler: */


int
MFBXError(dsp, err)

Display *dsp;
XErrorEvent *err;
{
    char errmsg[80];

    XGetErrorText(dsp, err->error_code, errmsg, 80);
    fprintf(stderr, "\
XError: %s\n\
  Serial number = %ld\n\
  Major op-code = %d\n\
  Minor op-code = %d\n\7\7\7",
                errmsg, err->serial,
                (int) err->request_code, (int) err->minor_code);
    return 0;
}

/***********************************************************************
 *
 *                            WINDOW CONTROL
 *
 ***********************************************************************/

#define WNDW        mfb_currentWindow
#define VWPRT       mfb_currentViewport


int
MFBScaleX(X)

int X;
{
    int XX;
    XX = (int)(((double)(X-WNDW.left)*VWPRT.length)/WNDW.length) + VWPRT.left;
    if (XX < VWPRT.left) XX = VWPRT.left;
    else if (XX > VWPRT.right) XX = VWPRT.right; 
    return XX;
}


int
MFBScaleY(Y)

int Y;
{
    int YY;
    YY = (int)(((double)(Y-WNDW.bottom)*VWPRT.width)/WNDW.width) + VWPRT.bottom;
    if(YY < VWPRT.bottom) YY = VWPRT.bottom;
    else if(YY > VWPRT.top) YY = VWPRT.top;
    return YY;
}


int
MFBDescaleX(X)

int X;
{
    int XX;
    XX = ((X-VWPRT.left)*WNDW.length)/VWPRT.length+WNDW.left;
    return XX;
}


int
MFBDescaleY(Y)

int Y;
{
    int YY;
    YY = ((Y-VWPRT.bottom)*WNDW.width)/VWPRT.width+WNDW.bottom;
    return YY;
}


void
MFBSetViewport(left,bottom,right,top)

int left,bottom,right,top;
{
    VWPRT.bottom = bottom;
    VWPRT.top = top;
    VWPRT.left = left;
    VWPRT.right = right;
    VWPRT.length = VWPRT.right - VWPRT.left;
    VWPRT.width = VWPRT.top - VWPRT.bottom;
}


void
MFBSetWindow(left,bottom,right,top)

int left,bottom,right,top;
{
    WNDW.bottom = bottom;
    WNDW.top = top;
    WNDW.left = left;
    WNDW.right = right;
    WNDW.length = WNDW.right - WNDW.left;
    WNDW.width = WNDW.top - WNDW.bottom;
}


void
MFBEraseCursor() {}


void
MFBDrawCursor(x,y)

int x,y;
{
    XWarpPointer(mfb_display,None,mfb_window,0,0,0,0,x+7,TRAN(y)+7);
}


void
MFBPointerInit() {}

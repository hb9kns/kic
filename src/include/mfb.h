/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header: (X support)
 *
 * Copyright -C- 1981 Giles C. Billingsley, Kenneth H. Keller, Brian Lee
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

#include <stdio.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* define this to malloc the main data area */
/*
#define MFBMALLOC
*/

/* define this to enable multiple windows, undefine MFBMALLOC */
/*
#define MFBFRAMES
*/

/* MFBInfo defines */
#define DEVICETYPE        0        /* type of device (TTY or HCOPY) */
#define MAXX              1        /* max x coordinate */
#define MAXY              2        /* max y coordinate */
#define MAXCOLORS         3        /* max number of colors */
#define MAXINTENSITY      4        /* max color intensity */
#define MAXFILLPATTERNS   5        /* max number of fill patterns */
#define MAXLINESTYLES     6        /* max number of line styles */
#define MAXBLINKERS       7        /* max number of blinkers */
#define POINTINGDEVICE    8        /* Bool: terminal has pointing device */
#define POINTINGBUTTONS   9        /* Bool: pointing device has buttons */
#define NUMBUTTONS       10        /* number of pointing device buttons */
#define BUTTON1          11        /* button value returned by button 1 */
#define BUTTON2          12        /* button value returned by button 2 */
#define BUTTON3          13        /* button value returned by button 3 */
#define BUTTON4          14        /* button value returned by button 4 */
#define BUTTON5          15        /* button value returned by button 5 */
#define BUTTON6          16        /* button value returned by button 6 */
#define BUTTON7          17        /* button value returned by button 7 */
#define BUTTON8          18        /* button value returned by button 8 */
#define BUTTON9          19        /* button value returned by button 9 */
#define BUTTON10         20        /* button value returned by button 10 */
#define BUTTON11         21        /* button value returned by button 11 */
#define BUTTON12         22        /* button value returned by button 12 */
#define TEXTPOSITIONABLE 30        /* Bool: accurately positionable text */
#define TEXTROTATABLE    31        /* Bool: rotateable text */
#define FONTHEIGHT       32        /* font height in pixels */
#define FONTWIDTH        33        /* font width in pixels */
#define FONTXOFFSET      34         /* font x offset in pixels */
#define FONTYOFFSET      35        /* font y offset in pixels */
#define DESTRUCTIVETEXT  36        /* Bool: text can be destructive */
#define OVERSTRIKETEXT   37        /* Bool: text can be overstrike */
#define VLT              38        /* Bool: definable colors (color table)*/
#define BLINKERS         39        /* Bool: terminal has blinkers */
#define FILLEDPOLYGONS   40        /* Bool: terminal has filled polygons */
#define DEFFILLPATTERNS  41        /* Bool: definable fill patterns */
#define DEFCHANNELMASK   42        /* Bool: definable write mask */
#define DEFLINEPATTERN   43        /* Bool: definable line styles */
#define CURFGCOLOR       44        /* current foreground color */
#define CURFILLPATTERN   45        /* current fill pattern */
#define CURLINESTYLE     46        /* current line style */
#define CURCHANNELMASK   47        /* current write mask */
#define CURREADMASK      48        /* current read mask */
#define NUMBITPLANES     49        /* number of bit planes */
#define DEFREADMASK      50        /* Bool: definable read mask */
#define RASTERCOPY       51        /* Bool: terminal has raster copy */
#define OFFSCREENX       52        /* left value of off screen memory */
#define OFFSCREENY       53        /* bottom value of off screen memory */
#define OFFSCREENDX      54        /* length of off screen memory */
#define OFFSCREENDY      55        /* width of off screen memory */
#define CURFONTSIZE      56        /* current text font size */
#define HORTPIXPERINCH   57        /* pixels per inch horizontally */
#define VERTPIXPERINCH   58        /* pixels per inch vertically */
#define ACTIONTIME       59        /* time of last user input action */
#define FONTNAME         60        /* pointer to current font name */
#define CURSORSHAPE      61        /* cursor shape index */
#define FULLSCREENCURSOR 62        /* full-screen cursor (true/false) */

extern int MFBInfo();

#define FALSE 0
#define TRUE 1
#ifndef Bool
#define Bool short
#endif
#define false FALSE
#define true TRUE

/*
 * By define DEBUG we enable the MFBCounters routine for measuring bandwidth
 */
/* #define DEBUG */

#define BUFSIZE            4096              /* format string storage */
#ifndef POLYGONBUFSIZE
#define POLYGONBUFSIZE     600               /* storage for polygons */
#endif
#ifndef MAXPOLYGONVERTICES
#define MAXPOLYGONVERTICES 300               /* max number of vertices */
#endif
#define RADTODEG           57.29577951
#define UNINITIALIZED      -1

#define MAXLSTYLE          32                /* max number of line styles */
#define MAXFPATT           32                /* max number of fill patterns*/

#define MFBabs(a) (a >= 0 ? a : -(a))
#define MFBmax(a,b) (a > b ? a : b)
#define MFBmin(a,b) (a < b ? a : b)
#define MFBSwapInt(f1,f2) {int f3; f3 = f1; f1 = f2; f2 = f3;}
#define MFBRound(x) ( (x)-(int)(x) >= .5 ? (int)(x)+1 : (int)(x) )

#define TTY   't'
#define HCOPY 'r'

/* Character control constants */

#define CR        015
#define LF        012
#define BS        010
#define DEL       0177
#define CTRLD     004
#define CTRLU     025
#define CTRLX     030


struct mfbwindow {
    int left;
    int right;
    int top;
    int bottom;
    double length,width;
};
typedef struct mfbwindow MFBWINDOW;
typedef struct mfbwindow VIEWPORT;


struct mfbpath {
    int nvertices;
    int *xy;
};
typedef struct mfbpath MFBPOLYGON;
typedef struct mfbpath MFBPATH;


struct mfb {
        /* Pointers */

        char *name;                     /* display name */
        char *memoryBlock;              /* mem for special drivers */
        int fileDesc;                        /* for compatibility */

        /* Numerics */

        int X,Y;                        /* saved coords for move/draw */
        int maxX;                       /* Horizontal resolution */
        int maxY;                       /* Vertical resolution */
        int hPixelsPerInch;             /* # pixels per horizontal inch */
        int vPixelsPerInch;             /* # pixels per vertical inch */
        int maxColors;                  /* maximum number of colors */
        int minOffScreenX;              /* left of off screen memory */
        int minOffScreenY;              /* bottom of off screen mem. */
        int offScreenDX;                /* length of off screen mem. */
        int offScreenDY;                /* width of off screen mem. */
        int maxIntensity;               /* max RGB or LS intensity */
        int lengthOfVLT;                /* number of bit planes */
        int keyboardYOffset;
        int keyboardXOffset;
        int lineDefineLength;           /* number of bytes in array */
        int maxLineStyles;              /* number of line styles */
        int fontHeight;                 /* font height in pixels */
        int fontWidth;                  /* font width in pixels */
        int fontXOffset;
        int fontYOffset;
        int fontSize;
        int maxBlinkers;                /* number of blinkers */
        int fillDefineHeight;           /* number of byte rows */
        int fillDefineWidth;            /* number of byte columns */
        int maxFillPatterns;            /* number of fill patterns */
        int cursorColor1Id;             /* blinked cursor color ID */
        int cursorColor2Id;             /* unblinked cursor color ID */
        int fgColorId;                  /* cur. foreground color ID */
        int fillPattern;                /* cur. fill pattern ID */
        int lineStyle;                  /* cur. line style ID */
        int channelMask;                /* cur. write mask */
        int readMask;                   /* cur. read mask */
        int textMode;                   /* text mode (1=dest, 0=rep) */
        int numBlinkers;                /* cur. number of blinkers */
        int stipplePattern[8];          /* cur. stipple pattern */
        int numberOfButtons;            /* 12 maximum */
        int buttonMask[12];             /* returned button masks */
#ifdef DEBUG
        int nChars;
        int nBoxes,sumBoxArea;
        int nLines,sumLineLength;
#endif

        /* Boolean Fields */

        Bool initializedBool;
        Bool vltBool;
        Bool vltUseHLSBool;
        Bool channelMaskBool;
        Bool readMaskBool;
        Bool pointingDeviceBool;
        Bool buttonsBool;
        Bool readImmediateBool;
        Bool keyboardBool;
        Bool linePatternDefineBool;
        Bool reissueLineStyleBool;
        Bool filledPlygnBool;
        Bool textPositionableBool;
        Bool textRotateBool;
        Bool replaceTextBool;
        Bool overstrikeTextBool; 
        Bool blinkersBool;
        Bool rastCopyBool;
        Bool rastRSCSFBool;
        Bool fillPtrnDefineBool;
        Bool fillDefineRowMajorBool;

        Bool privateColors;

        MFBWINDOW currentWindow;        /* current window */
        VIEWPORT  currentViewport;      /* current viewport */

        /* CHARACTERS */

        char deviceType;                /* TTY=tty, HCOPY=hard copy */

        /* X-WINDOWS structures, resource IDs, and other data */
        Display     *display;           /* X display */
        Window      window;             /* X window */
        GC          lineGC;             /* X GC for drawing lines */
        GC          fillGC;             /* X GC for filled areas */
        GC          dragGC;             /* X GC for rubber banding */
        Colormap    cmap;               /* X color map */
        Cursor      cursor;             /* X cursor (current) */
        Cursor      cursorCross;        /* X cursor (cross) */
        Cursor      cursorDot;          /* X cursor (dot) */
        XImage      *cursorImage;       /* X cursor image (cross) */
        int         cursorShape;        /* X cursor shape index */
        XFontStruct *font;              /* X text font */
        unsigned short *chartab;        /* character table */
        int         charwidth;          /* character width in table */
        int         charheight;         /* character height in table */
        char        fontName[81];       /* X font name text string */
        unsigned long eventMask;        /* X event mask */
        int         visState;           /* X visibility state */
        Pixmap      fillMap[MAXFPATT];  /* X fill pattern bitmaps */
        char        lStyles[MAXLSTYLE][10]; /* All defined line styles */
        unsigned long *colors;          /* All defined color entries */
        int         curALUMode;         /* Current ALU mode */
        Time        actionTime;         /* Time of last user input action */

        /* Fields for dealing with rubberbanding and the full-screen cursor */
        int lastX,lastY;                /* last cursor position */
        int bandX,bandY;                /* rubber-banding origin */
#if __STDC__                            /* ghost rendering function */
        void (*drawghost)(int,int,int,int);
#else
        void (*drawghost)();
#endif
        Bool fullScreenCursor;          /* full screen cursor active? */
        Bool inDwgwin;                  /* cursor is inside drawing window */
        Window dwgwin;                  /* drawing window */
        int dwgwinX;                    /* drawing window origin X */
        int dwgwinY;                    /* drawing window origin Y */
        int dwgwinXmax;                 /* drawing window maximum X */
        int dwgwinYmax;                 /* drawing window maximum Y */
};
typedef struct mfb MFB;

#ifdef MFBALLOCATE
/* this is called only from mfb.c */
MFB *MFBCurrent;
#if !defined(MFBFRAMES) && !defined(MFBMALLOC)
MFB MFBData;
#endif
#else
extern MFB *MFBCurrent;
#if !defined(MFBFRAMES) && !defined(MFBMALLOC)
extern MFB MFBData;
#endif
#endif

/* Define macros to reference the MFB structure via a pointer or directly */
#if defined(MFBMALLOC) || defined(MFBFRAMES)
#define mfb_name                        MFBCurrent->name
#define mfb_memoryBlock                 MFBCurrent->memoryBlock
#define mfb_X                           MFBCurrent->X
#define mfb_Y                           MFBCurrent->Y
#define mfb_maxX                        MFBCurrent->maxX
#define mfb_maxY                        MFBCurrent->maxY
#define mfb_hPixelsPerInch              MFBCurrent->hPixelsPerInch
#define mfb_vPixelsPerInch              MFBCurrent->vPixelsPerInch
#define mfb_maxColors                   MFBCurrent->maxColors
#define mfb_minOffScreenX               MFBCurrent->minOffScreenX
#define mfb_minOffScreenY               MFBCurrent->minOffScreenY
#define mfb_offScreenDX                 MFBCurrent->offScreenDX
#define mfb_offScreenDY                 MFBCurrent->offScreenDY
#define mfb_maxIntensity                MFBCurrent->maxIntensity
#define mfb_lengthOfVLT                 MFBCurrent->lengthOfVLT
#define mfb_keyboardYOffset             MFBCurrent->keyboardYOffset
#define mfb_keyboardXOffset             MFBCurrent->keyboardXOffset
#define mfb_lineDefineLength            MFBCurrent->lineDefineLength
#define mfb_maxLineStyles               MFBCurrent->maxLineStyles
#define mfb_fontHeight                  MFBCurrent->fontHeight
#define mfb_fontWidth                   MFBCurrent->fontWidth
#define mfb_fontXOffset                 MFBCurrent->fontXOffset
#define mfb_fontYOffset                 MFBCurrent->fontYOffset
#define mfb_fontSize                    MFBCurrent->fontSize
#define mfb_maxBlinkers                 MFBCurrent->maxBlinkers
#define mfb_fillDefineHeight            MFBCurrent->fillDefineHeight
#define mfb_fillDefineWidth             MFBCurrent->fillDefineWidth
#define mfb_maxFillPatterns             MFBCurrent->maxFillPatterns
#define mfb_cursorColor1Id              MFBCurrent->cursorColor1Id
#define mfb_cursorColor2Id              MFBCurrent->cursorColor2Id
#define mfb_fgColorId                   MFBCurrent->fgColorId
#define mfb_fillPattern                 MFBCurrent->fillPattern
#define mfb_lineStyle                   MFBCurrent->lineStyle
#define mfb_channelMask                 MFBCurrent->channelMask
#define mfb_readMask                    MFBCurrent->readMask
#define mfb_textMode                    MFBCurrent->textMode
#define mfb_numBlinkers                 MFBCurrent->numBlinkers
#define mfb_stipplePattern              MFBCurrent->stipplePattern
#define mfb_numberOfButtons             MFBCurrent->numberOfButtons
#define mfb_buttonMask                  MFBCurrent->buttonMask
#ifdef DEBUG
#define mfb_nChars                      MFBCurrent->nChars
#define mfb_nBoxes                      MFBCurrent->nBoxes
#define mfb_sumBoxArea                  MFBCurrent->sumBoxArea
#define mfb_nLines                      MFBCurrent->nLines
#define mfb_sumLineLength               MFBCurrent->sumLineLength
#endif
#define mfb_initializedBool             MFBCurrent->initializedBool
#define mfb_vltBool                     MFBCurrent->vltBool
#define mfb_vltUseHLSBool               MFBCurrent->vltUseHLSBool
#define mfb_channelMaskBool             MFBCurrent->channelMaskBool
#define mfb_readMaskBool                MFBCurrent->readMaskBool
#define mfb_pointingDeviceBool          MFBCurrent->pointingDeviceBool
#define mfb_buttonsBool                 MFBCurrent->buttonsBool
#define mfb_readImmediateBool           MFBCurrent->readImmediateBool
#define mfb_keyboardBool                MFBCurrent->keyboardBool
#define mfb_linePatternDefineBool       MFBCurrent->linePatternDefineBool
#define mfb_reissueLineStyleBool        MFBCurrent->reissueLineStyleBool
#define mfb_filledPlygnBool             MFBCurrent->filledPlygnBool
#define mfb_textPositionableBool        MFBCurrent->textPositionableBool
#define mfb_textRotateBool              MFBCurrent->textRotateBool
#define mfb_replaceTextBool             MFBCurrent->replaceTextBool
#define mfb_overstrikeTextBool          MFBCurrent->overstrikeTextBool
#define mfb_blinkersBool                MFBCurrent->blinkersBool
#define mfb_rastCopyBool                MFBCurrent->rastCopyBool
#define mfb_rastRSCSFBool               MFBCurrent->rastRSCSFBool
#define mfb_fillPtrnDefineBool          MFBCurrent->fillPtrnDefineBool
#define mfb_fillDefineRowMajorBool      MFBCurrent->fillDefineRowMajorBool
#define mfb_currentWindow               MFBCurrent->currentWindow
#define mfb_currentViewport             MFBCurrent->currentViewport
#define mfb_deviceType                  MFBCurrent->deviceType
#define mfb_display                     MFBCurrent->display
#define mfb_window                      MFBCurrent->window
#define mfb_lineGC                      MFBCurrent->lineGC
#define mfb_fillGC                      MFBCurrent->fillGC
#define mfb_dragGC                      MFBCurrent->dragGC
#define mfb_cmap                        MFBCurrent->cmap
#define mfb_cursor                      MFBCurrent->cursor
#define mfb_cursorCross                 MFBCurrent->cursorCross
#define mfb_cursorDot                   MFBCurrent->cursorDot
#define mfb_cursorImage                 MFBCurrent->cursorImage
#define mfb_cursorShape                 MFBCurrent->cursorShape
#define mfb_font                        MFBCurrent->font
#define mfb_chartab                     MFBCurrent->chartab
#define mfb_charwidth                   MFBCurrent->charwidth
#define mfb_charheight                  MFBCurrent->charheight
#define mfb_fontName                    MFBCurrent->fontName
#define mfb_eventMask                   MFBCurrent->eventMask
#define mfb_visState                    MFBCurrent->visState
#define mfb_fillMap                     MFBCurrent->fillMap
#define mfb_lStyles                     MFBCurrent->lStyles
#define mfb_colors                      MFBCurrent->colors
#define mfb_curALUMode                  MFBCurrent->curALUMode
#define mfb_actionTime                  MFBCurrent->actionTime
#define mfb_lastX                       MFBCurrent->lastX
#define mfb_lastY                       MFBCurrent->lastY
#define mfb_bandX                       MFBCurrent->bandX
#define mfb_bandY                       MFBCurrent->bandY
#define mfb_drawghost                   MFBCurrent->drawghost
#define mfb_fullScreenCursor            MFBCurrent->fullScreenCursor
#define mfb_inDwgwin                    MFBCurrent->inDwgwin
#define mfb_dwgwin                      MFBCurrent->dwgwin
#define mfb_dwgwinX                     MFBCurrent->dwgwinX
#define mfb_dwgwinY                     MFBCurrent->dwgwinY
#define mfb_dwgwinXmax                  MFBCurrent->dwgwinXmax
#define mfb_dwgwinYmax                  MFBCurrent->dwgwinYmax
#else
#define mfb_name                        MFBData.name
#define mfb_memoryBlock                 MFBData.memoryBlock
#define mfb_X                           MFBData.X
#define mfb_Y                           MFBData.Y
#define mfb_maxX                        MFBData.maxX
#define mfb_maxY                        MFBData.maxY
#define mfb_hPixelsPerInch              MFBData.hPixelsPerInch
#define mfb_vPixelsPerInch              MFBData.vPixelsPerInch
#define mfb_maxColors                   MFBData.maxColors
#define mfb_minOffScreenX               MFBData.minOffScreenX
#define mfb_minOffScreenY               MFBData.minOffScreenY
#define mfb_offScreenDX                 MFBData.offScreenDX
#define mfb_offScreenDY                 MFBData.offScreenDY
#define mfb_maxIntensity                MFBData.maxIntensity
#define mfb_lengthOfVLT                 MFBData.lengthOfVLT
#define mfb_keyboardYOffset             MFBData.keyboardYOffset
#define mfb_keyboardXOffset             MFBData.keyboardXOffset
#define mfb_lineDefineLength            MFBData.lineDefineLength
#define mfb_maxLineStyles               MFBData.maxLineStyles
#define mfb_fontHeight                  MFBData.fontHeight
#define mfb_fontWidth                   MFBData.fontWidth
#define mfb_fontXOffset                 MFBData.fontXOffset
#define mfb_fontYOffset                 MFBData.fontYOffset
#define mfb_fontSize                    MFBData.fontSize
#define mfb_maxBlinkers                 MFBData.maxBlinkers
#define mfb_fillDefineHeight            MFBData.fillDefineHeight
#define mfb_fillDefineWidth             MFBData.fillDefineWidth
#define mfb_maxFillPatterns             MFBData.maxFillPatterns
#define mfb_cursorColor1Id              MFBData.cursorColor1Id
#define mfb_cursorColor2Id              MFBData.cursorColor2Id
#define mfb_fgColorId                   MFBData.fgColorId
#define mfb_fillPattern                 MFBData.fillPattern
#define mfb_lineStyle                   MFBData.lineStyle
#define mfb_channelMask                 MFBData.channelMask
#define mfb_readMask                    MFBData.readMask
#define mfb_textMode                    MFBData.textMode
#define mfb_numBlinkers                 MFBData.numBlinkers
#define mfb_stipplePattern              MFBData.stipplePattern
#define mfb_numberOfButtons             MFBData.numberOfButtons
#define mfb_buttonMask                  MFBData.buttonMask
#ifdef DEBUG
#define mfb_nChars                      MFBData.nChars
#define mfb_nBoxes                      MFBData.nBoxes
#define mfb_sumBoxArea                  MFBData.sumBoxArea
#define mfb_nLines                      MFBData.nLines
#define mfb_sumLineLength               MFBData.sumLineLength
#endif
#define mfb_initializedBool             MFBData.initializedBool
#define mfb_vltBool                     MFBData.vltBool
#define mfb_vltUseHLSBool               MFBData.vltUseHLSBool
#define mfb_channelMaskBool             MFBData.channelMaskBool
#define mfb_readMaskBool                MFBData.readMaskBool
#define mfb_pointingDeviceBool          MFBData.pointingDeviceBool
#define mfb_buttonsBool                 MFBData.buttonsBool
#define mfb_readImmediateBool           MFBData.readImmediateBool
#define mfb_keyboardBool                MFBData.keyboardBool
#define mfb_linePatternDefineBool       MFBData.linePatternDefineBool
#define mfb_reissueLineStyleBool        MFBData.reissueLineStyleBool
#define mfb_filledPlygnBool             MFBData.filledPlygnBool
#define mfb_textPositionableBool        MFBData.textPositionableBool
#define mfb_textRotateBool              MFBData.textRotateBool
#define mfb_replaceTextBool             MFBData.replaceTextBool
#define mfb_overstrikeTextBool          MFBData.overstrikeTextBool
#define mfb_blinkersBool                MFBData.blinkersBool
#define mfb_rastCopyBool                MFBData.rastCopyBool
#define mfb_rastRSCSFBool               MFBData.rastRSCSFBool
#define mfb_fillPtrnDefineBool          MFBData.fillPtrnDefineBool
#define mfb_fillDefineRowMajorBool      MFBData.fillDefineRowMajorBool
#define mfb_currentWindow               MFBData.currentWindow
#define mfb_currentViewport             MFBData.currentViewport
#define mfb_deviceType                  MFBData.deviceType
#define mfb_display                     MFBData.display
#define mfb_window                      MFBData.window
#define mfb_lineGC                      MFBData.lineGC
#define mfb_fillGC                      MFBData.fillGC
#define mfb_dragGC                      MFBData.dragGC
#define mfb_cmap                        MFBData.cmap
#define mfb_cursor                      MFBData.cursor
#define mfb_cursorCross                 MFBData.cursorCross
#define mfb_cursorDot                   MFBData.cursorDot
#define mfb_cursorImage                 MFBData.cursorImage
#define mfb_cursorShape                 MFBData.cursorShape
#define mfb_font                        MFBData.font
#define mfb_chartab                     MFBData.chartab
#define mfb_charwidth                   MFBData.charwidth
#define mfb_charheight                  MFBData.charheight
#define mfb_fontName                    MFBData.fontName
#define mfb_eventMask                   MFBData.eventMask
#define mfb_visState                    MFBData.visState
#define mfb_fillMap                     MFBData.fillMap
#define mfb_lStyles                     MFBData.lStyles
#define mfb_colors                      MFBData.colors
#define mfb_curALUMode                  MFBData.curALUMode
#define mfb_actionTime                  MFBData.actionTime
#define mfb_lastX                       MFBData.lastX
#define mfb_lastY                       MFBData.lastY
#define mfb_bandX                       MFBData.bandX
#define mfb_bandY                       MFBData.bandY
#define mfb_drawghost                   MFBData.drawghost
#define mfb_fullScreenCursor            MFBData.fullScreenCursor
#define mfb_inDwgwin                    MFBData.inDwgwin
#define mfb_dwgwin                      MFBData.dwgwin
#define mfb_dwgwinX                     MFBData.dwgwinX
#define mfb_dwgwinY                     MFBData.dwgwinY
#define mfb_dwgwinXmax                  MFBData.dwgwinXmax
#define mfb_dwgwinYmax                  MFBData.dwgwinYmax
#endif


/* Define Macros for general program usage. */
#define TRAN(y) (MFBCurrent->maxY - y)
#define ON      1
#define OFF     0


/* MFBSetALUMode defines */
#define MFBALUJAM 0
#define MFBALUAND 1
#define MFBALUOR  2
#define MFBALUEOR 3
#define MFBALUNOR 4


/* Error defines */
#define MFBOK            1           /* Successful return */
#define MFBGENERR        -5          /* General error */
#define MFBBADENT        -10         /* Unknown terminal type */
#define MFBBADMCF        -20         /* Can't open mfbcap file */
#define MFBMCELNG        -30         /* MFBCAP entry too long */
#define MFBBADMCE        -40         /* Bad mfbcap entry */
#define MFBINFMCE        -50         /* Infinite mfbcap entry */
#define MFBBADTTY        -60         /* stdout not in /dev */
#define MFBBADLST        -70         /* Illegal line style */
#define MFBBADFST        -80         /* Illegal fill style */
#define MFBBADCST        -90         /* Illegal color style */
#define MFBBADTM1        -100        /* No destructive text */
#define MFBBADTM2        -110        /* No overstriking text */
#define MFBNODFLP        -120        /* No definable line styles */
#define MFBNODFFP        -130        /* No definable fill styles */
#define MFBNODFCO        -140        /* No definable colors */
#define MFBNOBLNK        -150        /* No blinkers */
#define MFBTMBLNK        -160        /* Too many blinkers */
#define MFBBADDEV        -180        /* Can't open or close device */
#define MFBBADOPT        -190        /* Can't access or set device stat */
#define MFBNOMASK        -170        /* No definable read or write mask */
#define MFBBADWRT        -200        /* Error in write */
#define MFBPNTERR        -210        /* Error in pointing device */
#define MFBNOPTFT        -220        /* No format for pointing device */
#define MFBNOPNT         -230        /* No pointing device */
#define MFBNORBND        -240        /* No Rubberbanding */
#define MFBBADALU        -250        /* Cannot set ALU mode */
#define MFBBADMEM        -260        /* Memory allocation error */

/* number of colors to allocate (default 40) */
extern int NUM_XMFB_COLORS;

/* application supplied routines */
#if __STDC__
extern void SetDisplayWindow(int,int,int*,int*,int*,int*);
extern void RepaintWindow(int);
#else
extern void SetDisplayWindow();
extern void RepaintWindow();
#endif


/* mfb.c */
#if __STDC__
extern MFB  *MFBOpen(char*,char*,int*);
extern int  MFBPutchar(int);
extern int  MFBPutstr(char*,int);
extern int  MFBGetchar(void);
extern int  MFBScaleX(int);
extern int  MFBScaleY(int);
extern int  MFBDescaleX(int);
extern int  MFBDescaleY(int);
extern MFBPOLYGON *MFBEllipse(int,int,int,int,int);
extern MFBPATH *MFBArcPath(int,int,int,int,int,int);
extern void SetCurrentMFB(MFB*);
extern void MFBDrawPath(MFBPATH*);
extern void MFBSetViewport(int,int,int,int);
extern void MFBSetWindow(int,int,int,int);
extern void MFBNaiveBoxFill(int,int,int,int);
extern int  MFBUpdate(void);
extern int  MFBInitialize(void);
extern int  MFBClose(void);
extern int  MFBHalt(void);
extern int  MFBSetLineStyle(int);
extern int  MFBSetFillPattern(int);
extern int  MFBSetChannelMask(int);
extern int  MFBSetReadMask(int);
extern int  MFBSetColor(int);
extern int  MFBSetTextMode(Bool);
extern int  MFBSetCursorColor(int,int);
extern int  MFBSetBlinker(int,int,int,int,int);
extern int  MFBSetALUMode(int);
extern int  MFBSetRubberBanding(int,int,int);
extern int  MFBSetGhost(void(*)(int,int,int,int),int,int);
extern int  MFBSetStorage(char*);
extern char *MFBGetStorage(void);
extern int  MFBDefineColor(int,int,int,int);
extern int  MFBDefineFillPattern(int,int*);
extern int  MFBDefineLineStyle(int,int);
extern int  MFBPoint(int*,int*,int*,int*);
extern void MFBAudio(void);
extern void MFBMoveTo(int,int);
extern void MFBDrawLineTo(int,int);
extern void MFBLine(int,int,int,int);
extern void MFBBox(int,int,int,int);
extern void MFBArc(int,int,int,int,int,int);
extern void MFBCircle(int,int,int,int);
extern void MFBFlash(int,int,int,int);
extern void MFBPolygon(MFBPOLYGON*);
extern void MFBText(char*,int,int,int);
extern void MFBFlood(void);
extern void MFBPixel(int,int);
extern void MFBRasterCopy(int,int,int,int,int,int);
extern char *MFBKeyboard(int,int,int,int);
extern char *MFBError(int);
extern int  MFBSetFullScreenCursor(int);
extern void MFBResizeDrawingWindow(int,int,int,int);
extern int  MFBntox(int, int);
#else
extern MFB  *MFBOpen();
extern int  MFBPutchar();
extern int  MFBPutstr();
extern int  MFBGetchar();
extern int  MFBScaleX();
extern int  MFBScaleY();
extern int  MFBDescaleX();
extern int  MFBDescaleY();
extern MFBPOLYGON *MFBEllipse();
extern MFBPATH *MFBArcPath();
extern void SetCurrentMFB();
extern void MFBDrawPath();
extern void MFBSetViewport();
extern void MFBSetWindow();
extern void MFBNaiveBoxFill();
extern int  MFBUpdate();
extern int  MFBInitialize();
extern int  MFBClose();
extern int  MFBHalt();
extern int  MFBSetLineStyle();
extern int  MFBSetFillPattern();
extern int  MFBSetChannelMask();
extern int  MFBSetReadMask();
extern int  MFBSetColor();
extern int  MFBSetTextMode();
extern int  MFBSetCursorColor();
extern int  MFBSetBlinker();
extern int  MFBSetALUMode();
extern int  MFBSetRubberBanding();
extern int  MFBSetGhost();
extern int  MFBSetStorage();
extern char *MFBGetStorage();
extern int  MFBDefineColor();
extern int  MFBDefineFillPattern();
extern int  MFBDefineLineStyle();
extern int  MFBPoint();
extern void MFBAudio();
extern void MFBMoveTo();
extern void MFBDrawLineTo();
extern void MFBLine();
extern void MFBBox();
extern void MFBArc();
extern void MFBCircle();
extern void MFBFlash();
extern void MFBPolygon();
extern void MFBText();
extern void MFBFlood();
extern void MFBPixel();
extern void MFBRasterCopy();
extern char *MFBKeyboard();
extern char *MFBError();
extern int  MFBSetFullScreenCursor();
extern void MFBResizeDrawingWindow();
extern int  MFBntox();
#endif

/* mfbcsdl.c */
#if __STDC__
extern void MFBBeep(int);
extern void MFBRect(int,int,int,int);
extern void MFBSetName(char*,char*);
extern int  MFBSelectFont(int,int,int,int);
extern int  MFBSetFont(char*);
extern void MFBSelectCursor(int,int,int,int);
extern void MFBSetCursor(int);
#else
extern void MFBBeep();
extern void MFBRect();
extern void MFBSetName();
extern int  MFBSelectFont();
extern int  MFBSetFont();
extern void MFBSelectCursor();
extern void MFBSetCursor();
#endif

/* mfblclip.c */
#if __STDC__
extern void MFB_Y_Intercept(long,long,long,long,long,long*);
extern void MFB_X_Intercept(long,long,long,long,long,long*);
extern Bool MFBLineClip(long*,long*,long*,long*,long,long,long,long);
#else
extern void MFB_Y_Intercept();
extern void MFB_X_Intercept();
extern Bool MFBLineClip();
#endif

/* mfbtext.c */
#if __STDC__
extern int  MFBScrollList(int,int,int,int,char**,int,int,int,int);
extern int  MFBScrollFont(int,int,int,int,XFontStruct*,GC,int,int);
extern void MFBMore(int,int,int,int,FILE*);
extern void MFBScroll(int,int,int,int,FILE*);
#else
extern int  MFBScrollList();
extern int  MFBScrollFont();
extern void MFBMore();
extern void MFBScroll();
#endif

/* newpclip.c */
#if __STDC__
extern void MFBNewPolygonClip(MFBPOLYGON*,long,long,long,long);
extern int  MFBNewPolygon(MFBPOLYGON*);
#else
extern void MFBNewPolygonClip();
extern int  MFBNewPolygon();
#endif

/* text1.c */
#if __STDC__
extern void MFBSetTextClip(int,int,int,int);
extern int  MFBScaledText(char*,int,int,int,int);
extern void MFBTextBB(char*,int*,int*);
#else
extern void MFBSetTextClip();
extern int  MFBScaledText();
extern void MFBTextBB();
#endif

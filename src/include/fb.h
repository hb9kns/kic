/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1994
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
 

struct polygn {
    int nvertices;
    int *xy;
};
typedef struct polygn Poly;


/*
 * Frame Buffer Desc;
 */
struct f {
    char *fDisplay;             /* display name */
    int fInterface;             /* index of graphics interface */
    int *fButtonMask;           /* pointer to array of button masks */
    int fMaxX,fMaxY;            /* raster dimensions of viewport */
    int fFontHeight,fFontWidth; /* standard font size */
    int fNumRows;               /* maximum number of horizontal rows */
    int fMaxIntensity;          /* always set to 255 (normalized intensity) */
    int fMaxP;                  /* maximum pixel intensity */
    int fNumColumns;            /* maximum number of vertical columns */
    int fNumColors;             /* maximum color index */
    int fNumFillPatterns;       /* maximum fill index */
    int fInitialized;           /* true if FB structure is initialized */
    int fNonDestructiveText;    /* true if text does not wipe background */
    int fLastCursorColumn;      /* last cursor column used by ShowPrompt() */
    int fFilledPolygons;        /* true if frame buffer has filled polygons */
    int fDefinableFillPatterns; /* true if fill styles are definable */
    int fButtons;               /* true if pointing device has buttons */
    int fNumButtons;            /* number of pointing device buttons */
    int fBlinkers;              /* true if layers can blink */
};
extern struct f FB;


/*
 * Interface Function Table
 */
struct sGinterface {
#ifdef __STDC__
    int  (*gInfo)(int);
    int  (*gOpen)(char*);
    int  (*gClose)(void);
    int  (*gUpdate)(void);
    void (*gPixel)(int,int);
    void (*gLine)(int,int,int,int);
    void (*gBox)(int,int,int,int);
    void (*gPolygon)(Poly*); 
    int  (*gDefLs)(int,int);
    int  (*gSetLs)(int);
    int  (*gDefFp)(int,int*);
    int  (*gSetFp)(int);
    int  (*gDefColor)(int,int,int,int);
    int  (*gSetColor)(int);
    void (*gText)(char*,int,int,int);
    void (*gScText)(char*,int,int,int,int);

    /* these are defined for the screen driver only */
    void (*gClear)(void);
    int  (*gPoint)(int*, int*, int*, int*);
    void (*gBeep)(int);
    int  (*gSetFullScreenCursor)(int);
    void (*gTextBB)(char*, int*, int*);
    void (*gMore)(int, int, int, int, FILE*);
    void (*gSetName)(char*, char*);
    int  (*gBlink)(int, int, int, int, int);
    int  (*gSelectFont)(int, int, int, int);
    void (*gSelectCursor)(int, int, int, int);
    void (*gSetTextClip)(int, int, int, int);
    void (*gResize)(int, int, int, int);

#else
    int  (*gInfo)();
    int  (*gOpen)();
    int  (*gClose)();
    int  (*gUpdate)();
    void (*gPixel)();
    void (*gLine)();
    void (*gBox)();
    void (*gPolygon)();
    int  (*gDefLs)();
    int  (*gSetLs)();
    int  (*gDefFp)();
    int  (*gSetFp)();
    int  (*gDefColor)();
    int  (*gSetColor)();
    void (*gText)();
    void (*gScText)();

    void (*gClear)();
    int  (*gPoint)();
    void (*gBeep)();
    int  (*gSetFullScreenCursor)();
    void (*gTextBB)();
    void (*gMore)();
    void (*gSetName)(*);
    int  (*gBlink)();
    int  (*gSelectFont)();
    void (*gSelectCursor)();
    void (*gSetTextClip)();
    void (*gResize)();
#endif
};

#define FBNUMINTERFACE 2
extern struct sGinterface GR[FBNUMINTERFACE];

/* printer driver codes */
#define HPLASER 'h'
#define POSTSC  'p'

#define ERASE                  'e'
#define DISPLAY                'd'
#define FILL                   'f'
#define OUTLINE                'o'
#define ROW_COLUMN             'r'
#define PIXEL_COORDINATE       'p'


#ifdef __STDC__
struct ka;
extern void FBBegin(char*);
extern void FBForeground(int,int);
extern void FBVLT(int,int,int,int);
extern void FBText(int,int,int,char*);
extern int  FBGetchar(int);
extern char *FBEdit(char*);
extern void FBSetCursorColor(int);
extern void FBPolygon(int,int,int,int*,int);
extern void FBPolygonClip(int*,int*,struct ka*);
extern void FBSetRubberBanding(int);
extern void FBKbRepaint(int,int);
extern unsigned int FBTime(void);
extern void FBFuncKeys(int,int);

extern void XORfineViewport(void);
extern int  GetWindowCoords(int*,int*,int);
extern int  *SetButtonMask(void);
extern void SelectKicFont(void);
extern void SelectKicCursor(void);
extern int  Xcheck(void);
extern void cprint(int,char*);
#else
extern void FBBegin();
extern void FBForeground();
extern void FBVLT();
extern void FBText();
extern int  FBGetchar();
extern char *FBEdit();
extern void FBSetCursorColor();
extern void FBPolygon();
extern void FBPolygonClip();
extern void FBSetRubberBanding();
extern void FBKbRepaint();
extern unsigned int FBTime();
extern void FBFuncKeys();

extern void XORfineViewport();
extern int  GetWindowCoords();
extern int  *SetButtonMask();
extern void SelectKicFont();
extern void SelectKicCursor();
extern int  Xcheck();
extern void cprint();
#endif


#define FBInfo                 (*GR[FB.fInterface].gInfo)
#define FBEnd                  (*GR[FB.fInterface].gClose)
#define FBTransfer             (*GR[FB.fInterface].gUpdate)
#define FBLine                 (*GR[FB.fInterface].gLine)
#define FBPixel                (*GR[FB.fInterface].gPixel)
#define FBDefineFillPattern    (*GR[FB.fInterface].gDefFp)
#define FBDefineLineStyle      (*GR[FB.fInterface].gDefLs)
#define FBSetFillPattern       (*GR[FB.fInterface].gSetFp)
#define FBSetLineStyle         (*GR[FB.fInterface].gSetLs)
#define FBScaledText           (*GR[FB.fInterface].gScText) 

#define FBSetColor             (*GR[0].gSetColor)
#define FBFlood                (*GR[0].gClear)
#define FBPoint                (*GR[0].gPoint)
#define FBBeep                 (*GR[0].gBeep)
#define FBSetFullScreenCursor  (*GR[0].gSetFullScreenCursor)
#define FBTextBB               (*GR[0].gTextBB)
#define FBMore                 (*GR[0].gMore)
#define FBSetName              (*GR[0].gSetName)
#define FBBlink                (*GR[0].gBlink)
#define FBSelectFont           (*GR[0].gSelectFont)
#define FBSelectCursor         (*GR[0].gSelectCursor)
#define FBSetTextClip          (*GR[0].gSetTextClip)
#define FBLineClip             (*GR[0].gLineClip)
#define FBResizeDrawingWindow  (*GR[0].gResize)


#define FBFilledBox(Pixel,DisplayOrErase,StyleId,L,B,R,T) { \
     FBForeground(DisplayOrErase,Pixel); \
     (*GR[FB.fInterface].gSetFp)(StyleId); \
     (*GR[FB.fInterface].gBox)((int)(L),(int)(B),(int)(R),(int)(T)); \
     } \

#define FBEmptyBox(Pixel,DisplayOrErase,StyleId,L,B,R,T) { \
     FBForeground(DisplayOrErase,Pixel); \
     (*GR[FB.fInterface].gLine)((int)(L),(int)(B),(int)(R),(int)(B)); \
     (*GR[FB.fInterface].gLine)((int)(R),(int)(B),(int)(R),(int)(T)); \
     (*GR[FB.fInterface].gLine)((int)(R),(int)(T),(int)(L),(int)(T)); \
     (*GR[FB.fInterface].gLine)((int)(L),(int)(T),(int)(L),(int)(B)); \
     } \

#define FBBox(Pixel,DisplayOrErase,Type,StyleId,L,B,R,T) { \
     FBForeground(DisplayOrErase,Pixel); \
     if(Type == OUTLINE){ \
      (*GR[FB.fInterface].gLine)((int)(L),(int)(B),(int)(R),(int)(B)); \
      (*GR[FB.fInterface].gLine)((int)(R),(int)(B),(int)(R),(int)(T)); \
      (*GR[FB.fInterface].gLine)((int)(R),(int)(T),(int)(L),(int)(T)); \
      (*GR[FB.fInterface].gLine)((int)(L),(int)(T),(int)(L),(int)(B)); \
     } \
     else { \
      (*GR[FB.fInterface].gSetFp)(StyleId); \
      (*GR[FB.fInterface].gBox)((int)(L),(int)(B),(int)(R),(int)(T)); \
     } \
    }


/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1994
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

/* declarations of MFB functions */


/* application supplied routines */
#if __STDC__
extern void SetDisplayWindow(int,int,int*,int*,int*,int*);
extern void RepaintWindow(int);
#else
extern void SetDisplayWindow();
extern void RepaintWindow();
#endif

#ifdef __STDC__

/* Library routines */
extern void  MFBArc(int,int,int,int,int,int);
extern MFBPATH *MFBArcPath(int,int,int,int,int,int);
extern void  MFBAudio(void);
extern void  MFBBox(int,int,int,int);      /* draw box cur colr, fillpatt  */
extern void  MFBCircle(int,int,int,int);
extern int   MFBClose(void);               /* exit MFB                     */
extern int   MFBDefineColor(int,int,int,int);/* define new color           */
extern int   MFBDefineFillPattern(int,int*);/* define new fill pattern     */
extern int   MFBDefineLineStyle(int,int);  /* define new line style        */
extern int   MFBDescaleX(int);
extern int   MFBDescaleY(int);
extern void  MFBDrawLineTo(int,int);       /* line from last move, draw    */
extern void  MFBDrawPath(MFBPATH*);
extern MFBPOLYGON *MFBEllipse(int,int,int,int,int);
extern char  *MFBError(int);               /* error handler                */
extern void  MFBFlash(int,int,int,int);
extern void  MFBFlood(void);               /* flood screen in current colr */
extern int   MFBGetchar(void);             /* get character from keybrd    */
extern char  *MFBGetStorage(void);
extern int   MFBHalt(void);                /* exit MFB                     */
extern int   MFBInfo(int);                 /* return MFB info              */
extern int   MFBInitialize(void);          /* initialize MFB               */
extern char  *MFBKeyboard(int,int,int,int);
extern void  MFBLine(int,int,int,int);     /* draw line, cur colr, style   */
extern Bool  MFBLineClip(int*,int*,int*,int*,int,int,int,int);
                                           /* line clipping routine        */
extern void  MFBMore(int,int,int,int,FILE*);/* display text by page        */
extern void  MFBMoveTo(int,int);           /* move internal coordinates    */
extern void  MFBNaiveBoxFill(int,int,int,int);
extern MFB   *MFBOpen(char*,char*,int*);   /* start graphics               */
extern void  MFBPixel(int,int);            /* draw pixel in current color  */
extern int   MFBPoint(int*,int*,int*,int*);/* enable pointing device      */
extern void  MFBPolygon(MFBPOLYGON*);      /* draw polygon                 */
extern int   MFBPutchar(int);
extern int   MFBPutstr(char*,int);
extern void  MFBRasterCopy(int,int,int,int,int,int);
extern int   MFBScaleX(int);
extern int   MFBScaleY(int);
extern int   MFBSetALUMode(int);           /* set pixel update mode        */
extern int   MFBSetBlinker(int,int,int,int,int);  /* no operation          */
extern int   MFBSetChannelMask(int);
extern int   MFBSetColor(int);             /* set current color            */
extern void  SetCurrentMFB(MFB*);
extern int   MFBSetCursorColor(int,int);   /* set cursor color             */
extern int   MFBSetFillPattern(int);       /* set fill pattern             */
extern int   MFBSetLineStyle(int);         /* set current line style       */
extern int   MFBSetReadMask(int);
extern int   MFBSetRubberBanding(int,int,int);
extern int   MFBSetStorage(char*);
extern int   MFBSetTextMode(Bool);         /* no operation                 */
extern void  MFBSetViewport(int,int,int,int);
extern void  MFBSetWindow(int,int,int,int);
extern void  MFBText(char*,int,int,int);   /* display text                 */
extern int   MFBUpdate(void);              /* update screen (nop here)     */
extern void  MFB_X_Intercept(int,int,int,int,int,int*);
extern void  MFB_Y_Intercept(int,int,int,int,int,int*);
                                           /* line intercepts              */
extern int   MFBntox(int, int);            /* raise int to poser           */

/* Nonstandard library routines */
extern void  MFBBeep(int);
extern void  MFBDrawCursor(int,int);       /* save background, draw marker */
extern void  MFBEraseCursor(int,int);      /* restore background           */
extern void  MFBFuncKeys(int,int);         /* set function key returns     */
extern int   MFBGetPixel(int,int);         /* get pixel color              */
extern int   MFBNewPolygon(MFBPOLYGON*);   /* polygon rendering            */
extern void  MFBNewPolygonClip(MFBPOLYGON*,int,int,int,int);
                                           /* polygon clipping routine     */
extern void  MFBPointerClose(void);        /* close pointing device        */
extern void  MFBPointerInit(void);         /* initialize pointing device   */
extern void  MFBRect(int,int,int,int);
extern void  MFBResizeDrawingWindow(int,int,int,int);
extern void  MFBScroll(int,int,int,int,FILE*);
#ifdef MFB_H
#ifndef VGAMFB
extern int   MFBScrollFont(int,int,int,int,XFontStruct*,GC,int,int);
#endif
#endif
extern int   MFBScrollList(int,int,int,int,char**,int,int,int,int);
extern void  MFBSelectCursor(int,int,int,int);
extern int   MFBSelectFont(int,int,int,int);
extern void  MFBSetCursor(int);
extern int   MFBSetFont(char*);
extern int   MFBSetFullScreenCursor(int);
extern void  MFBSetName(char*,char*);
extern void  MFBScaledText(char*,int,int,int,int);/* text with big pixels  */
extern int   MFBSetGhost(void(*)(int,int,int,int),int,int); /* xor image   */
extern void  MFBSetTextClip(int,int,int,int);/* clipping box for text      */
extern void  MFBTextBB(char*,int*,int*);   /* get pix wid and ht of string */

#else

/* Library routines */
extern void  MFBArc();
extern MFBPATH *MFBArcPath();
extern void  MFBAudio();
extern void  MFBBox();                    /* draw box cur colr, fillpatt  */
extern void  MFBCircle();
extern int   MFBClose();                  /* exit MFB                     */
extern int   MFBDefineColor();            /* define new color             */
extern int   MFBDefineFillPattern();      /* define new fill pattern      */
extern int   MFBDefineLineStyle();        /* define new line style        */
extern int   MFBDescaleX();
extern int   MFBDescaleY();
extern void  MFBDrawLineTo();             /* line from last move, draw    */
extern void  MFBDrawPath();
extern MFBPOLYGON *MFBEllipse();
extern char  *MFBError();                 /* error handler                */
extern void  MFBFlash();
extern void  MFBFlood();                  /* flood screen in current colr */
extern int   MFBGetchar();                /* get character from keybrd    */
extern char  *MFBGetStorage();
extern int   MFBHalt();                   /* exit MFB                     */
extern int   MFBInfo();                   /* return MFB info              */
extern int   MFBInitialize();             /* initialize MFB               */
extern char  *MFBKeyboard();
extern void  MFBLine();                   /* draw line, cur colr, style   */
extern Bool  MFBLineClip();               /* line clipping routine        */
extern void  MFBMore();                   /* display text by page         */
extern void  MFBMoveTo();                 /* move internal coordinates    */
extern void  MFBNaiveBoxFill();
extern MFB   *MFBOpen();                  /* start graphics               */
extern void  MFBPixel();                  /* draw pixel in current color  */
extern int   MFBPoint();                  /* enable pointing device       */
extern void  MFBPolygon();                /* draw polygon                 */
extern int   MFBPutchar();
extern int   MFBPutstr();
extern void  MFBRasterCopy();
extern int   MFBScaleX();
extern int   MFBScaleY();
extern int   MFBSetALUMode();             /* set pixel update mode        */
extern int   MFBSetBlinker();             /* no operation                 */
extern int   MFBSetChannelMask();
extern int   MFBSetColor();               /* set current color            */
extern void  SetCurrentMFB();
extern int   MFBSetCursorColor();         /* set cursor color             */
extern int   MFBSetFillPattern();         /* set fill pattern             */
extern int   MFBSetLineStyle();           /* set current line style       */
extern int   MFBSetStorage();
extern int   MFBSetReadMask();
extern int   MFBSetRubberBanding();
extern int   MFBSetTextMode();            /* no operation                 */
extern void  MFBSetViewport();
extern void  MFBSetWindow();
extern void  MFBText();                   /* display text                 */
extern int   MFBUpdate();                 /* update screen (nop here)     */
extern void  MFB_X_Intercept();
extern void  MFB_Y_Intercept();           /* line intercepts              */
extern int   MFBntox();                   /* raise int to poser           */

/* Nonstandard library routines */
extern void  MFBBeep();
extern void  MFBDrawCursor();             /* save background, draw marker */
extern void  MFBEraseCursor();            /* restore background           */
extern void  MFBFuncKeys();               /* set function key returns     */
extern int   MFBGetPixel();               /* get pixel color              */
extern int   MFBNewPolygon();             /* polygon rendering            */
extern void  MFBNewPolygonClip();         /* polygon clipping routine     */
extern void  MFBPointerClose();           /* close pointing device        */
extern void  MFBPointerInit();            /* initialize pointing device   */
extern void  MFBRect();
extern void  MFBResizeDrawingWindow();
extern void  MFBScroll();
extern int   MFBScrollFont();
extern int   MFBScrollList();
extern void  MFBSelectCursor();
extern int   MFBSelectFont();
extern void  MFBSetCursor();
extern int   MFBSetFont();
extern int   MFBSetFullScreenCursor();
extern void  MFBSetName();
extern void  MFBScaledText();             /* text with big pixels         */
extern int   MFBSetGhost();               /* turn on xor image            */
extern void  MFBSetTextClip();            /* clipping box for text        */
extern void  MFBTextBB();                 /* get pix wid and ht of string */

#endif

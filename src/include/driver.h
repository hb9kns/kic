/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1994
 *
 *************************************************************************/

#ifndef MFB_H

/* Defines for MFBInfo */
#define DEVICETYPE           0    /* type of device (TTY or HCOPY) */
#define MAXX                 1    /* max x coordinate */
#define MAXY                 2    /* max y coordinate */
#define MAXCOLORS            3    /* max number of colors */
#define MAXINTENSITY         4    /* max color intensity */
#define MAXFILLPATTERNS      5    /* max number of fill patterns */
#define MAXLINESTYLES        6    /* max number of line styles */
#define MAXBLINKERS          7    /* max number of blinkers */
#define POINTINGDEVICE       8    /* Bool: terminal has pointing device */
#define POINTINGBUTTONS      9    /* Bool: pointing device has buttons */
#define NUMBUTTONS          10    /* number of pointing device buttons */
#define BUTTON1             11    /* button value returned by button 1 */
#define BUTTON2             12    /* button value returned by button 2 */
#define BUTTON3             13    /* button value returned by button 3 */
#define BUTTON4             14    /* button value returned by button 4 */
#define BUTTON5             15    /* button value returned by button 5 */
#define BUTTON6             16    /* button value returned by button 6 */
#define BUTTON7             17    /* button value returned by button 7 */
#define BUTTON8             18    /* button value returned by button 8 */
#define BUTTON9             19    /* button value returned by button 9 */
#define BUTTON10            20    /* button value returned by button 10 */
#define BUTTON11            21    /* button value returned by button 11 */
#define BUTTON12            22    /* button value returned by button 12 */
#define TEXTPOSITIONABLE    30    /* Bool: accurately positionable text */
#define TEXTROTATABLE       31    /* Bool: rotateable text */
#define FONTHEIGHT          32    /* font height in pixels */
#define FONTWIDTH           33    /* font width in pixels */
#define FONTXOFFSET         34    /* font x offset in pixels */
#define FONTYOFFSET         35    /* font y offset in pixels */
#define DESTRUCTIVETEXT     36    /* Bool: text can be destructive */
#define OVERSTRIKETEXT      37    /* Bool: text can be overstrike */
#define VLT                 38    /* Bool: terminal has VLT */
#define BLINKERS            39    /* Bool: terminal has blinkers */
#define FILLEDPOLYGONS      40    /* Bool: terminal has filled polygons */
#define DEFFILLPATTERNS     41    /* Bool: defineable fill patterns */
#define DEFCHANNELMASK      42    /* Bool: defineable write mask */
#define DEFLINEPATTERN      43    /* Bool: defineable line styles */
#define CURFGCOLOR          44    /* current foreground color */
#define CURFILLPATTERN      45    /* current fill pattern */
#define CURLINESTYLE        46    /* current line style */
#define CURCHANNELMASK      47    /* current write mask */
#define CURREADMASK         48    /* current read mask */
#define NUMBITPLANES        49    /* number of bit planes */
#define DEFREADMASK         50    /* Bool: definable read mask */
#define RASTERCOPY          51    /* Bool: terminal has raster copy */
#define OFFSCREENX          52    /* left value of off screen memory */
#define OFFSCREENY          53    /* bottom value of off screen memory */
#define OFFSCREENDX         54    /* length of off screen memory */
#define OFFSCREENDY         55    /* width of off screen memory */
#define CURFONTSIZE         56    /* current text font size */
#define HORTPIXPERINCH      57    /* pixels per inch horizontally */
#define VERTPIXPERINCH      58    /* pixels per inch vertically */
#define ACTIONTIME          59    /* time of last user input action */
#define FONTNAME            60    /* pointer to current font name */
#define CURSORSHAPE         61    /* cursor shape index */
#define FULLSCREENCURSOR    62    /* full-screen cursor (true/false) */

struct mfbpath {
    int nvertices;
    int *xy;
};
typedef struct mfbpath MFBPOLYGON;
typedef struct mfbpath MFBPATH;

typedef short Bool;
typedef char MFB;

#endif


#ifdef __STDC__
extern int  GR_info(int);                 /* return info                  */
extern int  GR_open(char*);               /* start graphics               */
extern int  GR_close(void);               /* exit                         */
extern int  GR_update(void);              /* update screen (nop here)     */
extern void GR_pixel(int,int);            /* draw pixel in current color  */
extern void GR_line(int,int,int,int);     /* draw line, cur colr, style   */
extern void GR_box(int,int,int,int);      /* draw box cur colr, fillpatt  */
extern void GR_polygon(Poly*);            /* draw polygon                 */
extern int  GR_defLs(int,int);            /* define new line style        */
extern int  GR_setLs(int);                /* set current line style       */
extern int  GR_defFp(int,int*);           /* define new fill pattern      */
extern int  GR_setFp(int);                /* set fill pattern             */
extern int  GR_defColor(int,int,int,int); /* define new color             */
extern int  GR_setColor(int);             /* set current color            */
extern void GR_text(char*,int,int,int);   /* display text                 */
extern void GR_scText(char*,int,int,int,int);/* text with big pixels      */

#else

extern int  GR_info();                    /* return info                  */
extern int  GR_open();                    /* start graphics               */
extern int  GR_close();                   /* exit                         */
extern int  GR_update();                  /* update screen (nop here)     */
extern void GR_pixel();                   /* draw pixel in current color  */
extern void GR_line();                    /* draw line, cur colr, style   */
extern void GR_box();                     /* draw box cur colr, fillpatt  */
extern void GR_polygon();                 /* draw polygon                 */
extern int  GR_defLs();                   /* define new line style        */
extern int  GR_setLs();                   /* set current line style       */
extern int  GR_defFp();                   /* define new fill pattern      */
extern int  GR_setFp();                   /* set fill pattern             */
extern int  GR_defColor();                /* define new color             */
extern int  GR_setColor();                /* set current color            */
extern void GR_text();                    /* display text                 */
extern void GR_scText();                  /* text with big pixels         */

#endif



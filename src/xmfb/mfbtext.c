/*************************************************************************
 XMFB X-Window MFB emulation package
 Authore: Peter C. Vernam, C. S. Draper Laboratory, 1989
          Stephen R. Whiteley, 1992
 *************************************************************************/

/**************************************************************************
 *
 *                             MFBText
 *
 * This module contains the MFB routines for the management of graphtext:
 *   MFBMore       - display the contents of a file, one page at a time.
 *   MFBScroll     - display the contents of a file, with the capability of
 *                   scrolling forwards or backwards.  (Note: The output from
 *                   a pipe cannot be scrolled backwards.)
 *   MFBScrollList - display and scroll an array of text strings, with
 *                   optional selection (and optional display of font if the
 *                   text strings are font names).
 *   MFBScrollFont - display and scroll a text font, with optional selection.
 *
 **************************************************************************/

/*
 * Modified October 1989 by Peter C. Vernam, Charles Stark Draper Laboratory:
 *   08OCT89 - New routines MFBScrollList and MFBScrollFont added.
 *   11OCT89 - Revised considerably to correctly handle the variable-width
 *             text fonts that are available in X.
 *   06NOV89 - MFBScroll modified to use ftell() and fseek() for scrolling
 *             backwards through file.
 */


#include "mfb.h"        /* standard MFB header file */
#include <string.h>

#define MAXCHARLINE    200    /* maximum number of characters per line */
#define MAXLINEPAGE    100    /* maximum number of lines per page */
#define    MAXPAGES    200    /* maximum number of page top pointers */


#if __STDC__
static int get_bg_color(void);
static void display_scroll_help(int,int,int,int,int,int,int,int,int);
#else
static int get_bg_color();
static void display_scroll_help();
#endif


void
MFBMore(Left,Bottom,Right,Top,Textfile)

int Left, Bottom, Right, Top;
FILE *Textfile;
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
    Bool variable_width;    /* True if current font is variable width */

    /* test to be sure of window area */
    if (Top < Bottom)
        MFBSwapInt(Top, Bottom);
    if (Right < Left)
        MFBSwapInt(Left, Right);

    /* calculate parameters */
    dy = mfb_fontHeight;
    nlines = (Top - Bottom) / dy;
    if (nlines <= 0)
        return;
    width_screen = Right - Left;
    if (mfb_font->min_bounds.width != mfb_font->max_bounds.width) {
        variable_width = True;
        width_space = XTextWidth(mfb_font, " ", 1);
        width_screen -= 3;
    }
    else {
        variable_width = False;
        width_space = mfb_font->max_bounds.width;
    }

    /* save old style ID's */
    oldforeground = mfb_fgColorId;
    oldfillpattern = mfb_fillPattern;

    MFBSetFillPattern(0);
    if (oldforeground == 0)
        newbackground = get_bg_color();
    else
        newbackground = 0;

    pagecount = 0;
    done = 0;

    while (!done) {
        /* Display one page of text from the file */
        MFBSetColor(newbackground);
        MFBBox(Left, Bottom, Right, Top);   /* Erase the screen */
        MFBSetColor(oldforeground);
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
                                XTextWidth(mfb_font, &cbuf[i], 1) :
                                width_space) > width_screen)
                            break;
                        controlchar = 1;
                    }
                    else {
                        cbuf[i] = c + '@';
                        controlchar = 0;
                        if ((width_string += (variable_width) ?
                                XTextWidth(mfb_font, &cbuf[i], 1) :
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
                            XTextWidth(mfb_font,(char *) &c, 1) :
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
            MFBText(cbuf, Left, Top - j * dy, 0);
            if (c == EOF) {
                done = 1;
                break;
            }
        }
        if (done)
            strcpy(cbuf, "-DONE- (^U to scroll up, ? for help)");
        else
            strcpy(cbuf, "-MORE- (^U to scroll up, ^D to exit, ? for help)");
        MFBText(cbuf, Left, Bottom, 0);
        i = Left + XTextWidth(mfb_font, cbuf, strlen(cbuf));
        sprintf(cbuf, "Page %d", ++pagecount);
        j = Right - XTextWidth(mfb_font, cbuf, strlen(cbuf)) - 3;
        if (j < i)
            j = i;
        MFBText(cbuf, j, Bottom, 0);

        /* Wait for user interaction and perform requested function */
        for (;;) {
            MFBPoint(&x, &y, &key, &button);
            if (button == -1) {
                /* Keyboard key pressed */
                if (key == CTRLD) 
                    done = 1;
            }
            else {
                /* Mouse button pressed */
                if (x < Left || x > Right || y < Bottom || y > Top)
                    done = 1;
                else if (button == mfb_buttonMask[1]) {
                    /* MB2 - down */
                    if (done) {
                        MFBAudio();
                        continue;
                    }
                }
                else if (button != mfb_buttonMask[0]) {
                    MFBAudio();
                    continue;
                }
            }
            break;
        }
    }
    MFBSetFillPattern(oldfillpattern);
}


void
MFBScroll(Left,Bottom,Right,Top,Textfile)

int Left, Bottom, Right, Top;
FILE *Textfile;
{
    char cbuf[MAXCHARLINE+9];    /* add extra space for tab expansion */
    int key;
    int i,j,x,y,c,button,dy;
    int topline;        /* line at top of box */
    int curline;        /* current line */
    int pagecount;      /* count of pages of text displayed */
    int lastpage;       /* last page number */
    int pagetopline[MAXPAGES];    /* saved topline for each page */
    int done;
    int oldfillpattern;
    int oldforeground;
    int newbackground;
    int nlines;
    int controlchar;
    int width_screen;       /* width of the box */
    int width_string;       /* width of the current string */
    int width_space;        /* width of a space character */
    Bool variable_width;    /* True if current font is variable width */
    Bool display_switch;    /* True to display file; False to find end */

    /* test to be sure of window area */
    if (Top < Bottom)
        MFBSwapInt(Top, Bottom);
    if (Right < Left)
        MFBSwapInt(Left, Right);

    /* calculate parameters */
    dy = mfb_fontHeight;
    nlines = (Top - Bottom) / dy;
    if (nlines <= 0)
        return;
    width_screen = Right - Left;
    if (mfb_font->min_bounds.width != mfb_font->max_bounds.width) {
        variable_width = True;
        width_space = XTextWidth(mfb_font, " ", 1);
        width_screen -= 3;
    }
    else {
        variable_width = False;
        width_space = mfb_font->max_bounds.width;
    }

    /* save old style ID's */
    oldforeground = mfb_fgColorId;
    oldfillpattern = mfb_fillPattern;

    MFBSetFillPattern(0);
    if (oldforeground == 0)
        newbackground = get_bg_color();
    else
        newbackground = 0;

    pagecount = 0;
    lastpage = 0;
    topline = 0;
    display_switch = True;
    done = 0;

    while (!done ) {
        /* Display one page of text from the file */
        if (display_switch) {
            MFBSetColor(newbackground);
            MFBBox(Left, Bottom, Right, Top); /* Erase the screen */
            MFBSetColor(oldforeground);
        }
        fseek(Textfile, topline, 0);
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
                                XTextWidth(mfb_font, &cbuf[i], 1) :
                                width_space) > width_screen)
                            break;
                        controlchar = 1;
                    }
                    else {
                        cbuf[i] = c + '@';
                        controlchar = 0;
                        if ((width_string += (variable_width) ?
                                XTextWidth(mfb_font, &cbuf[i], 1) :
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
                            XTextWidth(mfb_font,(char *) &c, 1) :
                            width_space) > width_screen) {
                        ungetc(c,Textfile);
                        break;
                    }
                    cbuf[i++] = c;
                }
                if (i >= MAXCHARLINE)
                    break;
            }
            if (display_switch) {
                cbuf[i] = '\0';
                MFBText(cbuf, Left, Top - j * dy, 0);
            }
            if (c == EOF) {
                done = 1;
                break;
            }
        }
        curline = ftell(Textfile);
        if (pagecount < MAXPAGES)
            pagetopline[pagecount] = topline;
        if (display_switch)
            ++pagecount;
        else {
            if (done) {
                done = 0;
                display_switch = True;
            }
            else {
                topline = curline;
                ++pagecount;
            }
            continue;
        }
        if (done) {
            lastpage = pagecount;
            strcpy(cbuf, "-DONE- (^U to scroll up, ? for help)");
        }
        else
            strcpy(cbuf, "-MORE- (^U to scroll up, ^D to exit, ? for help)");
        MFBText(cbuf, Left, Bottom, 0);
        i = Left + XTextWidth(mfb_font, cbuf, strlen(cbuf));
        if (lastpage > 0)
            sprintf(cbuf, "Page %d of %d", pagecount, lastpage);
        else
            sprintf(cbuf, "Page %d", pagecount);
        j = Right - XTextWidth(mfb_font, cbuf, strlen(cbuf)) - 3;
        if (j < i)
            j = i;
        MFBText(cbuf, j, Bottom, 0);

        /* Wait for user interaction and perform requested function */
        for (;;) {
            MFBPoint(&x, &y, &key, &button);
                if (button == -1) {
                    /* Keyboard key pressed */
                    if (key == CTRLD)
                        /* <Ctrl/D> - done */
                        done = 1;
                    else if (key == CTRLU) {
                        /* <Ctrl/U> - scroll up */
                        if (topline == 0) {
                            MFBAudio();
                            continue;
                        }
                        --pagecount;
                        if (pagecount < 1)
                            pagecount = 1;
                        else if (pagecount > MAXPAGES)
                            pagecount = MAXPAGES;
                        topline = pagetopline[--pagecount];
                        done = 0;
                    }
                    else if (key == '?') {
                        /* '?' - help */
                        display_scroll_help(Left, Bottom, Right, Top, dy,
                            oldforeground, newbackground, 0, 0);
                        done = 0;
                    }
                    else
                        /* else - scroll down */
                        topline = curline;
                }
                else {
                    /* Mouse button pressed */
                    if (x < Left || x > Right || y < Bottom || y > Top)
                        done = 1;
                    else if (button == mfb_buttonMask[2]) {
                        /* MB3 - up */
                        if (topline == 0) {
                            MFBAudio();
                            continue;
                        }
                        --pagecount;
                        if (pagecount < 1)
                            pagecount = 1;
                        else if (pagecount > MAXPAGES)
                            pagecount = MAXPAGES;
                        topline = pagetopline[--pagecount];
                        done = 0;
                    }
                else if (button == mfb_buttonMask[5]) {
                    /* Shift/MB3 - top */
                    if (topline == 0) {
                        MFBAudio();
                        continue;
                    }
                    pagecount = 0;
                    topline = 0;
                    done = 0;
                }
                else if (button == mfb_buttonMask[1]) {
                    /* MB2 - down */
                    if (done) {
                        MFBAudio();
                        continue;
                    }
                    topline = curline;
                }
                else if (button == mfb_buttonMask[4]) {
                    /* Shift/MB2 - bottom */
                    if (done) {
                        MFBAudio();
                        continue;
                    }
                    if (lastpage > 0) {
                        pagecount = lastpage;
                        if (pagecount > MAXPAGES) {
                            pagecount = MAXPAGES;
                            display_switch = False;
                        }
                        topline = pagetopline[--pagecount];
                    }
                    else {
                        topline = curline;
                        display_switch = False;
                    }
                }
                else if (button != mfb_buttonMask[0]) {
                    MFBAudio();
                    continue;
                }
                else
                    /* MB1 - down */
                    topline = curline;
            }
            break;
        }
    }
    MFBSetFillPattern(oldfillpattern);
}


int
MFBScrollList(Left,Bottom,Right,Top,List,Nlist,SelectSW,FontSW,NumberSW)

int Left, Bottom, Right, Top;
char **List;    /* array of character strings */
int Nlist;      /* number of strings in array List */
int SelectSW;   /* if true, list items may be selected. The
                 * index (beginning with 0) of the selected List
                 * item will be returned if a selection is made,
                 * otherwise -1 will be returned. */
int FontSW;     /* if true, the List is assumed to be a list of
                 * font names.  If one is selected and '+' is
                 * entered, the corresponding font table will
                 * be displayed. */
int NumberSW;   /* if true, each string in the List will be
                 * displayed preceded by its number (beginning
                 * with 1), and ". " */
/*
 * New routine created 08OCT89 by Peter C. Vernam, C. S. Draper Laboratory
 *
 * This routine performs much like MFBScroll except that, instead of scrolling
 * through a file, an array of character strings (an in-memory text file, of
 * a sort) is scrolled.  This avoids the overhead of writing and then reading a
 * file when this is not strictly necessary.
 */
{
    char cbuf[MAXCHARLINE+9];    /* add extra space for tab expansion */
    int  ypos[MAXLINEPAGE];
    char c;
    int key;
    int i,j,x,y,button,dy;
    int topline;         /* line at top of box */
    int curline;         /* current line */
    int curchar;         /* current character within line */
    int linecount = 0;   /* count of source lines displayed */
    int pagecount;       /* count of pages of text displayed */
    int lastpage;        /* last page number */
    int pagetopline[MAXPAGES];    /* saved topline for each page */
    int done;
    int oldfillpattern;
    int oldforeground;
    int newbackground;
    int nlines;
    int controlchar;
    int selection = -1;  /* selection index return value */
    int width_screen;    /* width of the box */
    int width_string;    /* width of the current string */
    int width_space;     /* width of a space character */
    int xorg;            /* x coordinate of string origin */
    int xcont = 0;       /* x origin for continuation lines */
    Bool variable_width; /* True if current font is variable width */
    Bool display_switch; /* True to display file; False to find end */
    static XFontStruct *myfont = NULL;
    static char myfontname[129] = "";
    static GC mygc = None;

    /* test to be sure of window area */
    if (Top < Bottom)
        MFBSwapInt(Top, Bottom);
    if (Right < Left)
        MFBSwapInt(Left, Right);

    /* calculate parameters */
    dy = mfb_fontHeight;
    nlines = (Top - Bottom) / dy;
    if (nlines <= 0)
        return (-1);
    width_screen = Right - Left;
    if (mfb_font->min_bounds.width != mfb_font->max_bounds.width) {
        variable_width = True;
        width_space = XTextWidth(mfb_font, " ", 1);
        width_screen -= 3;
    }
    else {
        variable_width = False;
        width_space = mfb_font->max_bounds.width;
    }

    /* save old style ID's */
    oldforeground = mfb_fgColorId;
    oldfillpattern = mfb_fillPattern;

    MFBSetFillPattern(0);
    if (oldforeground == 0)
        newbackground = get_bg_color();
    else
        newbackground = 0;

    pagecount = 0;
    lastpage = 0;
    topline = 0;
    display_switch = True;
    done = 0;

    while (!done ) {
        /* Display one page of text from the List */
        if (display_switch) {
            MFBSetColor(newbackground);
            MFBBox(Left, Bottom, Right, Top);    /* Erase the screen */
            MFBSetColor(oldforeground);
        }
        curline = topline;
        curchar = 0;
        for (j = 1; j < nlines; ++j) {
            /* Loop to fill a page */
            i = 0;
            width_string = 0;
            xorg = Left;
            if (NumberSW) {
                /* Prefix line with the line number */
                if (curchar == 0) {
                    sprintf(cbuf, "%d. ", curline + 1);
                    i = strlen(cbuf);
                    if (variable_width)
                        width_string = XTextWidth(mfb_font, cbuf, i);
                    else
                        width_string = width_space * i;
                    xcont = xorg + width_string;
                }
                else {
                    width_string = xcont - xorg;
                    xorg = xcont;
                }
            }
            controlchar = 0;
            while ((c = List[curline][curchar]) != '\0') {
                /* Get a char */
                if (c == '\t') {
                    /* tab */
                    cbuf[i++] = ' ';
                    if ((width_string += width_space) > width_screen)
                        break;
                    ++curchar;
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
                        if ((width_string += (variable_width) ?
                                XTextWidth(mfb_font, &cbuf[i], 1) :
                                width_space) > width_screen)
                            break;
                        controlchar = 1;
                    }
                    else {
                        cbuf[i] = c + '@';
                        controlchar = 0;
                        if ((width_string += (variable_width) ?
                                XTextWidth(mfb_font, &cbuf[i], 1) :
                                width_space) > width_screen) {
                            --i;
                            break;
                        }
                        ++curchar;
                    }
                    ++i;
                }
                else if (c <= '~') {
                    /* < DEL character? */
                    if ((width_string += (variable_width) ?
                            XTextWidth(mfb_font, &c, 1) :
                            width_space) > width_screen)
                        break;
                    cbuf[i++] = c;
                    ++curchar;
                }
                if (i >= MAXCHARLINE)
                    break;
            }
            if (display_switch) {
                cbuf[i] = '\0';
                linecount = curline - topline;
                ypos[linecount] = Top - j * dy;
                if (curline == selection) {
                    MFBSetColor(oldforeground);
                    MFBBox(Left, ypos[linecount], Right, ypos[linecount] + dy);
                    MFBSetColor(newbackground);
                    MFBText(cbuf, xorg, ypos[linecount], 0);
                    MFBSetColor(oldforeground);
                }
                else
                    MFBText(cbuf, xorg, ypos[linecount], 0);
            }
            if (c == '\0') {
                curchar = 0;
                if (++curline >= Nlist) {
                    done = 1;
                    break;
                }
            }
        }
        if (pagecount < MAXPAGES)
            pagetopline[pagecount] = topline;
        if (display_switch)
            ++pagecount;
        else {
            if (done) {
                done = 0;
                display_switch = True;
            }
            else {
                topline = curline;
                ++pagecount;
            }
            continue;
        }
        if (done) {
            lastpage = pagecount;
            strcpy(cbuf, "-DONE- (^U to scroll up, ? for help)");
        }
        else
            strcpy(cbuf, "-MORE- (^U to scroll up, ^D to exit, ? for help)");
        MFBText(cbuf, Left, Bottom, 0);
        i = Left + XTextWidth(mfb_font, cbuf, strlen(cbuf));
        if (lastpage > 0)
            sprintf(cbuf, "Page %d of %d", pagecount, lastpage);
        else
            sprintf(cbuf, "Page %d", pagecount);
        j = Right - XTextWidth(mfb_font, cbuf, strlen(cbuf)) - 3;
        if (j < i)
            j = i;
        MFBText(cbuf, j, Bottom, 0);

        /* Wait for user interaction and perform requested function */
        for (;;) {
            MFBPoint(&x, &y, &key, &button);
            if (button == -1) {
                /* Keyboard key pressed */
                if (key == CTRLD)
                    /* <Ctrl/D> - done */
                    done = 1;
                else if (key == CTRLU) {
                    /* <Ctrl/U> - scroll up */
                    if (topline == 0) {
                        MFBAudio();
                        continue;
                    }
                    --pagecount;
                    if (pagecount < 1)
                        pagecount = 1;
                    else if (pagecount > MAXPAGES)
                        pagecount = MAXPAGES;
                    topline = pagetopline[--pagecount];
                    done = 0;
                }
                else if (key == '?') {
                    /* '?' - help */
                    display_scroll_help(Left, Bottom, Right, Top, dy,
                        oldforeground, newbackground, SelectSW, FontSW);
                    done = 0;
                }
                else
                    /* else - scroll down */
                    topline = curline;
            }
            else {
                /* Mouse button pressed */
                if (x < Left || x > Right || y < Bottom || y > Top)
                    /* outside window - done */
                    done = 1;
                else if (button == mfb_buttonMask[3] && FontSW) {
                    /* Shift/MB1 - show font */
                    for (i = 0; i <= linecount; ++i)
                        if (y >= ypos[i])
                            break;
                    if (i > linecount) {
                        MFBAudio();
                        continue;
                    }
                    if (mygc == None)
                        mygc = XCreateGC(mfb_display,mfb_window, 0, 0);
                    if (strcmp(myfontname, List[topline + i]) != 0) {
                        strcpy(myfontname, List[topline + i]);
                        if (myfont != NULL)
                            XFreeFont(mfb_display, myfont);
                        myfont = XLoadQueryFont(mfb_display, myfontname);
                        XSetFont(mfb_display, mygc, myfont->fid);
                    }
                    MFBScrollFont(Left,Bottom,Right,Top, myfont, mygc,0,0);
                    done = 0;
                }
                else if (button == mfb_buttonMask[2]) {
                    /* MB3 - up */
                    if (topline == 0) {
                        MFBAudio();
                        continue;
                    }
                    --pagecount;
                    if (pagecount < 1)
                        pagecount = 1;
                    else if (pagecount > MAXPAGES)
                        pagecount = MAXPAGES;
                    topline = pagetopline[--pagecount];
                    done = 0;
                }
                else if (button == mfb_buttonMask[5]) {
                    /* Shift/MB3 - top */
                    if (topline == 0) {
                        MFBAudio();
                        continue;
                    }
                    pagecount = 0;
                    topline = 0;
                    done = 0;
                }
                else if (button == mfb_buttonMask[1]) {
                    /* MB2 - down */
                    if (done) {
                        MFBAudio();
                        continue;
                    }
                    topline = curline;
                }
                else if (button == mfb_buttonMask[4]) {
                    /* Shift/MB2 - bottom */
                    if (done) {
                        MFBAudio();
                        continue;
                    }
                    if (lastpage > 0) {
                        pagecount = lastpage;
                        if (pagecount > MAXPAGES) {
                            pagecount = MAXPAGES;
                            display_switch = False;
                        }
                        topline = pagetopline[--pagecount];
                    }
                    else {
                        topline = curline;
                        display_switch = False;
                    }
                }
                else if (button != mfb_buttonMask[0]) {
                    MFBAudio();
                    continue;
                }
                else if (SelectSW) {
                    /* MB1 & selecting */
                    for (i = 0; i <= linecount; ++i)
                        if (y >= ypos[i]) {
                            if (selection != topline + i)
                                selection = topline + i;
                            else
                                selection = -1;
                            done = 0;
                            break;
                        }
                    if (i > linecount)
                        /* scroll down */
                        topline = curline;
                }
                else
                    /* MB1 & not selecting - down */
                    topline = curline;
            }
            break;
        }
    }
    MFBSetFillPattern(oldfillpattern);
    return selection;
}


int
MFBScrollFont(Left,Bottom,Right,Top,FontST,FontGC,SelectSW,CursorSW)

int Left, Bottom, Right, Top;
XFontStruct *FontST; /* pointer to an X font structure */
GC FontGC;           /* GC to use for drawing this font */
int SelectSW;        /* if true, list items may be selected. The
                      * index (beginning with 0) of the selected List
                      * item will be returned if a selection is made,
                      * otherwise -1 will be returned. */
int CursorSW;        /* if true, Font is a cursor font (even number
                      * indices contain font bits, odd number
                      * indices contain font mask bits);
                      * show only the even-numbered entries */
/*
 * New routine created 08OCT89 by Peter C. Vernam, C. S. Draper Laboratory
 *
 * This routine performs much like MFBScrollList except that, instead of
 * scrolling through an array of character strings, a table of font
 * characters is scrolled.
 */
{
    char cbuf[81];
    int key;
    int i,j,n,x,y,button,x0,y0,xn,yn,dx,dy,dx2,dy2,fw,fh;
    int dx2m1,dy2m1;
    int topchar;         /* character at top-left of box */
    int curchar;         /* current character being displayed */
    int oldfillpattern;
    int oldforeground;
    int newbackground;
    int nlines;          /* number of lines that fit on a page */
    int nchars;          /* number of chars that fit on a line */
    int nchars_per_page; /* number of chars that fit on a page */
    int lastpage;        /* page number of the last page */
    int inc;
    int done;
    int selection = -1;  /* selection index return value */
    unsigned char1, charN, charn, charn1;
    int cleft;
    char ctext[9];
    XCharStruct mychar;
    int direct, ascent, descent;

    /* test to be sure of window area */
    if (Top < Bottom)
        MFBSwapInt(Top, Bottom);
    if (Right < Left)
        MFBSwapInt(Left, Right);

    /* calculate parameters */
    char1 = FontST->min_char_or_byte2;
    charN = FontST->max_char_or_byte2;
    charn = charn1 = charN;
    dx = FontST->ascent + FontST->descent;
    fw = mfb_fontWidth;
    fh = mfb_fontHeight;
    if (CursorSW) {
        topchar = char1;
        inc = 2;
        if (SelectSW) {
            charn1 += 2;
            charN += 4;
        }
        nchars = (Right - Left) / dx;
        dx = (Right - Left) / nchars;
        nlines = (Top - Bottom - fh) / dx;
    }
    else {
        topchar = 0;
        inc = 1;
        dx *= 2;
        nchars = MFBmin(32, (Right - Left - 3 * fw) / dx);
        for (i = 1; i <= 64; i <<= 1)
            if (nchars < i) {
                nchars = i >> 1;
                break;
            }
        nlines = (Top - Bottom - 3 * fh) / dx;
    }
    if (nlines <= 0)
        return (-1);
    nchars_per_page = nlines * nchars * inc;
    lastpage = charN / nchars_per_page + 1;
    dy = dx;
    dx2 = dx / 2;
    dy2 = dy / 2;
    dx2m1 = dx2 - 1;
    dy2m1 = dy2 - 1;
    x0 = Left + dx2; 
    y0 = Top  - dy2;
    xn = Left + nchars * dx;

    /* save old style ID's */
    oldforeground = mfb_fgColorId;
    oldfillpattern = mfb_fillPattern;
    MFBSetFillPattern(0);
    if (oldforeground == 0)
        newbackground = get_bg_color();
    else
        newbackground = 0;

    done = 0;

    while (!done ) {
        /* Display one page of characters from the Font */
        MFBSetColor(newbackground);
        MFBBox(Left, Bottom, Right, Top);    /* Erase the screen */
        MFBSetColor(oldforeground);
        XSetBackground(mfb_display, FontGC, mfb_colors[newbackground]);
        XSetForeground(mfb_display, FontGC, mfb_colors[oldforeground]);
        curchar = topchar;
        for (j = 0; j < nlines; ++j) {
            /* Loop for each line of page */
            y = y0 - j * dy;
            cleft = curchar;
            for (i = 0; i < nchars; ++i) {
                /* Loop for each char of line */
                x = x0 + i * dx;
                if (curchar >= char1) {
                    cbuf[0] = curchar;
                    if (curchar == selection) {
                        /* Selected character: display in reverse video */
                        MFBBox(x-dx2m1, y-dy2m1, x+dx2m1, y+dy2m1);
                        if (curchar <= charn) {
                            /* Font character or cursor */
                            XSetForeground(mfb_display, FontGC,
                                mfb_colors[newbackground]);
                            XDrawString(mfb_display, mfb_window, FontGC,
                                x, TRAN(y), cbuf, 1);
                            XSetForeground(mfb_display, FontGC,
                                mfb_colors[oldforeground]);
                        }
                        else if (curchar <= charn1) {
                            /* MFB cursor */
                            XSetBackground(mfb_display, FontGC,
                                mfb_colors[oldforeground]);
                            XSetForeground(mfb_display, FontGC,
                                mfb_colors[newbackground]);
                            XPutImage(mfb_display, mfb_window, FontGC,
                                mfb_cursorImage, 0, 0,
                                x-7, TRAN(y)-7, 16, 16);
                            XSetBackground(mfb_display, FontGC,
                                mfb_colors[newbackground]);
                            XSetForeground(mfb_display, FontGC,
                                mfb_colors[oldforeground]);
                        }
                        else {
                            /* Full-screen cross-hair */
                            MFBSetColor(newbackground);
                            MFBLine(x-dx2m1, y, x+dx2m1, y);
                            MFBLine(x, y-dy2m1, x, y+dy2m1);
                            MFBSetColor(oldforeground);
                        }
                    }
                    else {
                        /* Not selected: display normally */
                        if (curchar <= charn)
                            /* Font character or cursor */
                            XDrawString(mfb_display, mfb_window, FontGC,
                                x, TRAN(y), cbuf, 1);
                        else if (curchar <= charn1)
                            /* MFB cursor */
                            XPutImage(mfb_display, mfb_window, FontGC,
                                mfb_cursorImage, 0, 0,
                                x-7, TRAN(y)-7, 16, 16);
                        else {
                            /* Full-screen cross-hair */
                            MFBLine(x-dx2m1, y, x+dx2m1, y);
                            MFBLine(x, y-dy2m1, x, y+dy2m1);
                        }
                    }
                }
                curchar += inc;
                if (curchar > charN) {
                    done = 1;
                    break;
                }
            }
            if (!CursorSW) {
                sprintf(ctext, "%02X", cleft);
                MFBText(ctext, xn+fw, y-fh/2, 0);
            }
            y -= dy2;
            MFBLine(Left, y, xn, y);
            if (done)
                break;
        }
        yn = y - fh * 3 / 2;
        for (i = 0; i < nchars; ++i) {
            x = Left + i * dx;
            if (!CursorSW) {
                sprintf(ctext, "%02X", i);
                XTextExtents(mfb_font, ctext, strlen(ctext),
                    &direct, &ascent, &descent, &mychar);
                MFBText(ctext, x+dx2-mychar.width/2, yn, 0);
            }
            x += dx;
            MFBLine(x, Top, x, y);
        }
        if (done)
            strcpy(cbuf, "-DONE- (^U to scroll up, ? for help)");
        else
            strcpy(cbuf, "-MORE- (^U to scroll up, ^D to exit, ? for help)");
        MFBText(cbuf, Left, Bottom, 0);
        i = Left + XTextWidth(mfb_font, cbuf, strlen(cbuf));
        sprintf(cbuf,
            "Page %d of %d", topchar / nchars_per_page + 1, lastpage);
        j = Right - XTextWidth(mfb_font, cbuf, strlen(cbuf)) - 3;
        if (j < i)
            j = i;
        MFBText(cbuf, j, Bottom, 0);

        /* Wait for user interaction and perform requested function */
        for (;;) {
            MFBPoint(&x, &y, &key, &button);
            if (button == -1) {
                /* Keyboard key pressed */
                if (key == CTRLD)
                    /* <Ctrl/D> - done */
                    done = 1;
                else if (key == CTRLU) {
                    /* <Ctrl/U> - scroll up */
                    if (topchar == 0) {
                        MFBAudio();
                        continue;
                    }
                    else {
                        topchar -= nchars_per_page;
                        if (topchar < 0)
                            topchar = 0;
                    }
                    done = 0;
                }
                else if (key == '?') {
                    /* '?' - help */
                    display_scroll_help(Left, Bottom, Right, Top, fh,
                        oldforeground, newbackground, SelectSW, 0);
                    done = 0;
                }
                else
                    /* else - scroll down */
                    topchar = curchar;
            }
            else {
                /* Mouse button pressed */
                if (x < Left || x > Right || y < Bottom || y > Top)
                    /* outside window - done */
                    done = 1;
                else if (button == mfb_buttonMask[2]) {
                    /* MB3 - up */
                    if (topchar == 0) {
                        MFBAudio();
                        continue;
                    }
                    else {
                        topchar -= nchars_per_page;
                        if (topchar < 0)
                            topchar = 0;
                    }
                    done = 0;
                }
                else if (button == mfb_buttonMask[5]) {
                    /* Shift/MB3 - top */
                    if (topchar == 0) {
                        MFBAudio();
                        continue;
                    }
                    topchar = 0;
                    done = 0;
                }
                else if (button == mfb_buttonMask[1]) {
                    /* MB2 - down */
                    if (done) {
                        MFBAudio();
                        continue;
                    }
                    topchar = curchar;
                }
                else if (button == mfb_buttonMask[4]) {
                    /* Shift/MB2 - bottom */
                    if (done) {
                        MFBAudio();
                        continue;
                    }
                    else {
                        for (topchar = 0; topchar <= charN;
                            topchar += nchars_per_page)
                            ;
                        topchar -= nchars_per_page;
                    }
                }
                else if (SelectSW) {        /* MB1 & selecting */
                    j = (Top - y) / dy;
                    if (j >= nlines)
                        topchar = curchar;    /* scroll down */
                    else {
                        i = (x - Left) / dx;
                        n = (i + j * nchars + topchar) * inc;
                        if (n >= curchar) {
                            MFBAudio();
                            continue;
                        }
                        if (selection >= topchar && selection < curchar) {
                            if (selection == n) {
                                y = y0 - j * dy;
                                x = x0 + i * dx;
                            }
                            else {
                                int i, j, n;
                                n = (selection - topchar) / inc;
                                j = n / nchars;
                                i = n % nchars;
                                y = y0 - j * dy;
                                x = x0 + i * dx;
                            }
                            cbuf[0] = selection;
                            MFBSetColor(newbackground);
                            MFBBox(x-dx2m1, y-dy2m1, x+dx2m1, y+dy2m1);
                            if (selection <= charn) {
                                XSetForeground(mfb_display, FontGC,
                                    mfb_colors[oldforeground]);
                                XDrawString(mfb_display, mfb_window, FontGC,
                                    x, TRAN(y), cbuf, 1);
                            }
                            else if (selection <= charn1) {
                                XSetBackground(mfb_display, FontGC,
                                    mfb_colors[newbackground]);
                                XSetForeground(mfb_display, FontGC,
                                    mfb_colors[oldforeground]);
                                XPutImage(mfb_display, mfb_window, FontGC,
                                    mfb_cursorImage, 0, 0,
                                    x-7, TRAN(y)-7, 16, 16);
                            }
                            else {
                                MFBSetColor(oldforeground);
                                MFBLine(x-dx2m1, y, x+dx2m1, y);
                                MFBLine(x, y-dy2m1, x, y+dy2m1);
                            }
                        }
                        if (selection != n) {
                            y = y0 - j * dy;
                            x = x0 + i * dx;
                            cbuf[0] = n;
                            MFBSetColor(oldforeground);
                            MFBBox(x-dx2m1, y-dy2m1, x+dx2m1, y+dy2m1);
                            if (n <= charn) {
                                XSetForeground(mfb_display, FontGC,
                                    mfb_colors[newbackground]);
                                XDrawString(mfb_display, mfb_window, FontGC,
                                    x, TRAN(y), cbuf, 1);
                            }
                            else if (n <= charn1) {
                                XSetBackground(mfb_display, FontGC,
                                    mfb_colors[oldforeground]);
                                XSetForeground(mfb_display, FontGC,
                                    mfb_colors[newbackground]);
                                XPutImage(mfb_display, mfb_window, FontGC,
                                    mfb_cursorImage, 0, 0,
                                    x-7, TRAN(y)-7, 16, 16);
                            }
                            else {
                                MFBSetColor(newbackground);
                                MFBLine(x-dx2m1, y, x+dx2m1, y);
                                MFBLine(x, y-dy2m1, x, y+dy2m1);
                            }
                            selection = n;
                        }
                        else
                            selection = -1;
                        continue;
                     }
                }
                else                /* MB1 & not selecting - down */
                    topchar = curchar;
            }
            break;
        }
    }
    MFBSetFillPattern(oldfillpattern);
    return selection;
}


static int
get_bg_color()

/*
 * This routine returns the color index to use for the background when the
 * KIC background color is requested as the foreground color.  The new color
 * created is an off-white, much like the default background color for
 * DECterm and the Session Manager.
 *
 * This routine is called by MFBText(), MFBScroll(), MFBScrollList(), and
 * MFBScrollFont().
 */
{
    XColor mycolor;
    static int index = -1;

    if (index < 0) {
        if (MFBCurrent->privateColors) {
            index = mfb_maxColors - 2;
            if (index <= 35)
                index = 1;
            else {
                mycolor.pixel = mfb_colors[index];
                mycolor.red   = (235 * 65535) / 255;
                mycolor.green = (235 * 65535) / 255;
                mycolor.blue  = (215 * 65535) / 255;
                mycolor.flags = -1;
                XStoreColor(mfb_display, mfb_cmap, &mycolor);
            }
        }
        else {
            for (index = mfb_maxColors; index > 0; --index)
                if (mfb_colors[index] == -1)
                    break;
            if (index <= 0)
                index = 1;
            else {
                mycolor.red   = (235 * 65535) / 255;
                mycolor.green = (235 * 65535) / 255;
                mycolor.blue  = (215 * 65535) / 255;
                if (XAllocColor(mfb_display, mfb_cmap, &mycolor))
                    mfb_colors[index] = mycolor.pixel;
                else
                    index = 1;
            }
        }
    }
    return index;
}


static void
display_scroll_help(Left, Bottom, Right, Top, dy, fg, bg,
                SelectSW, FontSW)

int Left, Bottom, Right, Top, dy, fg, bg, SelectSW, FontSW;
{
    int key;
    int x, button;
    int y = Top;

    MFBSetColor(bg);
    MFBBox(Left, Bottom, Right, Top);
    MFBSetColor(fg);
    MFBText("From the keyboard:", Left, (y-=dy), 0);
    MFBText("  <Ctrl/D>      => exit", Left, (y-=dy), 0);
    MFBText("  <Ctrl/U>      => scroll up and stop at top",
                            Left, (y-=dy), 0);
    MFBText("  any other key => scroll down and exit when done",
                            Left, (y-=dy), 0);
    MFBText("Button clicks outside this text window:", Left, (y-=2*dy), 0);
    MFBText("  any button    => exit", Left, (y-=dy), 0);
    MFBText("Button clicks inside this text window:", Left, (y-=2*dy), 0);
    if (SelectSW)
        MFBText("  Button_1      => select item", Left, (y-=dy), 0);
    else
        MFBText("  Button_1      => scroll down and exit when done",
                            Left, (y-=dy), 0);
    MFBText("  Button_2      => scroll down and stop at bottom",
                            Left, (y-=dy), 0);
    MFBText("  Button_3      => scroll up and stop at top",
                            Left, (y-=dy), 0);
    if (FontSW)
        MFBText("  Shift/Button_1=> display font", Left, (y-=dy), 0);
    MFBText("  Shift/Button_2=> scroll to bottom", Left, (y-=dy), 0);
    MFBText("  Shift/Button_3=> scroll to top", Left, (y-=dy), 0);
    MFBText("press any key or button to continue", Left, (y-=2*dy), 0);
    MFBPoint(&x, &y, &key, &button);
}

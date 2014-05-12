/*************************************************************************
 XMFB X-Window MFB emulation package
 Authore: Peter C. Vernam, C. S. Draper Laboratory, 1989
          Stephen R. Whiteley, 1992
 *************************************************************************/

#include "mfb.h"        /* standard MFB header file */
#include <string.h>


extern unsigned short mfb_6X8font[];
extern unsigned short mfb_8X8font[];
extern unsigned short mfb_8X14font[];
extern unsigned short mfb_8X16font[];


void
MFBBeep(volume)

/*
 * Ring the bell at a percentage volume (0 - 100).
 */
int volume;
{
    if (volume > 0)
        XBell(mfb_display, volume * 2 - 100);
}


void
MFBRect(left,bottom,right,top)

/*
 * Displays a rectangle (the outline of a box) with the given dimensions
 * using the current ALU mode, fill pattern, and color.
 * Leaves the "pen" at right, top.
 */
int left,bottom,right,top;
{
    XDrawRectangle(mfb_display, mfb_window, mfb_lineGC,
        left, TRAN(top), right-left+1, top-bottom+1);
    mfb_X = right;
    mfb_Y = top;
}


void
MFBSetName(wname,iname)

/*
 * Sets the name in the title bar and the icon name.
 */
char *wname, *iname;
{
    XStoreName(mfb_display, mfb_window, wname);
    XSetIconName(mfb_display, mfb_window, iname);
}


int
MFBSelectFont(Left,Bottom,Right,Top)

/*
 * Calls MFBScrollList to display all the available font names, and
 * calls MFBSetFont to set a new font if one is selected.
 * Returns 1 (true) if a new font is selected, else 0 (false).
 */
int Left, Bottom, Right, Top;
{
    char **fontlist;
    int ifont, nfonts;
    int iret = 0;

    fontlist = XListFonts(mfb_display, "*", 5000, &nfonts);

    ifont =
        MFBScrollList(Left, Bottom, Right, Top, fontlist, nfonts, 1, 1, 1);
    if (ifont >= 0)
        iret = MFBSetFont(fontlist[ifont]);
    XFreeFontNames(fontlist);
    return iret;
}


int
MFBSetFont(fontname)

/*
 * Returns 1 (true) if specified font name exists, else 0 (false).
 */
char *fontname;
{
    XFontStruct    *myfont;

    if (!strcmp(fontname,"MFB6X8")) {
        mfb_chartab = mfb_6X8font;
        mfb_charwidth = 6;
        mfb_charheight = 8;
    }
    else if (!strcmp(fontname,"MFB8X8")) {
        mfb_chartab = mfb_8X8font;
        mfb_charwidth = 8;
        mfb_charheight = 8;
    }
    else if (!strcmp(fontname,"MFB8X14")) {
        mfb_chartab = mfb_8X14font;
        mfb_charwidth = 8;
        mfb_charheight = 14;
    }
    else if (!strcmp(fontname,"MFB8X16")) {
        mfb_chartab = mfb_8X16font;
        mfb_charwidth = 8;
        mfb_charheight = 16;
    }
    else {
        myfont = XLoadQueryFont(mfb_display, fontname);
        if (myfont != NULL) {
            XSetFont(mfb_display, mfb_lineGC, myfont->fid);
            XFreeFont(mfb_display, mfb_font);
            mfb_font    = myfont;
            mfb_fontWidth    = myfont->max_bounds.width;
            mfb_fontHeight    = myfont->ascent + myfont->descent;
            mfb_fontXOffset    = 0;
            mfb_fontYOffset    = myfont->descent;
            mfb_fontSize    = myfont->max_char_or_byte2 + 1;
            strncpy(mfb_fontName, fontname, 80);
            mfb_fontName[80] = '\0';
            return 1;
        }
        return 0;
    }
    return 1;
}


void
MFBSelectCursor(Left,Bottom,Right,Top)

/*
 * Calls MFBScrollFont to display all the available characters in the
 * cursor font "cursor", and calls MFBSetCursor to set the new cursor.
 */
int Left, Bottom, Right, Top;
{
    static XFontStruct *cursorFont = NULL;
    static GC    cursorGC;
    int index;

    if (cursorFont == NULL) {
        cursorFont = XLoadQueryFont(mfb_display, "cursor");
        cursorGC = XCreateGC(mfb_display, mfb_window, 0, 0);
        XSetFont(mfb_display, cursorGC, cursorFont->fid);
    }
    index =
        MFBScrollFont(Left, Bottom, Right, Top, cursorFont, cursorGC, 1, 1);
    if (index >= 0) {
        if (index <= cursorFont->max_char_or_byte2 + 2) {
            if (index > cursorFont->max_char_or_byte2)
                index = -1;
            MFBSetCursor(index);
            MFBSetFullScreenCursor(false);
        }
        else
            MFBSetFullScreenCursor(true);
    }
}


void
MFBSetCursor(index)

int index;
{
    XColor forg, bakg;
    int screen;

    if (mfb_cursor != mfb_cursorCross)
        XFreeCursor(mfb_display, mfb_cursor);
    if (index >= 0)
        mfb_cursor = XCreateFontCursor(mfb_display, index);
    else
        mfb_cursor = mfb_cursorCross;
    XDefineCursor(mfb_display, mfb_window, mfb_cursor);
    screen = DefaultScreen(mfb_display);
    forg.pixel = WhitePixel(mfb_display,screen);
    bakg.pixel = BlackPixel(mfb_display,screen);
    XQueryColor(mfb_display, mfb_cmap, &forg);
    XQueryColor(mfb_display, mfb_cmap, &bakg);
    XRecolorCursor(mfb_display, mfb_cursor, &forg, &bakg);
    mfb_cursorShape = index;
}

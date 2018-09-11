/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/
/*************************************************************************
 *
 * Routines for "More" text presentation.
 * 
 *
 *************************************************************************/

#include "prefix.h"
#include "kic.h"


static int MoreOn;        /* Turns on "more" mode, set/reset in InitMore() */
static int DidInitMore;   /* Set if successful initialize in "more" mode */
static char *MoreBuffer;  /* page storage, malloc'ed */
static char *MoreLinePtr; /* pointer to next line in storage */
static int MoreCols;      /* Number of colums to save */
static int MoreRows;      /* Number of rows to show */
static int MoreAt;        /* current row */

#ifdef __STDC__
static void init_more(void);
static int  more_prompt(void);
static void fixtabs(char*,char*);
#else
static void init_more();
static int  more_prompt();
static void fixtabs();
#endif


int
MoreLine(line)

/* Add a line to the buffer, show buffer if page is full */
char *line;
{
    char *s, c;

    if (!MoreOn) return (False);
    if (!DidInitMore) init_more();
    if (!DidInitMore) return (False); /* malloc failed */

    /* line is terminated with return code, have to strip for DOS
     * and UNIX
     */
    s = strchr(line,(char)0xd);
    if (s) *s = '\0';
    s = strchr(line,(char)0xa);
    if (s) *s = '\0';
    s = line;
    while (strlen(s) > MoreCols) {
        c = *(s + MoreCols);
        *(s + MoreCols) = '\0';
        fixtabs(MoreLinePtr,s);
        MoreLinePtr += MoreCols + 1;
        MoreAt++;
        if (MoreAt >= MoreRows) {
            MorePageDisplay();
            if (more_prompt()) return (True);
        }
        *(s + MoreCols) = c;
        s += MoreCols;
        if (!*s) return (False);
    }
    fixtabs(MoreLinePtr,s);
    MoreLinePtr += MoreCols + 1;
    MoreAt++;
    if (MoreAt >= MoreRows) {
        MorePageDisplay();
        if (more_prompt()) return (True);
    }
    return (False);
}


void
EnableMore(On)

int On;
{
    if (On)
        MoreOn = True;
    else {
        MoreOn = False;
        if (!DidInitMore) return;
        DidInitMore = False;
        free(MoreBuffer);
        MoreBuffer = NULL;
    }
}


int
RepaintMore()

/* redraw the screen following an expose event */
{
    if (!MoreOn)
        return (False);
    FBBegin(FB.fDisplay);
    InitViewport();
    InitCoarseWindow(View->kvCoarseWindow->kaX,View->kvCoarseWindow->kaY,
        (int)View->kvCoarseWindow->kaWidth);
    InitFineWindow(View->kvFineWindow->kaX,View->kvFineWindow->kaY);
    InitVLT();
    FBForeground(ERASE,0);
    FBFlood();
    ShowCommandMenu();
    ShowLayerTable();
    RedrawPrompt();
    FBKbRepaint((1+FB.fLastCursorColumn)*FB.fFontWidth,
        FB.fMaxY - FB.fFontHeight*(FB.fNumRows-2));
    (void)MorePageDisplay();
    return (True);
}


int
MorePageDisplay()

/* returns False if nothing to print */
{
    char *s;
    int Y;

    if (MoreLinePtr == MoreBuffer)
        return (False);
    EraseLargeCoarseViewport();
    FBForeground(DISPLAY,ColorTable[MoreTextColor].Ent);
    Y = View->kvLargeCoarseViewport->kaTop - FB.fFontHeight - 1;
    for (s = MoreBuffer; s < MoreLinePtr; s += MoreCols + 1) {
        FBText(PIXEL_COORDINATE,
            (View->kvLargeCoarseViewport->kaLeft+7),Y,s);
        Y -= FB.fFontHeight;
    }
    return (True);
}


static void
init_more()

{
    int i;
    extern int hlp_width;

    MoreAt = 0;
    MoreRows = (View->kvLargeCoarseViewport->kaTop -
        View->kvLargeCoarseViewport->kaBottom)/FB.fFontHeight - 2;
    MoreCols = (View->kvLargeCoarseViewport->kaRight -
        View->kvLargeCoarseViewport->kaLeft - 8)/FB.fFontWidth;
    if (MoreBuffer == NULL) {
        MoreBuffer = malloc(MoreRows*(MoreCols+1));
        if (MoreBuffer == NULL) {
            ShowPrompt("Error, Out of memory!");
            return;
        }
    }
    MoreLinePtr = MoreBuffer;
    for (i = 0; i < MoreRows; i++) {
        *MoreLinePtr = '\0';
        MoreLinePtr += MoreCols + 1;
    }
    MoreLinePtr = MoreBuffer;
 
    DidInitMore = True;
    hlp_width = MoreCols;
}


static int
more_prompt()

{
    int i;
    char *s, InChar;

    FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
    FBText(PIXEL_COORDINATE,View->kvLargeCoarseViewport->kaLeft+7,
        View->kvLargeCoarseViewport->kaBottom+1,
        "      --More--");
    InChar = FBGetchar(ERASE);
    (void)InChar;

    MoreAt = 0;
    s = MoreBuffer;
    for (i = 0; i < MoreRows; i++) {
        *s = '\0';
        s += MoreCols + 1;
    }
    MoreLinePtr = MoreBuffer;

    /* can't do this */
    /*
    if (InChar == ESCAPE || InChar == 'q')
        return (True);
    */
    return (False);
}


static void
fixtabs(dst,src)

char *src, *dst;
{
    char *s, *t;
 
    for (s = src, t = dst; *s; ) {
         if (*s == '\t') {
             while ((t+1-dst)%8) *t++ = ' ';
             *t++ = ' ';
             s++;
         }
         else {
             *t++ = *s++;
         }
    }       
    *t = '\0';
}

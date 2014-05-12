/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
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

/*
 * KIC label management.
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuLABEL;
extern char *MenuUNDO;


/* Label transform field:
 * bits 0-1 : 0-no rotation, 1-90, 2-180, 3-270
 * bit 2 : mirror y
 * bit 3 : mirror x
 */

#ifdef __STDC__
static void redisplay_label(struct o*);
static void delete_label(struct o*);
static char get_current_xform(void);
#else
static void redisplay_label();
static void delete_label();
static char get_current_xform();
#endif

void
Label(LookedAhead)

int *LookedAhead;
{
    struct o *Pointer;
    char *c,*Label = NULL;
    int NumLabelsMade = 0;
    int X = 0,Y = 0;

    MenuSelect(MenuLABEL);
    ShowPrompt("Enter label: ");
    Label = FBEdit(NULL);
    if (Label == NULL Or *Label == '\0')
        goto quit;

    for (c = Label; *c; c++)
        if (isspace(*c)) *c = '_';
    loop {
        ShowPrompt("Point to where you want the label.");
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            if (NumLabelsMade == 0)
                goto quit;
            MenuSelect(MenuUNDO);
            delete_label(Pointer);
            NumLabelsMade--;
            MenuDeselect(MenuUNDO);
            continue;
        case PL_PCW:
            X = KicCursor.kcX;
            Y = KicCursor.kcY;
            if (Not CDMakeLabel(Parameters.kpCellDesc,Parameters.kpLayer,
                Label,X,Y,get_current_xform(),&Pointer)) MallocFailed();
            redisplay_label(Pointer);
            NumLabelsMade++;
        }
    }
quit:
    if (NumLabelsMade > 0)
        Parameters.kpModified = True;
    MenuDeselect(MenuLABEL);
    ErasePrompt();
}


void
BBLabel(Window,Pointer,BB)

/* Return the effective BB of the label as seen in Window. */
struct ka *Window;
struct o *Pointer;
struct ka *BB;
{
    struct la *LDesc;
    int Len,Hei;
    char Xform;

    LDesc = (struct la *)Pointer->oRep;
    Xform = LDesc->laXform;
    FBTextBB(LDesc->laLabel,&Len,&Hei);

    if (Parameters.kpDoingHardcopy) {
        Len *= Parameters.kpHardcopyTextScale;
        Hei *= Parameters.kpHardcopyTextScale;
    }
    if (Window == View->kvCoarseWindow) {
        Len /= View->kvCoarseRatio;
        Hei /= View->kvCoarseRatio;
    }
    else {
        Len /= View->kvFineRatio;
        Hei /= View->kvFineRatio;
    }
    BB->kaLeft = LDesc->laX;
    BB->kaBottom = LDesc->laY;

    if (Xform & 1) {
        if (Xform & 2) {
            if (!(Xform & 4))
                BB->kaBottom -= Len;
            if (Xform & 8)
                BB->kaLeft -= Hei;
        }
        else {
            if (!(Xform & 8))
                BB->kaLeft -= Hei;
            if (Xform & 4)
                BB->kaBottom -= Len;
        }
        BB->kaRight = BB->kaLeft + Hei;
        BB->kaTop = BB->kaBottom + Len;
    }
    else {
        if (Xform & 2) {
            if (!(Xform & 8))
                BB->kaLeft -= Len;
            if (!(Xform & 4))
                BB->kaBottom -= Hei;
        }
        else {
            if (Xform & 8)
                BB->kaLeft -= Len;
            if (Xform & 4)
                BB->kaBottom -= Hei;
        }
        BB->kaRight = BB->kaLeft + Len;
        BB->kaTop = BB->kaBottom + Hei;
    }
}


void
CDLabelBB(Pointer,L,B,R,T)

/* called from CD */
struct o *Pointer;
int *L,*B,*R,*T;
{
    struct ka BB;

    BBLabel(View->kvCoarseWindow,Pointer,&BB);
    *L = BB.kaLeft;
    *B = BB.kaBottom;
    *R = BB.kaRight;
    *T = BB.kaTop;
}


void
ShowLabel(Layer,Label,X,Y,Xform,Flag)

int Layer;
char *Label;
int X,Y;
int Xform;
int Flag;      /* If Flag, label is always displayed */
{

    int TF[9],XT,YT;
    char *c;

    TPoint(&X,&Y);

    /* have to fix transform */
    TPush();
    TIdentity();
    if (Xform & 4) TMY();
    if (Xform & 8) TMX();
    Xform &= 3;
    if (Xform != 0) {
        if (Xform == 1)   TRotate(0L,1L);
        elif (Xform == 2) TRotate(-1L,0L);
        elif (Xform == 3) TRotate(0L,-1L);
    }
    TPremultiply();
    TCurrent(TF);
    Xform = SetXform(TF);
    TPop();

    strcpy(TypeOut,Label);
    c = TypeOut;
    while ((c = strchr(c,'_')) != NULL) *c = ' ';

    FBForeground(DISPLAY,Layer);
    FBSetFillPattern(0);
    if ((Flag Or 1/View->kvCoarseRatio < HALFRESOLUTION) And
        Parameters.kpRedisplayControl != FINEVIEWPORTONLY) {

        CoarseLToP(X,Y,XT,YT);
        FBSetTextClip(
            View->kvCoarseViewport->kaLeft,
            View->kvCoarseViewport->kaBottom,
            View->kvCoarseViewport->kaRight,
            View->kvCoarseViewport->kaTop);

        if (Parameters.kpDoingHardcopy)
            FBScaledText(TypeOut,XT,YT,Xform,
                Parameters.kpHardcopyTextScale);
        else
            FBScaledText(TypeOut,XT,YT,Xform,1);
    }
    if (Parameters.kpRedisplayControl != COARSEVIEWPORTONLY) {

        FineLToP(X,Y,XT,YT);
        FBSetTextClip(
            View->kvFineViewport->kaLeft,
            View->kvFineViewport->kaBottom,
            View->kvFineViewport->kaRight,
            View->kvFineViewport->kaTop);

        FBScaledText(TypeOut,XT,YT,Xform,1);
    }
    FBSetFillPattern(LayerTable[Layer].klStyleID);
    FBSetTextClip(0,0,FB.fMaxX,FB.fMaxY);
}


char
SetXform(TF)
int *TF;
{
    int A,B,C,D;

    /*
     * Take the transformation defined in TF and set
     * the returned bit field accordingly.
     *
     *                  | a    c    0  |
     * Transform = TM = | b    d    0  |
     *                  | TX   TY   1  |
     *
     * A = TM[0][0] = TF[0];
     * B = TM[1][0] = TF[3];
     * C = TM[0][1] = TF[1];
     * D = TM[1][1] = TF[4];
     * TX = TM[2][0] = TF[6];
     * TY = TM[2][1] = TF[7];
     */

    A = TF[0]; B = TF[3]; C = TF[1]; D = TF[4];

    if (A == 0 && D == 0) {
        if (B == 1 && C == 1)   /* MX R 0 -1 */
            return (11);

        if (B == -1 && C == -1) /* MX R 0 1 */
            return (9);

        if (B == 1 && C == -1)  /* R 0 -1 */
            return (3);

        if (B == -1 && C == 1)  /* R 0 1 */
            return (1);
    }
    if (B == 0 && C == 0) {

        if (A == 1 && D == 1)   /* translate only */
            return (0);

        if (A == -1 && D == -1) /* R -1 0 */
            return (2);

        if (A == -1 && D == 1)  /* MX */
            return (8);
    }

    /* MY */
    return (4);
}


static void
redisplay_label(Pointer)

struct o *Pointer;
{
    struct ka BB;
    int OldRD;

    if (Pointer->oType != CDLABEL) return;

    OldRD = Parameters.kpRedisplayControl;

    if (OldRD != FINEVIEWPORTONLY) {
        Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
        BBLabel(View->kvCoarseWindow,Pointer,&BB);
        Redisplay(&BB);
    }
    if (OldRD != COARSEVIEWPORTONLY) {
        Parameters.kpRedisplayControl = FINEVIEWPORTONLY;
        BBLabel(View->kvFineWindow,Pointer,&BB);
        Redisplay(&BB);
    }
    Parameters.kpRedisplayControl = OldRD;
}


static void
delete_label(Pointer)

struct o *Pointer;
{

    struct ka BBC,BBF;
    char OldRd;

    if (Pointer->oType != CDLABEL) return;
    OldRd = Parameters.kpRedisplayControl;

    if (OldRd != FINEVIEWPORTONLY) {
        Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;

        BBLabel(View->kvCoarseWindow,Pointer,&BBC);

        TPoint(&BBC.kaLeft,&BBC.kaBottom);
        TPoint(&BBC.kaRight,&BBC.kaTop);

        EraseBox(&BBC);
    }
    if (OldRd != COARSEVIEWPORTONLY) {
        Parameters.kpRedisplayControl = FINEVIEWPORTONLY;

        BBLabel(View->kvFineWindow,Pointer,&BBF);

        TPoint(&BBF.kaLeft,&BBF.kaBottom);
        TPoint(&BBF.kaRight,&BBF.kaTop);

        EraseBox(&BBF);
    }

    CDDelete(Parameters.kpCellDesc,Pointer);

    if (OldRd != FINEVIEWPORTONLY) {
        Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
        Redisplay(&BBC);
    }
    if (OldRd != COARSEVIEWPORTONLY) {
        Parameters.kpRedisplayControl = FINEVIEWPORTONLY;
        Redisplay(&BBF);
    }
    Parameters.kpRedisplayControl = OldRd;
}


static char
get_current_xform()

{
    char c;

    c = (char)0;
    c += Parameters.kpRotationAngle/90;
    if (Parameters.kpMY) c |= 4;
    if (Parameters.kpMX) c |= 8;
    return (c);
}

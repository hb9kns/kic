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
 * KIC instance menu.
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuPLACE;
extern char *MenuLLREF;
extern char *MenuUNDO;
extern char *MenuARRAY;
extern char *MenuCRSYM;

#define Mtest(zap) if((zap) == 0) MallocFailed()

struct rd {
    struct s *Desc;
    char Name[82];
};
static struct rd CurInstance;

#ifdef __STDC__
static int locate_array(int*);
static struct o *make_array(int,int);
#else
static int locate_array();
static struct o *make_array();
#endif


void
Place(LookedAhead)

int *LookedAhead;
{
    char *TypeIn;

    MenuSelect(MenuPLACE);
    sprintf(TypeOut,"Current master cell is %s. New master? ",
        *CurInstance.Name == '\0' ? "unspecified" : CurInstance.Name);
    ShowPrompt(TypeOut);
    TypeIn = FBEdit(CurInstance.Name);
    if (TypeIn == NULL) {
        ErasePrompt();
        MenuDeselect(MenuPLACE);
        return;
    }
    else {
        if (*TypeIn != '\0' && *TypeIn != '\n')
            strcpy(CurInstance.Name,TypeIn);
        if (*CurInstance.Name == '\0') {
            MenuDeselect(MenuPLACE);
            return;
        }
    }

    /* strip off any path prefix */
    if (FixCellName(CurInstance.Name)) {
        ShowPromptAndWait("Warning: internal path overflow");
    }

    if (OpenCell(CurInstance.Name,&CurInstance.Desc)) {
        ShowPrompt("Bad subcell name, not found or error.");
        *CurInstance.Name = '\0';
        MenuDeselect(MenuPLACE);
        return;
    }
    ShowPrompt("Point to locations to place cell.");

    if (locate_array(LookedAhead))
        Parameters.kpModified = True;
    ErasePrompt();
    MenuDeselect(MenuPLACE);
}


static int
locate_array(LookedAhead)

int *LookedAhead;
{
    struct o *Pointer = NULL;
    struct ka BB;
    int X = 0,Y = 0;
    int Undo = False;
    int modified = 0;

    FBSetRubberBanding('p');

    loop {
        switch (PointLoop(LookedAhead)) {

        case PL_CMD:
        case PL_ESC:
            goto quit;

        case PL_UND:
            *LookedAhead = False;
            if (Pointer == NULL)
                continue;
            MenuSelect(MenuUNDO);
            if (Undo == False) {
                CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
                    &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);
                EraseBox(&BB);
                CDDelete(Parameters.kpCellDesc,Pointer);
                Redisplay(&BB);
                modified--;
                Undo = True;
            }
            else {
                FBSetRubberBanding(0);
                Pointer = make_array(X,Y);
                FBSetRubberBanding('p');
                modified++;
                Undo = False;
            }
            MenuDeselect(MenuUNDO);
            continue;

        case PL_PCW:
            X = KicCursor.kcX;
            Y = KicCursor.kcY;
            FBSetRubberBanding(0);
            Pointer = make_array(X,Y);
            FBSetRubberBanding('p');
            modified++;
            Undo = False;
            continue;
        }
    }

quit:
    FBSetRubberBanding(0);
    return (modified);
}


void
Handle()

{
    if (Parameters.kpSubrefLowerLeft) {
        Parameters.kpSubrefLowerLeft = False;
        MenuDeselect(MenuLLREF);
    }
    else {
        Parameters.kpSubrefLowerLeft = True;
        MenuSelect(MenuLLREF);
    }
}


/* ARGSUSED */
void
ShowNewInstance(X,Y,RefX,RefY)

int X,Y,RefX,RefY;
{
    struct ka BB, BB1;
    int TFold[9];
    int XX,YY;
    int DX,DY;
    int NumX,NumY,Int1,Int2;

    TCurrent(TFold);
    NumX = Parameters.kpNumX;
    NumY = Parameters.kpNumY;
    DX = Parameters.kpDX;
    DY = Parameters.kpDY;
    CDBB(CurInstance.Desc,(struct o *)NULL,&BB.kaLeft,
        &BB.kaBottom,&BB.kaRight,&BB.kaTop);

    if (Parameters.kpSubrefLowerLeft) {
        XX = BB.kaLeft;
        YY = BB.kaBottom;
    }
    else {
        XX = 0;
        YY = 0;
    }

    TPush();
    TIdentity();
    SetNewTransform(XX,YY,X,Y);
    TPremultiply();

    for (Int1 = NumY-1; Int1 >= 0; --Int1) {
        for (Int2 = 0; Int2 < NumX; ++Int2 ){
            TPush();
            TIdentity();
            TTranslate(Int2*DX,Int1*DY);
            TPremultiply();
            BB1.kaLeft = BB.kaLeft;
            BB1.kaRight = BB.kaRight;
            BB1.kaBottom = BB.kaBottom;
            BB1.kaTop = BB.kaTop;
            TPoint(&BB1.kaLeft,&BB1.kaBottom);
            TPoint(&BB1.kaRight,&BB1.kaTop);
            TLoadCurrent(TFold);
            ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB1);
            TPop();
        }
    }
    TPop();
}


void
MakeInstance(LookedAhead,name)

int *LookedAhead;
char *name;
{
    int NumX,NumY;
    char buf[82];
    struct s *stmp;

    NumX = Parameters.kpNumX;
    NumY = Parameters.kpNumY;
    strcpy(buf,CurInstance.Name);
    stmp = CurInstance.Desc;

    Parameters.kpNumX = 1;
    Parameters.kpNumY = 1;
    strcpy(CurInstance.Name,name);
    if (OpenCell(CurInstance.Name,&CurInstance.Desc)) {
        ShowPrompt("Bad subcell name, not found or error.");
    }
    else {
        if (locate_array(LookedAhead))
            Parameters.kpModified = True;
    }
    Parameters.kpNumX = NumX;
    Parameters.kpNumY = NumY;
    strcpy(CurInstance.Name,buf);
    CurInstance.Desc = stmp;
}


static struct o*
make_array(X,Y)

int X,Y;
{
    struct o *Pointer;
    struct ka BB;
    int NumX,NumY;
    int L,R,T,B;
    double DX,DY;

    /*
     * Create instance array.
     */

    NumX = Parameters.kpNumX;
    NumY = Parameters.kpNumY;
    DX = Parameters.kpDX;
    DY = Parameters.kpDY;

    CDBB(CurInstance.Desc,(struct o *)NULL,&L,&B,&R,&T);

    if (Not CDBeginMakeCall(Parameters.kpCellDesc,CurInstance.Name,NumX,
        R-L+DX*RESOLUTION,NumY,T-B+DY*RESOLUTION,&Pointer)) {
        MallocFailed();
    }

    if (Parameters.kpSubrefLowerLeft) {
        /* Translation so cell is always placed with lower left corner
         * at reference point.
         */
        Mtest(CDT(Pointer,CDTRANSLATE,-L,-B));
    }

    if (Parameters.kpMX) {
        Mtest(CDT(Pointer,CDMIRRORX,0L,0L));
    }
    if (Parameters.kpMY) {
        Mtest(CDT(Pointer,CDMIRRORY,0L,0L));
    }
    if (Parameters.kpRotationAngle == 90) {
        Mtest(CDT(Pointer,CDROTATE,0L,1L));
    }
    elif (Parameters.kpRotationAngle == 180) {
        Mtest(CDT(Pointer,CDROTATE,-1L,0L));
    }
    elif (Parameters.kpRotationAngle == 270) {
        Mtest(CDT(Pointer,CDROTATE,0L,-1L));
    }
    if (X || Y)
        Mtest(CDT(Pointer,CDTRANSLATE,X,Y));

    Mtest(CDEndMakeCall(Parameters.kpCellDesc,Pointer));

    /* CDBB will always return True if Pointer != NULL */
    CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
        &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);
    EraseBox(&BB);
    Redisplay(&BB);
    return (Pointer);
}


void
GetArraySpec()

{
    char *TypeIn;

    MenuSelect(MenuARRAY);
    sprintf(TypeOut,"Number X? (currently %d): ",Parameters.kpNumX);
    ShowPrompt(TypeOut);
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL)
        sscanf(TypeIn,"%d",&Parameters.kpNumX); 
    if (Parameters.kpNumX < 1 Or Parameters.kpNumX > 1000) {
        ShowPromptAndWait("Sorry, outside range 1 - 1000, set to 1.");
        Parameters.kpNumX = 1;
    }
    sprintf(TypeOut,"Number Y? (currently %d): ",Parameters.kpNumY);
    ShowPrompt(TypeOut);
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL)
        sscanf(TypeIn,"%d",&Parameters.kpNumY);
    if (Parameters.kpNumY < 1 Or Parameters.kpNumY > 1000) {
        ShowPromptAndWait("Sorry, outside range 1 - 1000, set to 1.");
        Parameters.kpNumY = 1;
    }
    if (Parameters.kpNumX > 1 Or Parameters.kpNumY > 1) {
        sprintf(TypeOut,"X Spacing? (currently %g): ",Parameters.kpDX);
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (TypeIn != NULL)
            sscanf(TypeIn,"%lg",&Parameters.kpDX); 
        sprintf(TypeOut,"Y Spacing? (currently %g): ",Parameters.kpDY);
        ShowPrompt(TypeOut);
        TypeIn = FBEdit(NULL);
        if (TypeIn != NULL)
            sscanf(TypeIn,"%lg",&Parameters.kpDY);
    }
    ErasePrompt();
    MenuDeselect(MenuARRAY);
}


void
NewSymbol()

/*
 * Create a new symbol from the contents of the selection queue.
 */
{
    struct ks *SQDesc;
    struct o *Pointer;
    struct t *TGen;
    struct p *Path;
    FILE *FileDesc;
    char *TypeIn;
    char *Label;
    char Type,Xform;
    char *SymbolName;
    int Layer, new;
    int NumX,NumY;
    int X,Y,DX,DY,Width,Length;
    int Xo, Yo;

    MenuSelect(MenuCRSYM);
    if (SelectQHead == NULL) {
        ShowPrompt("Objects must be selected first.");
        MenuDeselect(MenuCRSYM);
        return;
    }
    ShowPrompt("Name of new symbol? ");
    if ((TypeIn = FBEdit(NULL)) == NULL Or *TypeIn == EOS) {
        ErasePrompt();
        MenuDeselect(MenuCRSYM);
        return;
    }
    if ((FileDesc = POpen(TypeIn,"w",(char *)NULL,(char **)NULL)) == NULL) {
        sprintf(TypeOut,"Can't create symbol %s.",TypeIn);
        ShowPrompt(TypeOut);
        MenuDeselect(MenuCRSYM);
        return;
    }
    SQComputeBB();
    /* The new cell origin will be the lower left corner */
    Xo = SelectQBB.kaLeft;
    Yo = SelectQBB.kaBottom;
    fprintf(FileDesc,"(Symbol %s);\n",TypeIn);
    fprintf(FileDesc,"9 %s;\n",TypeIn);
    GenBeginSymbol(FileDesc,0,1L,1L);

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        Pointer = SQDesc->ksPointer;
        if (Pointer->oType == CDSYMBOLCALL) {

            CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);
            /* add symbol name extension */
            fprintf(FileDesc,"9 %s;\n",SymbolName);
            /* forget about property list info */
            /* add symbol array extension */
            if (NumX != 1 Or NumY != 1)
                fprintf(FileDesc,"1 Array %d %d %d %d;\n",NumX,DX,NumY,DY);
            fprintf(FileDesc,"C 0");
            CDInitTGen(Pointer,&TGen);
            loop {
                CDTGen(&TGen,&Type,&X,&Y);
                if (TGen == NULL) {
                    fprintf(FileDesc,";\n");
                    break;
                }
                elif (Type == CDROTATE)
                    fprintf(FileDesc," R %d %d",X,Y);
                elif (Type == CDTRANSLATE)
                    fprintf(FileDesc," T %d %d",X-Xo,Y-Yo);
                elif (Type == CDMIRRORX)
                    fprintf(FileDesc," MX");
                elif (Type == CDMIRRORY)
                    fprintf(FileDesc," MY");
            }
        }
    }

    new = True;
    for (Layer = 1; Layer <= CDNUMLAYERS; ++Layer, new = True) {
        for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
            Pointer = SQDesc->ksPointer;
            if (Pointer->oLayer == Layer) {
                if (new) {
                    GenLayer(FileDesc,CDLayer[Layer].lTechnology,
                        CDLayer[Layer].lMask);
                    new = False;
                }
                Type = Pointer->oType;

                if (Type == CDWIRE) {
                    CDWire(Pointer,&Layer,&Width,&Path);
                    GenWireOffset(FileDesc,Width,Path,Xo,Yo);
                }
                elif (Type == CDPOLYGON) {
                    CDPolygon(Pointer,&Layer,&Path);
                    GenPolygonOffset(FileDesc,Path,Xo,Yo);
                }
                elif (Type == CDLABEL) {
                    CDLabel(Pointer,&Layer,&Label,&X,&Y,&Xform);
                    fprintf(FileDesc,"94 %s %d %d %d",Label,
                        X-Xo,Y-Yo,Xform);
                    fprintf(FileDesc,";\n");
                }
                elif (Type == CDBOX) {
                    CDBox(Pointer,&Layer,&Length,&Width,&X,&Y);
                    GenBox(FileDesc,Length,Width,X-Xo,Y-Yo,1,0);
                }
            }
        }
    }
    GenEndSymbol(FileDesc);
    GenEnd(FileDesc);
    fclose(FileDesc);
    sprintf(TypeOut,"New symbol %s created and saved.",TypeIn);
    MenuDeselect(MenuCRSYM);
    ShowPrompt(TypeOut);
}


int
OpenCell(Master,MasterDesc)

char *Master;
struct s **MasterDesc;
{
    if (Not CDOpen(Master,MasterDesc,'r')) {
        if (CDStatusInt == CDPARSEFAILED) {
            sprintf(TypeOut,"Can't display cell %s. MORE",Master); 
            ShowPrompt(TypeOut);
            (void)FBGetchar(ERASE);
            ShowPrompt(CDStatusString);
        }
        if (CDStatusInt == CDMALLOCFAILED)
            MallocFailed();
        return (True);
    }
    if (CDStatusInt == CDNEWSYMBOL) {
        sprintf(TypeOut,
        "Can't display cell %s, because it doesn't seem to be around.",
        Master); 
        ShowPrompt(TypeOut);
        return (True);
    }
    return (False);
}

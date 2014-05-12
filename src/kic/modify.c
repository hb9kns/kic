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
 *  Stretch command
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuSTRCH;
extern char *MenuUNDO;
extern char *MenuTBRL;
extern char *MenuTB;
extern char *MenuRL;

#ifdef __STDC__
static void do_stretch(int,int,int,int,struct ka*,int);
static struct p *get_nearest_vertex(struct p*,int,int);
static void change_nearest_vertex(struct p*,int,int,int,int,int);
static void change_rect_vertex(struct ka*,struct ka*,int,int,int,int);
static int  set_ref_to_vertex(int*,int*);
#else
static void do_stretch();
static struct p *get_nearest_vertex();
static void change_nearest_vertex();
static void change_rect_vertex();
static int  set_ref_to_vertex();
#endif

static int RCode;


void
Stretch(LookedAhead)

int *LookedAhead;
{
    struct ka NBB,OBB;
    int RefX = 0,RefY = 0,LastRefX,LastRefY,MapX = 0,MapY = 0;
    int RefTmpX, RefTmpY;
    int FirstTime = True;
    int GotOne = False;
    int Undo = False;
    int Modified = 0;
    int LastCode;
    int Pt;
    char Types[4];

    MenuSelect(MenuSTRCH);

    Types[0] = CDPOLYGON;
    Types[1] = CDWIRE;
    Types[2] = CDBOX;
    Types[3] = '\0';

    if (AreTypesInQ(Types))
        GotOne = True;

    loop {
top:
        if (Not GotOne) {
            ShowPrompt("Point at geometry to stretch.");

            switch (PointLoop(LookedAhead)) {
            case PL_ESC:
            case PL_CMD:
                goto quit;
            case PL_UND:
                if (FirstTime) goto quit;
                if (Undo  == False) {
                    MenuSelect(MenuUNDO);
                    SQRestore(True);
                    /* restored objects have Info = SQ_NEWSEL */
                    Modified--;
                    Undo = True;
                    NBB = OBB;
                    EraseBox(&OBB);
                    Redisplay(&OBB);
                    MenuDeselect(MenuUNDO);
                    break;
                }
                else {
                    /* should have only Info = SQ_NEW objects here */
                    do_stretch(RefX,RefY,MapX,MapY,&NBB,False);
                    Modified++;
                    EraseBox(&NBB);
                    Redisplay(&NBB);
                    Undo = False;
                    MenuDeselect(MenuUNDO);
                    continue;
                }
            case PL_PCW:
                SelectTypes(Types);
                if (Not AreTypesInQ(Types))
                    goto top;
                SQComputeBB();
                NBB = SelectQBB;
            }
        }

next:
        ShowPrompt("Point to the reference vertex.");

        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            MenuSelect(MenuUNDO);
            if (Not GotOne)
                 SQDesel(Types);
                /* newly selected objects deleted, restored objects
                 * have Info = SQ_NEW.
                 */
            else {
                if (FirstTime) {
                    MenuDeselect(MenuUNDO);
                    goto quit;
                }
                if (Undo == False) {
                    SQRestore(True);
                    /* restored objects have Info = SQ_NEWSEL */
                    Modified--;
                    Undo = True;
                }
                else {
                    /* should have only Info = SQ_NEWSEL objects here */
                    do_stretch(RefX,RefY,MapX,MapY,&NBB,True);
                    Modified++;
                    Undo = False;
                }
            }
            EraseBox(&NBB);
            Redisplay(&NBB);
            MenuDeselect(MenuUNDO);
            goto top;
        }

        LastRefX = RefX;
        LastRefY = RefY;
        RefX = KicCursor.kcX;
        RefY = KicCursor.kcY;
        LastCode = RCode;
        RCode = set_ref_to_vertex(&RefX,&RefY);
        SetRelative(RefX,RefY,True);

        ShowPrompt("Point to where it should stretch.");

        RefTmpX = KicCursor.kcX;
        RefTmpY = KicCursor.kcY;
        KicCursor.kcX = RefX;
        KicCursor.kcY = RefY;
        FBSetRubberBanding('s');
        KicCursor.kcX = RefTmpX;
        KicCursor.kcY = RefTmpY;
        Pt = PointLoop(LookedAhead);
        FBSetRubberBanding(0);

        switch (Pt) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
            MenuSelect(MenuUNDO);
            RefX = LastRefX;
            RefY = LastRefY;
            RCode = LastCode;
            MenuDeselect(MenuUNDO);
            goto next;
        }

        SQRestore(False);
        /* should have only Info = SQ_OLDSEL objects here */
        FirstTime = False;
        SetRelative(0L,0L,False);

        MapX = KicCursor.kcX;
        MapY = KicCursor.kcY;

        do_stretch(RefX,RefY,MapX,MapY,&NBB,GotOne);
        EraseBox(&NBB);
        Redisplay(&NBB);
        OBB = NBB;
        Modified++;
        Undo = False;
    }

quit:
    SQRestore(False);
    if (Not GotOne And AreTypesInQ(Types)) {
        SQComputeBB();
        SQDesel(Types);
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
    }
    if (Modified)
        Parameters.kpModified = True;
    SetRelative(0L,0L,False);
    ErasePrompt();
    MenuDeselect(MenuUNDO);
    MenuDeselect(MenuSTRCH);
}


void
SetStretchMode()

{
    if (strcmp(Parameters.kpCommand,MenuTBRL) == 0) {
        AlterMenuEntries(MenuTBRL,MenuTB);
        MenuSelect(MenuTB);
        Parameters.kpStretchType = STR_TB;
        return;
    }
    if (strcmp(Parameters.kpCommand,MenuTB) == 0) {
        AlterMenuEntries(MenuTB,MenuRL);
        MenuSelect(MenuRL);
        Parameters.kpStretchType = STR_RL;
        return;
    }
    if (strcmp(Parameters.kpCommand,MenuRL) == 0) {
        AlterMenuEntries(MenuRL,MenuTBRL);
        MenuDeselect(MenuTBRL);
        Parameters.kpStretchType = STR_TBRL;
        return;
    }
}


static void
do_stretch(RefX,RefY,MapX,MapY,NBB,SelectNew)

int RefX,RefY,MapX,MapY;
struct ka *NBB;
int SelectNew;
{
    struct prpty *PrptyDesc;
    struct ks *SQDesc;
    struct ka OBB,BB;
    struct o *Pointer;
    struct p *Path;
    int X,Y,DX,DY;
    int Layer;

    NBB->kaLeft   = CDINFINITY;
    NBB->kaRight  = -CDINFINITY;
    NBB->kaBottom = CDINFINITY;
    NBB->kaTop    = -CDINFINITY;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oInfo == SQ_GONE) continue;

        Pointer = NULL;
        if (SQDesc->ksPointer->oType == CDWIRE) {
            CDWire(SQDesc->ksPointer,&Layer,&DX,&Path);
            CDStatusInt = CDBB(Parameters.kpCellDesc,SQDesc->ksPointer,
                &OBB.kaLeft,&OBB.kaBottom,&OBB.kaRight,&OBB.kaTop); 
            Path = CopyPath(Path);
            change_nearest_vertex(Path,RefX,RefY,MapX,MapY,CDWIRE);
            if (Not CDMakeWire(Parameters.kpCellDesc,Layer,DX,Path,
                &Pointer)) MallocFailed();
        }
        elif (SQDesc->ksPointer->oType == CDPOLYGON) {
            CDPolygon(SQDesc->ksPointer,&Layer,&Path);
            CDStatusInt = CDBB(Parameters.kpCellDesc,SQDesc->ksPointer,
                &OBB.kaLeft,&OBB.kaBottom,&OBB.kaRight,&OBB.kaTop); 
            Path = CopyPath(Path);
            change_nearest_vertex(Path,RefX,RefY,MapX,MapY,CDPOLYGON);
            if (Not CDMakePolygon(Parameters.kpCellDesc,Layer,Path,
                &Pointer)) MallocFailed();
        }
        elif (SQDesc->ksPointer->oType == CDBOX) {
            OBB.kaLeft = SQDesc->ksPointer->oLeft;
            OBB.kaRight = SQDesc->ksPointer->oRight;
            OBB.kaBottom = SQDesc->ksPointer->oBottom;
            OBB.kaTop = SQDesc->ksPointer->oTop;
            change_rect_vertex(&BB,&OBB,RefX,RefY,MapX,MapY);
            X = (BB.kaLeft + BB.kaRight)/2;
            Y = (BB.kaBottom + BB.kaTop)/2;
            DX = BB.kaRight - BB.kaLeft;
            if (DX < 0) DX = -DX;
            DY = BB.kaTop - BB.kaBottom;
            if (DY < 0) DY = -DY;
            if (DX == 0 || DY == 0 ||
                    DX < LayerTable[
                        (int)SQDesc->ksPointer->oLayer].klMinDimensions ||
                    DY < LayerTable[
                        (int)SQDesc->ksPointer->oLayer].klMinDimensions) {
                ShowPromptAndWait("Can't do it, width too small.");
                CDBox(SQDesc->ksPointer,&Layer,&DX,&DY,&X,&Y);
            }
            if (Not CDMakeBox(Parameters.kpCellDesc,SQDesc->ksPointer->oLayer,DX,DY,X,Y,
                &Pointer)) MallocFailed();
        }

        if (Pointer) {
            if (SelectNew)
                Pointer->oInfo = SQ_NEWSEL;
            else
                Pointer->oInfo = SQ_NEW;

            SQInsert(Pointer);

            CDStatusInt = CDBB(Parameters.kpCellDesc,Pointer,
                &BB.kaLeft,&BB.kaBottom,&BB.kaRight,&BB.kaTop);

            if (OBB.kaLeft   < BB.kaLeft)   BB.kaLeft   = OBB.kaLeft;
            if (OBB.kaRight  > BB.kaRight)  BB.kaRight  = OBB.kaRight;
            if (OBB.kaBottom < BB.kaBottom) BB.kaBottom = OBB.kaBottom;
            if (OBB.kaTop    > BB.kaTop)    BB.kaTop    = OBB.kaTop;

            if (BB.kaLeft   < NBB->kaLeft)   NBB->kaLeft   = BB.kaLeft;
            if (BB.kaRight  > NBB->kaRight)  NBB->kaRight  = BB.kaRight;
            if (BB.kaBottom < NBB->kaBottom) NBB->kaBottom = BB.kaBottom;
            if (BB.kaTop    > NBB->kaTop)    NBB->kaTop    = BB.kaTop;

            CDProperty(Parameters.kpCellDesc,SQDesc->ksPointer,&PrptyDesc);
            while (PrptyDesc) {
                CDAddProperty(Parameters.kpCellDesc,Pointer,
                    PrptyDesc->prpty_Value,PrptyDesc->prpty_String);
                PrptyDesc = PrptyDesc->prpty_Succ;
            }
        }
        SQDesc->ksPointer->oInfo = SQ_GONE;
    }
}


void
ShowStretch(MapX,MapY,RefX,RefY)

/* Called from rubber banding routine. */
int RefX,RefY,MapX,MapY;
{
    struct ks *SQDesc, *SQDesc1;
    struct ka OBB, BB;
    struct p *Path, *pp;
    int Layer;
    int Width;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oInfo != SQ_OLDSEL &&
            SQDesc->ksPointer->oInfo != SQ_NEWSEL) continue;

        /* draw only once if in queue more than once */
        for (SQDesc1 = SQDesc->ksSucc; SQDesc1; SQDesc1 = SQDesc1->ksSucc)
            if (SQDesc1->ksPointer == SQDesc->ksPointer) break;

        if (SQDesc1 != NULL) continue;

        switch (SQDesc->ksPointer->oType) {

            case CDWIRE:
                CDWire(SQDesc->ksPointer,&Layer,&Width,&Path);
                pp = get_nearest_vertex(Path,RefX,RefY);
                if (Parameters.kpStretchType != STR_TB)
                    pp->pX += MapX - RefX;
                if (Parameters.kpStretchType != STR_RL)
                    pp->pY += MapY - RefY;
                ShowWire(ColorTable[HighlightingColor].Ent,Width,Path);
                if (Parameters.kpStretchType != STR_TB)
                    pp->pX -= MapX - RefX;
                if (Parameters.kpStretchType != STR_RL)
                    pp->pY -= MapY - RefY;
                break;

            case CDPOLYGON:
                CDPolygon(SQDesc->ksPointer,&Layer,&Path);
                pp = get_nearest_vertex(Path,RefX,RefY);
                if (Parameters.kpStretchType != STR_TB)
                    pp->pX += MapX - RefX;
                if (Parameters.kpStretchType != STR_RL)
                    pp->pY += MapY - RefY;
                ShowPolygon(ColorTable[HighlightingColor].Ent,Path);
                if (Parameters.kpStretchType != STR_TB)
                    pp->pX -= MapX - RefX;
                if (Parameters.kpStretchType != STR_RL)
                    pp->pY -= MapY - RefY;
                break;

            case CDBOX:
                OBB.kaLeft = SQDesc->ksPointer->oLeft;
                OBB.kaRight = SQDesc->ksPointer->oRight;
                OBB.kaBottom = SQDesc->ksPointer->oBottom;
                OBB.kaTop = SQDesc->ksPointer->oTop;
                change_rect_vertex(&BB,&OBB,RefX,RefY,MapX,MapY);
                ShowEmptyBox(ColorTable[HighlightingColor].Ent,&BB);
                break;
        }
    }
}


static struct p *
get_nearest_vertex(Path,X,Y)

/* return the path vertex nearest X,Y. */
struct p *Path;
int X,Y;
{
    double dx,dy,d,mind;
    struct p *p;
    int i,indx = 0;

    mind = 1e30;
    for (p = Path,i = 0; p; p = p->pSucc,i++) {
        dx = p->pX - X;
        dy = p->pY - Y;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = i;
        }
    }
    for (p = Path, i = 0; i < indx; p = p->pSucc,i++) ;
    return p;
}


static void
change_nearest_vertex(Path,X,Y,NewX,NewY,Type)

/* Translate the path vertex nearest X,Y to NewX-X,NewY-Y. */
struct p *Path;
int X,Y,NewX,NewY;
int Type;
{
    double dx,dy,d,mind;
    struct p *p;
    int i,indx = 0;

    mind = 1e30;
    for (p = Path,i = 0; p; p = p->pSucc,i++) {
        dx = p->pX - X;
        dy = p->pY - Y;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = i;
        }
    }
    if (indx == 0 && Type == CDPOLYGON) {
        for (p = Path; p && p->pSucc; p = p->pSucc) ;
        if (Path->pX == p->pX And Path->pY == p->pY) {
            if (Parameters.kpStretchType != STR_TB)
                p->pX += NewX - X;
            if (Parameters.kpStretchType != STR_RL)
                p->pY += NewY - Y;
        }
    }
    for (p = Path, i = 0; i < indx; p = p->pSucc,i++) ;
    if (Parameters.kpStretchType != STR_TB)
        p->pX += NewX - X;
    if (Parameters.kpStretchType != STR_RL)
        p->pY += NewY - Y;
}


static void
change_rect_vertex(NBB,OBB,X,Y,NewX,NewY)

/* set NBB to the new box */
struct ka *NBB,*OBB;
int X,Y,NewX,NewY;
{
    int indx = 0, Code;
    double d,dx,dy,mind;

    Code = RCode;

    if (Code == 0) {
        mind = 1e30;
        dx = OBB->kaLeft - X;
        dy = OBB->kaBottom - Y;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 1;
        }
        dy = OBB->kaTop - Y;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 2;
        }
        dx = OBB->kaRight - X;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 3;
        }
        dy = OBB->kaBottom - Y;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 4;
        }
        Code = indx;
    }
    *NBB = *OBB;

    switch (Code) {
    case 1:
        if (Parameters.kpStretchType != STR_TB)
            NBB->kaLeft += NewX - X;
        if (Parameters.kpStretchType != STR_RL)
            NBB->kaBottom += NewY - Y;
        return;
    case 2:
        if (Parameters.kpStretchType != STR_TB)
            NBB->kaLeft += NewX - X;
        if (Parameters.kpStretchType != STR_RL)
            NBB->kaTop += NewY - Y;
        return;
    case 3:
        if (Parameters.kpStretchType != STR_TB)
            NBB->kaRight += NewX - X;
        if (Parameters.kpStretchType != STR_RL)
            NBB->kaTop += NewY - Y;
        return;
    case 4:
        if (Parameters.kpStretchType != STR_TB)
            NBB->kaRight += NewX - X;
        if (Parameters.kpStretchType != STR_RL)
            NBB->kaBottom += NewY - Y;
    }
}


static int
set_ref_to_vertex(X,Y)

/* Return in pointers the vertex closest to the given coordinates.
 * Checks rectangle vertices as well as polys and wires.  If a
 * rectangle vertex is closest, this function returns a code identifying
 * the vertex (1 BL, 2 TL, 3 TR, 4 BR), otherwise 0 is returned.
 */
int *X,*Y;
{
    struct ks *SQDesc;
    struct o *Pointer = NULL;
    struct p *p,*Path;
    double dx,dy,d,mind;
    int i,indx = 0;


    mind = 1e30;
    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {

        if (SQDesc->ksPointer->oInfo == SQ_GONE) continue;

        if (SQDesc->ksPointer->oType == CDBOX) {
            dx = SQDesc->ksPointer->oLeft - *X;
            dy = SQDesc->ksPointer->oBottom - *Y;
            d = dx*dx + dy*dy;
            if (d < mind) {
                mind = d;
                indx = 1;
                Pointer = SQDesc->ksPointer;
            }
            dy = SQDesc->ksPointer->oTop - *Y;
            d = dx*dx + dy*dy;
            if (d < mind) {
                mind = d;
                indx = 2;
                Pointer = SQDesc->ksPointer;
            }
            dx = SQDesc->ksPointer->oRight - *X;
            d = dx*dx + dy*dy;
            if (d < mind) {
                mind = d;
                indx = 3;
                Pointer = SQDesc->ksPointer;
            }
            dy = SQDesc->ksPointer->oBottom - *Y;
            d = dx*dx + dy*dy;
            if (d < mind) {
                mind = d;
                indx = 4;
                Pointer = SQDesc->ksPointer;
            }
            continue;
        }
        if (SQDesc->ksPointer->oType == CDWIRE)
            Path = ((struct w *)SQDesc->ksPointer->oRep)->wPath;
        else if (SQDesc->ksPointer->oType == CDPOLYGON)
            Path = ((struct po *)SQDesc->ksPointer->oRep)->poPath;
        else
            continue;

        for (p = Path,i = 0; p; p = p->pSucc,i++) {
            dx = p->pX - *X;
            dy = p->pY - *Y;
            d = dx*dx + dy*dy;
            if (d < mind) {
                mind = d;
                indx = i;
                Pointer = SQDesc->ksPointer;
            }
        }
        
    }
    if (Pointer->oType == CDBOX) {
        switch (indx) {
        case 1:
            *X = Pointer->oLeft;
            *Y = Pointer->oBottom;
            break;
        case 2:
            *X = Pointer->oLeft;
            *Y = Pointer->oTop;
            break;
        case 3:
            *X = Pointer->oRight;
            *Y = Pointer->oTop;
            break;
        case 4:
            *X = Pointer->oRight;
            *Y = Pointer->oBottom;
        }
        return (indx);
    }
    if (Pointer->oType == CDWIRE)
        Path = ((struct w *)Pointer->oRep)->wPath;
    else
        Path = ((struct po *)Pointer->oRep)->poPath;
    for (p = Path, i = 0; i < indx; p = p->pSucc,i++) ;
    *X = p->pX;
    *Y = p->pY;
    return (0);
}

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
 * The KIC flatten code.
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuFLATN;
extern char *MenuUNDO;

#ifdef __STDC__
static void flatten_cell(struct s*);
static int  is_call_in_sq(void);
#else
static void flatten_cell();
static int  is_call_in_sq();
#endif

void
Flatten(LookedAhead)

int *LookedAhead;
{
    struct s *MasterDesc;
    struct ks *SQDesc;
    struct ka OldSelectQBB;
    char *MasterName;
    int NumX,NumY;
    int Int1,Int2;
    int Undo = False;
    int Modified = 0;
    int DX,DY;

    if (Not is_call_in_sq()) {
        ShowPrompt("You haven't selected an instance to flatten.");
        return;
    }
top:
    MenuSelect(MenuFLATN);
    SQComputeBB(); 
    OldSelectQBB = SelectQBB;

    for (SQDesc = SelectQHead; SQDesc != NULL; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oType != CDSYMBOLCALL) continue;
        if (SQDesc->ksPointer->oInfo == SQ_GONE) continue;

        CDCall(SQDesc->ksPointer,&MasterName,&NumX,&DX,&NumY,&DY);
        if (OpenCell(MasterName,&MasterDesc)) continue;

        SetTransform(SQDesc->ksPointer);
        for (Int1 = NumY; Int1 >= 1; --Int1) {
            for (Int2 = 1; Int2 <= NumX; ++Int2) {
                TPush();
                TIdentity();
                TTranslate((Int2-1)*DX,(Int1-1)*DY);
                TPremultiply();
                flatten_cell(MasterDesc); 
                TPop();
            }
        }
        TPop();
        SQDesc->ksPointer->oInfo = SQ_GONE;
        Modified++;
        
    }
    /* take care of instance markers */
    OldSelectQBB.kaRight += 600;
    OldSelectQBB.kaLeft -= 600;
    OldSelectQBB.kaTop += 600;
    OldSelectQBB.kaBottom -= 600;
    EraseBox(&OldSelectQBB);
    Redisplay(&OldSelectQBB);
    MenuDeselect(MenuFLATN);
    loop {
        switch (PointLoop(LookedAhead)) {
        case PL_ESC:
        case PL_CMD:
            goto quit;
        case PL_UND:
           MenuSelect(MenuUNDO);
           if (Undo == False) {
               SQRestore(True);
               EraseBox(&OldSelectQBB);
               Redisplay(&OldSelectQBB);
               MenuDeselect(MenuUNDO);
               Modified--;
               Undo = True;
               continue;
           }
           else {
               Undo = False;
               MenuDeselect(MenuUNDO);
               goto top;
           }
        }
    }

quit:
    SQRestore(False);
    if (Modified)
        Parameters.kpModified = True;
}


static void
flatten_cell(CellDesc)

struct s *CellDesc;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct p *Path;
    struct s *MasterDesc;
    char Xform;
    char *Label;
    char *MasterName;
    int Layer;
    int Int1,Int2;
    int NumX,NumY;
    int Left,Bottom,Right,Top;
    int X,Y,Lngth,Width;
    int DX,DY;

    /*
     * Traverse calls first.
     * So, if instances aren't expanded, top level geometry 
     * isn't obscured by the symbolic pictures of any instances.
     */
    if (Not CDInitGen(CellDesc,0,-CDINFINITY,-CDINFINITY,
        CDINFINITY,CDINFINITY,&GenDesc)) MallocFailed();
    loop {
        CDGen(CellDesc,GenDesc,&Pointer);
        if (Pointer == NULL) break;
        if (Pointer->oInfo == SQ_GONE) continue;

        CDCall(Pointer,&MasterName,&NumX,&DX,&NumY,&DY);

        if (TFull()) return;

        if (OpenCell(MasterName,&MasterDesc)) return;

        SetTransform(Pointer);
        for (Int1 = NumY; Int1 >= 1; --Int1) {
            for (Int2 = 1; Int2 <= NumX; ++Int2) {
                TPush();
                TIdentity();
                TTranslate((Int2-1)*DX,(Int1-1)*DY);
                TPremultiply();
                flatten_cell(MasterDesc);
                TPop();
            }
        }
        TPop();
    }
    for (Layer = 1;Layer <= NumLayerTable;++Layer) {
        if (Not CDInitGen(CellDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc)) MallocFailed();
        loop {
            CDGen(CellDesc,GenDesc,&Pointer);
            if (Pointer == NULL) break;
            if (Pointer->oInfo == SQ_GONE) continue;

            switch (Pointer->oType) {

            case CDBOX:
                CDBox(Pointer,&Layer,&Lngth,&Width,&X,&Y);
                /*
                 * KLUDGE
                 * So that the length and widths are rotated, we have to
                 * convert to a ka box and transform the corner points.
                 */
                Left = Pointer->oLeft;
                Bottom = Pointer->oBottom;
                Right = Pointer->oRight;
                Top = Pointer->oTop;
                if (Right < Left)
                    SwapInts(Right,Left);
                if (Top < Bottom)
                    SwapInts(Top,Bottom);
                TPoint(&Left,&Bottom);
                TPoint(&Right,&Top);
                Lngth = Right - Left;
                Width = Top - Bottom;
                X = Left + (Lngth >> 1);
                Y = Bottom + (Width >> 1);
                if (Not CDMakeBox(Parameters.kpCellDesc,Layer,Lngth,Width,X,Y,
                    &Pointer)) MallocFailed();
                break;

            case CDLABEL:
                CDLabel(Pointer,&Layer,&Label,&X,&Y,&Xform);
                TPoint(&X,&Y);
                if (Not CDMakeLabel(Parameters.kpCellDesc,Layer,Label,
                    X,Y,Xform,&Pointer)) MallocFailed();
                break;

            case CDWIRE:
                CDWire(Pointer,&Layer,&Width,&Path);
                CopyPathWithXForm(&Path);
                if (Not CDMakeWire(Parameters.kpCellDesc,Layer,Width,Path,
                    &Pointer)) MallocFailed();
                break;

            case CDPOLYGON:
                CDPolygon(Pointer,&Layer,&Path);
                CopyPathWithXForm(&Path);
                if (Not CDMakePolygon(Parameters.kpCellDesc,Layer,Path,
                    &Pointer)) MallocFailed();
                break;

            default:
                continue;
            }
            SQInsert(Pointer);
            Pointer->oInfo = SQ_NEW;
        }
    }
}


static int
is_call_in_sq()

{
    struct ks *SQDesc;

    for (SQDesc = SelectQHead; SQDesc; SQDesc = SQDesc->ksSucc) {
        if (SQDesc->ksPointer->oType == CDSYMBOLCALL)
            return (True);
    }
    return (False);
}


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
 * Change layer selection operator.
 */

#include "prefix.h"
#include "kic.h"

extern char *MenuCHLYR;
extern char *MenuUNDO;


void
ChangeLayer(LookedAhead)

int *LookedAhead;
{
    struct ks *SQDesc;
    struct prpty *PrptyDesc;
    struct p *Path;
    char *Label;
    char *cp;
    int X,Y;
    int Layer;
    int NewLayer;
    int NewInfo;
    int Width;
    int Length;
    int Info;
    int FirstTime = True;
    int UndoTheChange = 0;
    char Type,Xform;

    cp = MenuCHLYR;
    MenuSelect(cp);
    if(SelectQHead == NULL){
        ShowPrompt("You haven't selected anything to change."); 
        MenuDeselect(cp);
        return;
    }
    *LookedAhead = False;
    if(Not *LookedAhead){
        ShowPrompt("Point to the new layer."); 
        loop{
            Point();
            if(Parameters.kpPointLayerTable)
                break;
            elif(Parameters.kpCommand[0] != EOS){
                MenuDeselect(cp);
                *LookedAhead = True;
                return;
            }
        }
    }
    loop {
        SQDesc = SelectQHead;
        if(FirstTime)
            FirstTime = False;
        else{
            Point();
            if(strcmp(Parameters.kpCommand,MenuUNDO) != 0){
                Parameters.kpModified = True;
                *LookedAhead = True;
                while(SQDesc != NULL){
                    SQDesc->ksPointer->oInfo = SQ_OLDSEL;
                    SQDesc = SQDesc->ksSucc;
                }
                return;
            }
            cp = MenuUNDO;
            MenuSelect(cp);
            UndoTheChange ^= 1;
            *LookedAhead = False;
        }
        while(SQDesc != NULL){
            CDType(SQDesc->ksPointer,&Type);
            RemovePropertyList(SQDesc->ksPointer,&PrptyDesc);
            Info = SQDesc->ksPointer->oInfo;
            if(Type == CDLABEL){
                CDLabel(SQDesc->ksPointer,&Layer,&Label,&X,&Y,&Xform);
                SQDesc->ksPointer->oInfo = SQ_GONE;
                CDDelete(Parameters.kpCellDesc,SQDesc->ksPointer);
                if(UndoTheChange){
                    NewLayer = Info - 10;
                    NewInfo = SQ_OLDSEL;
                }
                else{
                    NewLayer = Parameters.kpLayer;
                    NewInfo = Layer + 10;
                }
                if(Not CDMakeLabel(Parameters.kpCellDesc,NewLayer,Label,
                    X,Y,Xform,&(SQDesc->ksPointer))) MallocFailed();
                SQDesc->ksPointer->oInfo = NewInfo;
            }
            elif(Type == CDBOX){
                CDBox(SQDesc->ksPointer,&Layer,&Length,&Width,&X,&Y);
                SQDesc->ksPointer->oInfo = SQ_GONE;
                CDDelete(Parameters.kpCellDesc,SQDesc->ksPointer);
                if(UndoTheChange){
                    NewLayer = Info - 10;
                    NewInfo = SQ_OLDSEL;
                }
                else{
                    NewLayer = Parameters.kpLayer;
                    NewInfo = Layer + 10;
                }
                if(Not CDMakeBox(Parameters.kpCellDesc,NewLayer,Length,Width,
                    X,Y,&(SQDesc->ksPointer))) MallocFailed();
                SQDesc->ksPointer->oInfo = NewInfo;
            }
            elif(Type == CDSYMBOLCALL)
                ShowPrompt("Can't change the layers of an instance.");
            elif(Type == CDWIRE){
                CDWire(SQDesc->ksPointer,&Layer,&Width,&Path);
                SQDesc->ksPointer->oInfo = SQ_GONE;
                Path = CopyPath(Path);
                CDDelete(Parameters.kpCellDesc,SQDesc->ksPointer);
                if(UndoTheChange){
                    NewLayer = Info - 10;
                    NewInfo = SQ_OLDSEL;
                }
                else{
                    NewLayer = Parameters.kpLayer;
                    NewInfo = Layer + 10;
                }
                if(Not CDMakeWire(Parameters.kpCellDesc,NewLayer,Width,Path,
                    &(SQDesc->ksPointer))) MallocFailed();
                SQDesc->ksPointer->oInfo = NewInfo;
            }
            elif(Type == CDPOLYGON){
                CDPolygon(SQDesc->ksPointer,&Layer,&Path);
                SQDesc->ksPointer->oInfo = SQ_GONE;
                Path = CopyPath(Path);
                CDDelete(Parameters.kpCellDesc,SQDesc->ksPointer);
                if(UndoTheChange){
                    NewLayer = Info - 10;
                    NewInfo = SQ_OLDSEL;
                }
                else{
                    NewLayer = Parameters.kpLayer;
                    NewInfo = Layer + 10;
                }
                if(Not CDMakePolygon(Parameters.kpCellDesc,NewLayer,Path,
                    &(SQDesc->ksPointer))) MallocFailed();
                SQDesc->ksPointer->oInfo = NewInfo;
            }
            RestorePropertyList(SQDesc->ksPointer,PrptyDesc);
            SQDesc = SQDesc->ksSucc;
        }
        SQComputeBB(); 
        EraseBox(&SelectQBB);
        Redisplay(&SelectQBB);
        MenuDeselect(cp);
    }
}

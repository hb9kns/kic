/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Kenneth H. Keller, Giles C. Billingsley
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

#include "prefix.h"
#include "kic.h"

#ifdef vms
#define TEMPFILE  "SYSXXXXXX"
#else
#ifdef MSDOS
#define TEMPFILE  "ktXXXXXX"
#else
#define TEMPFILE  "/tmp/kicXXXXXX"
#endif
#endif

#ifdef __STDC__
static void show_properties(void);
#else
static void show_properties();
#endif

void
Properties(LookedAhead)

int *LookedAhead;
{
    *LookedAhead = False;
    Parameters.kpMenu = PROPERTYMENU;
    FixMenuPrefix(PropertyMenu);
    ShowCommandMenu();
    SQShow();
}


void
DoShowProperties()

{
    extern char *MenuSHOW;

    MenuSelect(MenuSHOW);
    show_properties();
    MenuDeselect(MenuSHOW);
}


void
AddProperty()

{
    struct ks *SQDesc;
    char *TypeIn;
    int Value;
    char String[256];
    extern char *MenuADPRP;

    MenuSelect(MenuADPRP);
    if ((SQDesc = SelectQHead) != NULL) {
        ShowPrompt("Property number? ");
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) goto quit;
        sscanf(TypeIn,"%d",&Value);
        ShowPrompt("Property string? ");
        TypeIn = FBEdit(NULL);
        if (TypeIn == NULL) goto quit;
        strcpy(String,TypeIn);
        while (SQDesc != NULL) {
            if (Not CDAddProperty(Parameters.kpCellDesc,
                SQDesc->ksPointer,Value,String))  MallocFailed();
            SQDesc = SQDesc->ksSucc;
        }
        Parameters.kpModified = True;
    }
    else {
        ShowPrompt("You haven't selected anything.");
        MenuDeselect(MenuADPRP);
        return;
    }
quit:
    ErasePrompt();
    MenuDeselect(MenuADPRP);
}


void
RemoveProperty()

{
    struct ks *SQDesc;
    char *TypeIn;
    int Value;
    extern char *MenuRMPRP;

    MenuSelect(MenuRMPRP);
    if ((SQDesc = SelectQHead) != NULL) {
        ShowPrompt("Property number to be removed? ");
        TypeIn = FBEdit(NULL);
        if (TypeIn != NULL) {
            sscanf(TypeIn,"%d",&Value);
            while (SQDesc != NULL) {
                if (Not CDRemoveProperty(Parameters.kpCellDesc,SQDesc->ksPointer,
                    Value)) MallocFailed();
                SQDesc = SQDesc->ksSucc;
            }
        }
        ErasePrompt();
    }
    else
        ShowPrompt("You haven't selected anything.");
    MenuDeselect(MenuRMPRP);
}

/*
 * These routines may more appropriately belong in CD.
 */

void
RemovePropertyList(Pointer,PrptyDesc)

struct o *Pointer;
struct prpty **PrptyDesc;
{
    if (Pointer == NULL)
        *PrptyDesc = NULL;
    else{
        *PrptyDesc = Pointer->oPrptyList;
        Pointer->oPrptyList = NULL;
    }
}


void
RestorePropertyList(Pointer,PrptyDesc)

struct o *Pointer;
struct prpty *PrptyDesc;
{
    if (Pointer != NULL)
        Pointer->oPrptyList = PrptyDesc;
}


static void
show_properties()

{
    struct prpty *PrptyDesc;
    struct ks *SQDesc;
    FILE *tmpfile;
    char buf1[120];
    char tbf[128];
    char oldRedisplayControl;
    char *tf;

    if ((SQDesc = SelectQHead) == NULL) {
        ShowPrompt("Nothing to show.");
        return;
    }
    strcpy(tbf, TEMPFILE);
    tf = mktemp(tbf);
    if (!tf) {
        ShowPrompt("Internal error: mktemp() failed.");
        return;
    }

#ifdef vms
    sprintf(buf1,"%s.LIS", tf);
#else
    strcpy(buf1, tf);
#endif

    oldRedisplayControl = Parameters.kpRedisplayControl;
    Parameters.kpRedisplayControl = COARSEVIEWPORTONLY;
    while (SQDesc != NULL) {
        if ((tmpfile = fopen(buf1,"w")) == NULL) {
            char *t = strrchr(buf1, '/');
            if (t) {
                t++;
                strcpy(tbf, t);
                strcpy(buf1, tbf);
                tmpfile = fopen(buf1, "w");
            }
            if (!tmpfile) {
                ShowPromptAndWait("Can't open temporary file.");
                break;
            }
        }
        ShowCurrentObject(SQDesc->ksPointer,DISPLAY);
        CDProperty(Parameters.kpCellDesc,SQDesc->ksPointer,&PrptyDesc);
        if (PrptyDesc == NULL)
            fprintf(tmpfile,"Object has no properties.\n");
        else{
            while (PrptyDesc != NULL) {
                fprintf(tmpfile,"%6d  %s\n",PrptyDesc->prpty_Value,
                    PrptyDesc->prpty_String);
                PrptyDesc = PrptyDesc->prpty_Succ;
            }
        }
        fclose(tmpfile);
        if ((tmpfile = fopen(buf1,"r")) == NULL) {
            ShowPromptAndWait("Can't open temporary file.");
            break;
        }
        FBForeground(DISPLAY,ColorTable[MenuTextColor].Ent);
        FBMore(View->kvFineViewport->kaLeft-1,
            View->kvFineViewport->kaBottom-1,
            View->kvFineViewport->kaRight,
            View->kvFineViewport->kaTop,tmpfile);
        fclose(tmpfile);
        unlink(buf1);
        ShowCurrentObject(SQDesc->ksPointer,ERASE);
        SQDesc = SQDesc->ksSucc;
    }
    Parameters.kpRedisplayControl = oldRedisplayControl;
    ShowFineViewport();
}

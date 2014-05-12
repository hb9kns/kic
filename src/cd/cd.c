/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Kenneth H. Keller, Giles C. Billingsley
 *
 *     CD is a CIF database package that was developed by the integrated
 * circuits group of the Electronics Research Laboratory and the
 * Department of Electrical Engineering and Computer Sciences at
 * the University of California, Berkeley, California.  The programs in
 * CD are available free of charge to any interested party.
 * The sale, resale, or use of these programs for profit without the
 * express written consent of the Department of Electrical Engineering
 * and Computer Sciences, University of California, Berkeley, California,
 * is forbidden.
 *
 *************************************************************************/

/*
 * cd.c
 *
 */

/* SRW - changes:
 * 1.  The DX and DY parameters used in CDCall(), CDBeginMakeCall() and
 *     elsewhere now refer to center-to-center spacing.  Previously,
 *     edge to adjacent edge spacing was used.  This creates problems
 *     when converting from GDSII, and also means that cell placement
 *     changes if cell size changes.  Now, arrayed cells stay put if the
 *     BB's change.  There may be compatibility problems with files
 *     which use the old scheme.
 *
 * 2.  Hooks have been added for manipulation of the property strings
 *     for use by a schematic layout program.  See the notes in
 *     CDBeginMakeCall() and CDEndMakeCall().
 *
 * 3.  A routine for searching libraries has been added to POpen() in
 *     paths.c.  See the note there concerning use.
 *
 * 4.  A callback to a user supplied routine
 *     void CDLabelBB(struct o *Pointer,int *L,int *B, int *R, int *T)
 *     has been added.  This routine returns the effective window of
 *     of the label, which depends on font size, etc,  This routine
 *     is called by the generators, not CDBB(), as the BB is window
 *     dependent.
 * 5.  Labels are always put in the residual bin, so the generator
 *     can find them if the origin is out of the AOI.
 *
 * 6.  CDInit() now calls malloc() to set up arrays for CDDesc.  The
 *     return value is True if malloc fails, False if OK.
 *
 * Two routines need to be supplied externally for 2,3 above.  If the
 * features are not used, add the following to the external code:
 *   void UpdateProperties() {}
 *   FILE *OpenDevice() {return NULL;}
 *
 */

/*======================================================================*
 *                                                                      *
 *                       CCCCCC       DDDDDDD                           *
 *                      CC    CC      DD    DD                          *
 *                     CC      CC     DD     DD                         *
 *                     CC             DD     DD                         *
 *                     CC             DD     DD                         *
 *                     CC      CC     DD     DD                         *
 *                      CC    CC      DD    DD                          *
 *                       CCCCCC       DDDDDDD                           *
 *                                                                      *
 *                                                                      *
 * CD package code.                                                     *
 *                                                                      *
 *                                                                      *
 *======================================================================*/

#include "prefix.h"
#include "cd.h" 
#include "parser.h"

/* callbacks */
#if __STDC__
extern void CDLabelBB(struct o*,int*,int*,int*,int*);
extern void UpdateProperties(struct o*);
#else
extern void CDLabelBB();
extern void UpdateProperties();
#endif
 
struct bu *CDSymbolTable[CDNUMLAYERS+1];
struct d  CDDesc;
struct l  CDLayer[CDNUMLAYERS+1];


/*
 * The following is the policy for handling errors in CD:
 * When a routine encounters difficulty, it will set CDStatusInt
 * to some identifying value, copy a diagnostic string into
 * CDStatusString, and return a 'False'.  Otherwise, the routine
 * will return 'True' and not alter the value of CDStatusInt.
 * Every routine that uses malloc will test the returned value
 * and return Flase via CDError() if malloc fails.
 */
char *CDStatusString;
int  CDStatusInt;
int  CDBogusPoly;

#define LARGEBUFFERSIZE 400

/*===========================================================================*
 *                                                                           *
 * III N   N III TTTTT III   A   L     III ZZZZZ   A   TTTTT III  OOO  N   N *
 *  I  NN  N  I    T    I   A A  L      I     Z   A A    T    I  O   O NN  N *
 *  I  N N N  I    T    I  A   A L      I    Z   A   A   T    I  O   O N N N *
 *  I  N  NN  I    T    I  AAAAA L      I   Z    AAAAA   T    I  O   O N  NN *
 * III N   N III   T   III A   A LLLLL III ZZZZZ A   A   T   III  OOO  N   N *
 *                                                                           *
 *             RRRR   OOO  U   U TTTTT III N   N EEEEE  SSSS                 *
 *             R   R O   O U   U   T    I  NN  N E     S                     *
 *             RRRR  O   O U   U   T    I  N N N EEE    SSS                  *
 *             R R   O   O U   U   T    I  N  NN E         S                 *
 *             R  R   OOO   UUU    T   III N   N EEEEE SSSS                  *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *        CDInit()                                                           *
 *        CDPath(Path)                                                       *
 *        CDSetLayer(Layer,Tech,Mask)                                        *
 *        CDDebug(Flag)                                                      *
 *                                                                           *
 *===========================================================================*/

int
CDInit()

{
    /*
     * This must be the first CD routine called.  It initializes
     * the layer table, search path, symbol table, and transform
     * stack.  Returns True if error, False otherwise.
     */
    static char CDDiagnosticString[LARGEBUFFERSIZE];
    int Layer,Int1;
    char *c;

    for (Int1 = 0;Int1 < CDNUMLAYERS;++Int1)
        CDSymbolTable[Int1] = NULL;
    for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
        CDLayer[Layer].lTechnology = ' ';
        CDLayer[Layer].lMask[0] = ' ';
        CDLayer[Layer].lMask[1] = ' ';
        CDLayer[Layer].lMask[2] = ' ';
        CDLayer[Layer].lCDFrom = False;
    }
    c = PGetPath();
    if (*c == '\0')
        CDPath(".");
    CDDebug(False);
    CDDesc.dPrptyList = NULL;
    CDDesc.dNumSymbolsAllocated = 0;
    CDStatusString = CDDiagnosticString;

    /* Vanilla operation. */
    CDDesc.dControl = DCONTROLVANILLA;

    /* Alocate arrays if not already done */
    if (CDDesc.dSymTabNames == NULL) {
        CDDesc.dSymTabNames = (char(*)[FILENAMESIZE])
            malloc(CDNUMREMEMBER*FILENAMESIZE);
        if (CDDesc.dSymTabNames == NULL) {
            CDStatusInt = CDError(CDMALLOCFAILED);
            return True;
        }
        CDDesc.dSymTabNumbers = (int *) malloc(CDNUMREMEMBER*sizeof(int));
        if (CDDesc.dSymTabNumbers == NULL) {
            CDStatusInt = CDError(CDMALLOCFAILED);
            return True;
        }
    }
    TInit();
    return False;
}


int
CDPath(Path)

char *Path;
{
    /*
     * Sets search rules for symbol name resolution.
     * Path is a list of directory names to search separated by blanks.
     * csh-style names are understood.
     * False is returned if the search path argument is invalid.
     */
    if (Not PSetPath(Path))
        return (CDError(CDBADPATH));
    else
        return (True);
}


void
CDSetLayer(Layer,Technology,Mask)

int  Layer;
char Technology,Mask[];
{
    /*
     * This routine sets the layer Layer to the name 'TechnologyMask'.
     * There is no returned value.
     */
    CDLayer[Layer].lTechnology = Technology;
    CDLayer[Layer].lMask[0] = Mask[0];
    CDLayer[Layer].lMask[1] = Mask[1];
    CDLayer[Layer].lMask[2] = Mask[2];
}


void
CDDebug(Debug)

int Debug;
{
    /*
     * If Debug is true, then CD will run in debug mode.  There is
     * no returned value.
     */
    CDDesc.dDebug = Debug;
}




/*======================================================================*
 *                                                                      *
 *                SSSS Y   Y M   M BBBB   OOO  L                        *
 *               S      Y Y  MM MM B   B O   O L                        *
 *                SSS    Y   M M M BBBB  O   O L                        *
 *                   S   Y   M   M B   B O   O L                        *
 *               SSSS    Y   M   M BBBB   OOO  LLLLL                    *
 *                                                                      *
 *    M   M   A   N   N   A    GGG  EEEEE M   M EEEEE N   N TTTTT       *
 *    MM MM  A A  NN  N  A A  G     E     MM MM E     NN  N   T         *
 *    M M M A   A N N N A   A G GGG EEE   M M M EEE   N N N   T         *
 *    M   M AAAAA N  NN AAAAA G   G E     M   M E     N  NN   T         *
 *    M   M A   A N   N A   A  GGG  EEEEE M   M EEEEE N   N   T         *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *        CDOpen(SymbolName,SymbolDesc,Access)                          *
 *        CDSymbol(SymbolName,SymbolDesc)                               *
 *        CDClose(SymbolDesc)                                           *
 *        CDReflect(SymbolDesc)                                         *
 *        CDPatchInstances(SymbolDesc,MasterName)                       *
 *                                                                      *
 *======================================================================*/

int
CDOpen(SymbolName,SymbolDesc,Access)

char Access;
char *SymbolName;
struct s **SymbolDesc;
{
    /*
     * Open symbol and return desc for it.
     * 
     * CDOpen returns False if parse failed or out of memory.  When CDOpen
     * returns, CDStatusInt assumes one of the following values:
     *   (FATAL)-CDPARSEFAILED if parser failed or out of memory.
     *           CDOLDSYMBOL if success and symbol already exists in memory. 
     *           CDNEWSYMBOL if success and symbol is a new (empty) one.
     *           CDSUCCEEDED if no problem was encountered.
     * If the return is fatal, CDStatusString contains a diagnostic message.
     * Only CDPARSEFAILED is returned as a fatal error; this simplifies
     * the diagnostic test.
     *
     * If Access == 'w', then create the cell if it doesn't already exist.
     * In other words, open cell for writing.  This solves the following
     * problem: if the user tries to create an instance of a cell that 
     * doesn't exist, it will not be created in the database.  If the cell
     * was added to the database, a second attempt to place the cell that
     * doesn't exist would succeed.  Bad News!
     *
     * If Access == 'n', then create a new cell if it doesn't already exist
     * in the database.  Unlike Access == 'w', the cell will not be parsed
     * if it exists in the current search path.
     */

    char *StatusString;
    char *cp;
    FILE *FDesc;
    struct bu *Bucket;
    struct m *MasterListDesc;
    struct s *MasterSymbolDesc;
    struct s *NewDesc;
    struct prpty PrptyCopy;
    int Key,Int1,Int2;
    int Layer;
    int StatusInt;
    unsigned size;
    static int RecursionLevel = 0; 

#ifdef DEBUGREFLECT
printf("Begin CDOpen of symbol %s.\n",SymbolName);
#endif

    if (SymbolName == NULL Or *SymbolName == '\0') {
        CDStatusInt = CDPARSEFAILED;
        sprintf(CDStatusString,"Null symbol name encountered.");
        return (False);
    }
    CDDesc.dControl = DCONTROLCDOPEN;
    ++RecursionLevel;

    /* Is symbol open already? */

    Key = 0;
    *SymbolDesc = NULL;
    Int2 = strlen(SymbolName);
    for (Int1 = 0; Int1 < Int2; ++Int1)
        Key += SymbolName[Int1];
    Bucket = CDSymbolTable[Key % CDNUMLAYERS];
    while(Bucket != NULL) {
        if (strcmp(Bucket->buSymbolDesc->sName,SymbolName) == 0) {
            *SymbolDesc = Bucket->buSymbolDesc;
            break;
        }
        Bucket = Bucket->buSucc;
    }
    if (*SymbolDesc != NULL) {
        CDStatusInt = CDOLDSYMBOL;
        *CDStatusString = '\0';
    }
    else {
        /* first, try to allocate memory */
        if ((NewDesc = alloc(s)) == NULL) {
            CDStatusInt = CDError(CDMALLOCFAILED);
            CDStatusInt = CDPARSEFAILED;
            return (False);
        }
        size = Int2 + 2;
        if ((cp = malloc(size)) == NULL) {
            CDStatusInt = CDError(CDMALLOCFAILED);
            CDStatusInt = CDPARSEFAILED;
            return (False);
        }
        if ((Bucket = alloc(bu)) == NULL) {
            CDStatusInt = CDError(CDMALLOCFAILED);
            CDStatusInt = CDPARSEFAILED;
            return (False);
        }
        if (Access != 'n' And (FDesc = POpen(SymbolName,"r",(char *)NULL,
            (char **)NULL)) != NULL) {
            /*
             * Symbol already exists, so user probably intends
             * to edit it or just read it.
             */

            /* put symbol into symbol table */
            *SymbolDesc = CDDesc.dSymbolDesc = NewDesc;
            ++CDDesc.dNumSymbolsAllocated;
            for (Layer = 0;Layer <= CDNUMLAYERS;++Layer)
                CDDesc.dSymbolDesc->sBin[Layer] = (struct o ***)NULL;
            CDDesc.dSymbolDesc->sName = cp;
            strcpy(CDDesc.dSymbolName,SymbolName); 
            strcpy(CDDesc.dSymbolDesc->sName,CDDesc.dSymbolName); 
            CDDesc.dNumX = CDDesc.dNumY = 1;
            CDDesc.dDX = CDDesc.dDY = 0;
            CDDesc.dSymbolDesc->sA = 1;
            CDDesc.dSymbolDesc->sB = 1;
            CDDesc.dSymbolDesc->sBBValid = True; 
            CDDesc.dSymbolDesc->sLeft = CDDesc.dSymbolDesc->sBottom =CDINFINITY;
            CDDesc.dSymbolDesc->sRight = CDDesc.dSymbolDesc->sTop = -CDINFINITY;
            CDDesc.dSymbolDesc->sInfo = 0;
            CDDesc.dSymbolDesc->sMasterList = NULL;
            CDDesc.dSymbolDesc->sPrptyList = NULL;
            /* add property list information */
            while(CDDesc.dPrptyList != NULL) {
                if (Not CDAddProperty(CDDesc.dSymbolDesc,(struct o *)NULL,
                    CDDesc.dPrptyList->prpty_Value,
                    CDDesc.dPrptyList->prpty_String)) {
                    CDStatusInt = CDError(CDMALLOCFAILED);
                    CDStatusInt = CDPARSEFAILED;
                    return (False);
                }
                /* free storage of CDDesc.dPrptyList */
                PrptyCopy = *CDDesc.dPrptyList;
                free(CDDesc.dPrptyList->prpty_String);
                afree(CDDesc.dPrptyList,prpty);
                CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
            }
            /*
             * Insert symbol desc in symbol table.
             * The hash table 'Key' was computed above.
             */
            if (CDSymbolTable[Key % CDNUMLAYERS] != NULL)
                CDSymbolTable[Key % CDNUMLAYERS]->buPred = Bucket;
            Bucket->buPred = NULL;
            Bucket->buSucc = CDSymbolTable[Key % CDNUMLAYERS];
            Bucket->buSymbolDesc = *SymbolDesc;
            CDSymbolTable[Key % CDNUMLAYERS] = Bucket; 
            fclose(FDesc);
            PCIF(SymbolName,&StatusString,&StatusInt);
            if (StatusInt == PSUCCEEDED)
                CDStatusInt = CDOLDSYMBOL;
            else {
                CDStatusInt = CDPARSEFAILED;
                strcpy(CDStatusString,StatusString);

                /* SRW ** so CDGen() doesn't puke */
                for (Layer = 0;Layer <= CDNUMLAYERS;++Layer)
                    CDDesc.dSymbolDesc->sBin[Layer] = (struct o ***)NULL;

                return (False);
            }
            /*
             * Now, the master list descs and instance descs, if any, have
             * to be filled in.  See the discussion in CDBeginMakeCall for
             * why this wasn't done earlier.
             */
            MasterListDesc = CDDesc.dSymbolDesc->sMasterList;
            while(MasterListDesc != NULL) {
                /*
                 * This recursive call is safe, because PCIF has done its work.
                 * Because StatusInt is checked, we ignore the returned value.
                 */
                CDOpen(MasterListDesc->mName,&MasterSymbolDesc,'r');
                if (CDStatusInt == CDNEWSYMBOL Or CDStatusInt == CDPARSEFAILED) {
                    if (CDStatusInt == CDNEWSYMBOL) {
                        CDStatusInt = CDPARSEFAILED;
                        sprintf(CDStatusString,
                            "Master %s doesn't seem to be around.\n",
                             MasterListDesc->mName);
                    }
                    return (False);
                }
                if (Not CDReflect(MasterSymbolDesc)) {
                    CDStatusInt = CDPARSEFAILED;
                    return (False);
                }
                MasterListDesc = MasterListDesc->mSucc;
            }
            CDStatusInt = CDSUCCEEDED;
        }
        else {
            if (Access == 'w' Or Access == 'n') {
                /* create cell in database */
                CDDesc.dSymbolDesc = NewDesc;
                ++CDDesc.dNumSymbolsAllocated;
                for (Layer = 0;Layer <= CDNUMLAYERS;++Layer)
                    CDDesc.dSymbolDesc->sBin[Layer] = (struct o ***)NULL;
                CDDesc.dSymbolDesc->sName = cp;
                strcpy(CDDesc.dSymbolName,SymbolName); 
                strcpy(CDDesc.dSymbolDesc->sName,CDDesc.dSymbolName); 
                CDDesc.dNumX = CDDesc.dNumY = 1;
                CDDesc.dDX = CDDesc.dDY = 0;
                CDDesc.dSymbolDesc->sA = 1;
                CDDesc.dSymbolDesc->sB = 1;
                CDDesc.dSymbolDesc->sBBValid = True; 
                CDDesc.dSymbolDesc->sLeft
                    = CDDesc.dSymbolDesc->sBottom = CDINFINITY;
                CDDesc.dSymbolDesc->sRight
                    = CDDesc.dSymbolDesc->sTop = -CDINFINITY;
                CDDesc.dSymbolDesc->sInfo = 0;
                CDDesc.dSymbolDesc->sMasterList = NULL;
                CDDesc.dSymbolDesc->sPrptyList = NULL;
                /* add property list information */
                while(CDDesc.dPrptyList != NULL) {
                    if (Not CDAddProperty(CDDesc.dSymbolDesc,(struct o *)NULL,
                        CDDesc.dPrptyList->prpty_Value,
                        CDDesc.dPrptyList->prpty_String)) {
                        CDStatusInt = CDError(CDMALLOCFAILED);
                        CDStatusInt = CDPARSEFAILED;
                        return (False);
                    }
                    /* free storage of CDDesc.dPrptyList */
                    PrptyCopy = *CDDesc.dPrptyList;
                    free(CDDesc.dPrptyList->prpty_String);
                    afree(CDDesc.dPrptyList,prpty);
                    CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
                }
                /*
                 * Insert symbol desc in symbol table.
                 * The hash table 'Key' was computed above.
                 */
                *SymbolDesc = CDDesc.dSymbolDesc;
                if (CDSymbolTable[Key % CDNUMLAYERS] != NULL)
                    CDSymbolTable[Key % CDNUMLAYERS]->buPred = Bucket;
                Bucket->buSucc = CDSymbolTable[Key % CDNUMLAYERS];
                Bucket->buSymbolDesc = *SymbolDesc;
                Bucket->buPred = NULL;
                CDSymbolTable[Key % CDNUMLAYERS] = Bucket; 
            }
            else {
                *SymbolDesc = NULL;
                /* free the previously allocated memory.  It's not needed */
                afree(NewDesc,s);
                afree(Bucket,bu);
                free(cp);
            }
            CDStatusInt = CDNEWSYMBOL;
        }
    }
    --RecursionLevel;
    if (RecursionLevel == 0 And CDDesc.dControl != DCONTROLPCIF)
        CDDesc.dControl = DCONTROLVANILLA;

#ifdef DEBUGREFLECT
printf("End CDOpen of symbol %s.\n",SymbolName);
#endif

    return (True);
}


void
CDSymbol(SymbolName,SymbolDesc)

char *SymbolName;
struct s **SymbolDesc;
{
    /*
     * Returns symbol desc if any for symbol.
     * If SymbolName is not in symbol table, the pointer SymbolDesc
     * is returned NULL.
     */
    int Key,Int1,Int2;
    struct bu *Bucket;

    Key = 0;
    *SymbolDesc = NULL;
    if (SymbolName == NULL)
        return;
    Int2 = strlen(SymbolName);
    for (Int1 = 0;Int1 < Int2;++Int1)
        Key += SymbolName[Int1];
    Bucket = CDSymbolTable[Key % CDNUMLAYERS];
    while(Bucket != NULL) {
        if (strcmp(Bucket->buSymbolDesc->sName,SymbolName) == 0) {
            *SymbolDesc = Bucket->buSymbolDesc;
            return;
        }
        Bucket = Bucket->buSucc;
    }
}


int
CDClose(SymbolDesc)

struct s *SymbolDesc;
{
    /*
     * Close symbol.  Free SymbolDesc.
     * If malloc fails, False is returned.  Otherwise, True is returned.
     */
    struct bu *Bucket;
    struct g *GenDesc;
    struct o *Pointer;
    struct prpty *PrptyDesc;
    struct prpty PrptyCopy;
    int Key,Int1,Int2;
    int Layer;

    /*
     * Delete symbol desc from symbol table.
     */
    Key = 0;
    Int2 = strlen(SymbolDesc->sName);
    for (Int1 = 0; Int1 < Int2; ++Int1)
        Key += SymbolDesc->sName[Int1];
    Bucket = CDSymbolTable[Key % CDNUMLAYERS];
    while(Bucket != NULL) {
        if (strcmp(Bucket->buSymbolDesc->sName,SymbolDesc->sName) == 0)
            break;
        Bucket = Bucket->buSucc;
    }
    if (Bucket == NULL)
        return (True);
    if (Bucket->buPred == NULL And Bucket->buSucc == NULL) {
        /*
         * Only desc--has no pred or succ. 
         */
        CDSymbolTable[Key % CDNUMLAYERS] = NULL;
    }
    elif (Bucket->buPred == NULL) {
        /*
         * First desc.    Has a succ, but no pred. 
         */
        CDSymbolTable[Key % CDNUMLAYERS] = Bucket->buSucc;
        Bucket->buSucc->buPred = NULL;
    }
    elif (Bucket->buSucc == NULL) {
        /*
         * Last desc--has a pred, but no succ. 
         */
        Bucket->buPred->buSucc = NULL; 
    }
    else {
        /*
         * Vanilla desc has a pred and a succ.
         */
        Bucket->buSucc->buPred = Bucket->buPred; 
        Bucket->buPred->buSucc = Bucket->buSucc;
    }
    /*
     * Free storage taken up by symbol.
     */
    for (Layer = 0;Layer <= CDNUMLAYERS;++Layer) {
        if (Not CDInitGen(SymbolDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
            return (CDError(CDMALLOCFAILED));
        loop {
            CDGen(SymbolDesc,GenDesc,&Pointer);
            if (Pointer == NULL)
                break;
            else {
                CDDelete(SymbolDesc,Pointer);
            }
        }
    }
    /*
     * Free storage of property list;
     */
    PrptyDesc = SymbolDesc->sPrptyList;
    while(PrptyDesc != NULL) {
        PrptyCopy = *PrptyDesc;
        if (PrptyDesc->prpty_String != NULL)
            free(PrptyDesc->prpty_String);
        afree(PrptyDesc,prpty);
        PrptyDesc = PrptyCopy.prpty_Succ;
    }
    afree(Bucket->buSymbolDesc,s);
    afree(Bucket,bu);
    return (True);
}


int
CDReflect(SymbolDesc)

struct s *SymbolDesc;
{
    /*
     * This routine must be invoked at certain times by the CD user. 
     * All bounding box information must be up-to-date if the indexing
     * method is to work.
     *
     * CDReflect will return False if malloc fails.  Otherwise, True is
     * returned.
     *
     * Here's the problem.
     * Suppose we have a symbol called Load and there is an instance
     * of it in NAND.  Then, there is a master list desc for Load in
     * NAND's master list.  Suppose Load is edited and its BB changes. 
     * Then, the instance desc for the instance of Load in NAND will be wrong. 
     * Calling CDReflect(Load) reflects the change to Load's BB to all
     * symbols that DIRECTLY or INDIRECTLY reference it. 
     *
     * In one sentence, here is when you must invoke CDReflect:
     *     You have opened a symbol and edited it so that its bounding box
     *     has changed and you are done editing it for the time being.
     */
    int Int1;
    int L,B,R,T;
    struct bu *Bucket;
    struct m *MasterListDesc;

    /*
     * Here's the algorithm.
     * Let the name of the symbol we are reflecting be S.
     *
     * Recompute BB of S.
     * CDUpdate does this.
     *
     * For each symbol named i, i != S, in CD's hash table of symbol descs, 
     * do the following. 
     *
     * For each master named M, if any, in master list of i, do the following.
     *     If M == S and M's BB != S's BB, do the following.
     *         M's BB = S's BB.
     *         Patch up i's instance descs that reference M.
     *         Update i's BB.
     *         Invoke Reflect(i) recursively.
     */

#ifdef DEBUGREFLECT
printf("Begin Reflect(%s).\n",SymbolDesc->sName);
printf("    Old BB is %d %d %d %d.\n",SymbolDesc->sLeft,SymbolDesc->sBottom,
    SymbolDesc->sRight,SymbolDesc->sTop);
#endif

    /*
     * Recompute BB of S.
     */
    if (Not CDBB(SymbolDesc,(struct o *)NULL,&L,&B,&R,&T))
        return (False);

#ifdef DEBUGREFLECT
printf("    New BB is %d %d %d %d.\n",SymbolDesc->sLeft,SymbolDesc->sBottom,
    SymbolDesc->sRight,SymbolDesc->sTop);
#endif

    for (Int1 = 0;Int1 < CDNUMLAYERS;++Int1) {
        Bucket = CDSymbolTable[Int1];
        loop
            if (Bucket == NULL)
                break;
            /*
             * SymbolDesc is the desc for S.
             * Bucket->buSymbolDesc is the desc for i.
             */
            elif (strcmp(Bucket->buSymbolDesc->sName,SymbolDesc->sName) == 0)
                Bucket = Bucket->buSucc;
            else {

#ifdef DEBUGREFLECT
printf("    Begin traversing master list of %s.\n",Bucket->buSymbolDesc->sName);
#endif

                MasterListDesc = Bucket->buSymbolDesc->sMasterList;
                loop
                    if (MasterListDesc == NULL) {

#ifdef DEBUGREFLECT
printf("    End traversing master list of %s.\n",Bucket->buSymbolDesc->sName);
#endif

                        break;
                    }
                    else {
                        /*
                         * MasterListDesc->mName is M.
                         */

#ifdef DEBUGREFLECT
printf("Considering %s.\n",MasterListDesc->mName);
printf("BB is %d %d %d %d.\n",MasterListDesc->mLeft,MasterListDesc->mBottom, 
    MasterListDesc->mRight,MasterListDesc->mTop); 
#endif

                        if (strcmp(MasterListDesc->mName,SymbolDesc->sName) == 0
                            And (MasterListDesc->mLeft != SymbolDesc->sLeft Or 
                            MasterListDesc->mBottom != SymbolDesc->sBottom Or 
                            MasterListDesc->mRight != SymbolDesc->sRight Or 
                            MasterListDesc->mTop != SymbolDesc->sTop)) {

#ifdef DEBUGREFLECT
printf("BB conflict.\n");
#endif

                            MasterListDesc->mLeft = SymbolDesc->sLeft; 
                            MasterListDesc->mBottom = SymbolDesc->sBottom; 
                            MasterListDesc->mRight = SymbolDesc->sRight; 
                            MasterListDesc->mTop = SymbolDesc->sTop;
                            /*
                             * Patch up instance descs.
                             * A very big loop.
                             * Warrants its own routine.
                             */
                            if (Not CDPatchInstances(Bucket->buSymbolDesc,
                                MasterListDesc->mName)) return (False);
                            /*
                             * Recompute i's BB.
                             */
                            Bucket->buSymbolDesc->sBBValid = False;
                            if (Not CDBB(Bucket->buSymbolDesc,(struct o *)NULL,
                                &L,&B,&R,&T)) return (False);
                            /*
                             * Reflect changes up the hierarchy of
                             * instance references.
                             */
                            if (Not CDReflect(Bucket->buSymbolDesc))
                                return (False);
                        }
                        MasterListDesc = MasterListDesc->mSucc;
                    };
                Bucket = Bucket->buSucc;
             };
    }

#ifdef DEBUGREFLECT
printf("End Reflect(%s).\n",SymbolDesc->sName);
#endif

    return (True);
}


int
CDPatchInstances(SymbolDesc,MasterName)

struct s *SymbolDesc;
char *MasterName;
{
    /*
     * This routine will delete all instances of MasterName in the symbol
     * pointed to by SymbolDesc, and then recreate the same instance in the
     * symbol.  The result is to reflect any change in the BB of MasterName
     * in the symbol.
     *
     * CDPatchInstances will return False if malloc fails.
     * Otherwise, True is returned.
     */
    struct g *GenDesc;
    struct o *OldPntr,*NewPntr;
    struct t *TGen;
    struct prpty *PrptyDesc;
    char *SymbolName;
    int X,Y;
    int NumX,NumY;
    int DX,DY;
    int OldL,OldB,OldR,OldT;
    int NewL,NewB,NewR,NewT;
    char Type;

#ifdef DEBUGREFLECT
printf("Begin patching instances of master %s for symbol %s.\n",MasterName,SymbolDesc->sName); 
#endif

    if (MasterName == NULL)
        return (True);
    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return (CDError(CDMALLOCFAILED));
    loop {
        CDGen(SymbolDesc,GenDesc,&OldPntr);
        if (OldPntr == NULL)
            break;
        CDCall(OldPntr,&SymbolName,&NumX,&DX,&NumY,&DY);
        if (SymbolName == NULL)
            continue;
        if (*SymbolName == '\0' Or *MasterName == '\0')
            continue;
        if (strcmp(SymbolName,MasterName) != 0)
            continue;
        if (Not CDBB(SymbolDesc,OldPntr,&OldL,&OldB,&OldR,&OldT))
            return (False);

#ifdef DEBUGREFLECT
printf("    Old BB is %d %d %d %d.\n",OldL,OldB,OldR,OldT); 
#endif

        /* we can assume here that the only error is CDMALLOCFAILED */
        if (Not CDBeginMakeCall(SymbolDesc,SymbolName,NumX,DX,NumY,DY,&NewPntr))
            if (CDStatusInt == CDMALLOCFAILED)
                return (CDError(CDMALLOCFAILED));
        CDInitTGen(OldPntr,&TGen);
        loop {
            CDTGen(&TGen,&Type,&X,&Y);
            if (TGen == NULL)
                break;
            else
                if (Not CDT(NewPntr,Type,X,Y))
                    return (CDError(CDMALLOCFAILED));
        }
        if (Not CDEndMakeCall(SymbolDesc,NewPntr))
            return (CDError(CDMALLOCFAILED));
        /* copy the property list */
        CDProperty(SymbolDesc,OldPntr,&PrptyDesc);
        while(PrptyDesc != NULL) {
            if (Not (CDAddProperty(SymbolDesc,NewPntr,PrptyDesc->prpty_Value,
                PrptyDesc->prpty_String))) {
                return (CDError(CDMALLOCFAILED));
            }
            PrptyDesc = PrptyDesc->prpty_Succ;
        }
        if (Not CDBB(SymbolDesc,NewPntr,&NewL,&NewB,&NewR,&NewT))
            return (CDError(CDMALLOCFAILED));

#ifdef DEBUGREFLECT
printf("    New BB is %d %d %d %d.\n",NewL,NewB,NewR,NewT); 
#endif

        /*
         * The generator may return an instance desc that was created in
         * this loop.  So, remove any duplicates.
         */
        if (OldL != NewL Or OldB != NewB Or OldR != NewR Or OldT != NewT)
            /*
             * Remove desc with invalid BB. 
             */
            CDDelete(SymbolDesc,OldPntr);
        else 
            /*
             * Remove duplicate.
             */
            CDDelete(SymbolDesc,NewPntr);
    }

#ifdef DEBUGREFLECT
printf("End patching instances of master %s for symbol %s.\n",MasterName,SymbolDesc->sName); 
#endif

    return (True);
}




/*======================================================================*
 *                                                                      *
 *                OOO  BBBB      J EEEEE  CCCC TTTTT                    *
 *               O   O B   B     J E     C       T                      *
 *               O   O BBBB      J EEE   C       T                      *
 *               O   O B   B J   J E     C       T                      *
 *                OOO  BBBB   JJJ  EEEEE  CCCC   T                      *
 *                                                                      *
 *          CCCC RRRR  EEEEE   A   TTTTT III  OOO  N   N                *
 *         C     R   R E      A A    T    I  O   O NN  N                *
 *         C     RRRR  EEE   A   A   T    I  O   O N N N                *
 *         C     R R   E     AAAAA   T    I  O   O N  NN                *
 *          CCCC R  R  EEEEE A   A   T   III  OOO  N   N                *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *      CDMakeBox(SymbolDesc,Layer,Length,Width,X,Y,Pointer)            *
 *      CDMakeLabel(SymbolDesc,Layer,Label,X,Y,Xform,Pointer)                 *
 *      CDMakePolygon(SymbolDesc,Layer,Path,Pointer)                    *
 *      CDMakeWire(SymbolDesc,Layer,Width,Path,Pointer)                 *
 *      CDMakeRoundFlash(SymbolDesc,Layer,Width,X,Y,Pointer)            *
 *      CDBeginMakeCall(SymbolDesc,SymbolName,NumX,DX,NumY,DY,Pointer)  *
 *      CDEndMakeCall(SymbolDesc,Pointer)                               *
 *      CDInsertObjectDesc(SymbolDesc,ObjectDesc)                       *
 *      CDCheckPath(Path)                                               *
 *                                                                      *
 *======================================================================*/

int
CDMakeBox(SymbolDesc,Layer,Length,Width,X,Y,Pointer)

struct s *SymbolDesc;
int Layer;
int Length,Width,X,Y;
struct o **Pointer;
{
    struct o *ObjectDesc;
    if (Length == 0 Or Width == 0)
        return (CDError(CDBADBOX));
    if ((ObjectDesc = alloc(o)) == NULL)
        return (CDError(CDMALLOCFAILED));
    Length = abs(Length);
    Width = abs(Width);
    ObjectDesc->oRep = NULL;
    ObjectDesc->oPrptyList = NULL;
    ObjectDesc->oInfo = 0;
    ObjectDesc->oType = CDBOX;
    ObjectDesc->oLayer = Layer;
    ObjectDesc->oLeft = X-Length/2;
    ObjectDesc->oBottom = Y-Width/2;
    ObjectDesc->oRight = X+Length/2;
    ObjectDesc->oTop = Y+Width/2;
    *Pointer = ObjectDesc; 
    if (Not CDInsertObjectDesc(SymbolDesc,ObjectDesc))
        return (False);
    return (True);
}


int
CDMakeLabel(SymbolDesc,Layer,Label,X,Y,Xform,Pointer)

struct s *SymbolDesc;
int Layer;
char *Label;
int X,Y;
char Xform;
struct o **Pointer;
{
    struct la *LabelDesc;
    struct o *ObjectDesc;
    unsigned int size;

    if ((LabelDesc = alloc(la)) == NULL)
        return (CDError(CDMALLOCFAILED));
    if ((ObjectDesc = alloc(o)) == NULL)
        return (CDError(CDMALLOCFAILED));
    size = strlen(Label) + 2;
    if ((LabelDesc->laLabel = malloc(size)) == NULL)
        return (CDError(CDMALLOCFAILED));
    strcpy(LabelDesc->laLabel,Label);
    LabelDesc->laX = X;
    LabelDesc->laY = Y;
    LabelDesc->laXform = Xform;
    ObjectDesc->oRep = (struct o *)LabelDesc;
    ObjectDesc->oPrptyList = NULL;
    ObjectDesc->oInfo = 0;
    ObjectDesc->oType = CDLABEL;
    ObjectDesc->oLayer = Layer;

    /* give label a small BB */
    ObjectDesc->oLeft   = X;
    ObjectDesc->oRight  = X + 400;
    ObjectDesc->oBottom = Y;
    ObjectDesc->oTop    = Y + 200;
    /*
    ObjectDesc->oLeft = ObjectDesc->oRight = X;
    ObjectDesc->oBottom = ObjectDesc->oTop = Y;
    */

    *Pointer = ObjectDesc; 
    if (Not CDInsertObjectDesc(SymbolDesc,ObjectDesc))
        return (False);
    return (True);
}


int
CDMakePolygon(SymbolDesc,Layer,Path,Pointer)

struct s *SymbolDesc;
int Layer;
struct p *Path;
struct o **Pointer;
{
    struct po *PolygonDesc;
    struct o *ObjectDesc;
    struct p *Pair;
    int i;

    CDCheckPath(Path);
    for (i = 0, Pair = Path; Pair; Pair = Pair->pSucc, i++) ;
    if (!CDBogusPoly && i < 4) {
        *Pointer = NULL;
        return (True);
    }

    if ((PolygonDesc = alloc(po)) == NULL)
        return (CDError(CDMALLOCFAILED));
    if ((ObjectDesc = alloc(o)) == NULL)
        return (CDError(CDMALLOCFAILED));
    PolygonDesc->poPath = Path;
    ObjectDesc->oRep = (struct o *)PolygonDesc;
    ObjectDesc->oPrptyList = NULL;
    ObjectDesc->oInfo = 0;
    ObjectDesc->oType = CDPOLYGON;
    ObjectDesc->oLayer = Layer;
    ObjectDesc->oLeft = ObjectDesc->oBottom = CDINFINITY;
    ObjectDesc->oRight = ObjectDesc->oTop = -CDINFINITY;
    Pair = Path;
    while(Pair != NULL) {
        if (ObjectDesc->oLeft > Pair->pX)
            ObjectDesc->oLeft = Pair->pX;
        if (ObjectDesc->oRight < Pair->pX)
            ObjectDesc->oRight = Pair->pX;
        if (ObjectDesc->oBottom > Pair->pY)
            ObjectDesc->oBottom = Pair->pY;
        if (ObjectDesc->oTop < Pair->pY)
            ObjectDesc->oTop = Pair->pY;
        Pair = Pair->pSucc;
    }
    *Pointer = ObjectDesc; 
    if (Not CDInsertObjectDesc(SymbolDesc,ObjectDesc))
        return (False);
    return (True);
}


int
CDMakeWire(SymbolDesc,Layer,Width,Path,Pointer)

struct s *SymbolDesc;
int Layer;
int Width;
struct p *Path;
struct o **Pointer;
{
    struct w *WireDesc;
    struct o *ObjectDesc;
    struct p *Pair;

    if ((WireDesc = alloc(w)) == NULL)
        return (CDError(CDMALLOCFAILED));
    if ((ObjectDesc = alloc(o)) == NULL)
        return (CDError(CDMALLOCFAILED));
    CDCheckPath(Path);
    if (Width  < 0) Width = -Width;
    WireDesc->wWidth = Width;
    WireDesc->wPath = Path;
    ObjectDesc->oRep = (struct o *)WireDesc;
    ObjectDesc->oPrptyList = NULL;
    ObjectDesc->oInfo = 0;
    ObjectDesc->oType = CDWIRE;
    ObjectDesc->oLayer = Layer;
    ObjectDesc->oLeft = ObjectDesc->oBottom = CDINFINITY;
    ObjectDesc->oRight = ObjectDesc->oTop = -CDINFINITY;
    Pair = Path;
    while(Pair != NULL) {
        if (ObjectDesc->oLeft > Pair->pX-Width/2)
            ObjectDesc->oLeft = Pair->pX-Width/2;
        if (ObjectDesc->oRight < Pair->pX+Width/2)
            ObjectDesc->oRight = Pair->pX+Width/2;
        if (ObjectDesc->oBottom > Pair->pY-Width/2)
            ObjectDesc->oBottom = Pair->pY-Width/2;
        if (ObjectDesc->oTop < Pair->pY+Width/2)
            ObjectDesc->oTop = Pair->pY+Width/2;
        Pair = Pair->pSucc;
    }
    *Pointer = ObjectDesc; 
    if (Not CDInsertObjectDesc(SymbolDesc,ObjectDesc))
        return (False);
    return (True);
}


int
CDMakeRoundFlash(SymbolDesc,Layer,Width,X,Y,Pointer)

struct s *SymbolDesc;
int Layer;
int Width,X,Y;
struct o **Pointer;
{
    struct r *RoundFlashDesc;
    struct o *ObjectDesc;

    if ((RoundFlashDesc = alloc(r)) == NULL)
        return (CDError(CDMALLOCFAILED));
    if ((ObjectDesc = alloc(o)) == NULL)
        return (CDError(CDMALLOCFAILED));
    RoundFlashDesc->rWidth = Width;
    RoundFlashDesc->rX = X;
    RoundFlashDesc->rY = Y;
    ObjectDesc->oRep = (struct o *)RoundFlashDesc;
    ObjectDesc->oPrptyList = NULL;
    ObjectDesc->oInfo = 0;
    ObjectDesc->oType = CDROUNDFLASH;
    ObjectDesc->oLayer = Layer;
    ObjectDesc->oLeft = X-Width/2;
    ObjectDesc->oBottom = Y-Width/2;
    ObjectDesc->oRight = X+Width/2;
    ObjectDesc->oTop = Y+Width/2;
    *Pointer = ObjectDesc; 
    if (Not CDInsertObjectDesc(SymbolDesc,ObjectDesc))
        return (False);
    return (True);
}


int
CDBeginMakeCall(SymbolDesc,SymbolName,NumX,DX,NumY,DY,Pointer)

struct s *SymbolDesc;
char *SymbolName;
struct o **Pointer;
int NumX,NumY;
int DX,DY;
{
    /*
     * CDBeginMakeCall will return False if a CDMALLOCFAILED error
     * occurs or if CDOpen fails.  Possible values for CDStatusInt
     * upon return are:
     *   (Fatal)-CDPARSEFAILED - Syntax error
     *   (Fatal)-CDMALLOCFAILED - Out of memory
     *   (Fatal)-CDNEWSYMBOL - symbol does not exist
     *           CDOLDSYMBOL - successful return, symbol exists
     * Only CDOLDSYMBOL is not a fatal return (i.e., CDBeginMakeCall
     * returns True).
     */
    struct c *CallDesc;
    struct o *ObjectDesc;
    struct m *MasterListDesc;
    struct s *MasterSymbolDesc;
    struct prpty *PrptyDesc;
    unsigned int size;

    /* The symbol call is inserted into the database CDEndMakeCall */
    if ((CallDesc = alloc(c)) == NULL)
        return (CDError(CDMALLOCFAILED));
    if ((ObjectDesc = alloc(o)) == NULL)
        return (CDError(CDMALLOCFAILED));
    CallDesc->cNumX = NumX;
    CallDesc->cDX = DX;
    CallDesc->cNumY = NumY;
    CallDesc->cT = NULL;
    CallDesc->cDY = DY;
    ObjectDesc->oRep = (struct o *)CallDesc;
    ObjectDesc->oPrptyList = NULL;
    ObjectDesc->oInfo = 0;
    ObjectDesc->oType = CDSYMBOLCALL;
    ObjectDesc->oLayer = 0;
    *Pointer = ObjectDesc; 
    MasterListDesc = SymbolDesc->sMasterList;
    /*
     * Search masterList for an instance of SymbolName.
     * If not found, insert into the masterList (which is a linked list).
     * An entry in the master list contains the symbol name, the bounding
     * box, and the number of references.
     */
    loop{
        if (MasterListDesc == NULL) {
            /*
             * Insert into list
             * Firstly, try to allocate memory
             */
            if ((MasterListDesc = alloc(m)) == NULL)
                return (CDError(CDMALLOCFAILED));
            size = strlen(SymbolName) + 2;
            if ((MasterListDesc->mName = malloc(size)) == NULL)
                return (CDError(CDMALLOCFAILED));
            /* insert new instance at end of (linked) masterList */
            if (SymbolDesc->sMasterList != NULL)
                SymbolDesc->sMasterList->mPred = MasterListDesc;
            MasterListDesc->mSucc = SymbolDesc->sMasterList;
            MasterListDesc->mPred = NULL;
            SymbolDesc->sMasterList = MasterListDesc;
            MasterListDesc->mReferenceCount = 0;
            strcpy(MasterListDesc->mName,SymbolName);
            if (CDDesc.dControl == DCONTROLVANILLA) {
                /*
                 * DCONTROLVANILLA tells us that CDOpen is safe to call here.
                 * Because StatusInt is checked, we ignore the returned value.
                 */
                CDOpen(MasterListDesc->mName,&MasterSymbolDesc,'r');
                if (CDStatusInt == CDPARSEFAILED Or CDStatusInt == CDNEWSYMBOL) {
                    /* CDOpen failed -- Don't put bad master in MasterList */
                    if (CDStatusInt == CDNEWSYMBOL)
                        sprintf(CDStatusString,"Symbol %s not found.",
                            MasterListDesc->mName);
                    if (SymbolDesc->sMasterList != NULL)
                        SymbolDesc->sMasterList->mPred = NULL;
                    strcpy(MasterListDesc->mName,"");
                    SymbolDesc->sMasterList = MasterListDesc->mSucc;
                    afree(MasterListDesc,m);
                    return (False);
                }
                if (Not CDBB(MasterSymbolDesc,(struct o *)NULL,
                    &(MasterListDesc->mLeft),&(MasterListDesc->mBottom),
                    &(MasterListDesc->mRight),&(MasterListDesc->mTop)))
                        return (False);
            }
            elif (CDDesc.dControl == DCONTROLCDOPEN Or
                CDDesc.dControl == DCONTROLPCIF) {
                /*
                 * We CANNOT invoke CDBB here, because if the master symbol
                 * hasn't already been mapped into main memory via CDOpen,
                 * CDBB will invoke CDOpen which will then invoke PCIF and
                 * since PCIF CANNOT be invoked recursively, all of PCIF's
                 * state will be broken.  The solution is to defer filling
                 * in the bounding box information for master list descs and
                 * instance descs until PCIF has returned in CDOpen.  Why not
                 * write PCIF so it can be invoked recursively? For a deep
                 * hierarchy, we might exceed the limit on open file
                 * descriptors and we clearly don't want to limit hierarchy
                 * depth.
                 */
                MasterListDesc->mLeft = MasterListDesc->mBottom = 
                    MasterListDesc->mRight = MasterListDesc->mTop = 0; 
            }
            break;
        }
        elif (strcmp(SymbolName,MasterListDesc->mName) == 0) {
            /*
             * A match!  This symbol is already in memory.
             */

            /* SRW */
            if (CDDesc.dControl == DCONTROLVANILLA)
                CDSymbol(MasterListDesc->mName,&MasterSymbolDesc);

            CDStatusInt = CDOLDSYMBOL;
            *CDStatusString = '\0';
            break;
        }
        else
            MasterListDesc = MasterListDesc->mSucc;
    }
    CallDesc->cMaster = MasterListDesc;
    MasterListDesc->mReferenceCount++;

    /* SRW
     * This version of CD is used for a schematic editor, in which
     * connectivity is established by maintaining terminal coordinates
     * in property strings.  As we add a cell (DCONTROLVANILLA set),
     * the property strings contain connection nodes with coordinates
     * relative to the cell.  These coordinates are transformed to be
     * relative to the parent cell by a routine called in
     * CDEndMakeCall().
     */
    if (CDDesc.dControl == DCONTROLVANILLA) {
        CDProperty(MasterSymbolDesc,(struct o *)NULL,&PrptyDesc);
        while (PrptyDesc) {
            CDAddProperty(SymbolDesc,ObjectDesc,
                PrptyDesc->prpty_Value,PrptyDesc->prpty_String);
            PrptyDesc = PrptyDesc->prpty_Succ;
        }
    }

    return (True);
}


int
CDT(Pointer,Type,X,Y)

struct o *Pointer;
char Type;
int X,Y;
{
    /*
     * After invoking BeginMakeCall, invoke T for each transformation in
     * the call.  The transformation is a linked list of transformation
     * descs headed by the ct field of the call desc.  Finally, invoke
     * EndMakeCall.
     */
    struct c *CDesc;
    struct t *TDesc;

    CDesc = (struct c *)Pointer->oRep; 
    TDesc = CDesc->cT;    
    if (TDesc == NULL) {
        if ((CDesc->cT = TDesc = alloc(t)) == NULL)
            return (CDError(CDMALLOCFAILED));
        TDesc->tSucc = NULL;
        TDesc->tX = X;
        TDesc->tY = Y;
        TDesc->tType = Type; 
        return (True);
    }
    while(TDesc->tSucc != NULL)
        TDesc = TDesc->tSucc;
    if ((TDesc = TDesc->tSucc = alloc(t)) == NULL)
        return (CDError(CDMALLOCFAILED));
    TDesc->tSucc = NULL;
    TDesc->tX = X;
    TDesc->tY = Y;
    TDesc->tType = Type;
    return (True);
}


int
CDEndMakeCall(SymbolDesc,Pointer)

struct s *SymbolDesc;
struct o *Pointer;
{
    struct c *CallDesc;
    struct o *ObjectDesc;
    struct m *MasterListDesc;
    struct t *TGen;
    int X,Y;
    char Type;
    int tf[9];
    int a, b, c, d, tx, ty;

    ObjectDesc = Pointer;
    CallDesc = (struct c *)ObjectDesc->oRep;
    MasterListDesc = CallDesc->cMaster;
    if (TFull())
        return (CDError(CDXFORMSTACKFULL));
    TPush();
    TIdentity();
    CDInitTGen(Pointer,&TGen);
    loop {
        CDTGen(&TGen,&Type,&X,&Y);
        if (TGen == NULL)
            break;
        if (Type == CDROTATE)
            TRotate(X,Y);
        elif (Type == CDTRANSLATE)
            TTranslate(X,Y);
        elif (Type == CDMIRRORX)
            TMX();
        elif (Type == CDMIRRORY)
            TMY();
    }

    /* Simplify the transform, since kictostr doesn't handle more than
     * one translation, so that LLREF fails.  The resulting transform
     * uses a maximum of one each MY, rotation, translation.  The matrix
     * is in the form | a  c  0 |
     *                | b  d  0 |
     *                | tx ty 1 |
     * where a = tf[0], c = tf[1], etc.
     */

    TCurrent(tf);
    a = tf[0];
    c = tf[1];
    b = tf[3];
    d = tf[4];
    tx = tf[6];
    ty = tf[7];

    TGen = CallDesc->cT;
    CallDesc->cT = 0;
    while (TGen) {
        struct t *tn = TGen->tSucc;
        free(TGen);
        TGen = tn;
    }

    /* Now reset the cell's transform */
    if ((a && (a == -d)) || (b && (b == c))) {
        if (!CDT(Pointer,CDMIRRORY,0L,0L))
            return (CDError(CDMALLOCFAILED));
    }
    if (!a && c) {
        if (c > 0) {
            /* 90 */
            if (!CDT(Pointer,CDROTATE,0L,1L))
                return (CDError(CDMALLOCFAILED));
        }
        else {
            /* 270 */
            if (!CDT(Pointer,CDROTATE,0L,-1L))
                return (CDError(CDMALLOCFAILED));
        }
    }
    else if (a && !c) {
        if (a < 0) {
            /* 180 */
            if (!CDT(Pointer,CDROTATE,-1L,0L))
                return (CDError(CDMALLOCFAILED));
        }
    }
    if (tx || ty)
        if (!CDT(Pointer,CDTRANSLATE,tx,ty))
            return (CDError(CDMALLOCFAILED));

#ifdef DEBUGREFLECT 
printf("Making call of master %s in symbol %s.\n",MasterListDesc->mName,
    SymbolDesc->sName);
printf("Untransformed (master's) BB is %d %d %d %d.\n",
    MasterListDesc->mLeft,MasterListDesc->mBottom,
    MasterListDesc->mRight,MasterListDesc->mTop);
#endif

    ObjectDesc->oLeft = MasterListDesc->mLeft; 
    ObjectDesc->oBottom = MasterListDesc->mBottom;
    TPoint(&(ObjectDesc->oLeft),&(ObjectDesc->oBottom));
    ObjectDesc->oRight = MasterListDesc->mRight; 
    ObjectDesc->oTop = MasterListDesc->mTop;

#ifdef DEBUGREFLECT 
    TPoint(&(ObjectDesc->oRight),&(ObjectDesc->oTop));
    if (ObjectDesc->oRight < ObjectDesc->oLeft)
        SwapInts(ObjectDesc->oLeft,ObjectDesc->oRight);
    if (ObjectDesc->oTop < ObjectDesc->oBottom)
        SwapInts(ObjectDesc->oBottom,ObjectDesc->oTop);
printf("Transformed, unarrayed BB is %d %d %d %d.\n",
    ObjectDesc->oLeft,ObjectDesc->oBottom,
    ObjectDesc->oRight,ObjectDesc->oTop);
#endif

    ObjectDesc->oRight += (CallDesc->cNumX-1)*CallDesc->cDX;
    ObjectDesc->oTop += (CallDesc->cNumY-1)*CallDesc->cDY;
    TPoint(&(ObjectDesc->oRight),&(ObjectDesc->oTop));
    if (ObjectDesc->oRight < ObjectDesc->oLeft)
        SwapInts(ObjectDesc->oLeft,ObjectDesc->oRight);
    if (ObjectDesc->oTop < ObjectDesc->oBottom)
        SwapInts(ObjectDesc->oBottom,ObjectDesc->oTop);

#ifdef DEBUGREFLECT 
printf("Transformed, arrayed BB is %d %d %d %d.\n",
    ObjectDesc->oLeft,ObjectDesc->oBottom,
    ObjectDesc->oRight,ObjectDesc->oTop);
#endif

    /* 
     * UpdateProperties() must be supplied externally.  The intended
     * purpose of this routine is to update properties that require as
     * input the transformed coordinates of points within the cell.
     * The property strings may not be present unless DCONTROLVANILLA.
     * See the note in CDBeginMakeCall().
     */
    if (CDDesc.dControl == DCONTROLVANILLA)
        UpdateProperties(ObjectDesc);

    TPop();
    if (Not CDInsertObjectDesc(SymbolDesc,ObjectDesc))
        return (False);
    return (True);
}


void
CDCheckPath(Path)

struct p *Path;
{
    /*
     * Check to see that the path does not have two identical and
     * adjacent vertices.
     */
    struct p *Pair;
    struct p *Copy;
    Pair = Path;
    while(Pair->pSucc != NULL) {
        if (Pair->pX == Pair->pSucc->pX And Pair->pY == Pair->pSucc->pY) {
            Copy = Pair->pSucc;
            Pair->pSucc = Copy->pSucc;
            afree(Copy,p);
        }
        else
            Pair = Pair->pSucc;
    }
}


int
CDInsertObjectDesc(SymbolDesc,ObjectDesc)

struct s *SymbolDesc;
struct o *ObjectDesc;
{
    int Int1,Int2,Layer;
    int X,Y;
    int BeginX,EndX,BeginY,EndY;

    CDIntersect(ObjectDesc->oLeft,ObjectDesc->oBottom,ObjectDesc->oRight,
        ObjectDesc->oTop,&BeginX,&EndX,&BeginY,&EndY);
    /* SRW ** always put labels in residual bin, since the size is
     * not known.
     */
    if (BeginX != EndX Or BeginY != EndY Or ObjectDesc->oType == CDLABEL)
        X = Y = 0;
    else {
        X = BeginX; 
        Y = BeginY; 
    }
    Layer = ObjectDesc->oLayer;
    if (SymbolDesc->sBin[Layer] == (struct o ***)NULL) {
        /* allocate Bin */
        if ((SymbolDesc->sBin[Layer] = (struct o ***)
            malloc(sizeof(char*) * (CDNUMBINS+1))) == NULL)
            return (CDError(CDMALLOCFAILED));
        for (Int1 = 0; Int1 <= CDNUMBINS; ++Int1) {
            if ((SymbolDesc->sBin[Layer][Int1] = (struct o **)
                malloc(sizeof(char*) * (CDNUMBINS+1))) == NULL)
                return (CDError(CDMALLOCFAILED));
            for (Int2 = 0; Int2 <= CDNUMBINS; ++Int2) {
                if ((SymbolDesc->sBin[Layer][Int1][Int2] = (struct o *)
                    malloc(sizeof(char*))) == NULL)
                    return (CDError(CDMALLOCFAILED));
                SymbolDesc->sBin[Layer][Int1][Int2] = (struct o *)NULL;
            }
        }
    }
    elif (SymbolDesc->sBin[Layer][X][Y] != NULL)
        SymbolDesc->sBin[Layer][X][Y]->oPred = ObjectDesc;
    ObjectDesc->oSucc = SymbolDesc->sBin[Layer][X][Y];
    SymbolDesc->sBin[Layer][X][Y] = ObjectDesc;
    ObjectDesc->oPred = NULL;
    SymbolDesc->sLeft = min(SymbolDesc->sLeft,ObjectDesc->oLeft); 
    SymbolDesc->sBottom = min(SymbolDesc->sBottom,ObjectDesc->oBottom); 
    SymbolDesc->sRight = max(SymbolDesc->sRight,ObjectDesc->oRight); 
    SymbolDesc->sTop = max(SymbolDesc->sTop,ObjectDesc->oTop); 

#ifdef DEBUGGEN
if (X == 0 And Y == 0)
    printf("Inserting a desc on layer %d in residual bin.\n",Layer);
else
    printf("Inserting a desc on layer %d in bin (%d,%d).\n",Layer,X,Y);
#endif

#ifdef DEBUGREFLECT
if (X == 0 And Y == 0)
    printf("Inserting a desc on layer %d in residual bin.\n",Layer);
else
    printf("Inserting a desc on layer %d in bin (%d,%d).\n",Layer,X,Y);
#endif

    return (True);
}




/*======================================================================*
 *                                                                      *
 *                 OOO  BBBB      J EEEEE  CCCC TTTTT                   *
 *                O   O B   B     J E     C       T                     *
 *                O   O BBBB      J EEE   C       T                     *
 *                O   O B   B J   J E     C       T                     *
 *                 OOO  BBBB   JJJ  EEEEE  CCCC   T                     *
 *                                                                      *
 *           DDDD  EEEEE L     EEEEE TTTTT III  OOO  N   N              *
 *           D   D E     L     E       T    I  O   O NN  N              *
 *           D   D EEE   L     EEE     T    I  O   O N N N              *
 *           D   D E     L     E       T    I  O   O N  NN              *
 *           DDDD  EEEEE LLLLL EEEEE   T   III  OOO  N   N              *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *        CDDeleteObjectDesc(SymbolDesc,ObjectDesc)                     *
 *                                                                      *
 *======================================================================*/

void
CDDeleteObjectDesc(SymbolDesc,ObjectDesc)

struct s *SymbolDesc;
struct o *ObjectDesc;
{
    int Layer;
    int X,Y;
    int BeginX,EndX,BeginY,EndY;
    struct p PCopy;
    struct t TCopy;
    struct prpty  PrptyCopy;
    struct prpty *PrptyDesc;

    /* we should test the descriptors as valid pointers */
    Layer = ObjectDesc->oLayer;
    /* is the Bin allocated? */
    if (SymbolDesc->sBin[Layer] == NULL Or ObjectDesc == NULL)
        return;
    CDIntersect(ObjectDesc->oLeft,ObjectDesc->oBottom,ObjectDesc->oRight,
        ObjectDesc->oTop,&BeginX,&EndX,&BeginY,&EndY);
    /* SRW ** labels are always in residual bin */
    if (BeginX != EndX Or BeginY != EndY Or ObjectDesc->oType == CDLABEL)
        X = Y = 0;
    else {
        X = BeginX; 
        Y = BeginY; 
    }
    if (SymbolDesc->sBin[Layer][X][Y] == NULL)
        /* Something's rotten */
        return;
    elif (ObjectDesc->oPred == NULL And ObjectDesc->oSucc == NULL)
        /* Only desc--has no pred or succ */
        SymbolDesc->sBin[Layer][X][Y] = NULL;
    elif (ObjectDesc->oPred == NULL) {
        /* First desc.    Has a succ, but no pred */
        SymbolDesc->sBin[Layer][X][Y] = ObjectDesc->oSucc;
        ObjectDesc->oSucc->oPred = NULL;
    }
    elif (ObjectDesc->oSucc == NULL)
        /* Last desc--has a pred, but no succ */
        ObjectDesc->oPred->oSucc = NULL; 
    else {
        /* Vanilla desc has a pred and a succ */
        ObjectDesc->oSucc->oPred = ObjectDesc->oPred; 
        ObjectDesc->oPred->oSucc = ObjectDesc->oSucc;
    }
    /*
     * Invalidate BB.
     */
    SymbolDesc->sBBValid = False;
    /*
     * Free storage of property list;
     */
    PrptyDesc = ObjectDesc->oPrptyList;
    while(PrptyDesc != NULL) {
        PrptyCopy = *PrptyDesc;
        if (PrptyDesc->prpty_String != NULL)
            free(PrptyDesc->prpty_String);
        afree(PrptyDesc,prpty);
        PrptyDesc = PrptyCopy.prpty_Succ;
    }
    /*
     * Free storage of oRep;
     */
    if (ObjectDesc->oType == CDROUNDFLASH)
        afree(ObjectDesc->oRep,r);
    elif (ObjectDesc->oType == CDSYMBOLCALL) {
        struct c *CallDesc;
        struct t *TDesc;

        CallDesc = (struct c *)ObjectDesc->oRep; 
        /* SRW ** reduce the master reference count */
        if (CallDesc->cMaster)
            CallDesc->cMaster->mReferenceCount --;
        TDesc = CallDesc->cT;
        while(TDesc != NULL) {
            TCopy = *TDesc;
            afree(TDesc,t);
            TDesc = TCopy.tSucc;
        }
        afree(CallDesc,c);
    }
    elif (ObjectDesc->oType == CDPOLYGON) {
        struct po *PolygonDesc;
        struct p *Pair;

        PolygonDesc = (struct po *)ObjectDesc->oRep; 
        Pair = PolygonDesc->poPath;
        while(Pair != NULL) {
            PCopy = *Pair;
            afree(Pair,p);
            Pair = PCopy.pSucc;
        }
        afree(PolygonDesc,po);
    }
    elif (ObjectDesc->oType == CDWIRE) {
        struct w *WireDesc;
        struct p *Pair;

        WireDesc = (struct w *)ObjectDesc->oRep; 
        Pair = WireDesc->wPath;
        while(Pair != NULL) {
            PCopy = *Pair;
            afree(Pair,p);
            Pair = PCopy.pSucc;
        }
        afree(WireDesc,w);
    }
    afree(ObjectDesc,o);

#ifdef DEBUGGEN
printf("Deleting a desc on layer %d in bin (%d,%d)\n.",Layer,X,Y);
#endif

}



/*======================================================================*
 *                                                                      *
 *           A    CCCC  CCCC EEEEE  SSSS  SSSS III N   N  GGGG          *
 *          A A  C     C     E     S     S      I  NN  N G              *
 *         A   A C     C     EEE    SSS   SSS   I  N N N G GGG          *
 *         AAAAA C     C     E         S     S  I  N  NN G   G          *
 *         A   A  CCCC  CCCC EEEEE SSSS  SSSS  III N   N  GGG           *
 *                                                                      *
 *             OOO  BBBB      J EEEEE  CCCC TTTTT  SSSS                 *
 *            O   O B   B     J E     C       T   S                     *
 *            O   O BBBB      J EEE   C       T    SSS                  *
 *            O   O B   B J   J E     C       T       S                 *
 *             OOO  BBBB   JJJ  EEEEE  CCCC   T   SSSS                  *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *        CDCall(Pointer,SymbolName,NumX,DX,NumY,DY)                    *
 *        CDBox(Pointer,Layer,Length,Width,X,Y)                         *
 *        CDLabel(Pointer,Layer,Label,X,Y,xform)                              *
 *        CDPolygon(Pointer,Layer,Path)                                 *
 *        CDWire(Pointer,Layer,Width,Path)                              *
 *        CDRoundFlash(Pointer,Layer,Width,X,Y)                         *
 *                                                                      *
 *======================================================================*/

void
CDCall(Pointer,SymbolName,NumX,DX,NumY,DY)

struct o *Pointer;
char **SymbolName;
int *NumX,*NumY;
int *DX,*DY;
{
    struct c *CallDesc;

    if (Pointer == NULL)
        return;
    if (Pointer->oType != CDSYMBOLCALL) {
        *SymbolName = NULL;
        *NumX = 0;
        *DX = 0;
        *NumY = 0;
        *DY = 0;
    }
    else {
        CallDesc = (struct c *)Pointer->oRep;
        *SymbolName = CallDesc->cMaster->mName;
        *NumX = CallDesc->cNumX;
        *DX = CallDesc->cDX;
        *NumY = CallDesc->cNumY;
        *DY = CallDesc->cDY;
    }
}


void
CDBox(Pointer,Layer,Length,Width,X,Y)

struct o *Pointer;
int *Layer;
int *Length,*Width,*X,*Y;
{

    if (Pointer == NULL)
        return;
    if (Pointer->oType != CDBOX)
        *Layer = *Length = *Width = *X = *Y = 0;
    else {
        *Layer = Pointer->oLayer;
        *Length = Pointer->oRight - Pointer->oLeft;
        *Width = Pointer->oTop - Pointer->oBottom;
        *X = Pointer->oLeft + (*Length >> 1);
        *Y = Pointer->oBottom + (*Width >> 1);
    }
}


void
CDLabel(Pointer,Layer,Label,X,Y,Xform)

struct o *Pointer;
int *Layer;
char **Label;
int *X,*Y;
char *Xform;
{
    struct la *LabelDesc;

    if (Pointer == NULL)
        return;
    if (Pointer->oType != CDLABEL) {
        *Layer = *X = *Y = 0;
        *Label = NULL;
        *Xform = (char)0;
    }
    else {
        *Layer = Pointer->oLayer;
        LabelDesc = (struct la *)Pointer->oRep;
        *Label = LabelDesc->laLabel;
        *X = LabelDesc->laX;
        *Y = LabelDesc->laY;
        *Xform = LabelDesc->laXform;
    }
}


void
CDPolygon(Pointer,Layer,Path)

struct o *Pointer;
int *Layer;
struct p **Path;
{
    struct po *PolygonDesc;

    if (Pointer == NULL)
        return;
    if (Pointer->oType != CDPOLYGON) {
        *Layer = 0;
        *Path = NULL;
    }
    else {
        *Layer = Pointer->oLayer;
        PolygonDesc = (struct po *)Pointer->oRep;
        *Path = PolygonDesc->poPath;
    }
}


void
CDWire(Pointer,Layer,Width,Path)

struct o *Pointer;
int *Layer;
int *Width;
struct p **Path;
{
    struct w *WireDesc;

    if (Pointer == NULL)
        return;
    if (Pointer->oType != CDWIRE) {
        *Layer = *Width = 0;
        *Path = NULL;
    }
    else {
        *Layer = Pointer->oLayer;
        WireDesc = (struct w *)Pointer->oRep;
        *Width = WireDesc->wWidth;
        *Path = WireDesc->wPath;
    }
}


void
CDRoundFlash(Pointer,Layer,Width,X,Y)

struct o *Pointer;
int *Layer;
int *Width,*X,*Y;
{
    struct r *RoundFlashDesc;

    if (Pointer == NULL)
        return;
    if (Pointer->oType != CDROUNDFLASH)
        return;
    *Layer = Pointer->oLayer;
    RoundFlashDesc = (struct r *)Pointer->oRep;
    *Width = RoundFlashDesc->rWidth;
    *X = RoundFlashDesc->rX;
    *Y = RoundFlashDesc->rY;
}




/*======================================================================*
 *                                                                      *
 *           A    CCCC  CCCC EEEEE  SSSS  SSSS III N   N  GGGG          *
 *          A A  C     C     E     S     S      I  NN  N G              *
 *         A   A C     C     EEE    SSS   SSS   I  N N N G GGG          *
 *         AAAAA C     C     E         S     S  I  N  NN G   G          *
 *         A   A  CCCC  CCCC EEEEE SSSS  SSSS  III N   N  GGG           *
 *                                                                      *
 *    III N   N FFFFF  OOO  RRRR  M   M   A   TTTTT III  OOO  N   N     *
 *     I  NN  N F     O   O R   R MM MM  A A    T    I  O   O NN  N     *
 *     I  N N N FFF   O   O RRRR  M M M A   A   T    I  O   O N N N     *
 *     I  N  NN F     O   O R R   M   M AAAAA   T    I  O   O N  NN     *
 *    III N   N F      OOO  R  R  M   M A   A   T   III  OOO  N   N     *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *        CDInfo(SymbolDesc,Pointer,Info)                               *
 *        CDSetInfo(SymbolDesc,Pointer,Info)                            *
 *        CDProperty(SymbolDesc,Pointer,Property)                       *
 *        CDAddProperty(SymbolDesc,Pointer,Value,String)                *
 *        CDRemoveProperty(SymbolDesc,Pointer,Value)                    *
 *        CDType(Pointer,Type)                                          *
 *        CDBB(SymbolDesc,Pointer,Left,Bottom,Right,Top)                *
 *        CDIntersect(Left,Bottom,Right,Top,BeginX,EndX,BeginY,EndY)    *
 *                                                                      *
 *======================================================================*/

void
CDInfo(SymbolDesc,Pointer,Info)

struct s *SymbolDesc;
struct o *Pointer;
int *Info;
{
    /*
     * Return info field of object.
     * If Pointer == NULL, object is symbol itself. 
     */
    if (Pointer == NULL)
        *Info = SymbolDesc->sInfo;
    else
        *Info = Pointer->oInfo; 
}


void
CDSetInfo(SymbolDesc,Pointer,Info)

struct s *SymbolDesc;
struct o *Pointer;
int Info;
{
    /*
     * Set info field of object.
     * If Pointer == NULL, object is symbol itself. 
     */
    if (Pointer == NULL)
        SymbolDesc->sInfo = Info;
    else
        Pointer->oInfo = Info; 
}


void
CDProperty(SymbolDesc,Pointer,Property)

struct s *SymbolDesc;
struct o *Pointer;
struct prpty **Property;
{
    /*
     * Return info field of object.
     * If Pointer == NULL, object is symbol itself. 
     */
    if (Pointer == NULL)
        *Property = SymbolDesc->sPrptyList;
    else
        *Property = Pointer->oPrptyList; 
}


int
CDAddProperty(SymbolDesc,Pointer,Value,String)

struct s *SymbolDesc;
struct o *Pointer;
int Value;
char *String;
{
    char * cp;
    struct prpty *prptyDesc;
    unsigned int size;

    if ((prptyDesc = alloc(prpty)) == NULL)
        return (CDError(CDMALLOCFAILED));
    size = strlen(String) + 2; 
    if ((cp = prptyDesc->prpty_String = malloc(size)) == NULL)
        return (CDError(CDMALLOCFAILED));
    prptyDesc->prpty_Value = Value;
    /* CD does not use the Info field */
    prptyDesc->prpty_Info = NULL;
    strcpy(prptyDesc->prpty_String,String);
    /* we can't allaow semicolons because of CIF */
    while(cp != NULL && *cp != '\0') {
        if (*cp == ';')
            *cp = ' ';
        ++cp;
    }
    if (Pointer == NULL) {
        prptyDesc->prpty_Succ = SymbolDesc->sPrptyList;
        SymbolDesc->sPrptyList = prptyDesc;
    }
    else {
        prptyDesc->prpty_Succ = Pointer->oPrptyList;
        Pointer->oPrptyList = prptyDesc; 
    }
    return (True);
}


int
CDRemoveProperty(SymbolDesc,Pointer,Value)

struct s *SymbolDesc;
struct o *Pointer;
int Value;
{
    struct prpty *prptyDesc;
    struct prpty *prptyHead;
    struct prpty *prptyCopy;
    struct prpty *prptyTemp;

    if (Pointer == NULL)
        prptyHead = prptyDesc = SymbolDesc->sPrptyList;
    else
        prptyHead = prptyDesc = Pointer->oPrptyList; 

    for (prptyCopy = NULL; prptyDesc != NULL;
        prptyCopy = prptyDesc,prptyDesc = prptyTemp) {
        prptyTemp = prptyDesc->prpty_Succ;

        if (prptyDesc->prpty_Value == Value) {
            if (prptyCopy == NULL)
                prptyHead = prptyDesc->prpty_Succ;
            else
                prptyCopy->prpty_Succ = prptyDesc->prpty_Succ;
            if (prptyDesc->prpty_String) free(prptyDesc->prpty_String);
            if (prptyDesc->prpty_Info) free(prptyDesc->prpty_Info);
            afree(prptyDesc,prpty);
            prptyDesc = prptyCopy;
        }
    }
    if (Pointer == NULL)
        SymbolDesc->sPrptyList = prptyHead;
    else
        Pointer->oPrptyList = prptyHead;
    return True;
}


void
CDType(Pointer,Type)

struct o *Pointer;
char *Type;
{
    /*
     * Returns type of object pointed to by Pointer.
     */
    *Type = Pointer->oType; 
}


int
CDBB(SymbolDesc,Pointer,Left,Bottom,Right,Top)

struct s *SymbolDesc;
struct o *Pointer;
int *Left,*Bottom,*Right,*Top;
{
    /*
     * Return BB of object pointed to by Pointer.
     * If Pointer == NULL, return BB of symbol itself.
     * Basically, we CAN'T afford to recompute the BB of the symbol each time
     * CDDelete or an object creation routine is invoked.
     *
     * If malloc fails, CDBB will return False via CDError.  Otherwise, True
     * is returned.
     */
    struct g *GenDesc;
    int Layer;

    if (Pointer == NULL And SymbolDesc->sBBValid) {
        *Left = SymbolDesc->sLeft;
        *Bottom = SymbolDesc->sBottom;
        *Right = SymbolDesc->sRight;
        *Top = SymbolDesc->sTop;

#ifdef DEBUGREFLECT
printf("CDBB1(%s,%d,%d,%d,%d)\n",SymbolDesc->sName,*Left,*Bottom,*Right,*Top);
#endif 

#ifdef DEBUGGEN
printf("CDBB1(%s,%d,%d,%d,%d)\n",SymbolDesc->sName,*Left,*Bottom,*Right,*Top);
#endif 

        return (True);
    }
    elif (Pointer != NULL) {
        *Left = Pointer->oLeft;
        *Bottom = Pointer->oBottom;
        *Right = Pointer->oRight;
        *Top = Pointer->oTop;

#ifdef DEBUGREFLECT
printf("CDBB2(%s,%d,%d,%d,%d)\n",SymbolDesc->sName,*Left,*Bottom,*Right,*Top);
#endif 

#ifdef DEBUGGEN
printf("CDBB2(%s,%d,%d,%d,%d)\n",SymbolDesc->sName,*Left,*Bottom,*Right,*Top);
#endif 

        return (True);
    }
    SymbolDesc->sLeft = SymbolDesc->sBottom = CDINFINITY;
    SymbolDesc->sRight = SymbolDesc->sTop = -CDINFINITY;
    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return (CDError(CDMALLOCFAILED));
    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            break;
        SymbolDesc->sLeft = min(SymbolDesc->sLeft,Pointer->oLeft); 
        SymbolDesc->sBottom = min(SymbolDesc->sBottom,Pointer->oBottom); 
        SymbolDesc->sRight = max(SymbolDesc->sRight,Pointer->oRight); 
        SymbolDesc->sTop = max(SymbolDesc->sTop,Pointer->oTop); 
    }
    for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
        if (Not CDInitGen(SymbolDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
            return (CDError(CDMALLOCFAILED));
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            continue;
        loop {
            SymbolDesc->sLeft = min(SymbolDesc->sLeft,Pointer->oLeft); 
            SymbolDesc->sBottom = min(SymbolDesc->sBottom,Pointer->oBottom); 
            SymbolDesc->sRight = max(SymbolDesc->sRight,Pointer->oRight); 
            SymbolDesc->sTop = max(SymbolDesc->sTop,Pointer->oTop); 
            CDGen(SymbolDesc,GenDesc,&Pointer);
            if (Pointer == NULL)
                break;
        }
    }
    if (SymbolDesc->sLeft == CDINFINITY)
        SymbolDesc->sLeft = SymbolDesc->sBottom = SymbolDesc->sRight = 
            SymbolDesc->sTop = 0;
    SymbolDesc->sBBValid = True;
    *Left = SymbolDesc->sLeft;
    *Bottom = SymbolDesc->sBottom;
    *Right = SymbolDesc->sRight;
    *Top = SymbolDesc->sTop;

#ifdef DEBUGGEN
printf("CDBB3(%s,%d,%d,%d,%d)\n",SymbolDesc->sName,*Left,*Bottom,*Right,*Top);
#endif 

#ifdef DEBUGREFLECT
printf("CDBB3(%s,%d,%d,%d,%d)\n",SymbolDesc->sName,*Left,*Bottom,*Right,*Top);
#endif 

    return (True);
}


/*
 * Test code for CDIntersect.
 * main()
 *     {
 *     int Left,Bottom,Right,Top,BeginX,EndX,BeginY,EndY;
 *
 *     printf("BB?");
 *     scanf("%d%d%d%d",&Left,&Bottom,&Right,&Top);
 *     CDIntersect(Left,Bottom,Right,Top,&BeginX,&EndX,&BeginY,&EndY);
 *     printf("Bin[.][%d..%d][%d..%d]\n",BeginX,EndX,BeginY,EndY);
 *     }
 */


void
CDIntersect(Left,Bottom,Right,Top,BeginX,EndX,BeginY,EndY)

int Left,Bottom,Right,Top;
int *BeginX,*EndX,*BeginY,*EndY;
{
    /*
     * Returns which bins overlap the AOI
     * The residual bin is always searched
     * Runs in constant time
     */
#ifdef FLOAT
    *BeginX = (int)((float)(Left-CDBINMINX) * (float)(CDNUMBINS)/(float)(CDBINMAXX-CDBINMINX) + 1);
    if (*BeginX > CDNUMBINS)
        *BeginX = CDNUMBINS;
    elif (*BeginX < 1)
        *BeginX = 1;
    *EndX = (int)((float)(Right-CDBINMINX) * (float)(CDNUMBINS)/(float)(CDBINMAXX-CDBINMINX) + 1);
    if (*EndX > CDNUMBINS)
        *EndX = CDNUMBINS;
    elif (*EndX < 1)
        *EndX = 1;
    *BeginY = (int)((float)(Bottom-CDBINMINY) * (float)(CDNUMBINS)/(float)(CDBINMAXY-CDBINMINY) + 1);
    if (*BeginY > CDNUMBINS)
        *BeginY = CDNUMBINS;
    elif (*BeginY < 1)
        *BeginY = 1;
    *EndY = (int)((float)(Top-CDBINMINY) * (float)(CDNUMBINS)/(float)(CDBINMAXY-CDBINMINY) + 1);
    if (*EndY > CDNUMBINS)
        *EndY = CDNUMBINS;
    elif (*EndY < 1)
        *EndY = 1;
#else
    *BeginX = ((Left-CDBINMINX) * (CDNUMBINS)/(CDBINMAXX-CDBINMINX) + 1);
    if (*BeginX > CDNUMBINS)
        *BeginX = CDNUMBINS;
    elif (*BeginX < 1)
        *BeginX = 1;
    *EndX = ((Right-CDBINMINX) * (CDNUMBINS)/(CDBINMAXX-CDBINMINX) + 1);
    if (*EndX > CDNUMBINS)
        *EndX = CDNUMBINS;
    elif (*EndX < 1)
        *EndX = 1;
    *BeginY = ((Bottom-CDBINMINY) * (CDNUMBINS)/(CDBINMAXY-CDBINMINY) + 1);
    if (*BeginY > CDNUMBINS)
        *BeginY = CDNUMBINS;
    elif (*BeginY < 1)
        *BeginY = 1;
    *EndY = ((Top-CDBINMINY) * (CDNUMBINS)/(CDBINMAXY-CDBINMINY) + 1);
    if (*EndY > CDNUMBINS)
        *EndY = CDNUMBINS;
    elif (*EndY < 1)
        *EndY = 1;
#endif
}




/*======================================================================*
 *                                                                      *
 *      GGGG EEEEE N   N EEEEE RRRR    A   TTTTT  OOO  RRRR   SSSS      *
 *     G     E     NN  N E     R   R  A A    T   O   O R   R S          *
 *     G GGG EEE   N N N EEE   RRRR  A   A   T   O   O RRRR   SSS       *
 *     G   G E     N  NN E     R R   AAAAA   T   O   O R R       S      *
 *      GGG  EEEEE N   N EEEEE R  R  A   A   T    OOO  R  R  SSSS       *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *        CDInitGen(SymbolDesc,Layer,Left,Bottom,Right,Top,GenDesc)     *
 *        CDGen(SymbolDesc,GenDesc,Pointer)                             *
 *        CDInitTGen(Pointer,TGen)                                      *
 *        CDTGen(TGen,Type,X,Y)                                         *
 *                                                                      *
 *======================================================================*/

int
CDInitGen(SymbolDesc,Layer,Left,Bottom,Right,Top,GenDesc)

struct s *SymbolDesc;
int Layer;
int Left,Bottom,Right,Top;
struct g **GenDesc;
{
    /*
     * Returns a pointer to a generator desc.
     * Layer == 0 denotes calls.
     */
    int BeginX,BeginY,EndX,EndY;

#ifdef DEBUGGEN
printf("Begin initializing generator to search symbol %s.\n",SymbolDesc->sName);
printf("Untransformed AOI is %d %d %d %d.\n",Left,Bottom,Right,Top);
#endif

    /*
     * Apply inverse of current transformation to AOI.
     */
    TInverse();
    TInversePoint(&Left,&Bottom);
    TInversePoint(&Right,&Top);
    if (Right < Left)
        SwapInts(Left,Right);
    if (Top < Bottom)
        SwapInts(Bottom,Top);

#ifdef DEBUGGEN
printf("Transformed AOI is %d %d %d %d.\n",Left,Bottom,Right,Top);
#endif

    CDIntersect(Left,Bottom,Right,Top,&BeginX,&EndX,&BeginY,&EndY);

#ifdef DEBUGGEN
printf("Initialized generator to search bins %d..%d,%d..%d on layer %d.\n",
    BeginX,EndX,BeginY,EndY,Layer);
#endif

    if ((*GenDesc = alloc(g)) == NULL)
        return (CDError(CDMALLOCFAILED));
    (*GenDesc)->gLeft = Left;
    (*GenDesc)->gBottom = Bottom;
    (*GenDesc)->gRight = Right;
    (*GenDesc)->gTop = Top;
    (*GenDesc)->gLayer = Layer;
    (*GenDesc)->gX = (*GenDesc)->gBeginX = BeginX;
    (*GenDesc)->gY = (*GenDesc)->gBeginY = EndY;
    (*GenDesc)->gEndX = EndX;
    (*GenDesc)->gEndY = BeginY;
    /*
     * CDGen will ALWAYS search the residual bin FIRST.
     * The vanilla bins will be searched in the order 
     *  for Y = EndY..BeginY 
     *      for X = BeginX..EndX
     *          ...
     * so that redisplays will flow top down.
     */
    if (SymbolDesc->sBin[Layer] == NULL)
        (*GenDesc)->gPointer = NULL;
    else
        (*GenDesc)->gPointer = SymbolDesc->sBin[Layer][0][0];

#ifdef DEBUGGEN
printf("End initializing generator to search symbol %s.\n",SymbolDesc->sName);
#endif

    return (True);
}


void
CDGen(SymbolDesc,GenDesc,Pointer)

struct s *SymbolDesc;
struct g *GenDesc;
struct o **Pointer;
{
    /*
     * Returns pointer to next object.
     * You should invoke CDType to access object's type and dispatch off
     * of type.  See traversal code in CDUpdate.  Pointer == NULL if last
     * object at which time GenDesc is freed.
     */
    int i;
    int L,B,R,T;

    loop {
        if (GenDesc->gPointer != NULL) {
            /*
             * gPointer points to an object desc.    Is it in the AOI?
             * This test is necessary, because of the granularity of the bins.
             * Suppose AOI lies entirely within one bin.
             * Then there may, in general, be descs in the bin whose BBs lie
             * outside the AOI. 
             */

            /* callback to user supplied routine */
            if (GenDesc->gPointer->oType == CDLABEL)
                CDLabelBB(GenDesc->gPointer,&L,&B,&R,&T);
            else {
                L = GenDesc->gPointer->oLeft;
                B = GenDesc->gPointer->oBottom;
                R = GenDesc->gPointer->oRight;
                T = GenDesc->gPointer->oTop;
            }

#ifdef DEBUGGEN
printf("Generator intersecting %d %d %d %d to AOI.\n",L,B,R,T);
#endif

            if (L > GenDesc->gRight Or B > GenDesc->gTop Or
                R < GenDesc->gLeft Or T < GenDesc->gBottom) {
                /*
                 * Object isn't visible, so consider the next one, if any, in
                 * the bin currently being searched.
                 */
                GenDesc->gPointer = GenDesc->gPointer->oSucc;

#ifdef DEBUGGEN
printf("Invisible.\n");
#endif

            }
            else {

#ifdef DEBUGGEN
printf("Visible.\n");
#endif

                /*
                 * Object is visible, so return object desc.
                 */
                *Pointer = GenDesc->gPointer;
                GenDesc->gPointer = GenDesc->gPointer->oSucc;
                return;
            } 
        }
        else {
            if (GenDesc->gY < GenDesc->gEndY) {
                /* The generator is done */
                afree(GenDesc,g);
                *Pointer = NULL;
                return;
            }
            /*
             * Consider first object in next bin.
             * If the bin is empty, we will pass through the loop again.
             */
            i = GenDesc->gLayer;
            if (SymbolDesc->sBin[i] == NULL) {
                /* The generator is done */
                afree(GenDesc,g);
                *Pointer = NULL;
                return;
            }
            GenDesc->gPointer = SymbolDesc->sBin[i][GenDesc->gX][GenDesc->gY];
            ++(GenDesc->gX);
            if (GenDesc->gX > GenDesc->gEndX) {
                GenDesc->gX = GenDesc->gBeginX;
                --(GenDesc->gY);
            }
        }
    }
}


void
CDInitTGen(Pointer,TGen)

struct o *Pointer;
struct t **TGen;
{
    struct c *CallDesc;

    if (Pointer == NULL)
        return;
    if (Pointer->oType != CDSYMBOLCALL)
        return;
    CallDesc = (struct c *)Pointer->oRep;
    *TGen = CallDesc->cT;
}


void
CDTGen(TGen,Type,X,Y)

struct t **TGen;
char *Type;
int *X,*Y;
{
    static int FirstDesc = True;

    if (*TGen == NULL)
        return;
    elif (FirstDesc) {
        FirstDesc = False;
        *X = (*TGen)->tX;
        *Y = (*TGen)->tY;
        *Type = (*TGen)->tType;
    }
    else {
        *TGen = (*TGen)->tSucc;
        if (*TGen == NULL) {
            FirstDesc = True;
            return;
        }
        *X = (*TGen)->tX;
        *Y = (*TGen)->tY;
        *Type = (*TGen)->tType;
    }
}




/*======================================================================*
 *                                                                      *
 *                          CCCC III FFFFF                              *
 *                         C      I  F                                  *
 *                         C      I  FFFF                               *
 *                         C      I  F                                  *
 *                          CCCC III F                                  *
 *                                                                      *
 *  TTTTT RRRR    A   N   N  SSSS L       A   TTTTT III  OOO  N   N     *
 *    T   R   R  A A  NN  N S     L      A A    T    I  O   O NN  N     *
 *    T   RRRR  A   A N N N  SSS  L     A   A   T    I  O   O N N N     *
 *    T   R R   AAAAA N  NN     S L     AAAAA   T    I  O   O N  NN     *
 *    T   R  R  A   A N   N SSSS  LLLLL A   A   T   III  OOO  N   N     *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *        CDUpdate(SymbolDesc,SymbolFile)                               *
 *        CDGenCIF(FileDesc,SymbolDesc,SymbolNum,A,B)                   *
 *        CDTo(CIFFile,Root,A,B,Program)                                *
 *        CDFrom(Root,CIFFile,A,B,Layers,NumLayers,Program)             *
 *        CDUnmark(SymbolDesc)                                          *
 *                                                                      *
 *======================================================================*/

int
CDUpdate(SymbolDesc,SymbolFile)

struct s *SymbolDesc;
char *SymbolFile;
{
    /*
     * Update symbol to symbol file.
     * If SymbolFile == NULL, update to file SymbolDesc->sName.
     * Returns True if success, else returns False.
     */
    FILE *FileDesc;
    struct g *GenDesc;
    struct o *Pointer;
    struct t *TGen;
    struct p *Path;
    struct prpty *PrptyDesc;
    char *Label;
    char *SymbolName;
    int Layer;
    int X,Y,Length,Width;
    int NumX,NumY;
    int DX,DY;
    char Type,Xform;

    if (SymbolFile == NULL) {
        if ((FileDesc = POpen(SymbolDesc->sName,"w",(char *)NULL,(char **)NULL))
            == NULL) return (False);
        fprintf(FileDesc,"(Symbol %s);\n",SymbolDesc->sName);
    }
    else {
        char *s, *strrchr();
        int i;

        if ((FileDesc = POpen(SymbolFile,"w",(char *)NULL,(char **)NULL))
            == NULL) return (False);
        /* SRW  strip off path prefix */
        s = strrchr(SymbolFile,DIRC);
        if (s) {
            *s = 0;
            for (i = 0, s++; *s; i++, s++)
                SymbolFile[i] = *s;
            SymbolFile[i] = '\0';
        }
        fprintf(FileDesc,"(Symbol %s);\n",SymbolFile);
    } 
    fprintf(FileDesc,"9 %s;\n",SymbolDesc->sName);
    /* add property list info */
    CDProperty(SymbolDesc,(struct o *)NULL,&PrptyDesc);
    while(PrptyDesc != NULL) {
        fprintf(FileDesc,"5 %d %s;\n",PrptyDesc->prpty_Value,
            PrptyDesc->prpty_String);
        PrptyDesc = PrptyDesc->prpty_Succ;
    }
    GenBeginSymbol(FileDesc,0,1L,1L);
    SymbolDesc->sLeft = SymbolDesc->sBottom = CDINFINITY;
    SymbolDesc->sRight = SymbolDesc->sTop = -CDINFINITY;
    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return (CDError(CDMALLOCFAILED));
    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            break;
        CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);
        /* add symbol name extension */
        fprintf(FileDesc,"9 %s;\n",SymbolName);
        /* add property list info */
        CDProperty(SymbolDesc,Pointer,&PrptyDesc);
        while(PrptyDesc != NULL) {
            fprintf(FileDesc,"5 %d %s;\n",PrptyDesc->prpty_Value,
                PrptyDesc->prpty_String);
            PrptyDesc = PrptyDesc->prpty_Succ;
        }
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
                fprintf(FileDesc," T %d %d",X,Y);
            elif (Type == CDMIRRORX)
                fprintf(FileDesc," MX");
            elif (Type == CDMIRRORY)
                fprintf(FileDesc," MY");
        }
        SymbolDesc->sLeft = min(SymbolDesc->sLeft,Pointer->oLeft); 
        SymbolDesc->sBottom = min(SymbolDesc->sBottom,Pointer->oBottom); 
        SymbolDesc->sRight = max(SymbolDesc->sRight,Pointer->oRight); 
        SymbolDesc->sTop = max(SymbolDesc->sTop,Pointer->oTop); 
    }
    for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
        if (Not CDInitGen(SymbolDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
            return (CDError(CDMALLOCFAILED));
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            continue;
        GenLayer(FileDesc,CDLayer[Layer].lTechnology,CDLayer[Layer].lMask);
        loop{
            CDProperty(SymbolDesc,Pointer,&PrptyDesc);
            while(PrptyDesc != NULL) {
                fprintf(FileDesc,"5 %d %s;\n",PrptyDesc->prpty_Value,
                    PrptyDesc->prpty_String);
                PrptyDesc = PrptyDesc->prpty_Succ;
            }
            CDType(Pointer,&Type);
            if (Type == CDWIRE) {
                CDWire(Pointer,&Layer,&Width,&Path);
                GenWire(FileDesc,Width,Path);
            }
            elif (Type == CDPOLYGON) {
                CDPolygon(Pointer,&Layer,&Path);
                GenPolygon(FileDesc,Path);
            }
            elif (Type == CDLABEL) {
                CDLabel(Pointer,&Layer,&Label,&X,&Y,&Xform);
                fprintf(FileDesc,"94 %s %d %d %d",Label,X,Y,Xform);
                fprintf(FileDesc,";\n");
            }
            elif (Type == CDBOX) {
                CDBox(Pointer,&Layer,&Length,&Width,&X,&Y);
                GenBox(FileDesc,Length,Width,X,Y,1,0);
            }
            SymbolDesc->sLeft = min(SymbolDesc->sLeft,Pointer->oLeft); 
            SymbolDesc->sBottom = min(SymbolDesc->sBottom,Pointer->oBottom); 
            SymbolDesc->sRight = max(SymbolDesc->sRight,Pointer->oRight); 
            SymbolDesc->sTop = max(SymbolDesc->sTop,Pointer->oTop); 
            CDGen(SymbolDesc,GenDesc,&Pointer);
            if (Pointer == NULL)
                break;
        }
    }
    if (SymbolDesc->sLeft == CDINFINITY)
        SymbolDesc->sLeft = SymbolDesc->sBottom = SymbolDesc->sRight = 
            SymbolDesc->sTop = 0;
    GenEndSymbol(FileDesc);
    GenEnd(FileDesc);
    fclose(FileDesc);
    CDDesc.dSymbolDesc->sBBValid = True; 
    return (True);
}


int
CDGenCIF(FileDesc,SymbolDesc,SymbolNum,A,B,Program)

FILE *FileDesc;
struct s *SymbolDesc;
int *SymbolNum;
int A,B;
char Program;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct s *MasterDesc;
    struct p *Pair,*Path;
    struct t *TGen;
    struct prpty *PrptyDesc;
    char *SymbolName;
    char *Label;
    int Layer;
    int X,Y,Length,Width;
    int NumX,NumY;
    int DX,DY;
    int Info;
    int i,j;
    int Left,Bottom,Right,Top;
    int OutputLayer;
    char Type,Xform;

    *SymbolNum += 1;
    /*
     * Mark symbol associated withSymbolDesc as visited by storing
     * its symbol # in its info field.  VERY NICE.
     */
    CDSetInfo(SymbolDesc,(struct o *)NULL,*SymbolNum);

    /*
     * First write to the CIF file any symbol definitions below
     * the symbol associated with SymbolDesc.
     */
    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return (CDError(CDMALLOCFAILED));
    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            break;
        CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);
        if (Not CDOpen(SymbolName,&MasterDesc,'w'))
            return (False);
        CDInfo(MasterDesc,(struct o *)NULL,&Info);
        if (Info == 0)
            /* Write master's definition to CIF file. */
            if (Not CDGenCIF(FileDesc,MasterDesc,SymbolNum,A,B,Program))
                return (False);
    }

    /*
     * Write to the CIF file the definition of the symbol associated with
     * SymbolDesc.  Instance calls first--then geometries.
     */
    if (Program == 'e') {
        CDProperty(SymbolDesc,(struct o *)NULL,&PrptyDesc);
        while(PrptyDesc != NULL) {
            fprintf(FileDesc,"5 %d %s;\n",PrptyDesc->prpty_Value,
                PrptyDesc->prpty_String);
            PrptyDesc = PrptyDesc->prpty_Succ;
        }
    }
    CDInfo(SymbolDesc,(struct o *)NULL,&Info);
    fprintf(FileDesc,"DS %d 1 1;\n",Info);
    /* write symbol rename extension */
    if (Program == 'b' Or Program == 'a')                /* NCA/Stanford CIF */
        fprintf(FileDesc,"( %s );\n",SymbolDesc->sName);
    elif (Program == 'i')                                /* Icarus style CIF */
        fprintf(FileDesc,"( 9 %s );\n",SymbolDesc->sName);
    elif (Program == 's')                                /* SIF style CIF */
        fprintf(FileDesc,"( Name: %s );\n",SymbolDesc->sName);
    else                                                /* KIC/CD default */
        fprintf(FileDesc,"9 %s;\n",SymbolDesc->sName);
    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return (CDError(CDMALLOCFAILED));
    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            break;
        CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);
        if (Not CDOpen(SymbolName,&MasterDesc,'w'))
            return (False);
        CDInfo(MasterDesc,(struct o *)NULL,&Info);
        if (Not CDBB(MasterDesc,(struct o *)NULL,&Left,&Bottom,&Right,&Top))
            return (False);
        for (i = 1;i <= NumY;++i) {
            for (j = 1;j <= NumX;++j) {
                /* write property list extension */
                if (Program == 'e') {
                    CDProperty(SymbolDesc,Pointer,&PrptyDesc);
                    while(PrptyDesc != NULL) {
                        fprintf(FileDesc,"5 %d %s;\n",PrptyDesc->prpty_Value,
                            PrptyDesc->prpty_String);
                        PrptyDesc = PrptyDesc->prpty_Succ;
                    }
                }
                fprintf(FileDesc,"C %d",Info);
                if (i > 1 || j > 1)
                    fprintf(FileDesc," T %d %d",
                        ((j-1)*DX)*A/B,((i-1)*DY)*A/B);
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
                        fprintf(FileDesc," T %d %d",X*A/B,Y*A/B);
                    elif (Type == CDMIRRORX)
                        fprintf(FileDesc," MX");
                    elif (Type == CDMIRRORY)
                        fprintf(FileDesc," MY");
                }
            }
        }
    }
    for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
        int FirstT = True;

        if (CDLayer[Layer-1].lCDFrom) {
            OutputLayer = True;
        }
        else
            OutputLayer = False;

        if (Not CDInitGen(SymbolDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
            return (CDError(CDMALLOCFAILED));

        loop {
            CDGen(SymbolDesc,GenDesc,&Pointer);
            if (Pointer == NULL)
                break;
            if (FirstT) {
                if (Program == 'b')                   /* NCA style CIF */
                    fprintf(FileDesc,"L %d;\n",Layer);
                else
                    GenLayer(FileDesc,CDLayer[Layer].lTechnology,
                        CDLayer[Layer].lMask);
                FirstT = False;
            }

            /* write property list extension */
            if (Program == 'e') {
                CDProperty(SymbolDesc,Pointer,&PrptyDesc);
                while(PrptyDesc != NULL And Program == 'e') {
                    fprintf(FileDesc,"5 %d %s;\n",PrptyDesc->prpty_Value,
                        PrptyDesc->prpty_String);
                    PrptyDesc = PrptyDesc->prpty_Succ;
                }
            }
            CDType(Pointer,&Type);
            if (!OutputLayer && Type != CDLABEL) {
                continue;
                /* output all labels */
            }
            elif (Type == CDBOX) {
                CDBox(Pointer,&Layer,&Length,&Width,&X,&Y);
                GenBox(FileDesc,Length*A/B,Width*A/B,X*A/B,Y*A/B,1,0);
            }
            elif (Type == CDWIRE) {
                CDWire(Pointer,&Layer,&Width,&Path);
                if (Path->pSucc == NULL)
                    fprintf(FileDesc,"W %d %d %d",Width*A/B,
                        Path->pX*A/B,Path->pY*A/B);
                else {
                    fprintf(FileDesc,"W %d",Width*A/B);
                    Pair = Path;
                    while(Pair != NULL) {
                        fprintf(FileDesc," %d %d",Pair->pX*A/B,Pair->pY*A/B);
                        Pair = Pair->pSucc;
                    }
                }
                fprintf(FileDesc,";\n");
            }
            elif (Type == CDPOLYGON) {
                CDPolygon(Pointer,&Layer,&Path);
                fprintf(FileDesc,"P");
                Pair = Path;
                while(Pair != NULL) {
                    fprintf(FileDesc," %d %d",Pair->pX*A/B,Pair->pY*A/B);
                    Pair = Pair->pSucc;
                }
                fprintf(FileDesc,";\n");
            }
            elif (Type == CDLABEL) {
                CDLabel(Pointer,&Layer,&Label,&X,&Y,&Xform);
                if (Program == 'k' Or Program == 'e')   /* KIC/CD label */
                    fprintf(FileDesc,"94 %s %d %d %d;\n",
                        Label,X*A/B,Y*A/B,(char)Xform);
                elif (Program == 'b')                    /* NCA label */
                    fprintf(FileDesc,"94 %s %d %d %d;\n",
                        Label,X*A/B,Y*A/B,Layer);
                elif (Program == 'm') {                  /* mextra label */
                    fprintf(FileDesc,"94 %s %d %d",Label,X*A/B,Y*A/B);
                    if (CDLayer[Layer].lTechnology != ' ') {
                        fprintf(FileDesc," %c",CDLayer[Layer].lTechnology);
                        i = 0;
                        while(i < 3 And CDLayer[Layer].lMask[i] > 040) {
                                fprintf(FileDesc,"%c",CDLayer[Layer].lMask[i]);
                            i++;
                        }
                    }
                    fprintf(FileDesc,";\n");
                }
            }
        }
    }
    GenEndSymbol(FileDesc);
    return (True);
}


int
CDTo(CIFFile,Root,A,B,Program)

char *CIFFile,*Root;
int A,B;
char Program;
{
    /*
     * Translate from CIF file into symbol files.
     * Each time we see a symbol definition, we write it in its own file.
     * The problem is that commands may be in the file that aren't part of a
     * symbol definition.  The solution is to have a file named Root for
     * the commands. 
     */
    int Int1;
    int StatusInt;
    char *StatusString;
    CDDesc.dControl = DCONTROLCDTO;
    
    /*
     * On the first pass, we just fill the symbol name table.
     */
    CDDesc.dFirstPass = True;
    CDDesc.dNumSymbolTable = 0;
    for (Int1 = 0;Int1 < CDNUMREMEMBER;++Int1) {
        CDDesc.dSymTabNames[Int1][0] = EOS;
        CDDesc.dSymTabNumbers[Int1] = -1; 
    }
    if ((CDDesc.dProgram = Program) != 'n') {
        PCIF(CIFFile,&StatusString,&StatusInt);
        if (StatusInt == PFAILED) {
            CDStatusInt = CDPARSEFAILED;
            strcpy(CDStatusString,StatusString);
            return (False);
        }
    }
    /*
     * On the second pass, we do the sequential translation. 
     */
    CDDesc.dFirstPass = False;
    CDDesc.dPrptyList = NULL;
    CDDesc.dA = A;
    CDDesc.dB = B;
    CDDesc.dDSA = CDDesc.dDSB = 1;
    CDDesc.dRoot = True;
    CDDesc.dSymbolName[0] = EOS;
    if ((CDDesc.dRootFileDesc = POpen(Root,"w",(char *)NULL,(char **)NULL))
        == NULL) {
        sprintf(CDStatusString,"Can't open file Root.");
        CDStatusInt = CDPARSEFAILED;
        return (False);
    }
    fprintf(CDDesc.dRootFileDesc,"(Symbol %s.);\n",Root);
    fprintf(CDDesc.dRootFileDesc,"(Microns/lambda = %d/%d);\n",A,B);
    fprintf(CDDesc.dRootFileDesc,"9 %s;\n",Root);
    GenBeginSymbol(CDDesc.dRootFileDesc,0,1,1);
    PCIF(CIFFile,&StatusString,&StatusInt);
    if (StatusInt == PFAILED) {
        CDStatusInt = CDPARSEFAILED;
        strcpy(CDStatusString,StatusString);
        return (False);
    }
    else
        CDStatusInt = CDSUCCEEDED;
    GenEndSymbol(CDDesc.dRootFileDesc);
    GenEnd(CDDesc.dRootFileDesc);
    fclose(CDDesc.dRootFileDesc);
    CDDesc.dControl = DCONTROLVANILLA;
    return (True);
}


int
CDFrom(Root,CIFFile,A,B,Layers,NumLayers,Program)

char *Root,*CIFFile,Program;
int A,B;
int Layers[],NumLayers;
{
    /*
     * Translate symbol hierarchy rooted with symbol named Root into
     * CIF file named CIFFile. 
     */
    struct s *SymbolDesc;
    FILE *FileDesc;
    int SymbolNum = 0;
    int Info;
    int Layer;

    if ((FileDesc = POpen(CIFFile,"w",(char *)NULL,(char **)NULL)) == NULL) {        
        CDStatusInt = CDPARSEFAILED;
        sprintf(CDStatusString,"Can't open CIF file.");
        return (False);
    }
    if (Not CDOpen(Root,&SymbolDesc,'r')) {
        CDStatusInt = CDPARSEFAILED;
        return (False);
    }
    if (CDStatusInt == CDNEWSYMBOL) {
        sprintf(CDStatusString,"Can't open file %s.",Root);
        return (False);
    }

    if (Layers[0])
        CDLayer[0].lCDFrom = True;
    else
        CDLayer[0].lCDFrom = False;

    for (Layer = 1; Layer < NumLayers; ++Layer)
        CDLayer[Layer].lCDFrom = True;

    fprintf(FileDesc,"(CIF file of symbol hierarchy rooted at %s);\n",Root);
    if (Not CDGenCIF(FileDesc,SymbolDesc,&SymbolNum,A,B,Program))
        return (False);
    CDInfo(SymbolDesc,(struct o *)NULL,&Info);
    fprintf(FileDesc,"C %d;\nE\n",Info);
    fclose(FileDesc);
    /*
     * Really should set all of the info fields in all symbol descs to 0.
     * CDUnmark(SymbolDesc);
     */
    return (True);
}


int
CDParseCIF(Root,CIFFile,Program)

char *Root,*CIFFile,Program;
{
    /*
     * Construct CD database from a CIF file rather than a hierarchy
     * of cell files.
     */
    struct m *MasterListDesc1;
    struct m *MasterListDesc2;
    struct s *MasterSymbolDesc1;
    struct s *MasterSymbolDesc2;
    char *StatusString;
    int StatusInt;
    int Int1;

    CDDesc.dProgram = Program;
    CDDesc.dA = CDDesc.dB = 1;
    CDDesc.dDSA = CDDesc.dDSB = 1;
    CDDesc.dRoot = True;
    CDDesc.dControl = DCONTROLPCIF;
    CDDesc.dSymbolDesc = CDDesc.dRootCellDesc;
    if (Not CDOpen(Root,&CDDesc.dRootCellDesc,'n')) {
        CDStatusInt = CDMALLOCFAILED;
        return (False);
    }
    CDDesc.dControl = DCONTROLPCIF;

    /*
     * On the first pass, we just fill the symbol name table.
     */
    CDDesc.dFirstPass = True;
    CDDesc.dNumSymbolTable = 0;
    for (Int1 = 0;Int1 < CDNUMREMEMBER;++Int1) {
        CDDesc.dSymTabNames[Int1][0] = EOS;
        CDDesc.dSymTabNumbers[Int1] = -1;
    }
    PCIF(CIFFile,&StatusString,&StatusInt);
    if (StatusInt == PFAILED) {
        CDStatusInt = CDPARSEFAILED;
        strcpy(CDStatusString,StatusString);
        return (False);
    }
    
    /*
     * On the second pass, we do the sequential translation. 
     */
    CDDesc.dFirstPass = False;
    PCIF(CIFFile,&StatusString,&StatusInt);
    if (StatusInt == PFAILED) {
        CDStatusInt = CDPARSEFAILED;
        strcpy(CDStatusString,StatusString);
        return (False);
    }
    MasterListDesc1 = CDDesc.dRootCellDesc->sMasterList;
    while(MasterListDesc1 != NULL) {
        CDOpen(MasterListDesc1->mName,&MasterSymbolDesc1,'r');
        if (CDStatusInt == CDNEWSYMBOL Or CDStatusInt == CDPARSEFAILED) {
            if (CDStatusInt == CDNEWSYMBOL) {
                CDStatusInt = CDPARSEFAILED;
                sprintf(CDStatusString,"Master %s doesn't seem to be around.\n",
                    MasterListDesc1->mName);
            }
            return (False);
        }
        MasterListDesc2 = MasterSymbolDesc1->sMasterList;
        while(MasterListDesc2 != NULL) {
            CDOpen(MasterListDesc2->mName,&MasterSymbolDesc2,'r');
            if (CDStatusInt == CDNEWSYMBOL Or CDStatusInt == CDPARSEFAILED) {
                if (CDStatusInt == CDNEWSYMBOL) {
                    CDStatusInt = CDPARSEFAILED;
                    sprintf(CDStatusString,
                        "Master %s doesn't seem to be around.\n",
                        MasterListDesc2->mName);
                }
                return (False);
            }
            if (Not CDReflect(MasterSymbolDesc2)) {
                CDStatusInt = CDPARSEFAILED;
                return (CDError(CDMALLOCFAILED));
            }
            MasterListDesc2 = MasterListDesc2->mSucc;
        }
        if (Not CDReflect(MasterSymbolDesc1)) {
            CDStatusInt = CDPARSEFAILED;
            return (CDError(CDMALLOCFAILED));
        }
        MasterListDesc1 = MasterListDesc1->mSucc;
    }
    if (Not CDReflect(CDDesc.dRootCellDesc)) {
        CDStatusInt = CDPARSEFAILED;
        return (CDError(CDMALLOCFAILED));
    }
    return (True);
}


int
CDUnmark(SymbolDesc)

struct s *SymbolDesc;
{
    struct g *GenDesc;
    struct o *Pointer;
    char *SymbolName;
    int NumX,NumY;
    int DX,DY;
    int Info;
    int Layer;
    struct s *MasterDesc;

    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return (CDError(CDMALLOCFAILED));
#ifdef DEBUG_CDUNMARK
fprintf(stderr,"\n\n");
fprintf(stderr,"1CDUnmark: Inititialezed generator on instance layer.\n\n");
#endif
    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            break;

#ifdef DEBUG_CDUNMARK
fprintf(stderr,"2CDUnmark: CDGen found instance: Pointer = 0x%x\n\n",Pointer);
#endif

        CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);

#ifdef DEBUG_CDUNMARK
fprintf(stderr,"3CDUnmark: instance name = %s\n\n",SymbolName);
#endif

        /*
         * Cell has already been mapped into memory.  Therefore,
         * we can assume that CDOpen does not fail in the parse.
         */
        if (Not CDOpen(SymbolName,&MasterDesc,'r'))
            return (CDError(CDMALLOCFAILED));

#ifdef DEBUG_CDUNMARK
fprintf(stderr,"4CDUnmark: CDOpen returned MasterDesc = 0x%x\n\n",MasterDesc);
#endif

        CDInfo(MasterDesc,(struct o *)NULL,&Info);
        if (Info != 0) {
            /* Unmark master */
            CDSetInfo(MasterDesc,(struct o *)NULL,0);
            if (Not CDUnmark(MasterDesc))
                return (False);
        }
    }
    for (Layer = 1; Layer <= CDNUMLAYERS; ++Layer) {
        if (Not CDInitGen(SymbolDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
            return (CDError(CDMALLOCFAILED));

#ifdef DEBUG_CDUNMARK
fprintf(stderr,"5CDUnmark: Inititialezed generator on layer %d.\n\n",Layer);
#endif

        loop{
            CDGen(SymbolDesc,GenDesc,&Pointer);
            if (Pointer == NULL)
                break;

#ifdef DEBUG_CDUNMARK
fprintf(stderr,"6CDUnmark: CDGen found instance: Pointer = 0x%x\n\n",Pointer);
#endif

            CDInfo(SymbolDesc,(struct o *)NULL,&Info);
            if (Info != 0) {
                /* Unmark geometry */
                CDSetInfo(SymbolDesc,Pointer,0);
            }
        }
    }
    return (True);
}



/*======================================================================*
 *                                                                      *
 *  EEEEE RRRR  RRRR   OOO  RRRR                                        *
 *  E     R   R R   R O   O R   R                                       *
 *  EEE   RRRR  RRRR  O   O RRRR                                        *
 *  E     R R   R R   O   O R R                                         *
 *  EEEEE R  R  R  R   OOO  R  R                                        *
 *                                                                      *
 *  RRRR   OOO  U   U TTTTT III N   N EEEEE  SSSS                       *
 *  R   R O   O U   U   T    I  NN  N E     S                           *
 *  RRRR  O   O U   U   T    I  N N N EEE    SSS                        *
 *  R R   O   O U   U   T    I  N  NN E         S                       *
 *  R  R   OOO   UUU    T   III N   N EEEEE SSSS                        *
 *                                                                      *
 *                                                                      *
 *                                                                      *
 *        CDError(ID)                                                   *
 *                                                                      *
 *======================================================================*/

int
CDError(ID)

int ID;
{
    CDStatusInt = ID;
    switch(ID) {

    case CDMALLOCFAILED:
        sprintf(CDStatusString,"CD Out of memory.");
        return (False);

    case CDBADBOX:
        sprintf(CDStatusString,"Can't allow a zero width box.");
        /* not a fatal error */
        return (True);

    case CDXFORMSTACKFULL:
        sprintf(CDStatusString,"Transform stack is full.");
        return (False);

    case CDBADPATH:
        sprintf(CDStatusString,"Can't set search path.");
        return (False);

    default:
        sprintf(CDStatusString,"Unknown Error.");
        return (False);

    }
}

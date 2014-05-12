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
 * Action routines for fast CIF parser.
 * 
 * These routines handle both conversion from and to CIF.
 * AEnd will be the last routine invoked in a successful parse.
 */

#include "prefix.h"
#include "cd.h"
#include "parser.h"

#ifdef VMS
#include <types.h>
#include <timeb.h>
#else
#include <sys/types.h>
#include <time.h>
#endif

#define RADTODEG  57.29577951

static int CurrentLayer;
static char TypeOut[200];
static double ScaleFactor;

#define SCALE(x) ((int)(x*ScaleFactor))


void
AEnd()

{
    /*
     * The CIF parsing has ended.
     */
#ifdef TRACEPARSER
GenEnd(stderr);
#endif
}


int
ABeginSymbol(SymbolNum,A,B)

int SymbolNum;
int A,B;
{
    /*
     * This routine begins the parsing action for a symbol definition
     * and performs all necessary initialization for the new symbol.
     *
     * DCONTROLCDTO:
     *    On the first pass, we add the symbol name to the symbol table
     *    which is in CDDesc.dSymTabNames.  To do this we have to switch
     *    according to the value of 'CDDesc.dProgram' which specifies
     *    the style of the CIF.  On the second pass, we open the FILE
     *    descriptor for the KIC cell that coresponds to the respective
     *    CIF symbol and write the header information in the KIC cell.
     *
     * DCONTROLPCIF:
     *    On the first pass, we add the symbol name to the symbol table
     *    which is in CDDesc.dSymTabNames.  To do this we have to switch
     *    according to the value of 'CDDesc.dProgram' which specifies
     *    the style of the CIF.  Also, we open the symbol in the database
     *    via CDOpen() which places the symbol name in the hash table
     *    which is in CDSymbolTable.  We must not invoke CDClose since
     *    that will remove the symbol from memory;  the purpose of
     *    DCONTROLPCIF is to construct the database in memory without
     *    relying on a KIC cell directory in the current search path.
     *    On the second pass, we need only invoke CDSymbol() to obtain
     *    the symbol desc. for the respective symbol.
     *
     * DCONTROLCDOPEN
     *    No action.  It is assumed that the file being parsed is a KIC
     *    cell which will always contain exactly one CIF symbol.
     */

    time_t Long1;
    int Int1 = 0;

#ifdef TRACEPARSER
GenBeginSymbol(stderr,SymbolNum,A,B);
#endif
    CurrentLayer = 0;
    if (CDDesc.dControl == DCONTROLCDTO Or CDDesc.dControl == DCONTROLPCIF) {
        CDDesc.dRoot = False;
        CDDesc.dDSA = A;
        CDDesc.dDSB = B;
        /*
         * We switch on the following:
         * k KIC:      A KIC symbol name follows a DS command as in 9 PadIn;
         * a Stanford: Stanford symbol name follows a DS command as in (PadIn);
         * b NCA:      An NCA symbol name follows a DS command as in (PadIn);
         * h IGS:      An IGS symbol name follows a DS command as in 9 PadIn;
         * i Icarus:   An Icarus name follows a DS command as in (9 PadIn);
         * s Sif:      A Sif name follows a DS command as in (Name: PadIn);
         * n none of the above
         */

        if (CDDesc.dProgram == 'i') {
            /*
             * Icarus files have the symbol name in a comment of the form
             * (9 SymbolName); after the DS command.
             */
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == '9')
                    break;
            }
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == ')') {
                    CDDesc.dSymbolName[Int1] = EOS;
                    if (CDDesc.dNumSymbolTable < CDNUMREMEMBER) {
                        strcpy(CDDesc.dSymTabNames[CDDesc.dNumSymbolTable],
                            CDDesc.dSymbolName);
                        CDDesc.dSymTabNumbers[CDDesc.dNumSymbolTable]
                            = SymbolNum;
                        ++CDDesc.dNumSymbolTable;
                    }
                }
                elif (PChar == ';')
                    break;
                else
                    if (Int1 < FILENAMESIZE)
                        CDDesc.dSymbolName[Int1++] = PChar;
            }
        }
        elif (CDDesc.dProgram == 'a' Or CDDesc.dProgram == 'b') {
            /*
             * Some files have the symbol name in a comment of the form
             * (SymbolName); after the DS command.
             */
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == '(')
                    break;
            }
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == ')') {
                    CDDesc.dSymbolName[Int1] = EOS;
                    if (CDDesc.dNumSymbolTable < CDNUMREMEMBER) {
                        strcpy(CDDesc.dSymTabNames[CDDesc.dNumSymbolTable],
                            CDDesc.dSymbolName);
                        CDDesc.dSymTabNumbers[CDDesc.dNumSymbolTable]
                            = SymbolNum;
                        ++CDDesc.dNumSymbolTable;
                    }
                }
                elif (PChar == ';')
                    break;
                else {
                    if (Int1 < FILENAMESIZE)
                        CDDesc.dSymbolName[Int1++] = PChar;
                }
            }
        }
        elif (CDDesc.dProgram == 's') {
            /*
             * Sif files have the symbol name in a comment of the form
             * ( Name: SymbolName); after the DS command.
             */
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == ':')
                    break;
            }
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == ')') {
                    CDDesc.dSymbolName[Int1] = EOS;
                    if (CDDesc.dNumSymbolTable < CDNUMREMEMBER) {
                        strcpy(CDDesc.dSymTabNames[CDDesc.dNumSymbolTable],
                            CDDesc.dSymbolName);
                        CDDesc.dSymTabNumbers[CDDesc.dNumSymbolTable]
                            = SymbolNum;
                        ++CDDesc.dNumSymbolTable;
                    }
                }
                elif (PChar == ';')
                    break;
                else
                    if (Int1 < FILENAMESIZE)
                        CDDesc.dSymbolName[Int1++] = PChar;
            }
        }
        elif (CDDesc.dProgram == 'q') {
            /*
             * SQUID files have the symbol name in the form 
             * 9 FullName; after the DS command where FullName
             * is the full pathname to the cell or directory.
             *
             * NOTE: This code only works for UNIX file names.
             */
            char PrevName[FILENAMESIZE];
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == '9')
                    break;
            }
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == ';') {
                    CDDesc.dSymbolName[Int1] = EOS;
                    if (Int1 == 1 And CDDesc.dSymbolName[0] == '.')
                        strcpy(CDDesc.dSymbolName,PrevName);
                    if (CDDesc.dNumSymbolTable < CDNUMREMEMBER) {
                        strcpy(CDDesc.dSymTabNames[CDDesc.dNumSymbolTable],
                            CDDesc.dSymbolName);
                        CDDesc.dSymTabNumbers[CDDesc.dNumSymbolTable]
                            = SymbolNum;
                        ++CDDesc.dNumSymbolTable;
                    }
                    break;
                }
                elif (PChar == '/') {
                    /* begin new name; last name was a directory */
                    Int1 = 0;
                    strcpy(PrevName,CDDesc.dSymbolName);
                }
                else
                    if (Int1 < FILENAMESIZE)
                        CDDesc.dSymbolName[Int1++] = PChar;
            }
        }
        elif (CDDesc.dProgram == 'h' Or CDDesc.dProgram == 'k') {
            /*
             * IGS and KIC files have the symbol name in the form 
             * 9 SymbolName; after the DS command.
             */
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == '9')
                    break;
            }
            loop {
                PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
                if (PReturned == PFAILED)
                    return (PFAILED); 
                elif (PChar == ';') {
                    CDDesc.dSymbolName[Int1] = EOS;
                    if (CDDesc.dNumSymbolTable < CDNUMREMEMBER) {
                        strcpy(CDDesc.dSymTabNames[CDDesc.dNumSymbolTable],
                            CDDesc.dSymbolName);
                        CDDesc.dSymTabNumbers[CDDesc.dNumSymbolTable]
                            = SymbolNum;
                        ++CDDesc.dNumSymbolTable;
                    }
                    break;
                }
                else
                    if (Int1 < FILENAMESIZE)
                        CDDesc.dSymbolName[Int1++] = PChar;
            }
        }
        if (CDDesc.dControl == DCONTROLPCIF) {
            if (CDDesc.dFirstPass) {
                /*
                 * Open the symbol, but don't search the current directory
                 * for a KIC cell.  Also, don't close the symbol since
                 * that would remove the symbol from memory.
                 */
                for (Int1 = 0; Int1 < CDDesc.dNumSymbolTable; ++Int1)
                    if (CDDesc.dSymTabNumbers[Int1] == SymbolNum) break;
                if (Int1 == CDDesc.dNumSymbolTable) {
                    if (CDDesc.dNumSymbolTable < CDNUMREMEMBER) {
                        sprintf(CDDesc.dSymTabNames[CDDesc.dNumSymbolTable],
                            "Symbol%d",SymbolNum);
                        CDDesc.dSymTabNumbers[CDDesc.dNumSymbolTable]
                            = SymbolNum;
                        ++CDDesc.dNumSymbolTable;
                    }
                    else
                        return (PFAILED);
                }
                if (!CDOpen(CDDesc.dSymTabNames[Int1],
                        &CDDesc.dSymbolDesc,'n'))
                    return (PFAILED);
            }
            else{
                /*
                 * Symbol is already open.  Just get the desc for it.
                 */
                for (Int1 = 0; Int1 < CDDesc.dNumSymbolTable; ++Int1)
                    if (CDDesc.dSymTabNumbers[Int1] == SymbolNum) break;
                if (Int1 == CDDesc.dNumSymbolTable)
                    return (PFAILED);
                CDSymbol(CDDesc.dSymTabNames[Int1],&CDDesc.dSymbolDesc);
                if (CDDesc.dSymbolDesc == NULL)
                    return (PFAILED);
            }
            CDDesc.dControl = DCONTROLPCIF;
        }
        elif (CDDesc.dControl == DCONTROLCDTO) {
            if (CDDesc.dFirstPass)
                return (PSUCCEEDED);
            if (CDDesc.dSymbolName[0] == EOS)
                sprintf(CDDesc.dSymbolName,"Symbol%d",SymbolNum);
            if ((CDDesc.dSymbolFileDesc = POpen(CDDesc.dSymbolName,"w",
                (char *)NULL,(char **)NULL)) == NULL)
                return (PFAILED);

            ScaleFactor = CDDesc.dB;
            ScaleFactor *= CDDesc.dDSA;
            ScaleFactor /= CDDesc.dA;
            ScaleFactor /= CDDesc.dDSB;

            sprintf(TypeOut," Symbol %s ",CDDesc.dSymbolName);
            GenComment(CDDesc.dSymbolFileDesc,TypeOut);
            Long1 = time((time_t *)NULL);
            sprintf(TypeOut," Creation Date: %.24s ",ctime((time_t *)&Long1));
            GenComment(CDDesc.dSymbolFileDesc,TypeOut);
            GenUserExtension(CDDesc.dSymbolFileDesc,'9',CDDesc.dSymbolName);
            GenBeginSymbol(CDDesc.dSymbolFileDesc,0,1,1);
            CDDesc.dSymbolName[0] = EOS;
        }
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN) {
        /* add property list information */
        while(CDDesc.dPrptyList != NULL) {
            struct prpty PrptyCopy;
            if (Not CDAddProperty(CDDesc.dSymbolDesc,(struct o *)NULL,
                CDDesc.dPrptyList->prpty_Value,
                CDDesc.dPrptyList->prpty_String))
                return (PFAILED);
            /* free storage of CDDesc.dPrptyList */
            PrptyCopy = *CDDesc.dPrptyList;
            free(CDDesc.dPrptyList->prpty_String);
            afree(CDDesc.dPrptyList,prpty);
            CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
        }
    }
    return (PSUCCEEDED);
}


void
AEndSymbol()

{
    /*
     * This routine performs the necessary actions to close a symbol
     * definition.
     *
     * DCONTROLCDTO:
     *     Return on first pass (we are only building the symbol table).
     *     On the second pass, we terminate and close the KIC cell
     *     containing the respective CIF symbol.
     *
     * DCONTROLPCIF:
     *     We set the current cell desc in CDDesc.dSymbolDesc to that of
     *     the root symbol.
     *
     * DCONTROLCDOPEN:
     *     No action.
     */
#ifdef TRACEPARSER
GenEndSymbol(stderr);
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return;
        GenEndSymbol(CDDesc.dSymbolFileDesc);
        GenEnd(CDDesc.dSymbolFileDesc);
        fclose(CDDesc.dSymbolFileDesc);
        CDDesc.dRoot = True;
    }
    elif (CDDesc.dControl == DCONTROLPCIF) {
        CDDesc.dSymbolDesc = CDDesc.dRootCellDesc;
        CDDesc.dRoot = True;
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN) {
        if (CDDesc.dSymbolDesc->sLeft == CDINFINITY)
            CDDesc.dSymbolDesc->sLeft = CDDesc.dSymbolDesc->sBottom
                = CDDesc.dSymbolDesc->sRight = CDDesc.dSymbolDesc->sTop = 0;
        /*
         * Force the dummy call command at the end of the symbol
         * to be ignored by ABeginCall.
         */
        CDDesc.dSymbolName[0] = EOS;
    }
}


void
ADeleteSymbol(SymbolNum)

int SymbolNum;
{
    /*
     * We do not deal with definition deletes.
     * It could be handled by using the symbol table to obtain the
     * respective symbol numbers, and invoking CDClose on those cell
     * definitions to be deleted.
     */

#ifdef TRACEPARSER
fprintf(stderr,"DD %d;\n",SymbolNum);
#endif
    /*
     *Ignore DD commands.
     */
    fprintf(stderr,"Definition Delete of Symbol %d - ignored\n",SymbolNum);
}


int
AEndCall()

{
#ifdef TRACEPARSER
fprintf(stderr,";\n");
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);
        if (CDDesc.dRoot)
            GenEndCall(CDDesc.dRootFileDesc);
        else
            GenEndCall(CDDesc.dSymbolFileDesc);
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        if (CDDesc.dFirstPass And CDDesc.dControl == DCONTROLPCIF)
            return (PSUCCEEDED);
        if (Not CDEndMakeCall(CDDesc.dSymbolDesc,CDDesc.dPointer))
            return (PFAILED);
        while(CDDesc.dPrptyList != NULL) {
            struct prpty PrptyCopy;
            if (Not CDAddProperty(CDDesc.dSymbolDesc,CDDesc.dPointer,
                CDDesc.dPrptyList->prpty_Value,
                CDDesc.dPrptyList->prpty_String))
                return (PFAILED);
            /* free storage of CDDesc.dPrptyList */
            PrptyCopy = *CDDesc.dPrptyList;
            free(CDDesc.dPrptyList->prpty_String);
            afree(CDDesc.dPrptyList,prpty);
            CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
        }
    }
    return (PSUCCEEDED);
}


int
AT(Type,X,Y)

char Type;
int X,Y;
{
#ifdef TRACEPARSER
fprintf(stderr," T:%c %d %d",Type,X,Y);
#endif

    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);
        if (Type == CDTRANSLATE) {
            if (CDDesc.dRoot)
                GenTranslation(CDDesc.dRootFileDesc,SCALE(X),SCALE(Y));
            else
                GenTranslation(CDDesc.dSymbolFileDesc,SCALE(X),SCALE(Y));
        }
        elif (Type == CDROTATE) {
            if (abs(X) > 1 Or abs(Y) > 1) {
                X = SCALE(X);
                Y = SCALE(Y);
            }
            if (CDDesc.dRoot)
                GenRotation(CDDesc.dRootFileDesc,X,Y);
            else
                GenRotation(CDDesc.dSymbolFileDesc,X,Y);
        }
        elif (Type == CDMIRRORX) {
            if (CDDesc.dRoot)
                GenMirrorX(CDDesc.dRootFileDesc);
            else
                GenMirrorX(CDDesc.dSymbolFileDesc);
        }
        elif (Type == CDMIRRORY) {
            if (CDDesc.dRoot)
                GenMirrorY(CDDesc.dRootFileDesc);
            else
                GenMirrorY(CDDesc.dSymbolFileDesc);
        }
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        if (CDDesc.dFirstPass And CDDesc.dControl == DCONTROLPCIF)
            return (PSUCCEEDED);
        return (CDT(CDDesc.dPointer,Type,X,Y));
    }
    return (PSUCCEEDED);
}


int
ABeginCall(SymbolNum)

int SymbolNum;
{
    int Int1 = 0;
#ifdef TRACEPARSER
fprintf(stderr,"C %d ",SymbolNum);
#endif
    if (CDDesc.dControl == DCONTROLPCIF Or CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);
        if (CDDesc.dProgram != 'n') {
            for (Int1 = 0; Int1 < CDDesc.dNumSymbolTable; ++Int1)
                if (CDDesc.dSymTabNumbers[Int1] == SymbolNum) break;
        }
    }
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dProgram != 'n' And Int1 < CDDesc.dNumSymbolTable) {
            sprintf(TypeOut," %s",CDDesc.dSymTabNames[Int1]);
            if (CDDesc.dRoot)
                GenUserExtension(CDDesc.dRootFileDesc,'9',TypeOut);
            else
                GenUserExtension(CDDesc.dSymbolFileDesc,'9',TypeOut);
        }
        elif (CDDesc.dSymbolName[0] != EOS) {
            sprintf(TypeOut," %s",CDDesc.dSymbolName);
            if (CDDesc.dRoot)
                GenUserExtension(CDDesc.dRootFileDesc,'9',TypeOut);
            else
                GenUserExtension(CDDesc.dSymbolFileDesc,'9',TypeOut);
        }
        else {
            sprintf(CDDesc.dSymbolName,"Symbol%d",SymbolNum);
            sprintf(TypeOut," Symbol%d",SymbolNum);
            if (CDDesc.dRoot)
                GenUserExtension(CDDesc.dRootFileDesc,'9',TypeOut);
            else
                GenUserExtension(CDDesc.dSymbolFileDesc,'9',TypeOut);
        }
        if (CDDesc.dRoot)
            GenBeginCall(CDDesc.dRootFileDesc,0);
        else 
            GenBeginCall(CDDesc.dSymbolFileDesc,SymbolNum);
        CDDesc.dSymbolName[0] = EOS;
    }
    elif (CDDesc.dControl == DCONTROLPCIF) {
        if (CDDesc.dProgram != 'n' And Int1 < CDDesc.dNumSymbolTable)
            strcpy(CDDesc.dSymbolName,CDDesc.dSymTabNames[Int1]);
        else
            sprintf(CDDesc.dSymbolName,"Symbol%d",SymbolNum);
        if (Not CDBeginMakeCall(CDDesc.dSymbolDesc,CDDesc.dSymbolName,
            CDDesc.dNumX,CDDesc.dDX,CDDesc.dNumY,CDDesc.dDY,
            &CDDesc.dPointer))
            return (PFAILED);
        CDDesc.dSymbolName[0] = EOS;
        CDDesc.dNumX = CDDesc.dNumY = 1;
        CDDesc.dDX = CDDesc.dDY = 0;
        CDDesc.dControl = DCONTROLPCIF;
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN) {
        if (CDDesc.dSymbolName[0] != EOS)
            if (Not CDBeginMakeCall(CDDesc.dSymbolDesc,CDDesc.dSymbolName,
                CDDesc.dNumX,CDDesc.dDX,CDDesc.dNumY,CDDesc.dDY,
                &CDDesc.dPointer))
                return (PFAILED);
        CDDesc.dSymbolName[0] = EOS;
        CDDesc.dNumX = CDDesc.dNumY = 1;
        CDDesc.dDX = CDDesc.dDY = 0;
    }
    return (PSUCCEEDED);
}


int
APolygon(Path)

struct p *Path;
{
    struct p *Pair;
#ifdef TRACEPARSER
GenPolygon(stderr,Path);
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        int NumVertices;
        int Left,Bottom,Right,Top;
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);
        Pair = Path;
        NumVertices = 0;
        Left = Bottom = CDINFINITY;
        Right = Top = -CDINFINITY;
        while(Pair != NULL) {
            if (Pair->pX < Left)
                Left = Pair->pX;
            if (Pair->pX > Right)
                Right = Pair->pX;
            if (Pair->pY < Bottom)
                Bottom = Pair->pY;
            if (Pair->pY > Top)
                Top = Pair->pY;
            ++NumVertices;
            Pair = Pair->pSucc;
        }
        if (NumVertices == 4) {
            if ((Path->pX == Path->pSucc->pX And
                Path->pSucc->pY == Path->pSucc->pSucc->pY And
                Path->pSucc->pSucc->pX == Path->pSucc->pSucc->pSucc->pX And
                Path->pY == Path->pSucc->pSucc->pSucc->pY)
                Or
                (Path->pY == Path->pSucc->pY And
                Path->pSucc->pX == Path->pSucc->pSucc->pX And
                Path->pSucc->pSucc->pY == Path->pSucc->pSucc->pSucc->pY And
                Path->pX == Path->pSucc->pSucc->pSucc->pX))
                {
                return (ABox(Right-Left,Top-Bottom,Left+((Right-Left)/2),
                    Bottom+((Top-Bottom)/2),1,0));
            }
        }
        Pair = Path;
        while(Pair != NULL) {
            Pair->pX = SCALE(Pair->pX);
            Pair->pY = SCALE(Pair->pY);
            Pair = Pair->pSucc;
        }
        if (CDDesc.dRoot)
            GenPolygon(CDDesc.dRootFileDesc,Path);
        else
            GenPolygon(CDDesc.dSymbolFileDesc,Path);
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        if (CDDesc.dFirstPass And CDDesc.dControl == DCONTROLPCIF)
            return (PSUCCEEDED);
        if (Not CDMakePolygon(CDDesc.dSymbolDesc,CurrentLayer,Path,
            &CDDesc.dPointer))
            return (PFAILED);
        while(CDDesc.dPrptyList != NULL) {
            struct prpty PrptyCopy;
            /* ignore a low vertex count poly */
            if (CDDesc.dPointer) {
                if (Not CDAddProperty(CDDesc.dSymbolDesc,CDDesc.dPointer,
                    CDDesc.dPrptyList->prpty_Value,
                    CDDesc.dPrptyList->prpty_String))
                    return (PFAILED);
            }
            /* free storage of CDDesc.dPrptyList */
            PrptyCopy = *CDDesc.dPrptyList;
            free(CDDesc.dPrptyList->prpty_String);
            afree(CDDesc.dPrptyList,prpty);
            CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
        }
    }
    return (PSUCCEEDED);
}


int
AWire(Width,Path)

int Width;
struct p *Path;
{
    struct p *Pair;
#ifdef TRACEPARSER
GenWire(stderr,Width,Path);
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);
        Pair = Path;
        while(Pair != NULL) {
            Pair->pX = SCALE(Pair->pX);
            Pair->pY = SCALE(Pair->pY);
            Pair = Pair->pSucc;
        }
        if (CDDesc.dRoot)
            GenWire(CDDesc.dRootFileDesc,SCALE(Width),Path);
        else 
            GenWire(CDDesc.dSymbolFileDesc,SCALE(Width),Path);
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        if (CDDesc.dFirstPass And CDDesc.dControl == DCONTROLPCIF)
            return (PSUCCEEDED);
        if (Not CDMakeWire(CDDesc.dSymbolDesc,CurrentLayer,Width,Path,
            &CDDesc.dPointer))
            return (PFAILED);
        while(CDDesc.dPrptyList != NULL) {
            struct prpty PrptyCopy;
            if (Not CDAddProperty(CDDesc.dSymbolDesc,CDDesc.dPointer,
                CDDesc.dPrptyList->prpty_Value,
                CDDesc.dPrptyList->prpty_String))
                return (PFAILED);
            /* free storage of CDDesc.dPrptyList */
            PrptyCopy = *CDDesc.dPrptyList;
            free(CDDesc.dPrptyList->prpty_String);
            afree(CDDesc.dPrptyList,prpty);
            CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
        }
    }
    return (PSUCCEEDED);
}


int
ABox(Length,Width,X,Y,XDirection,YDirection)

int Length,Width,X,Y;
int XDirection,YDirection;
{
#ifdef TRACEPARSER
GenBox(stderr,Length,Width,X,Y,XDirection,YDirection);
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);
        Length = SCALE(Length);
        Width = SCALE(Width);
        X = SCALE(X);
        Y = SCALE(Y);
        if (XDirection == 1 And YDirection == 0) {
            if (CDDesc.dRoot)
                GenBox(CDDesc.dRootFileDesc,Length,Width,X,Y,
                    XDirection,YDirection);
            else
                GenBox(CDDesc.dSymbolFileDesc,Length,Width,X,Y,
                    XDirection,YDirection);
        }
        else {
            /*
             * Transform non-Manhattan box to polygon.
             */
            float C;
            int Left,Bottom,Right,Top; 
            struct p *Path,*Pair;

            Left = X-(Length >> 1);
            Right = X+(Length >> 1);
            Bottom = Y-(Width >> 1);
            Top = Y+(Width >> 1);
            C = sqrt((double)(XDirection*XDirection+YDirection*YDirection));
            if ((Pair = Path = alloc(p)) == NULL)
                return (AMallocFailed());
            Pair->pX = (Left*XDirection-Bottom*YDirection-
                XDirection*X+YDirection*Y)/C+X;
            Pair->pY = (Left*YDirection+Bottom*XDirection-
                YDirection*X-XDirection*Y)/C+Y;
            if ((Pair = Pair->pSucc = alloc(p)) == NULL)
                return (AMallocFailed());
            Pair->pX = (Left*XDirection-Top*YDirection-
                XDirection*X+YDirection*Y)/C+X;
            Pair->pY = (Left*YDirection+Top*XDirection-
                YDirection*X-XDirection*Y)/C+Y;
            if ((Pair = Pair->pSucc = alloc(p)) == NULL)
                return (AMallocFailed());
            Pair->pX = (Right*XDirection-Top*YDirection-
                XDirection*X+YDirection*Y)/C+X;
            Pair->pY = (Right*YDirection+Top*XDirection-
                YDirection*X-XDirection*Y)/C+Y;
            if ((Pair = Pair->pSucc = alloc(p)) == NULL)
                return (AMallocFailed());
            Pair->pX = (Right*XDirection-Bottom*YDirection-
                XDirection*X+YDirection*Y)/C+X;
            Pair->pY = (Right*YDirection+Bottom*XDirection-
                YDirection*X-XDirection*Y)/C+Y;
            Pair->pSucc = NULL;
            if (CDDesc.dRoot)
                GenPolygon(CDDesc.dRootFileDesc,Path);
            else
                GenPolygon(CDDesc.dSymbolFileDesc,Path);
        }
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        if (CDDesc.dFirstPass And CDDesc.dControl == DCONTROLPCIF)
            return (PSUCCEEDED);
        if (Not CDMakeBox(CDDesc.dSymbolDesc,CurrentLayer,Length,Width,
            X,Y,&CDDesc.dPointer))
            return (PFAILED);
        while(CDDesc.dPrptyList != NULL) {
            struct prpty PrptyCopy;
            if (Not CDAddProperty(CDDesc.dSymbolDesc,CDDesc.dPointer,
                CDDesc.dPrptyList->prpty_Value,
                CDDesc.dPrptyList->prpty_String))
                return (PFAILED);
            /* free storage of CDDesc.dPrptyList */
            PrptyCopy = *CDDesc.dPrptyList;
            free(CDDesc.dPrptyList->prpty_String);
            afree(CDDesc.dPrptyList,prpty);
            CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
        }
    }
    return (PSUCCEEDED);
}


int
ARoundFlash(Width,X,Y)

int Width,X,Y;
{
    struct p *Path, *NewPath;
    struct p Pair;
    /*
     *KIC DOES NOT SUPPORT ROUND FLASHES: convert to a wire with one vertex.
     *Therefore, KIC will never try to generate a Roundflash.
     */
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);
        Pair.pX = SCALE(X);
        Pair.pY = SCALE(Y);
        Pair.pSucc = NULL;
        if (CDDesc.dRoot)
            GenWire(CDDesc.dRootFileDesc,SCALE(Width),&Pair);
        else 
            GenWire(CDDesc.dSymbolFileDesc,SCALE(Width),&Pair);
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        if (CDDesc.dFirstPass And CDDesc.dControl == DCONTROLPCIF)
            return (PSUCCEEDED);
        if ((NewPath = Path = alloc(p)) != NULL)
            return (AMallocFailed());
        NewPath->pX = X;
        NewPath->pY = Y;
        NewPath->pSucc = NULL;
        if (Not CDMakeWire(CDDesc.dSymbolDesc,CurrentLayer,Width,Path,
            &CDDesc.dPointer))
            return (PFAILED);
        while(CDDesc.dPrptyList != NULL) {
            struct prpty PrptyCopy;
            if (Not CDAddProperty(CDDesc.dSymbolDesc,CDDesc.dPointer,
                CDDesc.dPrptyList->prpty_Value,
                CDDesc.dPrptyList->prpty_String))
                return (PFAILED);
            /* free storage of CDDesc.dPrptyList */
            PrptyCopy = *CDDesc.dPrptyList;
            free(CDDesc.dPrptyList->prpty_String);
            afree(CDDesc.dPrptyList,prpty);
            CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
        }
    }
    return (PSUCCEEDED);
}


int
ALayer(Technology,Mask)

char Technology,Mask[];
{
    int Layer;
#ifdef TRACEPARSER
GenLayer(stderr,Technology,Mask);
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass)
            return (PSUCCEEDED);

        for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
            if (CDLayer[Layer].lTechnology == Technology And
                CDLayer[Layer].lMask[0] == Mask[0] And
                CDLayer[Layer].lMask[1] == Mask[1] And
                CDLayer[Layer].lMask[2] == Mask[2]) {
                break;
            }
        }
        if (Layer == CDNUMLAYERS+1) {
            /* no matching layer name */
            char lname[8];
            int i, lnum;

            lname[0] = Technology;
            lname[1] = Mask[0];
            lname[2] = Mask[1];
            lname[3] = Mask[2];
            lname[4] = '\0';
            i = 0;
            while (lname[i] != '\0') {
                if (lname[i] != ' ') {
                    if (lname[i] < '0' || lname[i] > '9')
                        break;
                }
                i++;
            }
            if (lname[i] == '\0') {
                /* layer name is an integer */
                lnum = atoi(lname);
                if (lnum > 0 && lnum <= CDNUMLAYERS) {
                    if (CDLayer[lnum].lTechnology != ' ' Or
                        CDLayer[lnum].lMask[0] != ' ' Or
                        CDLayer[lnum].lMask[1] != ' ' Or
                        CDLayer[lnum].lMask[2] != ' ') {

                        Technology = CDLayer[lnum].lTechnology;
                        Mask[0] = CDLayer[lnum].lMask[0];
                        Mask[1] = CDLayer[lnum].lMask[1];
                        Mask[2] = CDLayer[lnum].lMask[2];
                    }
                }
            }
        }

        if (CDDesc.dRoot)
            GenLayer(CDDesc.dRootFileDesc,Technology,Mask);
        else
            GenLayer(CDDesc.dSymbolFileDesc,Technology,Mask);
        return (PSUCCEEDED);
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
            if (CDLayer[Layer].lTechnology == Technology And
                CDLayer[Layer].lMask[0] == Mask[0] And
                CDLayer[Layer].lMask[1] == Mask[1] And
                CDLayer[Layer].lMask[2] == Mask[2]) {
                CurrentLayer = Layer;
                return (PSUCCEEDED);
            }
        }
        /*
         * Layer is not defined in CD layer table!
         * If parsing CIF and layer is unknown, put it in the layer table.
         * If opening a cell and layer is unknown, complain about it.
         */
        if (CDDesc.dControl == DCONTROLPCIF) {
            for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
                if (CDLayer[Layer].lTechnology == ' ') {
                    CDSetLayer(Layer, Technology, Mask);
                    CurrentLayer = Layer;
                    return (PSUCCEEDED);
                }
            }
        }
        CurrentLayer = 1;
    }
    return (PFAILED);
}


int
AUserExtension(Digit,Text)

char Digit;
char *Text;
{
    int X,Y;
    int Layer,Xform;
    char Label[81];

#ifdef TRACEPARSER
GenUserExtension(stderr,Digit,Text);
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (CDDesc.dFirstPass Or CDDesc.dProgram == 'n')
            return (PSUCCEEDED);
        /*
         * When converting to KIC format, we pass CD user extensions only.
         * If we find an illegal extension, we ignore it.
         */ 
        if (Digit == '9') {
            if (Text[0] == '4') {
                if (CDDesc.dProgram == 'm') {     /* mextra text label */
                    if (sscanf(&(Text[1]),"%s%d%d%s",Label,&X,&Y,TypeOut) < 4)
                        return (PFAILED);
                    if (CDDesc.dRoot)
                        GenLayer(CDDesc.dRootFileDesc,
                            TypeOut[0],&TypeOut[1]);
                    else
                        GenLayer(CDDesc.dSymbolFileDesc,
                        TypeOut[0],&TypeOut[1]);
                }
                else                            /* normal CD label */
                    sscanf(&(Text[1]),"%s%d%d",Label,&X,&Y);
                sprintf(TypeOut,"4 %s %d %d",Label,SCALE(X),SCALE(Y));
                if (CDDesc.dRoot)
                    GenUserExtension(CDDesc.dRootFileDesc,'9',TypeOut);
                else
                    GenUserExtension(CDDesc.dSymbolFileDesc,'9',TypeOut);
            }
            elif (Text[0] == '2') {               /* NCA Label */
                sscanf(&(Text[1]),"%s%d%d%d",Label,&X,&Y,&Layer);
                sprintf(TypeOut,"%d    ",Layer);
                if (CDDesc.dRoot)
                    GenLayer(CDDesc.dRootFileDesc,TypeOut[0],&TypeOut[1]);
                else
                    GenLayer(CDDesc.dSymbolFileDesc,TypeOut[0],&TypeOut[1]);
                sprintf(TypeOut,"4 %s %d %d",Label,SCALE(X),SCALE(Y));
                if (CDDesc.dRoot)
                    GenUserExtension(CDDesc.dRootFileDesc,'9',TypeOut);
                else
                    GenUserExtension(CDDesc.dSymbolFileDesc,'9',TypeOut);
            }
            else {
                /* symbol name */
                if (Text[0] == ' ')
                    strcpy(CDDesc.dSymbolName,&(Text[1]));
                else strcpy(CDDesc.dSymbolName,Text);
                if (CDDesc.dRoot)
                    GenUserExtension(CDDesc.dRootFileDesc,Digit,Text);
                else
                    GenUserExtension(CDDesc.dSymbolFileDesc,Digit,Text);
            }
        }
        elif (Digit == '5' And (Text[0] < '0' Or Text[0] > '9')) {
            /* Reserved for CD property list extensions */
            if (CDDesc.dRoot)
                GenUserExtension(CDDesc.dRootFileDesc,Digit,Text);
            else
                GenUserExtension(CDDesc.dSymbolFileDesc,Digit,Text);
        }
    }
    elif (CDDesc.dControl == DCONTROLCDOPEN Or
            CDDesc.dControl == DCONTROLPCIF) {
        /*
         * When parsing CIF, we accept only CD user extensions and
         * ignore any illegal extensions.
         */
        if (Digit == '9') {
            if (Text[0] == '4') {
                /* Label */
                Xform = 0;
                sscanf(&(Text[1]),"%s%d%d%d",Label,&X,&Y,&Xform);
                if (CDDesc.dFirstPass And CDDesc.dControl == DCONTROLPCIF)
                    return (PSUCCEEDED);
#ifdef TRACEPARSER
fprintf(stderr,"Making label on layer %d\n",CurrentLayer);
#endif
                if (Not CDMakeLabel(CDDesc.dSymbolDesc,CurrentLayer,Label,
                    X,Y,(char)Xform,&CDDesc.dPointer))
                    return (PFAILED);
                while(CDDesc.dPrptyList != NULL) {
                    struct prpty PrptyCopy;
                    if (Not CDAddProperty(CDDesc.dSymbolDesc,
                        CDDesc.dPointer,CDDesc.dPrptyList->prpty_Value,
                        CDDesc.dPrptyList->prpty_String)) return (PFAILED);
                    /* free storage of CDDesc.dPrptyList */
                    PrptyCopy = *CDDesc.dPrptyList;
                    free(CDDesc.dPrptyList->prpty_String);
                    afree(CDDesc.dPrptyList,prpty);
                    CDDesc.dPrptyList = PrptyCopy.prpty_Succ;
                }
            }
            elif (Text[0] < '0' Or Text[0] > '9') {
                /* Symbol name */
                X = 0;
                while(Text[X] <= ' ') ++X;
                strcpy(CDDesc.dSymbolName,Text+X);
            }
        }
        elif (Digit == '1' And (Text[0] < '0' Or Text[0] > '9')) {
            /* Reserved for CD Array extensions */
            sscanf(Text,"%s",TypeOut);
            if (strcmp(TypeOut,"Array") == 0) {
                sscanf(Text,"%s%d%d%d%d",TypeOut,&CDDesc.dNumX,&CDDesc.dDX,
                    &CDDesc.dNumY,&CDDesc.dDY);
            }
        }
        elif (Digit == '5' And (Text[0] < '0' Or Text[0] > '9')) {
            /* Reserved for CD Property List extensions */
            struct prpty *PDesc;
            unsigned int size;
            int i;

            if ((PDesc = alloc(prpty))==NULL)
                return (PFAILED);
            if (sscanf(Text,"%d",&PDesc->prpty_Value) < 1)
                return (PFAILED);
            i = 0;
            /* skip white space before property integer */
            while((Text[i] < '0' Or Text[i] > '9') And Text[i] != '\0') ++i;
            /* skip property integer */
            while(Text[i] >= '0' And Text[i] <= '9') ++i;
            /* skip white space and control chars after property integer */
            while(Text[i] <= ' ' And Text[i] != '\0') ++i;
            size = strlen(&(Text[i])) + 2; 
            if ((PDesc->prpty_String = malloc(size)) == NULL)
                return (PFAILED);
            strcpy(PDesc->prpty_String,&(Text[i]));
            PDesc->prpty_Succ = CDDesc.dPrptyList;
            CDDesc.dPrptyList = PDesc; 
        }
    }
    return (PSUCCEEDED);
}


void
AComment(Text)

char *Text;
{
#ifdef TRACEPARSER
GenComment(stderr,Text);
#endif
    if (CDDesc.dControl == DCONTROLCDTO) {
        if (Not CDDesc.dFirstPass) {
            if (CDDesc.dRoot)
                GenComment(CDDesc.dRootFileDesc,Text);
            else
                GenComment(CDDesc.dSymbolFileDesc,Text);
        }
    }
}


int
AMallocFailed()

{
    sprintf(CDStatusString,"Out of memory.");
    CDStatusInt = CDMALLOCFAILED;
    return (PFAILED);
}

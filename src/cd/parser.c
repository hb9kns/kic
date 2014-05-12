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
 * Fast CIF file parser.
 * 
 * It is easy to use it to write programs that traverse a CIF file. 
 *
 * Examples follows.
 * 1.    A program that translates CIF to STREAM.
 * 2.    A pattern factoring program for PG tape generation.
 *       See TI's PFP.
 * 3.    A statistics generation program. 
 * 4.    A program that extracts each symbol, generates a name for it, and
 *       writes it to a file.  See the CD package.
 * 5.    A program that builds a binary representation of the CIF file that can
 *       then be edited via procedure calls and then written back to the CIF
 *       file.  See the CD package.
 * 
 * Here's how you use it.
 * 
 * First, you include the files actions.c and parser.h.
 * Then you invoke PCIF(CIFFileName,StatusString,StatusInt). 
 * Each time the parser recognizes a CIF command, it invokes an action routine.
 * You should fill in the action routines in the file Actions.c.
 * Each action routine name is prefixed by A.
 * 
 * Each parser routine and global variable name is prefixed by P.
 * StatusInt == PFAILED if the parse failed and an error message along with
 * about where in the CIF file the error is in the StatusString.
 * Else StatusInt == PSUCCEEDED and StatusString == "".
 * 
 */

#include "prefix.h"
#include "cd.h"
#include "parser.h"


FILE *PCIFFileDesc;
char PStatus[80*3+1];
int PInt;
int  PChar;
char PString[PSTRINGSIZE];
int  PReturned;


void
PCIF(CIFFileName,StatusString,StatusInt)

char *CIFFileName,**StatusString;
int *StatusInt;
{
    PStatus[0] = EOS;
    *StatusString = PStatus;
    if ((PCIFFileDesc = POpen(CIFFileName,"r",(char *)NULL,(char **)NULL))
        == NULL) {
        PChar = EOF;
        PError("Can't open CIF file.");
        *StatusInt = PFAILED;
        return;
    }
    loop{
        PReturned = PCharacter(PSTRIPWHITESPACE2,PDONTFAILONEOF);
        if (PReturned == PFAILED) {
            fclose(PCIFFileDesc);
            *StatusInt = PFAILED;
            return;
        }
        elif (PChar == 'D') {
            PReturned = PCharacter(PSTRIPWHITESPACE2,PFAILONEOF);
            if (PReturned == PFAILED) {
                fclose(PCIFFileDesc);
                *StatusInt = PFAILED;
                return;
            }
            elif (PChar == 'S') {
                if (PSymbol() == PFAILED) {
                    fclose(PCIFFileDesc);
                    *StatusInt = PFAILED;
                    return;
                }
            }
            elif (PChar == 'D') {
                if (PDeleteSymbol() == PFAILED) {
                    fclose(PCIFFileDesc);
                    *StatusInt = PFAILED;
                    return;
                }
            }
        }
        elif (PChar == 'E') {
            *StatusInt = PEnd();
            return;
        }
        elif ((PReturned = PPrimitiveCommand()) == PFAILED) {
            fclose(PCIFFileDesc);
            *StatusInt = PFAILED;
            return;
        }
        elif (PReturned == PNOTAPPLICABLE) {
            PError("Can't understand next command.");
            fclose(PCIFFileDesc);
            *StatusInt = PFAILED;
            return;
        }
    }
}


int
PPrimitiveCommand()

{
    if (PChar == 'P')
        return (PPolygon());                
    elif (PChar == 'B')
        return (PBox());
    elif (PChar == 'W')
        return (PWire());
    elif (PChar == 'L')
        return (PLayer());
    elif (PChar == 'C')
        return (PCall());
    elif (PChar == 'R')
        return (PRoundFlash());
    elif ('0' <= PChar And PChar <= '9')
        return (PUserExtension());
    elif (PChar == '(')
        return (PComment());
    elif (PChar == ';')
        return (PSUCCEEDED);
    else
        return (PNOTAPPLICABLE);
}


int
PEnd()

{
    AEnd();
    fclose(PCIFFileDesc);
    return (PSUCCEEDED);
}


int
PSymbol()

{
    char For;
    int SymbolNum;
    int A,B;

#ifdef TRACE
fprintf(stderr," ENTERING PSYMBOL\n");
#endif
    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    SymbolNum = PInt;
    A = B = 1;
    /* look for scaling factor */
    for (For = '0'; For <= '9'; ++For) {
        PReturned = PLookAhead(PSTRIPWHITESPACE3,For);
        if (PReturned == PFAILED)
            return (PFAILED);
        if (PChar == For) {
            ungetc(For,PCIFFileDesc);
            if (PPoint(&A,&B) == PFAILED)
                return (PFAILED);
            break;
        }
    }
    PReturned = PLookForSemi();
    if (PReturned == PFAILED)
        return (PFAILED);
    if (ABeginSymbol((int)SymbolNum,(int)A,(int)B) == PFAILED) {
        PErrorCD();
        return (PFAILED);
    }
    if (PReturned == PFAILED)
        return (PFAILED);
    loop{
        PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
        if (PReturned == PFAILED)
            return (PFAILED);
        if ((PReturned = PPrimitiveCommand()) == PFAILED)
            return (PFAILED);
        elif (PReturned == PNOTAPPLICABLE)
            break;
    }
    if (PChar != 'D')
        return (PFAILED);
    PReturned = PCharacter(PSTRIPWHITESPACE2,PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    if (PChar != 'F')
        return (PFAILED);
    PReturned = PLookForSemi();
    if (PReturned == PFAILED)
        return (PFAILED);
    AEndSymbol();
    return (PSUCCEEDED);
}


int
PDeleteSymbol()

{
    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    else{
        PReturned = PLookForSemi();
        if (PReturned == PFAILED)
            return (PFAILED);
        ADeleteSymbol(PInt);
        return (PSUCCEEDED);
    }
}


int
PCall()

{
    int SymbolNum;
    int X,Y;

    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    SymbolNum = PInt;
    if (ABeginCall(SymbolNum) == PFAILED) {
        PErrorCD();
        return (PFAILED);
    }
    loop{
        PReturned = PCharacter(PSTRIPWHITESPACE2,PFAILONEOF);
        if (PReturned == PFAILED)
            return (PFAILED);
        elif (PChar == 'T') {
            if (PPoint(&X,&Y) == PFAILED) {
                PError("Can't parse translation transform.");
                return (PFAILED);
                }
            elif (AT(CDTRANSLATE,X,Y) == PFAILED) {
                PErrorCD();
                return (PFAILED);
            }
        }
        elif (PChar == 'M') {
            PReturned = PCharacter(PSTRIPWHITESPACE2,PFAILONEOF);
            if (PReturned == PFAILED) {
                PErrorEOF();
                return (PFAILED);
            }
            elif (PChar == 'X') {
                if (AT(CDMIRRORX,X,Y) == PFAILED) {
                    PErrorCD();
                    return (PFAILED);
                }
            }
            elif (PChar == 'Y') {
                if (AT(CDMIRRORY,X,Y) == PFAILED) {
                    PErrorCD();
                    return (PFAILED);
                }
            }
            else{
                PError("Can't parse mirror transform.");
                return (PFAILED);
            }
        }
        elif (PChar == 'R') {
            if (PPoint(&X,&Y) == PFAILED) {
                PError("Can't parse rotation transform.");
                return (PFAILED);
            }
            if (AT(CDROTATE,X,Y) == PFAILED) {
                PErrorCD();
                return (PFAILED);
            }
        }
        elif (PChar == ';')
            break;
        else{
            PError("Can't parse transformation.");
            return (PFAILED);
        }
    } 
    if (AEndCall() == PFAILED) {
        PErrorCD();
        return (PFAILED);
    }
    return (PSUCCEEDED);
}


int
PPolygon()

{
    struct p *Path;

#ifdef TRACE
fprintf(stderr," ENTERING PPOLYGON\n");
#endif
    if (PPath(&Path) == PFAILED)
        return (PFAILED);
#ifdef TRACE
fprintf(stderr," LEAVING PPATH\n");
#endif
    if (APolygon(Path) == PFAILED) {
        PErrorCD();
        return (PFAILED);
    }
#ifdef TRACE
fprintf(stderr," LEAVING PPOLYGON\n");
#endif
    return (PSUCCEEDED);
}


int
PBox()

{
    int Length,Width,X,Y;
    int XDirection,YDirection;

    XDirection = 1L;
    YDirection = 0L;
    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    Length = PInt;
    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    Width = PInt;
    if (PPoint(&X,&Y) == PFAILED)
        return (PFAILED);
    PReturned = PLookForSemi();
    if (PReturned == PFAILED)
        return (PFAILED);
    elif (PChar != ';') {
        if (PPoint(&XDirection,&YDirection) == PFAILED)
            return (PFAILED);
        PReturned = PLookForSemi();
        if (PReturned == PFAILED Or PChar != ';') {
            PErrorNoSemicolon();
            return (PFAILED);
        }
    }
    if (ABox(Length,Width,X,Y,(int)XDirection,(int)YDirection) == PFAILED) {
        PErrorCD();
        return (PFAILED);
    }
    return (PSUCCEEDED);
}


int
PRoundFlash()

{
    int Width,X,Y;

    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    Width = PInt;
    if (PPoint(&X,&Y) == PFAILED)
        return (PFAILED);
    PReturned = PLookForSemi();
    if (PReturned == PFAILED)
        return (PFAILED);
    elif (PChar != ';') {
        PErrorNoSemicolon();
        return (PFAILED);
    }
    if (ARoundFlash(Width,X,Y) == PFAILED) {
        PErrorCD();
        return (PFAILED);
    }
    return (PSUCCEEDED);
}


int
PWire()

{
    int Width;
    struct p *Path;

#ifdef TRACE
fprintf(stderr," ENTERING PWIRE\n");
#endif
    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    Width = PInt;
#ifdef TRACE
fprintf(stderr," ENTERING PPATH\n");
#endif
    if (PPath(&Path) == PFAILED)
        return (PFAILED);
    if (AWire(Width,Path) == PFAILED) {
        PErrorCD();
        return (PFAILED);
    }
    return (PSUCCEEDED);
}


int
PPath(Path)

struct p **Path;
{
    int X,Y;
    struct p *Pair;

    *Path = NULL;
    loop{
        PReturned = PLookForSemi();
#ifdef TRACE
if (PReturned == PFAILED)
    fprintf(stderr," PPATH FAILED AFTER PLOOKAHEAD\n");
#endif
        if (PReturned == PFAILED)
            return (PFAILED);
        elif (PChar == ';')
            break; 
        else{
            if (PPoint(&X,&Y) == PFAILED)
                return (PFAILED);
#ifdef TRACE
fprintf(stderr," PPATH POINT %ld, %ld\n",X,Y);
#endif
            if ((Pair = alloc(p)) == NULL) {
                PError("Out of memory.");
                return (PFAILED);
            }
            Pair->pSucc = *Path;
            Pair->pX = X;
            Pair->pY = Y;
            *Path = Pair;
        }
    }
    return (PSUCCEEDED);
}


int
PPoint(X,Y)

int *X,*Y;
{
    /* it is assumed that a LookAhead is done prior to calling PPoint */
    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    *X = PInt;
    PReturned = PLookForSemi();
    if (PReturned == PFAILED)
        return (PFAILED);
    elif (PChar == ';') {
        PError("Bad X,Y path element.");
        return (PFAILED);
    }
    PReturned = PInteger(PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    *Y = PInt;
    return (PSUCCEEDED);
}


int
PLayer()

{
    char Technology,Mask[4];
    int i;

    Mask[0] = Mask[1] = Mask[2] = ' '; Mask[3] = '\0';
    /* SRW ** used to be STRIPWHITESPACE2, let in lower case */
    PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    if (PChar == ';') {
        PError("At least one character expected after L in layer command.");
        return (PFAILED);
    }
    Technology = PChar;
    for (i=0; i<3; ++i) {
        if ((PChar = getc(PCIFFileDesc)) == EOF) {
            PErrorEOF();
            return (PFAILED);
        }
        elif (PChar == ';') {
            if (ALayer(Technology,Mask) == PSUCCEEDED)
                return (PSUCCEEDED);
            else{
                PErrorUndefinedLayer(Technology,Mask);
                return (PFAILED);
            }
        }
        Mask[i] = PChar;
        /* check for valid CIF layer name character */
        /* SRW ** what the hell, let in lower case,
         * KIC can handle it.
         */
        if (Not ((PChar >= '0' And PChar <= '9') Or
            (PChar >= 'A' And PChar <= 'Z') Or
            (PChar >= 'a' And PChar <= 'z'))) {
            PErrorUndefinedLayer(Technology,Mask);
            return (PFAILED);
        }
    }
    /* clear the semicolon */
    PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
    if (PReturned == PFAILED)
        return (PFAILED);
    elif (PChar == ';') {
        if (ALayer(Technology,Mask) == PSUCCEEDED)
            return (PSUCCEEDED);
        else{
            PErrorUndefinedLayer(Technology,Mask);
            return (PFAILED);
        }
    }
    PError("Illegal CIF layer name with > 4 characters discovered.");
    return (PFAILED);
}


int
PUserExtension()

{
    char Digit;
    int Int1;

    Digit = PChar; 
    Int1 = 0;
    loop{
        PReturned = PCharacter(PLEAVEWHITESPACE,PFAILONEOF);
        if (PReturned == PFAILED)
            return (PFAILED);
        elif (PChar == ';') {
            PString[Int1] = EOS;
            if (AUserExtension(Digit,PString) == PFAILED) {
                PErrorCD();
                return (PFAILED);
            }
#ifdef TRACE
fprintf(stderr,"PString = %s\n",PString);
#endif
            return (PSUCCEEDED);
        }
        elif (Int1 == PSTRINGSIZE) {
            PError("User extension command longer than 1920 characters.");
            return (PFAILED);
        }
        else PString[Int1++] = PChar;
    }
}


int
PComment()

{
    int Int1;
    int parenctr; /* keep track of and ignore nested parens */

    Int1 = 0;
    parenctr = 1;
    loop{
        PReturned = PCharacter(PLEAVEWHITESPACE,PFAILONEOF);
        if (PReturned == PFAILED)
            return (PFAILED);
        elif (PChar == ')' && !--parenctr) {
            PReturned = PCharacter(PSTRIPWHITESPACE1,PFAILONEOF);
            if (PReturned == PFAILED)
                return (PFAILED);
            elif (PChar == ';') {
                PString[Int1] = EOS;
                AComment(PString);
                return (PSUCCEEDED);
            }
            elif (Int1 == PSTRINGSIZE) {
                PError("Comment command longer than 1920 characters.");
                return (PFAILED);
            }
            else{
                PErrorNoSemicolon();
                return (PFAILED);
            }
        }
        else {
            PString[Int1++] = PChar;
            if (PChar == '(') parenctr++;
        }
    }
}


void
PError(PErrorMessage)

char *PErrorMessage;
{
    int Int1;
    
    for (Int1 = 0;Int1 < 20;++Int1) {
        if (PChar == EOF)
            break;
        PReturned = PCharacter(PLEAVEWHITESPACE,PFAILONEOF);
        if (PReturned == PFAILED) 
            break;
        PString[Int1] = PChar;
    }
    PString[Int1] = EOS;
    sprintf(PStatus,"%s  Failed at around %s.",PErrorMessage,PString);
#ifdef TRACE
fprintf(stderr,"%s\n",PErrorMessage);
#endif
}


void
PErrorEOF()

{
    PError("Early EOF.");
}


void
PErrorNoSemicolon()

{
    PError("; expected and not found.");
}


void
PErrorUndefinedLayer(Tech,Mask)

char Tech,*Mask;
{
    char buf[35];
    sprintf(buf,"Undefined layer: %c%s. ",Tech,Mask);
    PError(buf);
}


void
PErrorCD()

{
    PError(CDStatusString);
}


/* the following functions used to be macros */

int
PCharacter(WhiteSpaceControl,EOFControl)

int WhiteSpaceControl,EOFControl;
{
    int Returned;

    Returned = PWhiteSpace(WhiteSpaceControl,EOFControl);
    if (Returned != PFAILED) {
        PChar = getc(PCIFFileDesc);
        if (PChar == EOF) {
            if (EOFControl != PDONTFAILONEOF) {
                PErrorEOF();
                Returned = PFAILED;
            }
        }
    }
    return (Returned);
}


int
PWhiteSpace(WhiteSpaceControl,EOFControl)

int WhiteSpaceControl,EOFControl;
{
    int Returned = PSUCCEEDED;

    if (WhiteSpaceControl == PSTRIPWHITESPACE1) {
        while((PChar = getc(PCIFFileDesc)) == ' ' Or PChar == '\t' Or
            PChar == '\n' Or PChar == ',') {
            if (PChar == EOF) {
                if (EOFControl != PDONTFAILONEOF) {
                    PErrorEOF();
                    Returned = PFAILED;
                }
                break;
            }
         }
         ungetc((char)PChar,PCIFFileDesc);
    }
    elif (WhiteSpaceControl == PSTRIPWHITESPACE2) {
        while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '('
            And PChar != ')' And (PChar < 'A' Or PChar > 'Z')
                And (PChar < '0' Or PChar > '9') And PChar != ';') {
            if (PChar == EOF) {
                if (EOFControl != PDONTFAILONEOF) {
                    PErrorEOF();
                    Returned = PFAILED;
                }
                break;
            }
        }
        ungetc((char)PChar,PCIFFileDesc);
    }
    elif (WhiteSpaceControl == PSTRIPWHITESPACE3) {
        while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '('
            And PChar != ')' And (PChar< '0' Or PChar > '9') And
                PChar != ';') {
            if (PChar == EOF) {
                if (EOFControl != PDONTFAILONEOF) {
                    PErrorEOF();
                    Returned = PFAILED;
                }
                break;
            }
        }
        ungetc((char)PChar,PCIFFileDesc);
    }
    return (Returned);
}


int
PWhiteSpace1(EOFControl)

int EOFControl;
{
    int Returned = PSUCCEEDED;

    while((PChar = getc(PCIFFileDesc)) == ' ' Or PChar == '\t' Or
        PChar == '\n' Or PChar == ',') {
        if (PChar == EOF) {
            if (EOFControl != PDONTFAILONEOF) {
                PErrorEOF();
                Returned = PFAILED;
            }
            break;
        }
    }
    ungetc((char)PChar,PCIFFileDesc);
    return (Returned);
}


int
PWhiteSpace2(EOFControl)

int EOFControl;
{
    int Returned = PSUCCEEDED;
    while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '('
        And PChar != ')' And (PChar < 'A' Or PChar > 'Z')
            And (PChar < '0' Or PChar > '9') And PChar != ';') {
        if (PChar == EOF) {
            if (EOFControl != PDONTFAILONEOF) {
                PErrorEOF();
                Returned = PFAILED;
            }
            break;
        }
    }
    ungetc((char)PChar,PCIFFileDesc);
    return (Returned);
}


int
PWhiteSpace3(EOFControl)

int EOFControl;
{
    int Returned = PSUCCEEDED;

    while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '('
        And PChar != ')' And (PChar< '0' Or PChar > '9') And
            PChar != ';') {
        if (PChar == EOF) {
            if (EOFControl != PDONTFAILONEOF) {
                PErrorEOF();
                Returned = PFAILED;
            }
            break;
        }
    }
    ungetc((char)PChar,PCIFFileDesc);
    return (Returned);
}


int
PLookAhead(WhiteSpaceControl, For)

int WhiteSpaceControl, For;
{
    int Returned;

    Returned = PCharacter(WhiteSpaceControl,PFAILONEOF);
    if (PChar != For && Returned != PFAILED)
        ungetc((char)PChar,PCIFFileDesc);
    return (Returned);
}


int
PLookForSemi()

{
    int Returned;

    Returned = PWhiteSpace3(PFAILONEOF);
    if (Returned != PFAILED) {
        PChar = getc(PCIFFileDesc);
        if (PChar == EOF) {
            PErrorEOF();
            Returned = PFAILED;
        }
        elif (PChar != ';')
            ungetc((char)PChar,PCIFFileDesc);
    }
    return (Returned);
}


int
PInteger(EOFControl)

int EOFControl;
{
    int Returned;

    loop{
        PReturned = PWhiteSpace3(EOFControl);
        if (PReturned == PFAILED) {
            Returned = PFAILED;
            break;
        }
        if ((PChar >= '0' And PChar <= '9') Or
            (PChar == '-' Or PChar == '+')) {
            /* read integer */
            if ((fscanf(PCIFFileDesc,"%d",&PInt)) == EOF) {
                if (EOFControl != PDONTFAILONEOF) {
                    PChar = EOF;
                    PErrorEOF();
                    Returned = PFAILED;
                }
            }
            Returned = PSUCCEEDED;
            break;
        }
        /* drop unwanted characters */
        PReturned = PCharacter(PSTRIPWHITESPACE1,EOFControl);
        if (PReturned == PFAILED) {
            Returned = PFAILED;
            break;
        }
    }
    return (Returned);
}

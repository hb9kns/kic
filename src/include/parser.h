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

/*
 * Fast CIF parser's data structures.
 * 
 */

#include "macros.h"
#include <stdio.h>

/*
 * 24*80+1=1921 characters of string.
 */
#define PSTRINGSIZE 1921

/*
 * Status of parse when parser returns.
 */
extern char PStatus[80*3+1];

/*
 * CIF file desc.
 */
extern FILE *PCIFFileDesc;

/*
 * string for input line buffering
 */
extern char PString[PSTRINGSIZE];

/*
 * Global token variables.
 */
extern int  PChar;
extern int PInt;
extern int  PReturned;

/*
 * Ousterhout's path package.
 */
extern FILE *POpen();

/*
 * Values routines return.
 */
#define PSUCCEEDED         1    /* successful return */
#define PFAILED            2    /* parser failed */
#define PNOTAPPLICABLE     3    /* parser failed due to syntax */

/*
 * Arguments to handle an EOF in PCharacter and PInteger
 */
#define PFAILONEOF         1
#define PDONTFAILONEOF     2

/*
 * Arguments to specify characters to be ignored by PWhiteSpace
 */
#define PSTRIPWHITESPACE1  1    /* strip blanks, tabs, commas, or    */
                                /* new lines.                        */
#define PSTRIPWHITESPACE2  2    /* strip everything but upper case,  */
                                /* hyphens, digits, parens, and ;'s. */
#define PSTRIPWHITESPACE3  3    /* strip everything but digits,      */
                                /* hyphens, parens, and ;'s.         */
#define PLEAVEWHITESPACE   4


/* parser.c */
extern FILE *PCIFFileDesc;
extern char PStatus[80*3+1];
extern int PInt;
extern int  PChar;
extern char PString[PSTRINGSIZE];
extern int  PReturned;
#if __STDC__
extern void PCIF(char*,char**,int*);
extern int  PPrimitiveCommand(void);
extern int  PEnd(void);
extern int  PSymbol(void);
extern int  PDeleteSymbol(void);
extern int  PCall(void);
extern int  PPolygon(void);
extern int  PBox(void);
extern int  PRoundFlash(void);
extern int  PWire(void);
extern int  PPath(struct p**);
extern int  PPoint(int*,int*);
extern int  PLayer(void);
extern int  PUserExtension(void);
extern int  PComment(void);
extern void PError(char*);
extern void PErrorEOF(void);
extern void PErrorNoSemicolon(void);
extern void PErrorUndefinedLayer(int,char*);
extern void PErrorCD(void);
extern int  PCharacter(int,int);
extern int  PWhiteSpace(int,int);
extern int  PWhiteSpace1(int);
extern int  PWhiteSpace2(int);
extern int  PWhiteSpace3(int);
extern int  PLookAhead(int,int);
extern int  PLookForSemi(void);
extern int  PInteger(int);
#else
extern void PCIF();
extern int  PPrimitiveCommand();
extern int  PEnd();
extern int  PSymbol();
extern int  PDeleteSymbol();
extern int  PCall();
extern int  PPolygon();
extern int  PBox();
extern int  PRoundFlash();
extern int  PWire();
extern int  PPath();
extern int  PPoint();
extern int  PLayer();
extern int  PUserExtension();
extern int  PComment();
extern void PError();
extern void PErrorEOF();
extern void PErrorNoSemicolon();
extern void PErrorUndefinedLayer();
extern void PErrorCD();
extern int  PCharacter();
extern int  PWhiteSpace();
extern int  PWhiteSpace1();
extern int  PWhiteSpace2();
extern int  PWhiteSpace3();
extern int  PLookAhead();
extern int  PLookForSemi();
extern int  PInteger();
#endif

/* the following macros are now functions in parser.c */

#ifdef notdef

    /* PWhiteSpace returns either PSUCCEEDED or PFAILED */
#define PCharacter(Returned,WhiteSpaceControl,EOFControl){ \
    PWhiteSpace(Returned,WhiteSpaceControl,EOFControl); \
    if(Returned != PFAILED){ \
     PChar = getc(PCIFFileDesc); \
     if(PChar == EOF){ \
      if(EOFControl != PDONTFAILONEOF) { \
       PErrorEOF(); \
       Returned = PFAILED; \
      } \
     } \
    } \
}

#define PWhiteSpace(Returned,WhiteSpaceControl,EOFControl){ \
    Returned = PSUCCEEDED; \
    if(WhiteSpaceControl == PSTRIPWHITESPACE1){ \
     while((PChar = getc(PCIFFileDesc)) == ' ' Or PChar == '\t' Or \
      PChar == '\n' Or PChar == ','){ \
      if(PChar == EOF){ \
       if(EOFControl != PDONTFAILONEOF){ \
        PErrorEOF(); \
        Returned = PFAILED; \
       } \
       break; \
      } \
     } \
     ungetc((char)PChar,PCIFFileDesc); \
    } \
    elif(WhiteSpaceControl == PSTRIPWHITESPACE2){ \
     while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '(' \
      And PChar != ')' And (PChar < 'A' Or PChar > 'Z') \
      And (PChar < '0' Or PChar > '9') And PChar != ';'){ \
       if(PChar == EOF){ \
        if(EOFControl != PDONTFAILONEOF) { \
         PErrorEOF(); \
         Returned = PFAILED; \
        } \
        break; \
       } \
      } \
      ungetc((char)PChar,PCIFFileDesc); \
     } \
     elif(WhiteSpaceControl == PSTRIPWHITESPACE3){ \
      while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '(' \
       And PChar != ')' And (PChar< '0' Or PChar > '9') And \
       PChar != ';'){ \
       if(PChar == EOF){ \
        if(EOFControl != PDONTFAILONEOF){ \
         PErrorEOF(); \
         Returned = PFAILED; \
        } \
        break; \
       } \
      } \
      ungetc((char)PChar,PCIFFileDesc); \
     } \
}

#define PWhiteSpace1(Returned,EOFControl){ \
    Returned = PSUCCEEDED; \
    while((PChar = getc(PCIFFileDesc)) == ' ' Or PChar == '\t' Or \
     PChar == '\n' Or PChar == ','){ \
     if(PChar == EOF){ \
      if(EOFControl != PDONTFAILONEOF){ \
       PErrorEOF(); \
       Returned = PFAILED; \
      } \
      break; \
     } \
    } \
    ungetc((char)PChar,PCIFFileDesc); \
}

#define PWhiteSpace2(Returned,EOFControl){ \
    Returned = PSUCCEEDED; \
    while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '(' \
     And PChar != ')' And (PChar < 'A' Or PChar > 'Z') \
     And (PChar < '0' Or PChar > '9') And PChar != ';'){ \
     if(PChar == EOF){ \
      if(EOFControl != PDONTFAILONEOF) { \
       PErrorEOF(); \
       Returned = PFAILED; \
      } \
     break; \
     } \
    } \
    ungetc((char)PChar,PCIFFileDesc); \
}

#define PWhiteSpace3(Returned,EOFControl){ \
    Returned = PSUCCEEDED; \
    while((PChar = getc(PCIFFileDesc)) != '-' And PChar != '(' \
     And PChar != ')' And (PChar< '0' Or PChar > '9') And \
     PChar != ';'){ \
     if(PChar == EOF){ \
      if(EOFControl != PDONTFAILONEOF){ \
       PErrorEOF(); \
       Returned = PFAILED; \
      } \
      break; \
     } \
    } \
    ungetc((char)PChar,PCIFFileDesc); \
}

#define PLookAhead(Returned, WhiteSpaceControl, For){ \
    /* PCharaceter will return either PSUCCEEDED or PFAILED */ \
    PCharacter(Returned,WhiteSpaceControl,PFAILONEOF); \
    if(PChar != For && Returned != PFAILED){ \
     ungetc((char)PChar,PCIFFileDesc); \
    } \
}

#define PLookForSemi(Returned){ \
    /* PWhiteSpace returns either PSUCCEEDED or PFAILED */ \
    PWhiteSpace3(Returned,PFAILONEOF); \
    if(Returned != PFAILED){ \
     PChar = getc(PCIFFileDesc); \
     if(PChar == EOF){ \
      PErrorEOF(); \
      Returned = PFAILED; \
     } \
     elif(PChar != ';'){ \
      ungetc((char)PChar,PCIFFileDesc); \
     } \
    } \
}

#define PInteger(Returned,EOFControl) { \
    loop{ \
     PWhiteSpace3(PReturned,EOFControl); \
     if(PReturned == PFAILED){ \
      Returned = PFAILED; \
      break; \
     } \
     if((PChar >= '0' And PChar <= '9') Or \
      (PChar == '-' Or PChar == '+')) { \
      /* read integer */ \
      if((fscanf(PCIFFileDesc,"%ld",&PInt)) == EOF){ \
       if(EOFControl != PDONTFAILONEOF){ \
        PChar = EOF; \
        PErrorEOF(); \
        Returned = PFAILED; \
       } \
      } \
      Returned = PSUCCEEDED; \
      break; \
     } \
     /* drop unwanted characters */ \
     PCharacter(PReturned,PSTRIPWHITESPACE1,EOFControl); \
     if(PReturned == PFAILED){ \
      Returned = PFAILED; \
      break; \
     } \
    } \
}

#endif

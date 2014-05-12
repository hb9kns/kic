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

/*                                                              */
/*    Confidential -- All proprietary rights reserved           */
/*    Licensed Material -- Property of Tektronix                */
/*                                                              */
/*    STRMTOTEXT -- converts a Calma Stream file (3.0) to       */
/*              a text image.                                   */
/*                                                              */
/*         strmtotext [-id] [-n12345678] [streamfile            */
/*         [textfile]]                                          */
/*                                                              */
/*         -id        prints the version of the program         */
/*                    and copyright information.                */
/*         -n         indicates a non-standard Stream file      */
/*                    is to be read, that is, one struc-        */
/*                    ture beginning with BGBSTR and end-       */
/*                    ing with ENDSTR.                          */
/*         1-8        indicate the number of Stream rec-        */
/*                    ords per line in the output text.         */
/*                    (Default is one per line).                */
/*         streamfile a Calma Stream file (3.0) input to        */
/*                    this program. (Standard input de-         */
/*                    fault).                                   */
/*         textfile   the name of the file to receive the       */
/*                    program's output. (Standard output        */
/*                    default).                                 */
/*                                                              */
/*                                                              */
/*    Author:  Giles Billingsley                                */
/*                                                              */
/*    Maintenance:  David Inman                                 */
/*                    (503) 627-4083                            */
/*                                                              */

/*----------------------------------------------------------------------
 * Modified by S. R. Whiteley 12/3/91
 *
 * In read_stream(), the original version defined 2 buffers of length
 * MAXRECSIZE: int buf[] and char cbuf[].  The arrays were filled
 * as
 *    buf[i] = getc(workfile);
 *    cbuf[i] = buf[i];
 *
 * In the following code, &buf[] was used as an argument to strm_ival(),
 * and elsewhere, except when an 8 bit char was specifically needed.
 * This works on some systems because getc() actually returns an int,
 * which is not always true.  Here, the buf buffer is not used, and
 * an index into cbuf is always passed.
 *
 * This version is self-contained, as the (modified) routines from
 * stream.c have been appended to this file. Defining the variable IEEE
 * below specifies IEEE floating point.  Comment this out for DEC
 * floating point.
 *---------------------------------------------------------------------
 */

#include "prefix.h"
#include <math.h>
#include <stdlib.h>
#include "stream.h"

/* undefine this for DEC floating point */
#define IEEE

FILE *STREAMFILE;
int IbigEndian;
int FbigEndian;

char *version[] ={
    /*
    "strmtotext 1.6 4/7/83",
    "strmtotext 1.7 12/3/91",
    */
    "strmtext 1.8 1/6/94",
    /*
    "@(#)version.c    1.6 (Tektronix) 4/7/83",
    */
    0
    };

char *Trade_Secret_License[] ={

    /* This is BS
    "This program is the property of Tektronix, Inc.  or  others  from",
    "whom  Tektronix has obtained a licensing right, and is considered",
    "by Tektronix to be confidential.  It is protected by  U.S.  copy-",
    "right  law  as an unpublished work and is furnished pursuant to a",
    "written license agreement.  It may not be used, copied or  other-",
    "wise reproduced, or disclosed to others except in accordance with",
    "the terms and conditions of that agreement.",
    */
    "Written by Kenneth H. Keller and Giles C. Billingsley, 1981,",
    "under funding from Tektronix, Inc.",
    "",
    "Modified to work with IEEE floating point on a larger class of",
    "machines, by S. R. Whiteley (12/3/91).",
    0
    };

#if __STDC__
extern void read_stream(FILE*,FILE*,int,int);
extern int fptest(int*,int*);
extern short strm_ival(char*);
extern int strm_longval(char*);
extern double strm_doubleval(char*);
#else
extern void read_stream();
extern int fptest();
extern short strm_ival();
extern int strm_longval();
extern double strm_doubleval();
#endif


int
main(argc,argv)
    int argc;
    char *argv[];
    {
    FILE *outfile = NULL;
    char *p;
    int jobtype = 0,lcount = 1,i;

#ifdef TEKTRONIX
    setname(*argv);            /* set name of program for prterr */
#else
#define prterr fprintf
#endif

usage:

    if(--argc == 0){
        printf("Usage:  strmtotext [-id] [-n2345678] [ streamfile [textfile] ]\n");
        exit(1);
        }

    if (fptest(&IbigEndian,&FbigEndian)) {
        fprintf(stderr,"Error: incompatible floating point format.\n");
        exit(1);
    }
    /* evaluate options, if any */
    while(*(p = *++argv) == '-'){
        --argc;
        while(*++p != '\0'){
            if(*p == 'n')
                jobtype = 1;
            else if( (i = *p) > 48 && i < 57)
                lcount = i - 48;
            else if(*p == 'i'){            /* id flag */
                if(*(p+1) == 'd'){
                    extern char *version[];
                    extern char *Trade_Secret_License[];
                    int j;

                    fprintf(stdout,"%s\n\n",version[0]);
                    for(j = 0;Trade_Secret_License[j];j++)
                        fprintf(stdout,"%s\n",Trade_Secret_License[j]);
                    exit(0);
                    }
                }
            else{ 
                argc = 0; 
                goto usage; 
                }
            }
        }
    --argv;

    /* evaluate arguments */
    /* one argument = streamfile */
    if(argc >= 1){

#if (__NDPC__)
/* SRW ** Under DOS, need to open stream file in binary mode.  With my
 * compiler (Microway NDP C), setting the external _pmode does this.
 * Other compilers do this differently, e.g., by using "rb" as the mode
 * string to fopen().
 */
_pmode = 0x8000;
#endif

        if((STREAMFILE = fopen(*++argv,"rb")) == NULL){
            prterr(stderr,"Can't open %s\n",*argv);
            exit(1);
            }
        if(argc == 1)
            outfile = stdout;
        }

    /* two arguments = streamfile and textfile */
    if(argc == 2){
        if((outfile = fopen(*++argv,"w")) == NULL){
            prterr(stderr,"Can't open %s\n",*argv);
            exit(1);
            }
        }

    /* no arguments = stdio */
    if(argc == 0){
        STREAMFILE = stdin;
        outfile = stdout;
        }

    read_stream(outfile,STREAMFILE,jobtype,lcount);
    return (0);
    }


/*                                                                     */
/*    function READ_STREAM                                             */
/*    Function to transfer STREAM to ASCII output file.  If the        */
/*        transfer variable 'jobtype' equals unity, this function      */
/*        will terminate with the appearance of ENDLIB or ENDSTR.      */
/*        This allows processing of non-standard STREAM libraries.     */
/*        Concatenated records are separated by a semicolon.           */
/*                                    */

void
read_stream(outfile,workfile,jobtype,lcount)
    FILE *outfile,*workfile;
    int jobtype,lcount;
    {
    unsigned type = 0;        /* current record type                   */
    unsigned numb;            /* number of bytes in current record     */
    int datatype;
    int level=0;
    int colcntr = 0;
    int lmod[6],lacc[6];      /* library modification and access dates */
    int cred[6],accd[6];      /* structure creation and access dates   */
    char cbuf[MAXRECSIZE];
    int i,l;

    while(type != 4){
        numb = (unsigned char)getc(workfile);
        numb = numb * 256 + (unsigned char)getc(workfile) - 4;
        type = (unsigned char)getc(workfile);
        datatype = getc(workfile);
        for(i=0; i<numb; ++i)
            cbuf[i] = (unsigned char)getc(workfile);

    cbuf[numb] = '\0';
        switch(type){

        case HEADER:
            if(strm_ival(cbuf) != 3)
                printf("WARNING:  NOT A VERSION 3 LIBRARY\n");
            break;

        case BGNLIB:
            for(i=0; i<6; ++i){
                lmod[i] = strm_ival(&cbuf[i * 2]);
                lacc[i] = strm_ival(&cbuf[i * 2 + 12]);
                }
            break;
        
    case LIBNAME:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
            }
            fprintf(outfile,"LIBRARY\t\t%s;\n",cbuf);
            fprintf(outfile,"MODIFICATION\t%d/%d/%d  %d:%d:%d;\n",
        lmod[1],lmod[2],lmod[0],lmod[3],lmod[4],lmod[5]);
            fprintf(outfile,"ACCESS\t\t%d/%d/%d  %d:%d:%d;\n",
        lacc[1],lacc[2],lacc[0],lacc[3],lacc[4],lacc[5]);
            break;
        
    case UNITS:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"U-UNIT\t\t%.6e;\n",strm_doubleval(cbuf));
            fprintf(outfile,"M-UNIT\t\t%.6e;\n",strm_doubleval(&cbuf[8]));
            break;
        
    case ENDLIB:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"END LIBRARY;\n");
            break;
        
    case BGNSTR:
            for(i=0; i<6; ++i){
                cred[i] = strm_ival(&cbuf[i * 2]);
                accd[i] = strm_ival(&cbuf[i * 2 + 12]);
                }
            break;
        
    case STRNAME:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"STRUCTURE\t%s;\n",cbuf);
            fprintf(outfile,"\tCREATION\t%d/%d/%d  %d:%d:%d;\n",
        cred[1],cred[2],cred[0],cred[3],cred[4],cred[5]);
            fprintf(outfile,"\tACCESS\t\t%d/%d/%d  %d:%d:%d;\n",
        accd[1],accd[2],accd[0],accd[3],accd[4],accd[5]);
            break;
        
    case ENDSTR:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\tEND STRUCTURE;\n");
            fprintf(outfile,"\n");
            if(jobtype == 1)  type = 4;
            break;
        
    case BOUNDARY:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\tBOUNDARY;\n");
            level = 3;
            break;
        
    case PATH:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\tPATH;\n");
            level = 4;
            break;
        
    case SREF:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\tSREF\t");
            level = 5;
            break;
        
    case AREF:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\tAREF\t");
            level = 6;
            break;
        
    case TEXT:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\tTEXT;\n");
            level = 7;
            break;
        
    case LAYER:
            fprintf(outfile,"\t\tLAYER\t\t%d;",strm_ival(cbuf));
            ++colcntr;
            break;
        
    case DATATYPE:
            fprintf(outfile,"\t\tDATATYPE\t%d;",strm_ival(cbuf));
            ++colcntr;
            break;
        
    case WIDTH:
            ++colcntr;
            fprintf(outfile,"\t\tWIDTH\t\t%d;",strm_longval(cbuf));
            break;
        
    case XY:
            l = numb/8;
            if(level == 6){
                fprintf(outfile,"\t\tPREF\t\t%d, %d;",
            strm_longval(&cbuf[0]),strm_longval(&cbuf[4]));
                if(++colcntr >= lcount){
                    colcntr = 0; 
                    fprintf(outfile,"\n"); 
                    }
                fprintf(outfile,"\t\tPCOL\t\t%d, %d;",
            strm_longval(&cbuf[8]),strm_longval(&cbuf[12]));
                if(++colcntr >= lcount){
                    colcntr = 0; 
                    fprintf(outfile,"\n"); 
                    }
                fprintf(outfile,"\t\tPROW\t\t%d, %d;",
            strm_longval(&cbuf[16]),strm_longval(&cbuf[20]));
                if(++colcntr >= lcount){
                    colcntr = 0; 
                    fprintf(outfile,"\n"); 
                    }
                }
            else{
                if(l > 1){
                    if(colcntr) fprintf(outfile,"\n");
                    fprintf(outfile,"\t\tCOORDINATES\t%d\t%d, %d\n",
            l,strm_longval(cbuf),strm_longval(&cbuf[4]));
                    for(i=1; i<l; ++i){
                        fprintf(outfile,"\t\t\t\t\t%d, %d",
                strm_longval(&cbuf[8 * i]),
                strm_longval(&cbuf[8 * i + 4]));
                        if(i < l-1) fprintf(outfile,"\n");
                        }
                    colcntr = 0;
                    fprintf(outfile,";\n");
                    }
                else{
                    fprintf(outfile,"\t\tCOORDINATE\t%d, %d;",
            strm_longval(cbuf),strm_longval(&cbuf[4]));
                    ++colcntr;
                    }
                }
            break;
        
    case ENDEL:
            if(colcntr) fprintf(outfile,"\n");
            fprintf(outfile,"\t\tEND ELEMENT;\n");
            fprintf(outfile,"\n");
            level = 2;
            break;
        
    case SNAME:
            fprintf(outfile,"%s;\n",cbuf);
            break;
        
    case COLROW:
            fprintf(outfile,"\t\tCOLUMNS\t\t%d;",strm_ival(cbuf));
            if(++colcntr >= lcount){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\t\tROWS\t\t%d;",strm_ival(&cbuf[2]));
            ++colcntr;
            break;
        
    case TEXTNODE:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\t\tTEXTNODE;\n");
            break;
        
    case SNAPNODE:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"\tSNAPNODE;\n");
            break;
        
    case TEXTTYPE:
            fprintf(outfile,"\t\tTEXTTYPE\t%d;",strm_ival(cbuf));
            ++colcntr;
            break;
        
    case PRESENTATION:
            l = 3;
            l = (cbuf[1] & l);
            fprintf(outfile,"\t\tHJUSTIFICATION\t%d;",l);
            if(++colcntr >= lcount){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            l = 12;
            l = (cbuf[1] & l) >> 2;
            fprintf(outfile,"\t\tVJUSTIFICATION\t%d;",l);
            if(++colcntr >= lcount){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            l = 48;
            l = (cbuf[1] & l) >> 4;
            fprintf(outfile,"\t\tFONT\t\t%d;",l);
            ++colcntr;
            break;
        
    case SPACING:
            break;
        
    case STRING:
            fprintf(outfile,"\t\tSTRING\t\t%s;",cbuf);
            ++colcntr;
            break;
        
    case STRANS:
            l = 128;
            l = (cbuf[0] & l) >> 7;
            if(l == 1){
                fprintf(outfile,"\t\tREFLECTION;");
                if(++colcntr >= lcount){
                    colcntr = 0; 
                    fprintf(outfile,"\n"); 
                    }
                }
            l = 4;
            l = (cbuf[1] & l);
            if(l == 4){
                fprintf(outfile,"\t\tABSOLUTE MAGNIFICATION;");
                if(++colcntr >= lcount){
                    colcntr = 0; 
                    fprintf(outfile,"\n"); 
                    }
                }
            l = 2;
            l = (cbuf[1] & l);
            if(l == 2){
                fprintf(outfile,"\t\tABSOLUTE ANGLE;");
                ++colcntr;
                }
            break;
        
    case MAG:
            fprintf(outfile,"\t\tMAGNIFICATION\t%f;",strm_doubleval(cbuf));
            ++colcntr;
            break;
        
    case ANGLE:
            fprintf(outfile,"\t\tANGLE\t\t%f;",strm_doubleval(cbuf));
            ++colcntr;
            break;
        
    case UINTEGER:
            break;
        
    case USTRING:
            break;
        
    case REFLIBS:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"REFLIB\t\t%s;\n",cbuf);
            fprintf(outfile,"REFLIB\t\t%s;\n",&cbuf[44]);
            break;
        
    case FONTS:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"FONT0\t\t%s;\n",cbuf);
            fprintf(outfile,"FONT1\t\t%s;\n",&cbuf[44]);
            fprintf(outfile,"FONT2\t\t%s;\n",&cbuf[88]);
            fprintf(outfile,"FONT3\t\t%s;\n",&cbuf[132]);
            break;
        
    case PATHTYPE:
            fprintf(outfile,"\t\tPATHTYPE\t%d;",strm_ival(cbuf));
            ++colcntr;
            break;
        
    case GENERATIONS:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"GENERATIONS\t%d;\n",strm_ival(cbuf));
            break;
        
    case ATTRTABLE:
            if(colcntr){
                colcntr = 0; 
                fprintf(outfile,"\n"); 
                }
            fprintf(outfile,"ATTRIBUTE-FILE\t%s;\n",cbuf);
            break;
        
    case PROPATTR:
            if(colcntr) fprintf(outfile,"\n");
            colcntr = 0;
            fprintf(outfile,"\t\tATTRIBUTES\t%d=",strm_ival(cbuf));
            break;
        
    case PROPVALUE:
            fprintf(outfile,"%s;\n",cbuf);
            break;

        default:
            fprintf(outfile,"\n");
/*
            fprintf(outfile,"ERROR HAS OCCURRED\n");
*/
            fprintf(outfile,"WARNING: UNKNOWN RECORD TYPE\n");
            fprintf(outfile,"RECORD TYPE = %d\tLENGTH = %d\tDATATYPE = %d\n",type,numb,datatype);
/*
            exit(0);
*/
            }

        if(colcntr >= lcount){
            colcntr = 0;
            fprintf(outfile,"\n");
            }
        }
    }


#define False 0
#define True 1


int
fptest(iflg,dflg)

/* Determine whether the CPU stores integers and doubles in big or little
 * endian format.  If the least significant bits are found in the
 * char at the data item address, then the format is little endian.
 * The sign bit and exponent are the most significant bits of the
 * double.
 * The flags are set true if big endian.  If either format is not
 * recognized, true is returned.
 */
int *iflg, *dflg;
{
    union {double d; unsigned int l[2];
        unsigned short i[4]; unsigned char c[8];} u;

    u.d = -2.0;
    if (u.c[7] == 0xc0)
        *dflg = False;
    else if (u.c[0] == 0xc0)
        *dflg = True;
    else
        return (True);

    u.l[0] = 1L;
    if (u.c[0] == 1)
        *iflg = False;
    else if (u.c[3] == 1)
        *iflg = True;
    else
        return (True);
    return (False);
}


/* Functions used in reading stream format */

typedef union {
    short w[4];     /* double precision number as four shorts */
    int  l[2];     /* double precision value as two int    */
    double dval;    /* value of double precision number       */
}
    dbltype;

#if __STDC__
static double ieeed(dbltype);
static void rev_bytes(double*);
static unsigned int getbits(unsigned int,int,int);
#else
static double ieeed();
static void rev_bytes();
static unsigned int getbits();
#endif


/***********************************************************************/
/* function STRM_IVAL                                                  */
/*      Function to evaluate STREAM short integer.                     */
/***********************************************************************/

short
strm_ival(b)

char *b;
{
    union { short i; char c[2];} si;

    /* Stream format is big-endian */
    if (IbigEndian) {
        si.c[0] = b[0];
        si.c[1] = b[1];
    }
    else {
        si.c[0] = b[1];
        si.c[1] = b[0];
    }
    return si.i;
}


/***********************************************************************/
/* function STRM_LONGVAL                                               */
/*      Function to evaluate STREAM long integer.                      */
/***********************************************************************/

int
strm_longval(b)

char *b;
{
    union {int l; char c[4];} sl;

    /* Stream format is big-endian */
    if (IbigEndian) {
        sl.c[0] = b[0];
        sl.c[1] = b[1];
        sl.c[2] = b[2];
        sl.c[3] = b[3];
    }
    else {
        sl.c[0] = b[3];
        sl.c[1] = b[2];
        sl.c[2] = b[1];
        sl.c[3] = b[0];
    }
    return sl.l;
}


/*
 * function STRM_DOUBLEVAL
 *      Function to convert from STREAM to VAX double precision.
 *      The argument is a integer buffer containing the eight bytes
 *      of the STREAM double precision field.  The first character in
 *      the buffer contains the exponent, the second contains the most
 *      significant byte of the mantissa, etc.
 *
 *
 *    VAX's double precision field:
 *
 *    Mantissa is base 2 (1/2 <= mantissa < 1).  Exponent is excess-128.
 *
 *              111111 1111222222222233 3333333344444444 4455555555556666
 *    0123456789012345 6789012345678901 2345678901234567 8901234567890123
 *    ---------------- ---------------- ---------------- ----------------
 *    FFFFFFFEEEEEEEES FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
 *    L     M          L              M L              M L              M
 *
 *
 *
 *    CALMA's double precision field:
 *
 *    Mantissa is base 16 (1/16 <= mantissa < 1).  Exponent is excess-64.
 *
 *              111111 1111222222222233 3333333344444444 4455555555556666
 *    0123456789012345 6789012345678901 2345678901234567 8901234567890123
 *    ---------------- ---------------- ---------------- ----------------
 *    FFFFFFFFEEEEEEES FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
 *    L      M         L              M L              M L              M
 *
 *
 *    where  E  =  exponent field
 *           S  =  sign bit
 *           F  =  fraction field
 *           FL =  least sig. bit of word or byte
 *           FM =  most sig. bit of word or byte
 */

double
strm_doubleval(ip)

char *ip;
{
    int exp;
    int sign;
    int i,j;
    dbltype Strm_DblAndShrt;
    union { short i; char c[2];} si;

    sign = 0;
    i = j = 0;
    exp = ip[0];
    /* test the sign bit */
    if (exp & 0x80) {
        sign = 1;
        exp &= 0x7f;
    }
    /*
     * We will transfer the chars of the double precision field to
     * simplify this routine.  i.e., it is easier to keep track of
     * all the bits when we shift 16 bit fields rather than 8 bit
     * fields.
     */
    for (i = 0; i <= 3; ++i) {
        if (IbigEndian) {
            si.c[1] = ip[i+i+1];
            si.c[0] = ip[i+i];
        }
        else {
            si.c[0] = ip[i+i+1];
            si.c[1] = ip[i+i];
        }
        Strm_DblAndShrt.w[i] = si.i;
    }
    /* mask the exponent */
    Strm_DblAndShrt.w[0] &= 0xff;
    /* multiply by 2 until the most significant mantissa bit is set */
    while (!(Strm_DblAndShrt.w[0] & 0x80) && (j < 64)) {
        ++j;
        for (i = 0; i <= 2; ++i) {
            Strm_DblAndShrt.w[i] = (Strm_DblAndShrt.w[i] << 1);
            if (Strm_DblAndShrt.w[i+1] & 0x8000) Strm_DblAndShrt.w[i] |= 1;
        }
        Strm_DblAndShrt.w[3] = (Strm_DblAndShrt.w[3] << 1) & 0xffff;
    }
    if (j == 64) return (0.0);
    /*
     * Add exponent to the first word in the working buffer.
     * We must subtract j from the exponent which is the number of
     * times the mantissa was multiplied by 2.
     *
     * There is another trick which is not so obvious.  We multiplied
     * by 2 until the most significant bit of the STREAM mantissa was
     * set.  By dropping that bit, we convert from excess-64 to
     * excess-128.
     */
    Strm_DblAndShrt.w[0] =
        ((Strm_DblAndShrt.w[0] & 0x7f) | (((exp << 2)+128-j) << 7)) & 0x7fff;
    /* is it negative? */
    if (sign)
        Strm_DblAndShrt.w[0] |= 0x8000;
#ifdef IEEE
    return ieeed(Strm_DblAndShrt);        /* IEEE double */
#else
    return(Strm_DblAndShrt.dval);          /* VAX double */
#endif
}


#ifdef IEEE

/* SRW */

static double
ieeed(d)        /* return IEEE double given DEC double */

dbltype d;
{
    unsigned int e, m;

    if (!IbigEndian) {
        e = d.w[1];
        d.w[1] = d.w[0];
        d.w[0] = e;
        e = d.w[3];
        d.w[3] = d.w[2];
        d.w[2] = e;
    }

    e = (getbits(d.l[0],30,8) + 01576) << 20;
    e += (d.l[0] & 0x80000000);
    m = getbits(d.l[0],22,23);
    d.l[0] = (d.l[1] >> 3) + ((m & 7) << 29);
    d.l[1] = e + (m >> 3);
    if (IbigEndian && FbigEndian) {
        m = d.l[0];
        d.l[0] = d.l[1];
        d.l[1] = m;
    }
    else if (IbigEndian || FbigEndian) {
        rev_bytes(&d.dval);
    }
    return (d.dval);
}


static void
rev_bytes(d)

double *d;
{
    union {double d; char c[8];} bf;
    char *c;

    bf.d = *d;
    c = (char*)d;
    c[0] = bf.c[7];
    c[1] = bf.c[6];
    c[2] = bf.c[5];
    c[3] = bf.c[4];
    c[4] = bf.c[3];
    c[5] = bf.c[2];
    c[6] = bf.c[1];
    c[7] = bf.c[0];
}


static unsigned int
getbits(x,p,n)        /* get n bits from position p    */

unsigned int x;
int p, n;

    { return((x >> (p + 1 - n)) & ~(~0L << n)); }

#endif

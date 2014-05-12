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
 * CIF file generator.
 * Here's an example.
 * The following code
 * {
 *     FileDesc = fopen("CIFFile","w");
 *     GenBeginSymbol(FileDesc,3,1,1);
 *     GenComment(FileDesc,"This is easy.");
 *     GenBox(FileDesc,1,2,3,4,1,0);
 *     GenEndSymbol(FileDesc);
 *     GenEnd(FileDesc);
 *     fclose(FileDesc; 
 * }
 * 
 * generates
 * 
 * DS 3 1 1;
 * (This is easy.);
 * B 1 2 3 4; 
 * DF;
 * E
 * 
 */

#include "prefix.h"
#include "cd.h"

#if __STDC__
static void out_path(FILE*,struct p*,char*,int,int);
#else
static void out_path();
#endif


void
GenEnd(FileDesc)

FILE *FileDesc;
{
    fprintf(FileDesc,"E\n");
}


void
GenBeginSymbol(FileDesc,SymbolNum,A,B)

FILE *FileDesc;
int SymbolNum;
int A,B;
{
    fprintf(FileDesc,"DS %d %d %d;\n",SymbolNum,A,B);
}


void
GenEndSymbol(FileDesc)

FILE *FileDesc;
{
    fprintf(FileDesc,"DF;\n");
}


void
GenBeginCall(FileDesc,Number)

FILE *FileDesc;
int Number;
{
    fprintf(FileDesc,"C %d",Number);
}


void
GenEndCall(FileDesc)

FILE *FileDesc;
{
    fprintf(FileDesc,";\n");
}


void
GenTranslation(FileDesc,X,Y)

FILE *FileDesc;
int X,Y;
{
    fprintf(FileDesc," T %d %d",X,Y);
}


void
GenRotation(FileDesc,X,Y)

FILE *FileDesc;
int X,Y;
{
    fprintf(FileDesc," R %d %d",X,Y);
}


void
GenMirrorX(FileDesc)

FILE *FileDesc;
{
    fprintf(FileDesc," MX");
}


void
GenMirrorY(FileDesc)

FILE *FileDesc;
{
    fprintf(FileDesc," MY");
}


void
GenPolygon(FileDesc,Path)

FILE *FileDesc;
struct p *Path;
{
    char buf[80];

    *buf = 'P';
    *(buf+1) = '\0';
    out_path(FileDesc,Path,buf,0L,0L);
}


void
GenPolygonOffset(FileDesc,Path,X,Y)

FILE *FileDesc;
struct p *Path;
int X,Y;
{
    char buf[80];

    *buf = 'P';
    *(buf+1) = '\0';
    out_path(FileDesc,Path,buf,X,Y);
}


void
GenWire(FileDesc,Width,Path)

FILE *FileDesc;
int Width;
struct p *Path;
{
    char buf[80];

    sprintf(buf,"W %d",Width);
    out_path(FileDesc,Path,buf,0L,0L);
}


void
GenWireOffset(FileDesc,Width,Path,X,Y)

FILE *FileDesc;
struct p *Path;
int Width,X,Y;
{
    char buf[80];

    sprintf(buf,"W %d",Width);
    out_path(FileDesc,Path,buf,X,Y);
}


static void
out_path(FileDesc,Path,buf,X,Y)

FILE *FileDesc;
struct p *Path;
char *buf;
int X,Y;
{
    struct p *Pair;
    char buf1[80];
    int len, len1;

    len = strlen(buf);
    Pair = Path;
    while(Pair != NULL) {
        sprintf(buf1," %d %d",Pair->pX-X,Pair->pY-Y);
        len1 = strlen(buf1);
        if (len+len1 < 79) {
            strcat(buf,buf1);
            len += len1;
        }
        else {
            fprintf(FileDesc,"%s\n ",buf);
            strcpy(buf,buf1);
            len = len1+1;
        }
        Pair = Pair->pSucc;
    }
    fprintf(FileDesc,"%s;\n",buf);
}


void
GenBox(FileDesc,Length,Width,X,Y,XDir,YDir)

FILE *FileDesc;
int Length;
int Width,X,Y;
int  XDir,YDir;
{
    fprintf(FileDesc,"B %d %d %d %d",Length,Width,X,Y);
    if (XDir != 1 Or YDir != 0)
        fprintf(FileDesc," %d %d",XDir,YDir);
    fprintf(FileDesc,";\n");
}

/*
 * GenRoundFlash(FileDesc,Width,X,Y)
 *     FILE *FileDesc;
 *     int Width,X,Y;
 *     {
 *     fprintf(FileDesc,"R %d %d %d;\n",Width,X,Y);
 *     }
 */


void
GenLayer(FileDesc,Technology,Mask)

FILE *FileDesc;
char Technology,Mask[];
{
    if (Technology > ' ') {
        fprintf(FileDesc,"L %c",Technology);
        if (Mask[0] > ' ') {
            fprintf(FileDesc,"%c",Mask[0]);
            if (Mask[1] > ' ') {
                fprintf(FileDesc,"%c",Mask[1]);
                if (Mask[2] > ' ')
                    fprintf(FileDesc,"%c",Mask[2]);
            } 
        }
        fprintf(FileDesc,";\n");
    }
}


void
GenUserExtension(FileDesc,Digit,Text)

FILE *FileDesc;
char Digit;
char *Text;
{
    if (Text[0] >= '0' And Text[0] <= '9')
        fprintf(FileDesc,"%c%s;\n",Digit,Text);
    else
        fprintf(FileDesc,"%c %s;\n",Digit,Text);
}


void
GenComment(FileDesc,Text)

FILE *FileDesc;
char *Text;
{
    fprintf(FileDesc,"(%s);\n",Text);
}

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

#include <stdio.h>
#include <ctype.h>
#include "prefix.h"
#include "cd.h"


int  argc;
char **argv;

#if __STDC__
extern void help(void);
extern void blather(void);
extern char *nextarg(void);
extern char get_cif_file_type(FILE*);
#else
extern void help();
extern void blather();
extern char *nextarg();
extern char get_cif_file_type();
#endif


int
main(ac, av)

int ac;
char *av[];
{
    float Float1 = 1.0;
    char Program = '\0';
    char CIFFile[81], *Tmp;
    FILE *fp;

    argc = ac;
    argv = av;
    InitGlobal();
    CIFFile[0] = '\0';
    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {

            case 'L':
                sscanf(nextarg(), "%f", &Float1);
                break;
    
            case 'a':
            case 'b':
            case 'h':
            case 'i':
            case 'k':
            case 'n':
            case 'q':
            case 's':
                Program = argv[1][1];
                break;
    
            default:
                help();
        }
        argc--;
        argv++;
    }
    if (argc > 1)
        strcpy(CIFFile,argv[1]);
    else {
        printf("CIF file's name? (hit return for help) ");
        Tmp = malloc(81);
        *Tmp = '\0';
        fgets(Tmp,81,stdin);
        if (sscanf(Tmp,"%s",CIFFile) != 1)
            help();
        free(Tmp);
    }
    if (Program == '\0') {
        fp = fopen(CIFFile,"r");
        if (fp == NULL) {
            printf("Error: can't read CIF input file %s\n",CIFFile);
            exit(1);
        }
        Program = get_cif_file_type(fp);
        fclose(fp);
    }
    printf("CIF file type: %c\n",Program);
    printf("Microns per lambda: %15.8e\n",Float1);

    CDInit();
    CDPath(".");
    if (Not CDTo(CIFFile,"Root",(int)(Float1*RESOLUTION),RESOLUTION,Program))
        printf("Translation of %s failed.\n%s\n",CIFFile,CDStatusString);
    else
        printf("Translation of %s succeeded.\n",CIFFile);
    return (0);
}


void
help()
{
    printf("\nciftokic-%s\n\n",VersionString);
    printf("Usage: ciftokic [options] [cif_file]\n\n");
    printf("options:\n");
    printf("  -Lmicprl    micron per lambda (default 1.0)\n");
    printf("  -prefix     (cif dialect) where prefix =\n");
    blather();
    printf("\n");
    exit(0);
}


void
blather()
{
    printf("k Generated from KIC\n");
    printf(
"a Stanford: A Stanford symbol name follows a DS command as in (PadIn);\n");
    printf(
"b NCA:      An NCA symbol name follows a DS command as in (PadIn);\n");
    printf(
"h IGS:      A KIC or IGS symbol name follows a DS command as in 9 PadIn;\n"); 
    printf(
"i Icarus:   An Icarus symbol name follows a DS command as in (9 PadIn);\n");
    printf(
"q Squid:    A Squid symbol name follows a DS command as in 9 /usr/joe/PadIn;\n");
    printf(
"s Sif:      A Sif symbol name follows a DS command as in (Name: PadIn);\n"); 
    printf(
"n none of the above\n");
}


char
*nextarg()

{
    if (argv[1][2] != '\0')
        return &argv[1][2];
    argv++;
    argc--;
    return argv[1];
}


void
MallocFailed()

{
    if (CDStatusInt != CDMALLOCFAILED) return;
    fprintf(stderr,"OUT OF MEMORY.  This is a fatal error!\n");
    exit(1);
}


/* for callbacks in cd, not used */

void UpdateProperties() {}

FILE *OpenDevice() {return NULL;}

/* ARGSUSED */
void CDLabelBB(p,a,b,c,d)
struct o *p;
int *a, *b, *c, *d;
{}


char
get_cif_file_type(cfile)

/* Return the code for the structure name.  Skip to the first DS command,
 * and look at the following line.
 *
 * a Stanford: A Stanford symbol name follows a DS command as in (PadIn);
 * b NCA:      An NCA symbol name follows a DS command as in (PadIn);
 * h IGS:      A KIC or IGS symbol name follows a DS command as in 9 PadIn;
 * k KIC:      A KIC or IGS symbol name follows a DS command as in 9 PadIn;
 * i Icarus:   An Icarus symbol name follows a DS command as in (9 PadIn);
 * q Squid:    A Squid symbol name follows a DS command as in 9 /usr/joe/PadIn;
 * s Sif:      A Sif symbol name follows a DS command as in (Name: PadIn);
 * n none of the above
 */

FILE *cfile;
{
    int c;

    if (cfile == NULL)
        return ('n');

    while ((c = getc(cfile)) != EOF) {
        if (isspace(c)) continue;
        if (c != 'D') {
            while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }
        if ((c = getc(cfile)) == EOF)
            return ('n');
        if (c != 'S') {
            while (((c = getc(cfile)) != EOF) && (c != ';')) ;
            continue;
        }

        /* found a DS command, skip to ; */
        while (((c = getc(cfile)) != EOF) && (c != ';')) ;

        if (c == EOF)
            return ('n');

        while ((c = getc(cfile)) != EOF) {
            if (isspace(c)) continue;

            if (c == '(') {
                /* a comment line */
                while ((c = getc(cfile)) != EOF)
                    if (isspace(c)) continue;
                if (c == EOF)
                    return ('n');
                if (c == '9')
                    /* Icarus */
                    return ('i');
                while ((c = getc(cfile)) != EOF) {
                    if (isspace(c)) continue;
                    if (c == ':')
                        /* Sif */
                        return ('s');
                    if (c == ';')
                        /* Stanford/NCA */
                        return ('a');
                }
                return ('n');
            }
            else if (c == '9') {
                /* user extension line */
                while ((c = getc(cfile)) != EOF) {
                    if (isspace(c)) continue;
                    if (c == '/')
                        /* Squid */
                        return ('q');
                    if (c == ';')
                        /* IGS/KIC */
                        return ('k');
                }
                return ('n');
            }
            else
                return ('n');
        }
    }
    return ('n');
}

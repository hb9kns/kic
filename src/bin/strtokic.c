/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1983 Giles C. Billingsley
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
 * strmtokic.c
 *
 * This program was extensively modified by S.R. Whiteley 2/20/91.
 * Notes:
 * 1. Convert to rectangles is off by default. The -P option turns
 *    conversion on.  Originally, this logic was reversed.  However,
 *    four sided manhattan polygons are always converted to rectangles.
 * 2. Arrays are created in-place rather than as new symbols.  The
 *    Array user extension stores the unrotated center to center
 *    spacing in the DX and DY fields, which  is not compatible with
 *    the original KIC format.
 */

#include "prefix.h"
#include "stream.h"
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Define to 1 to convert symbol names to lower case.  This was true in
 * previous versions of Kic.
 */
#define TOLWR 0

/*
 * We use the property list of KIC symbols to save the library information;
 * The value of the property is the numeric value of the STREAM record
 * type offset by 7000 (e.g. 7000 is the KIC property value describing the
 * STREAM version number, 7002 is the KIC property value describing the
 * STREAM library name, etc.)  The offset of 7000 was arbitrarily selected,
 * and care must be taken so that this value does not conflict with any
 * other convention.  The PROPERTYOFFSET define is for convenience.
 *
 * The STREAM-specific property list is attributed to every KIC symbol,
 * and 'kictostrm' will look for this information.
 */

struct headerlist{
    int hd_RecordType;
    char hd_Text[512];
    struct headerlist *hd_Succ;
};

typedef struct headerlist HEADLIST;

int IbigEndian;
int FbigEndian;

#define TMPFILE    "yyXXXXXX"

int     ByteSwap;
int     ConvertToRectangles;
int     NumLayerTable;
int     LayerNumbers[256];
int     CurrentLayer;
int     CurrentAttribute;
int     DataTypeNumbers[256];
int     CurrentDataType;
int     NumSymbols;
int     UseLayerTable;
int     struct_dates[12];
int     CurrentSize;
int     Root_flag;
int     CurrentOffset;
int     XYbuf[MAXSTRMCOORDS];
FILE    *stderror;
FILE    *SymDesc;
char    *SymbolNames[MAXSYMBOLS];
char    *LayerNames[256];
char    CurrentSymbol[45];
char    RootSymbol[45];

double  ScaleFactor;
FILE    *STREAMFILE;

static int NumArefs;
static int RootSymbolNumber = -1;


#if __STDC__
extern void help(void);
extern int  fptest(int*,int*);
extern void symdef(char*,HEADLIST*);
extern void s_bndry(char*);
extern void s_path(char*);
extern void s_sref(char*);
extern void s_aref(char*);
extern void s_text(char*);
extern char *nextarg(void);
extern int  get_record(char*);
extern int  struct_index(char*);
extern void PrintLayer(int,int);
extern FILE *open_symbol(char*);
extern void new_symbol(char*);
extern void set_property_value(char*,int);
extern int  path_to_rect(int,int*);
extern void set_path(char*,int,int*);
extern void set_instance(int,int,int,int,double);
extern void set_array(int,int,int,int,double,int*);
extern void read_layer_table(FILE*);
extern void read_tech_layers(FILE*);
extern int  set_angle(double,int*,int*);
extern int  strm_ival(char*);
extern int  strm_longval(char*);
extern double strm_doubleval(char*);
extern void err_fatal(char*,char*);
extern void err_fatal_1(char*,char*);
extern void err_fatal_2(char*,int);
extern void err_warn_1(char*,char*,char*);
extern void err_warn_2(char*,int,char*);
extern char *alias(char*);
extern void dumpalias(void);
extern void convert_pathtype(int*,int*,int,int,int);
extern RECT *pgtorex(PATHLIST*);
extern int lowx(const void*,const void*);
extern int lowy(const void*,const void*);
extern int orient(PATHLIST**,int,int*);
extern int cross(PATHLIST*,int,int,int);
extern RECT* makerect(int,int,int,int,RECT*);
extern void freepath(PATHLIST**);
extern char *tmalloc(unsigned);
static void file_open(char*);
static char *get_token(FILE*);
#else
extern void help();
extern int  fptest();
extern void symdef();
extern void s_bndry();
extern void s_path();
extern void s_sref();
extern void s_aref();
extern void s_text();
extern char *nextarg();
extern int  get_record();
extern int  struct_index();
extern void PrintLayer();
extern FILE *open_symbol();
extern void new_symbol();
extern void set_property_value();
extern int  path_to_rect();
extern void set_path();
extern void set_instance();
extern void set_array();
extern void read_layer_table();
extern void read_tech_layers();
extern int  set_angle();
extern int  strm_ival();
extern int  strm_longval();
extern double strm_doubleval();
extern void err_fatal();
extern void err_fatal_1();
extern void err_fatal_2();
extern void err_warn_1();
extern void err_warn_2();
extern char *alias();
extern void dumpalias();
extern void convert_pathtype();
extern RECT *pgtorex();
extern int lowx();
extern int lowy();
extern int orient();
extern int cross();
extern RECT* makerect();
extern void freepath();
extern char *tmalloc();
static void file_open();
static char *get_token();
#endif

#define    LONGSCALE(n)  (int)(floor(ScaleFactor * ((double)(n)) + 0.5))
#define    NEWPATH       (PATHLIST *) tmalloc((unsigned) sizeof(PATHLIST))
#define    NEWHEAD       (HEADLIST *) tmalloc((unsigned) sizeof(HEADLIST))

char **argv;
int  argc;


int
main(ac, av)

int ac;
char *av[];
{

    double a,b;
    double micprl;
    FILE *RootDesc;
    HEADLIST *HeaderCopy;
    HEADLIST *SymbolProps;
    char *cp;
    int StandardError;
    int i;
    int type;
    int UseStreamNames = 0;
    char cbuf[MAXRECSIZE + 4];
    char RootFileName[81];
    char StreamFile[81];
    char LayerFile[81];
    char *Tmp, *Tech;

    /*  STREAMTOKIC
     *
     *  Giles Billingsley    3/30/83
     *
     *  options:
     *      -P         convert manhattan polygons to boxes
     *      -E         print errors in file "strtokic.err"
     *      -Csname    sname = Root structure name
     *      -Rfilename filename = name of root cell
     *      -Xfilename filename = cif layer reference file
     *      -N         use stream names
     *      -Lmicprl   micron per lambda
     */

    if (fptest(&IbigEndian,&FbigEndian)) {
        fprintf(stderr,"Error: incompatible floating point format.\n");
        exit(1);
    }
    InitGlobal();
#ifdef MSDOS
    readalias();
#endif
    stderror         = stderr;
    RootSymbol[0]    = '\0';
    RootFileName[0]  = '\0';
    LayerFile[0]     = '\0';
    SymbolProps      = NULL;
    Root_flag        = 0;
    UseLayerTable    = 0;
    StandardError    = 0;
    ScaleFactor      = 1.0;
    micprl           = 1.0;

    argc = ac;
    argv = av;

    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {

        case 'C':
        case 'c':
            strcpy(RootSymbol,nextarg());
            Root_flag = 1;
            /* convert to upper case */
            i = 0;
            while (RootSymbol[i] != '\0') {
                if (RootSymbol[i] >= 'a' && RootSymbol[i] <= 'z')
                    RootSymbol[i] -= 32;
                i++;
            }
            break;

        case 'R':
        case 'r':
            strcpy(RootFileName,nextarg());
            break;

        case 'L':
        case 'l':
            if (sscanf(nextarg(),"%lg",&micprl) != 1)
                err_fatal("Incorrect microns per lambda",*argv);
            break;

        case 'E':
        case 'e':
            StandardError = 1;
            break;

        case 'X':
        case 'x':
            strcpy(LayerFile,nextarg());
            break;

        case 'P':
        case 'p':
            ConvertToRectangles = 1;
            break;

        case 'N':
        case 'n':
            UseStreamNames = 1;
            break;

        case 'T':
        case 't':
            Tech = nextarg();
#ifdef MSDOS
            if (strlen(Tech) > 3)
                Tech[3] = '\0';
#endif
            TECH_EXT = malloc(strlen(Tech) + 1);
            if (TECH_EXT == NULL) {
                (void)fprintf(stderr,
                    "Memory allocation failure on startup.\n");
                exit(1);
            }
            strcpy(TECH_EXT,Tech);
            break;

        default:
            help();
        }
        argc--;
        argv++;
    }

    if (argc > 1) {
        if ((STREAMFILE = fopen((cp = argv[1]),"rb")) == NULL) {
            err_fatal_1("Can't open stream file",cp);
        }
        printf("Opening Stream file: %s\n",cp);
    }
    else {
        printf("Stream file to convert? (hit return for help) ");
        Tmp = malloc(81);
        *Tmp = '\0';
        fgets(Tmp,81,stdin);
        if (sscanf(Tmp,"%s",StreamFile) != 1)
            help();
        free(Tmp);
        if ((STREAMFILE = fopen(StreamFile,"rb")) == NULL)
            err_fatal_1("Can't open stream file",StreamFile);
    }

    if (!UseStreamNames) {
        if (LayerFile[0]) {

            file_open(LayerFile);
            UseLayerTable = 1;
        }
        else {
            FILE *fp = OpenTechFile();
            if (fp) {
                printf("Using tech file %s\n",TECHFILE);
                read_tech_layers(fp);
                sprintf(LayerFile,"%s",TECHFILE);
                UseLayerTable = 1;
                fclose(fp);
            }
            else {
                sprintf(LayerFile,"%s%c%s",DEFAULTLTAB,DIRC,"ltab");
                if ((fp = fopen(LayerFile,"r")) != NULL) {
                    printf("Using default layer table %s\n",LayerFile);
                    read_layer_table(fp);
                    UseLayerTable = 1;
                    fclose(fp);
                }
            }
            if (!UseLayerTable) {
                err_fatal("Layer table not found,","(use -n option?)");
            }
            if (!NumLayerTable) {
                err_fatal("Layer table has no entries,","(use -n option?)");
            }
        }
    }
    else {
        printf("Using stream input layer data for layer names\n");
    }


    if (StandardError) {
        if ((stderror = fopen(cp = "strtokic.err","w")) == NULL)
            err_fatal_1("Can't open errors file",cp);
    }
    else
        stderror = stderr;

    if (RootFileName[0] == '\0')
        strcpy(RootFileName,"Root");
    if ((RootDesc = fopen(RootFileName,"w")) == NULL)
        err_fatal_1("Can't open root file file",RootFileName);

    printf("Root file name     : %s\n",RootFileName);
    if (Root_flag)
        printf("Root Symbol        : %s\n",RootSymbol);
    printf("Microns Per Lambda : %f\n",micprl);
    if (ConvertToRectangles)
        printf("Will convert manhattan polygons to rectangles.\n");
    else
        printf("Will NOT convert manhattan polygons to rectangles.\n");

    NumSymbols = 0;

    /* byte swap test */
    if ((i = getc(STREAMFILE)) != 0) {
        printf("Swapping bytes of input file.\n");
        ByteSwap = 1;
    }
    ungetc(i,STREAMFILE);

    /* build the symbol table */
    printf("Creating symbol table...  ");
    while ((type = get_record(cbuf)) != 4)
        if (type == 6) {
            if (TOLWR)
                to_lower_case(cbuf);
            new_symbol(cbuf);
        }
    printf("done.\n");
    rewind(STREAMFILE);

    /* loop through the records and exit on ENDLIB */
    while ((type = get_record(cbuf))!= ENDLIB) {
        switch(type) {

            case HEADER:
                HeaderCopy = NEWHEAD;
                HeaderCopy->hd_RecordType = HEADER + PROPERTYOFFSET;
                HeaderCopy->hd_Succ = SymbolProps;
                SymbolProps = HeaderCopy;
                sprintf(HeaderCopy->hd_Text,"%d",strm_ival(cbuf));
                fprintf(RootDesc,"( VERSION %d );\n",strm_ival(cbuf));
                fprintf(RootDesc,"5 %d %d;\n",PROPERTYOFFSET + HEADER,
                    strm_ival(cbuf));
                break;

            case BGNLIB:
                fprintf(RootDesc,"( MOD DATE ");
                for (i = 0; i < 12; i += 2)
                    fprintf(RootDesc,"%d ",strm_ival(cbuf+i));
                fprintf(RootDesc,": ACCESS DATE ");
                for (i = 12; i < 24; i += 2)
                    fprintf(RootDesc,"%d ",strm_ival(cbuf+i));
                fprintf(RootDesc,");\n");
                break;

            case LIBNAME:
                HeaderCopy = NEWHEAD;
                HeaderCopy->hd_RecordType = LIBNAME + PROPERTYOFFSET;
                HeaderCopy->hd_Succ = SymbolProps;
                SymbolProps = HeaderCopy;
                strcpy(HeaderCopy->hd_Text,cbuf);
                fprintf(RootDesc,"( LIBNAME %s );\n",cbuf);
                fprintf(RootDesc,"5 %d %s;\n",PROPERTYOFFSET + LIBNAME,cbuf);
                break;

            case UNITS:
                a = strm_doubleval(cbuf);
                b = strm_doubleval(cbuf+8);
                fprintf(RootDesc,"( SCALE: %e UNITS/DBU, %e METERS/DBU );\n",
                    a,b);
                fprintf(RootDesc,"( MICRONS PER LAMBDA = %f );\n",micprl);

                /*
                 * The length of the database unit is 1e-8 meters.
                 *
                 *   meters     .01 microns       lambdas         lambdas
                 *   ------  *  -----------  *  -----------   =   -------
                 *    DBU          meter        .01 microns         DBU
                 */

                ScaleFactor = (1e8 * b)/(micprl);
                break;

            case BGNSTR:
                for (i = 0; i < 12; i++)
                    struct_dates[i] = strm_ival(cbuf + (i << 1));
                break;

            case STRNAME:
                symdef(cbuf,SymbolProps);
                fprintf(SymDesc,"DF;\n");
                fprintf(SymDesc,"E\n");
                fclose(SymDesc);
                break;

            case REFLIBS:
                if (cbuf[0] != '\0')
                    fprintf(RootDesc,"( REFLIB1 %s );\n",cbuf);
                if (cbuf[44] != '\0')
                    fprintf(RootDesc,"( REFLIB2 %s );\n",cbuf+44);
                HeaderCopy = NEWHEAD;
                HeaderCopy->hd_RecordType = REFLIBS + PROPERTYOFFSET;
                HeaderCopy->hd_Succ = SymbolProps;
                SymbolProps = HeaderCopy;
                sprintf(HeaderCopy->hd_Text,"%s %s",cbuf,cbuf+44);
                fprintf(RootDesc,"5 %d %s %s;\n",
                    PROPERTYOFFSET + REFLIBS,cbuf,cbuf+44);
                break;

            case FONTS:
                if (cbuf[0] != '\0')
                    fprintf(RootDesc,"( FONT1 %s );\n",cbuf);
                if (cbuf[44] != '\0')
                    fprintf(RootDesc,"( FONT2 %s );\n",cbuf+44);
                if (cbuf[88] != '\0')
                    fprintf(RootDesc,"( FONT3 %s );\n",cbuf+88);
                if (cbuf[132] != '\0')
                    fprintf(RootDesc,"( FONT4 %s );\n",cbuf+132);
                HeaderCopy = NEWHEAD;
                HeaderCopy->hd_RecordType = FONTS + PROPERTYOFFSET;
                HeaderCopy->hd_Succ = SymbolProps;
                SymbolProps = HeaderCopy;
                sprintf(HeaderCopy->hd_Text,"%s %s %s %s",cbuf,
                    cbuf+44,cbuf+88,cbuf+132);
                fprintf(RootDesc,"5 %d %s %s %s %s;\n",
                    PROPERTYOFFSET + FONTS,cbuf,cbuf+44,cbuf+88,cbuf+132);
                break;

            case GENERATIONS:
                HeaderCopy = NEWHEAD;
                HeaderCopy->hd_RecordType = GENERATIONS + PROPERTYOFFSET;
                HeaderCopy->hd_Succ = SymbolProps;
                SymbolProps = HeaderCopy;
                sprintf(HeaderCopy->hd_Text,"%d",strm_ival(cbuf));
                fprintf(RootDesc,"( GENERATIONS %d );\n",strm_ival(cbuf));
                fprintf(RootDesc,"5 %d %d;\n",PROPERTYOFFSET + GENERATIONS,
                strm_ival(cbuf));
                break;

            case ATTRTABLE:
                HeaderCopy = NEWHEAD;
                HeaderCopy->hd_RecordType = ATTRTABLE + PROPERTYOFFSET;
                HeaderCopy->hd_Succ = SymbolProps;
                SymbolProps = HeaderCopy;
                strcpy(HeaderCopy->hd_Text,cbuf);
                if (cbuf[0] != '\0')
                    fprintf(RootDesc,"( ATTRIBUTE TABLE %s );\n",cbuf);
                fprintf(RootDesc,"5 %d %s;\n",PROPERTYOFFSET + ATTRTABLE,cbuf);
                break;

            default:
                /*
                err_fatal_2("Illegal record type",type);
                */
                fprintf(stderr,"Warning: unknown record type %d\n",type);

        }
    }

    fprintf(RootDesc,"9 %s;\n",RootFileName);
    fprintf(RootDesc,"DS 0 1 1;\n");
    if (Root_flag) {
        if (RootSymbolNumber < 0) {
            fprintf(RootDesc,"( Root structure %s not found. );\n",RootSymbol);
            fprintf(RootDesc,"9 %s;\n",SymbolNames[0]);
            fprintf(RootDesc,"C 1;\n");
            fprintf(stderror,"Warning: Root structure not found.\n");
        }
        else {
            fprintf(RootDesc,"9 %s;\n",SymbolNames[RootSymbolNumber-1]);
            fprintf(RootDesc,"C %d;\n",RootSymbolNumber);
        }
    }
    else {
        if (NumSymbols > 0) {
            fprintf(RootDesc,"9 %s;\n",SymbolNames[0]);
            fprintf(RootDesc,"C 1;\n");
        }
    }
    fprintf(RootDesc,"(** STREAM-KIC SYMBOL TABLE **);\n");
    for (i = 0; i < NumSymbols; ++i)
    fprintf(RootDesc,"(  %-16s = %d  );\n",SymbolNames[i],i+1);
    fprintf(RootDesc,"DF;\n");
    fprintf(RootDesc,"E\n");
    fclose(RootDesc);

#ifdef MSDOS
    dumpalias();
#endif
    return (0);

}


void
help()
{
    printf("\nstrtokic-%s\n\n",VersionString);
    printf("Usage: strtokic [options] [streamfile]\n\n");
    printf("options (case insensitive):\n");
    printf("  -P          convert manhattan polygons to boxes,\n");
    printf("              four-sided polygons are always converted\n");
    printf("  -E          print errors in file \"strmtokic.err\" (default stderr)\n");
    printf("  -Csname     sname = Root structure name (default, convert everything)\n");
    printf("  -Rfilename  filename = name of root cell (default \"Root\")\n");
    printf("  -Xfilename  filename = layer table reference file\n");
    printf("  -N          use stream layer numbers for layer names\n");
    printf("  -Text       use tech.ext for layers\n");
    printf("  -Lmicprl    micron per lambda (default 1.0)\n\n");
    printf("Default layer info is from tech file.\n\n");
    exit(0);
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


void
symdef(cbuf,SymbolProps)

char *cbuf;
HEADLIST *SymbolProps;
{
/*
 * BEGIN SYMBOL DEFINITION
 */
    int i;
    int index;
    int type;

    if (TOLWR)
        to_lower_case(cbuf);
    CurrentLayer = CurrentDataType = CurrentAttribute = -1;
    /* search for symbol number */
    index = struct_index(cbuf);
    strcpy(CurrentSymbol,cbuf);

#ifdef MSDOS
    { char *c;
    SymDesc = open_symbol(c = alias(cbuf));
    if (!strcmp(c,cbuf))
        printf("Converting: %s\n",cbuf);
    else
        printf("Converting: %-30s(new name: %s)\n",cbuf,c);}
#else
    SymDesc = open_symbol(cbuf);
    printf("Converting: %s\n",cbuf);
#endif
    /* test for RootSymbol */
    if (Root_flag) {
        if (strcmp(RootSymbol,cbuf) == 0)
            RootSymbolNumber = index;
    }
    /* add the symbol property list */
    while (SymbolProps != NULL) {
        fprintf(SymDesc,"5 %d %s;\n",SymbolProps->hd_RecordType,
        SymbolProps->hd_Text);
        SymbolProps = SymbolProps->hd_Succ;
    }
    fprintf(SymDesc,"9 %s;\n",cbuf);
    fprintf(SymDesc,"DS %d 1 1;\n",index);
    fprintf(SymDesc,"( CREATION DATE ");
    for (i = 0; i < 6; ++i)
        fprintf(SymDesc,"%d ",struct_dates[i]);
    fprintf(SymDesc,": MOD DATE ");
    for (i = 0; i < 6; ++i)
        fprintf(SymDesc,"%d ",struct_dates[i+6]);
    fprintf(SymDesc,");\n");
    /*
     * loop through records and exit on ENDSTR
     */
    while ((type = get_record(cbuf)) != ENDSTR) {
        switch(type) {

        case BOUNDARY:
            s_bndry(cbuf);
            break;

        case PATH:
            s_path(cbuf);
            break;

        case SREF:
            s_sref(cbuf);
            break;

        case AREF:
            s_aref(cbuf);
            break;

        case TEXT:
            s_text(cbuf);
            break;

        case SNAPNODE:
            /*
             * snapnodes are not used
             */
            while ((type = get_record(cbuf)) != 17) ;
            break;

        default:
            err_fatal_2("Illegal record type",type);
        }
    }
}


void
s_bndry(cbuf)

char *cbuf;
{

/*
 * loop through records and exit on ENDEL
 */

    int i;
    int type;
    int ncoords = 0;
    int dtype = CurrentDataType;
    int layer = CurrentLayer;

    while ((type = get_record(cbuf)) != ENDEL) {
        switch(type) {

        case LAYER:
            layer = strm_ival(cbuf);
            break;

        case DATATYPE:
            dtype = strm_ival(cbuf);
            break;

        case XY:
            ncoords = CurrentSize/4;
            for (i = 0; i < ncoords; ++i)
                XYbuf[i] = LONGSCALE( strm_longval(cbuf + (i << 2)) );
            ncoords /= 2;
            break;

        case PROPATTR:
            CurrentAttribute = strm_ival(cbuf);
            break;

        case PROPVALUE:
            set_property_value(cbuf,type);
            break;

        default:
            err_fatal_2("Illegal record type",type);
        }
    }
    PrintLayer(layer,dtype);
    if (ConvertToRectangles || ncoords == 5)
        if (path_to_rect(ncoords,XYbuf)) return;
    /* limit to 80 cols per line */
    *cbuf = 'P';
    *(cbuf+1) = '\0';
    set_path(cbuf,ncoords,XYbuf);
}


void
s_path(cbuf)

char *cbuf;
{

/*
 * loop through records and exit on ENDEL
 */
    int pathwidth = 0;
    int ptype = 0;
    int i;
    int type;
    int ncoords = 0;
    int dtype = CurrentDataType;
    int layer = CurrentLayer;

    while ((type = get_record(cbuf)) != ENDEL) {
        switch(type) {

        case LAYER:
            layer = strm_ival(cbuf);
            break;

        case DATATYPE:
            dtype = strm_ival(cbuf);
            break;

        case WIDTH:
            pathwidth = LONGSCALE( strm_longval(cbuf) );
            break;

        case XY:
            ncoords = CurrentSize/4;
            for (i = 0; i < ncoords; ++i)
                XYbuf[i] = LONGSCALE( strm_longval(cbuf + (i << 2)) );
            ncoords /= 2;
            break;

        case PATHTYPE:
            /*
             * STREAM pathtypes
             * 0 = square ended paths with ends that are flush
             *     with the endpoints
             * 1 = round ended (CIF like) paths
             * 2 = square ended paths with ends that are 
             *     offset from the endpoint by a half width.
             *
             * In KIC, we assume a pathtype of 2.
             */

            if ((ptype = strm_ival(cbuf)) != 2)
                fprintf(SymDesc,"( PATHTYPE %d CONVERTED TO PATHTYPE 2 );\n",ptype);
            break;

        case PROPATTR:
            CurrentAttribute = strm_ival(cbuf);
            break;

        case PROPVALUE:
            CurrentAttribute = -1;
            break;

        default:
            err_fatal_2("Illegal record type",type);
        }
    }
    PrintLayer(layer,dtype);
    if (ptype == 0 && ncoords > 1) {
        /*
         * subtract a half width from the endpoints.
         */
        convert_pathtype(&XYbuf[0],&XYbuf[1],XYbuf[2],XYbuf[3],pathwidth);
        i = ncoords+ncoords-2;
        convert_pathtype(&XYbuf[i],&XYbuf[i+1],XYbuf[i-2],XYbuf[i-1],pathwidth);
    }
    if (ptype != 2)
        fprintf(SymDesc,"5 %d PATHTYPE %d;\n",PROPERTYOFFSET + PATHTYPE,ptype);

    sprintf(cbuf,"W %d",pathwidth);
    set_path(cbuf,ncoords,XYbuf);
}


void
s_sref(cbuf)

char *cbuf;
{
/*
 * loop through records and exit on ENDEL
 */

    double magn = 1,rotn = 0;
    FILE *afile;
    int cx = 0,cy = 0;
    int lone;
    int reflection = 0;
    int j;
    int index = 0;
    int type;

    while ((type = get_record(cbuf)) != ENDEL) {
        switch(type) {
    
        case XY:
            cx = LONGSCALE( strm_longval(cbuf) );
            cy = LONGSCALE( strm_longval(cbuf+4) );
            break;

        case SNAME:
            if (TOLWR)
                to_lower_case(cbuf);
            index = struct_index(cbuf);
            break;

        case STRANS:
            if (cbuf[1] & 4)
                err_warn_2("Absolute magnification",1,CurrentSymbol);
            if (cbuf[1] & 2)
                err_warn_2("Absolute angle",1,CurrentSymbol);
            if (cbuf[0] & 128)
                reflection = 1;
            break;

        case MAG:
            magn = strm_doubleval(cbuf);
            break;

        case ANGLE:
            rotn = strm_doubleval(cbuf)/RADTODEG;
            break;
    
        case PROPATTR:
            CurrentAttribute = strm_ival(cbuf);
            break;

        case PROPVALUE:
            set_property_value(cbuf,type);
            break;

        default:
            err_fatal_2("Illegal record type",type);
        }
    }
    if (magn == 1.0)
        set_instance(index,reflection,cx,cy,rotn);

    else {
        err_warn_1("Magnification not unity",
            SymbolNames[index-1],CurrentSymbol);
        lone = (int)(100.0 * magn);
        /*
         * If magn is less than .01, then KIC does not
         * have the resolution.
         */
        if (lone > 0) {
            j = 100;
            while ( !(lone % 10) ) {
                j /= 10;
                lone /= 10;
            }
            ++NumArefs;
            new_symbol("AREF    ");
            sprintf(SymbolNames[NumSymbols-1]+4,"%d",NumArefs);
            afile = open_symbol(SymbolNames[NumSymbols-1]);
            fprintf(afile,"9 %s;\n",SymbolNames[NumSymbols-1]);
            fprintf(afile,"DS %d %d %d;\n",NumSymbols,lone,j);
            fprintf(afile,"9 %s;\n",SymbolNames[index-1]);
            fprintf(afile,"C %d;\n",index);
            fprintf(afile,"DF;\n");
            fprintf(afile,"E\n");
            fclose(afile);
            set_instance(NumSymbols,reflection,cx,cy,rotn);
        }
        else
            err_warn_1("Magnification too small",SymbolNames[index-1],
                CurrentSymbol);
        
    }
}


void
s_aref(cbuf)

char *cbuf;
{

/*
 * loop through records and exit on ENDEL
 */
    double magn = 1,rotn = 0;
    FILE *afile;
    int lone;
    int reflection = 0;
    int nx = 0,ny = 0;
    int i,j;
    int index = 0;
    int type;

    while ((type = get_record(cbuf)) != ENDEL) {
        switch(type) {

        case XY:
            for (i = 0; i < 6; ++i)
                XYbuf[i] = LONGSCALE( strm_longval(cbuf + (i << 2)) );

            break;

        case SNAME:
            if (TOLWR)
                to_lower_case(cbuf);
            index = struct_index(cbuf);
            break;

        case COLROW:
            nx = strm_ival(cbuf);
            ny = strm_ival(cbuf+2);
            break;

        case STRANS:
            if (cbuf[1] & 4)
                err_warn_2("Absolute magnification",1,CurrentSymbol);
            if (cbuf[1] & 2)
                err_warn_2("Absolute angle",1,CurrentSymbol);
            if (cbuf[0] & 128)
                reflection = 1;
            break;

        case MAG:
            magn = strm_doubleval(cbuf);
            break;

        case ANGLE:
            rotn = strm_doubleval(cbuf)/RADTODEG;
            break;

        case PROPATTR:
            CurrentAttribute = strm_ival(cbuf);
            break;

        case PROPVALUE:
            set_property_value(cbuf,type);
            break;

        default:
            err_fatal_2("Illegal record type",type);
        }
    }
    if (magn == 1.0)
        set_array(index,reflection,nx,ny,rotn,XYbuf);

    else {
        err_warn_1("Magnification not unity",
            SymbolNames[index-1],CurrentSymbol);
        lone = (int)(100.0 * magn);
        /*
         * If magn is less than .01, then KIC does not
         * have the resolution.
         */
        if (lone > 0) {
            j = 100;        
            while ( !(lone % 10) ) {
                j /= 10;
                lone /= 10;
            }
            ++NumArefs;
            new_symbol("AREF    ");
            sprintf(SymbolNames[NumSymbols-1]+4,"%d",NumArefs);
            afile = open_symbol(SymbolNames[NumSymbols-1]);
            fprintf(afile,"9 %s;\n",SymbolNames[NumSymbols-1]);
            fprintf(afile,"DS %d %d %d;\n",NumSymbols,lone,j);
            fprintf(afile,"9 %s;\n",SymbolNames[NumSymbols-2]);
            fprintf(afile,"C %d;\n",index);
            fprintf(afile,"DF;\n");
            fprintf(afile,"E\n");
            fclose(afile);
            set_array(NumSymbols,reflection,nx,ny,rotn,XYbuf);
        }
        else
            err_warn_1("Magnification too small",SymbolNames[index-1],
                CurrentSymbol);
    }
}


void
s_text(cbuf)

char *cbuf;
{
    double magn = 1,rotn = 0;
    char *cp, string[48];
    int cx = 0,cy = 0;
    int width = 0;
    int ptype = 0;
    int reflection = 0;
    int present = 0;
    int type;
    int dtype = CurrentDataType;
    int layer = CurrentLayer;

    while ((type = get_record(cbuf)) != ENDEL) {
        switch(type) {

        case LAYER:
            layer = strm_ival(cbuf);
            break;

        case WIDTH:
            width = LONGSCALE( strm_longval(cbuf) );
            break;

        case XY:
            cx = LONGSCALE( strm_longval(cbuf) );
            cy = LONGSCALE( strm_longval(cbuf+4) );
            break;

        case TEXTTYPE:
            dtype = strm_ival(cbuf);
            break;

        case PRESENTATION:
            present = strm_ival(cbuf);
            break;

        case STRING:
            cp = cbuf;
            while (*cp != '\0') {
                if (*cp == ' ')
                    *cp = '_';
                cp++;
            }
            strcpy(string,cbuf);
            break;

        case STRANS:
            if (cbuf[1] & 4)
                err_warn_2("Absolute magnification",1,CurrentSymbol);
            if (cbuf[1] & 2)
                err_warn_2("Absolute angle",1,CurrentSymbol);
            if (cbuf[0] & 128)
                reflection = 1;
            else
                reflection = 0;
            break;

        case MAG:
            magn = strm_doubleval(cbuf);
            break;

        case ANGLE:
            rotn = strm_doubleval(cbuf);
            break;

        case PATHTYPE:
            /*
             * STREAM pathtypes
             * 0 = square ended paths with ends that are flush
             *     with the endpoints
             * 1 = round ended (CIF like) paths
             * 2 = square ended paths with ends that are 
             *     offset from the endpoint by a half width.
             *
             * In KIC, we assume a pathtype of 2.
             */
            ptype = strm_ival(cbuf);
            break;

        case PROPATTR:
            CurrentAttribute = strm_ival(cbuf);
            break;

        case PROPVALUE:
            set_property_value(cbuf,type);
            break;

        default:
            err_fatal_2("Illegal record type",type);
        }
    }
    PrintLayer(layer,dtype);
    fprintf(SymDesc,
        "5 %d WIDTH %d PRESENT %d PTYPE %d MAG %f ANGLE %f REFLECT %d;\n",
        PROPERTYOFFSET + TEXT,width,present,ptype,magn,rotn,reflection);
    fprintf(SymDesc,"94 %s %d %d;\n",string,cx,cy);
}


char *
nextarg()
{
    if (argv[1][2] != '\0')
        return(&argv[1][2]);
    argv++;
    argc--;
    return(argv[1]);
}


int
get_record(cbuf)

char *cbuf;
{
    int i;
    int size;
    unsigned byte0;
    unsigned byte1;
    unsigned dtype;
    unsigned rtype;

    CurrentOffset = ftell(STREAMFILE);
    if (!ByteSwap) {
        byte0 = getc(STREAMFILE);
        byte1 = getc(STREAMFILE);
        rtype = getc(STREAMFILE);
        dtype = getc(STREAMFILE);
    }
    else {
        byte1 = getc(STREAMFILE);
        byte0 = getc(STREAMFILE);
        dtype = getc(STREAMFILE);
        rtype = getc(STREAMFILE);
    }
    (void)dtype;
    size  = byte0*256 + byte1 - 4;
    if (size < 0) size = 0;

    if (size > MAXRECSIZE)
        err_fatal_2("Oversized record, size",size);
    CurrentSize = size;
    size >>= 1;

    if (size && fread(cbuf,(size_t)size,(size_t)2,STREAMFILE) != 2)
        err_fatal_1("Read error on input","");

    size <<= 1;
    if (CurrentSize != size) {
        cbuf[size] = getc(STREAMFILE);
        cbuf[size+1] = getc(STREAMFILE);
    }
    if (ByteSwap) {
        for (i = 0; i < size; i += 2) {
            byte0 = cbuf[i];
            cbuf[i] = cbuf[i+1];
            cbuf[i+1] = byte0;
        }
        if (CurrentSize != size)
            cbuf[size] = cbuf[size+1];
    }
    if (CurrentSize != size)
        ungetc(cbuf[size+1],STREAMFILE);
    cbuf[CurrentSize] = '\0';
    return rtype;
}


int
struct_index(cbuf)

char *cbuf;
{
    int i;

    /* the symbol table should be complete at this point */
    for (i = 0; i < NumSymbols; ++i) {
#ifdef MSDOS
        if (strcmp(SymbolNames[i],alias(cbuf)) == 0)
#else
        if (strcmp(SymbolNames[i],cbuf) == 0)
#endif
            return(i+1);
    }
    fprintf(stderror,"Warning: Undefined symbol %s.\n",cbuf);
    new_symbol(cbuf);
    return(NumSymbols);
}


void
PrintLayer(layer,datatype)

int layer;
int datatype;
{
    int l;
    char buf[128];

    if (layer < 0 || datatype < 0 || layer > 255 || datatype > 255) {
        sprintf(buf,"Illegal layer %d datatype",layer);
        err_fatal_2(buf,datatype);
    }
    if (layer == CurrentLayer && datatype == CurrentDataType) return;
    if (UseLayerTable) {
        for (l = 0; l < NumLayerTable; ++l) {
            if (LayerNumbers[l] == layer &&
                    (DataTypeNumbers[l] < 0 ||
                    DataTypeNumbers[l] == datatype)) {

                if (layer == CurrentLayer && DataTypeNumbers[l] < 0)
                    return;
                fprintf(SymDesc,"L %s;\n",LayerNames[l]);
                break;
            }
            else if (l == NumLayerTable-1) {
                sprintf(buf,"Undefined layer %d datatype",layer);
                err_warn_2(buf,datatype,CurrentSymbol);
                fprintf(SymDesc,"L %02d\n",layer);
                return;
            }
        }
    }
    else {
        if (layer < 10)
            fprintf(SymDesc,"L 0%d",layer);
        else
            fprintf(SymDesc,"L %d",layer);
        if (datatype < 10)
            fprintf(SymDesc,"0%d;\n",datatype);
        else
            fprintf(SymDesc,"%d;\n",datatype);
    }
    CurrentLayer = layer;
    CurrentDataType = datatype;
}


FILE *
open_symbol(name)

char *name;
{
    FILE *fp;

    if ((fp = fopen(name,"w")) == 0)
        err_fatal_1("Can't open symbol",name);
    return fp;
}


void
new_symbol(name)

char *name;
{
#ifdef MSDOS
    name = alias(name);
#endif
    if (NumSymbols >= MAXSYMBOLS)
        err_fatal_2("Exceeded maximum symbol count of",MAXSYMBOLS);
    SymbolNames[NumSymbols] = tmalloc(strlen(name)+1);
    strcpy(SymbolNames[NumSymbols],name);
    NumSymbols++;
}


void
set_property_value(string,type)

char *string;
int type;
{
    if (CurrentAttribute < 1 || CurrentAttribute > 127) {
        err_warn_2("Bad property record",type,CurrentSymbol);
        CurrentAttribute = 127;
    }
    fprintf(SymDesc,"5 %d %s;\n",CurrentAttribute,string);
    CurrentAttribute = -1;
}


int
path_to_rect(ncoords,xy)

int ncoords;
int *xy;
{
    PATHLIST *StartPolyPathp;
    PATHLIST *PolyPathp;
    RECT *rectp;
    RECT *reclistp;
    int i;
    int cx,cy;
    int width,length;

    PolyPathp = StartPolyPathp = NEWPATH;
    PolyPathp->pathlink = StartPolyPathp;
    for (i = 0; i < ncoords; ++i) {
        PolyPathp = PolyPathp->pathlink;
        PolyPathp->pathpoint.pointx = xy[i+i];
        PolyPathp->pathpoint.pointy = xy[i+i+1];
        PolyPathp->pathlink = NEWPATH;
    }
    PolyPathp->pathlink = NULL;
    if ((reclistp = pgtorex(StartPolyPathp)) != NULL) {
        rectp = reclistp;
        while (rectp != NULL) {
            length = (rectp->right - rectp->left);
            width = (rectp->top - rectp->bottom);
            cx = (rectp->left) + length/2;
            cy = (rectp->bottom) + width/2;
            if (length < 0)
                length = -length;
            if (width < 0)
                width = -width;
            fprintf(SymDesc,"B %d %d %d %d;\n",length,width,cx,cy);
            reclistp = rectp;
            rectp = rectp->r_next;
            free(reclistp);
        }
        freepath(&StartPolyPathp);
        return 1;
    }
    freepath(&StartPolyPathp);
    return 0;
}


void
set_path(cbuf,ncoords,xy)

char *cbuf;
int ncoords;
int *xy;
{
    int i, len, len1;
    char buf1[40];

    len = strlen(cbuf);
    for (i = 0; i < ncoords; ++i) {
        sprintf(buf1," %d %d",xy[i+i],xy[i+i+1]);
        len1 = strlen(buf1);
        if (len+len1 < 79) {
            strcat(cbuf,buf1);
            len += len1;
        }
        else {
            fprintf(SymDesc,"%s\n ",cbuf);
            strcpy(cbuf,buf1);
            len = len1+1;
        }
    }
    fprintf(SymDesc,"%s;\n",cbuf);
}


void
set_instance(index,reflection,cx,cy,rotn)

int index, reflection;
int cx,cy;
double rotn;
{
    int i, j;

    fprintf(SymDesc,"9 %s;\n",SymbolNames[index-1]);
    fprintf(SymDesc,"C %d",index);
    if (reflection)    fprintf(SymDesc," MY");
    if (set_angle(rotn,&i,&j))
        fprintf(SymDesc," R %d %d",i,j);
    fprintf(SymDesc," T %d %d;\n",cx,cy);
}

#define MIN(x,y) ((x) < (y) ? (x) : (y))


void
set_array(index,reflection,nx,ny,rotn,xy)

int index, reflection;
int nx,ny;
double rotn;
int *xy;
{
    int i, j, k;
    int dx,dy;

    fprintf(SymDesc,"9 %s;\n",SymbolNames[index-1]);
    k = set_angle(rotn,&i,&j);

/* xy[0] - xy[5] are coords of 3 points of BB, 0,1 reference */ 

    /* one of two terms is zero */
    dx = ((xy[2]-xy[0]) + (xy[3]-xy[1]))/nx;
    if (dx < 0) dx = -dx;
    dy = ((xy[5]-xy[1]) + (xy[4]-xy[0]))/ny;
    if (dy < 0) dy = -dy;
    fprintf(SymDesc,"1 Array %d %d %d %d;\n",nx,dx,ny,dy);
    fprintf(SymDesc,"C 0");
    if (reflection) fprintf(SymDesc," MY");
    if (k == 1) /* 90 */
        fprintf(SymDesc," R 0 1");
    else if (k == 2) /* 180 */
        fprintf(SymDesc," R -1 0");
    else if (k == 3) /* 270 */
        fprintf(SymDesc," R 0 -1");
    fprintf(SymDesc," T %d %d;\n",xy[0],xy[1]);
}


void
read_layer_table(fp)

FILE *fp;
{
    int i;
    char buf[16];
    char *errmsg = "Incorrect layer table format in";

    fscanf(fp,"%d",&NumLayerTable);
    if (NumLayerTable <= 0 || NumLayerTable > 127)
        err_fatal(errmsg,"line 1");
    for (i = 0; i < NumLayerTable; i++) {
        LayerNames[i] = tmalloc(45);
        if (fscanf(fp,"%s %d %d",LayerNames[i],
            &LayerNumbers[i],&DataTypeNumbers[i]) != 3) {
            sprintf(buf,"line %d",i+2);
            err_fatal(errmsg,buf);
        }
    }
    fclose(fp);
}


void
read_tech_layers(fp)

FILE *fp;
{
    char buf[512], name[45];
    char *errmsg = "Syntax error in tech file:";
    int lnum = -1, dtyp = -1;

    NumLayerTable = 0;
    name[0] = '\0';
    while (fgets(buf,512,fp) != NULL) {
        if (!strncasecmp(buf,"layername",9)) {
            if (sscanf(buf + 10,"%s",name) != 1)
                err_fatal(errmsg,buf);
            continue;
        }
        if (!strncasecmp(buf,"streamdata",10)) {
            char *t = buf + 10;
            if (!name[0])
                err_fatal(errmsg,"misplaced streamdata");
            while (*t == '?' || isspace(*t))
                t++;
            if (isdigit(*t))
                lnum = atoi(t);
            else
                err_fatal(errmsg,buf);
            while (isdigit(*t))
                t++;
            while (*t == ',' || isspace(*t))
                t++;
            if (isdigit(*t))
                dtyp = atoi(t);
            else
                err_fatal(errmsg,buf);

            if (lnum < 0 || lnum > 255 || dtyp < -1 || dtyp > 255)
                err_fatal(errmsg,buf);
            LayerNames[NumLayerTable] = tmalloc(45);
            strcpy(LayerNames[NumLayerTable],name);
            LayerNumbers[NumLayerTable] = lnum;
            DataTypeNumbers[NumLayerTable] = dtyp;
            NumLayerTable++;
            name[0] = '\0';
        }
    }
    fclose(fp);
}


int
set_angle(rotn,i,j)

double rotn;
int *i, *j;
{
    int ii, jj;

    ii = 100*cos(rotn);
    jj = 100*sin(rotn);

    if      (ii == -99) ii = -100;
    else if (ii ==  99) ii =  100;
    else if (jj == -99) jj = -100;
    else if (jj ==  99) jj =  100;

    *i = ii/100;
    *j = jj/100;

    if (*i == 1 && *j == 0)     /* 0 */
        return 0;

    if (*i == 0 && *j == 1)     /* 90 */
        return 1;

    if (*i == -1 && *j == 0)    /* 180 */
        return 2;

    if (*i == 0 && *j == -1)    /* 270 */
        return 3;

    err_warn_1("Nonorthogonal rotation","",CurrentSymbol);
    *i = *j = 0;
    return 0;
}

/***********************************************************************
 *
 * Functions used in reading stream format
 *
 ***********************************************************************/

extern int CurrentOffset;
extern FILE *stderror;

/***********************************************************************/
/* function STRM_IVAL                                                  */
/*      Function to evaluate STREAM short integer.                     */
/***********************************************************************/

int
strm_ival(s)

char *s;
{
    unsigned int i;
    unsigned char *b = (unsigned char *)s;

    /* Stream format is big-endian */
    i = b[1] | ((b[0] & 0x7f) << 8);
    if (b[0] & 0x80)
        i |= (-1 << 15);
    return ((int)i);
}


/***********************************************************************/
/* function STRM_LONGVAL                                               */
/*      Function to evaluate STREAM long integer.                      */
/***********************************************************************/

int
strm_longval(s)

char *s;
{
    unsigned int i;
    unsigned char *b = (unsigned char *)s;

    /* Stream format is big-endian */
    i = b[3] | (b[2] << 8) | (b[1] << 16) | ((b[0] & 0x7f) << 24);
    if (b[0] & 0x80)
        i |= (-1 << 31);
    return ((int)i);
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

/*
 * This function is really inefficient, but there are typically few
 * conversions required, and protability is an issue.
 */
double
strm_doubleval(s)

char *s;
{
    int exp, sign, i;
    double d, mantissa;
    unsigned char *b = (unsigned char*)s;

    sign = 0;
    exp = s[0];
    /* test the sign bit */
    if (exp & 0x80) {
        sign = 1;
        exp &= 0x7f;
    }
    exp -= 64;
 
    /* Construct the mantissa */
    mantissa = 0.0;
    for (i = 7; i > 0; i--) {
        mantissa += b[i];
        mantissa /= 256.0;
    }
     
    /* Now raise the mantissa to the exponent */
    d = mantissa;  
    if (exp > 0) {
        while (exp-- > 0)
            d *= 16.0;
    }
    else if (exp < 0) {
        while (exp++ < 0)
            d /= 16.0;
    }

    /* Make it negative if necessary */
    if (sign)
        d = -d;

    return (d);
}


/***** Error Functions *************************************************/

void
err_fatal(str,what)

char *str,*what;
{
    char buf[128];

    sprintf(buf,"\nError: %s %s.\n",str,what);
    fprintf(stderror,"%s",buf);
    exit(1);
}


void
err_fatal_1(str,what)

char *str,*what;
{
    char buf[128];

    sprintf(buf,"\nError: %s %s at offset %d.\n",str,what,CurrentOffset);
    fprintf(stderror,"%s",buf);
    exit(1);
}


void
err_fatal_2(str,type)

char *str;
int type;
{
    char buf[128];

    sprintf(buf,"\nError: %s %d at offset %d.\n",str,type,CurrentOffset);
    fprintf(stderror,"%s",buf);
    exit(1);
}


void
err_warn_1(str,which,what)

char *str,*which,*what;
{
    char buf[128];

    sprintf(buf,"\nWarning: %s for instance %s in symbol %s at offset %d.\n",
        str,which,what,CurrentOffset);
    fprintf(stderror,"%s",buf);
}


void
err_warn_2(str,type,what)

char *str,*what;
int type;
{
    char buf[128];

    sprintf(buf,"\nWarning: %s %d in symbol %s at offset %d.\n",
        str,type,what,CurrentOffset);
    fprintf(stderror,"%s\nIgnored.\n",buf);
}

/*****  Alias For DOS **************************************************/

#ifdef MSDOS

/* Map the symbol names into a DOS compatible file name, i.e., 8        */
/* characters. The first 6 are mapped directly, last 2 are index digits */

struct aliastab *aliasbase;

char *
alias(strname)

char *strname;
{
    char dosname[10], *ext;
    char *valid = "_^$~!#%&-{}()@'`.";
    struct aliastab *wl;
    int i;
    char *index();

    for (ext = strname; *ext; ext++) {
        if (isalpha(*ext) || isdigit(*ext) || index(valid,*ext))
            continue;
        break;
    }
    if (!*ext) {

        if (strlen(strname) <= 8)
            return (strname);
        ext = index(strname,'.');
        if (ext && ((int)(ext - strname) <= 8) && strlen(ext) <= 3)
            return (strname);

        strncpy(dosname,strname,6);
    }
    else {
        strncpy(dosname,strname,6);
        for (i = ext - strname; i < 6; i++) {
            ext = dosname + i;
            if (isalpha(*ext) || isdigit(*ext) || index(valid,*ext))
                continue;
            dosname[i] = '_';
        }
    }
    dosname[6] = '\0';

    for (i = 0,wl = aliasbase; wl; wl = wl->next) {
        if (!strcmp(strname,wl->strname)) return (wl->dosname);
        if (!strncmp(dosname,wl->dosname,6)) i++;
        if (!wl->next) {
            wl->next = (struct aliastab*) tmalloc(sizeof(struct aliastab));
            wl = wl->next;
            strcpy(wl->strname,strname);
            strncpy(wl->dosname,dosname,7);
            wl->next = NULL;
            wl->dosname[6] = '0' + i/10;
            wl->dosname[7] = '0' + i%10;
            wl->dosname[8] = '\0';
            return (wl->dosname);
        }
    }
    wl = (struct aliastab*) tmalloc(sizeof(struct aliastab));
    aliasbase = wl;
    strcpy(wl->strname,strname);
    strncpy(wl->dosname,dosname,7);
    wl->next = NULL;
    wl->dosname[6] = '0' + i/10;
    wl->dosname[7] = '0' + i%10;
    wl->dosname[8] = '\0';
    return (wl->dosname);
}


void
dumpalias()

{
    FILE *fp;
    struct aliastab *wl;

    if (aliasbase == NULL) return;
    fp = fopen(ALIASFILE,"w");
    if (fp == NULL) return;
    for (wl = aliasbase; wl; wl = wl->next)
        fprintf(fp,"%-10s%s\n",wl->dosname,wl->strname);
    fclose(fp);
}


void
readalias()

{
    FILE *fp;
    char s[80], *c;
    struct aliastab *wl;

    if ((fp = fopen(ALIASFILE,"r")) == NULL) return;
    while (fgets(s,80,fp) != NULL) {
        if (!aliasbase) {
            aliasbase = (struct aliastab *) tmalloc(sizeof(struct aliastab));
            wl = aliasbase;
        }
        else {
            wl->next = (struct aliastab *) tmalloc(sizeof(struct aliastab));
            wl = wl->next;
        }
        for (c = s; isspace(*c); c++) ;
        if (*c == '\0') break;
        if (sscanf(s,"%s %s",wl->dosname,wl->strname) != 2) {
            aliasbase = NULL;
            fclose(fp);
            return;
        }
    }
    wl->next = NULL;
    fclose(fp);
}

#endif

/***** Convert Path Type ***********************************************/

void
convert_pathtype(xe,ye,xb,yb,width)

int *xe,*ye;      /* coordinate of endpoint */
int xb,yb;        /* coordinate of previous or next point in path */
int width;        /* path width */
{
    double angle;
    double deltaX,deltaY;

    if(width == 0)
        return;
    else if(width < 0)
        width = -width;
    width /= 2;
    if(*xe == xb){
        if(*ye > yb)
            *ye -= width;
        else
            *ye += width;
    }
    else if(*ye == yb){
        if(*xe > xb)
            *xe -= width;
        else
            *xe += width;
    }
    else{
        deltaX = (double)(*xe - xb);
        deltaY = (double)(*ye - yb);
        angle = atan2(deltaY,deltaX);
        deltaX = (double)(width) * cos(angle);
        deltaY = (double)(width) * sin(angle);
        *xe -= (int)(deltaX);
        *ye -= (int)(deltaY);
    }
}


/***** Path To Rectangle Conversion ************************************/


/***********************************************************************
 * This file contains the following functions:
 * RECT *pgtorex(PATHLIST *pg)
 *       convert path list for polygon to rectangle list
 * int lowx(void *a, void *b)
 *       qsort compare routine for comparison by low x coordinate
 * int lowy(void *a, void *b)
 *       qsort compare routine for comparison by low y coordinate
 * int orient(PATHLIST *edges, int nedges, int dir[])
 *       assign direction to edges and insure manhattanness
 * int cross(PATHLIST *edge, int dir, int ybot, int ytop)
 *       would horizontal line at height between ybot and ytop cross edge?
 * RECT *makerect (int xbot, int ybot, int xtop, int ytop, RECT *next)
 *       make a new rectangle and assign values to its components
 ************************************************************************/


#define MAXPG 500     /* maximum # of points in polygon */
#define HEDGE 0       /* Horizontal edge */
#define REDGE 1       /* Rising edge */
#define FEDGE -1      /* Falling edge */


RECT *
pgtorex(pg)

PATHLIST *pg;
{
    /*
     * Convert path list representing manhattan polygon into
     * linked list of rectangles.
     * Return rectangle list, or null pointer if something goes wrong.
     */

    int npts = 0, n, dir[MAXPG], curr, wrapno;
    int xbot = 0, xtop, ybot, ytop;
    POINT *pts[MAXPG];
    PATHLIST *p, *edges[MAXPG], *tail = 0;
    RECT *rex = 0;

    for(p = pg; p; p = p->pathlink){
        if(++npts >= MAXPG){
            /*Polygon with more than 200 points*/
            goto exit;
        }
        pts[npts-1] = &(p->pathpoint);
        edges[npts-1] = p;
    }

    if(npts < 4){
        /*Polygon with fewer than 4 points*/
        goto exit;
    }

    /* close path list - don't worry, it's disconnected later */
    (tail = edges[npts-1])->pathlink = pg;

    /* sort points by low y, edges by low x */
    qsort ((char *) pts, npts, (int) sizeof (POINT *), lowy);
    qsort ((char *) edges, npts, (int) sizeof (PATHLIST *), lowx);

    /* orient edges */
    if(!orient (edges, npts, dir)){
        /*WARNING , non-manhattan Polygon*/
        goto exit;
    }

    /*
     * Start at the bottom of the polygon and scan upwards,
     * building rectangles as you go.
     */
    for(curr = 1; curr < npts; curr++){
        ybot = pts[curr-1]->pointy;
        while (ybot == pts[curr]->pointy)
            if(++curr >= npts)
                /* At top of polygon and done */
                goto done;
        ytop = pts[curr]->pointy;

        for(wrapno=0, n=0; n < npts; n++){
            if(wrapno == 0)
                xbot = edges[n]->pathpoint.pointx;
            if(!cross (edges[n], dir[n], ybot, ytop))
                continue;
            wrapno += dir[n] == REDGE ? 1 : -1;
            if(wrapno == 0){
                xtop = edges[n]->pathpoint.pointx;
                if(xbot == xtop)
                    continue;
                rex = makerect (xbot, ybot, xtop, ytop, rex);
                if(!rex){
                    /*makerect ran out of memory*/
                    goto exit;
                }
            }
        }
    }

exit:
    /* shouldn't get here ... */
    rex = (RECT *) 0;

done:
    /* this is the ONLY way out of pgtorex */
    /* disconnect start of polygon from its tail */
    if(tail)
        tail->pathlink = (PATHLIST *) 0;
    return (rex);
}


int
lowx(a,b)

#ifdef __STDC__
const void *a;
const void *b;
#else
char **a, **b;
#endif
{
    /*
     * compare points a and b, after following indirection.
     * Return 
     *    0   if identical
     *    1   if a.x > b.x
     *   -1   if a.x < b.x
     */

    POINT *p, *q;

    p = &(**(PATHLIST**)a).pathpoint;
    q = &(**(PATHLIST**)b).pathpoint;
    if(p->pointx < q->pointx)
        return (-1);
    if(p->pointx > q->pointx)
        return (1);
    return (0);
}


int
lowy(a,b)

#ifdef __STDC__
const void *a;
const void *b;
#else
char **a, **b;
#endif
{
    /*
     * compare points a and b, after following indirection.
     * Return 
     *    0   if identical
     *    1   if a.y > b.y
     *   -1   if a.y < b.y
     */

    if((**(POINT**)a).pointy < (**(POINT**)b).pointy)
        return (-1);
    if((**(POINT**)a).pointy > (**(POINT**)b).pointy)
        return (1);
    return (0);
}


int
orient(edges, nedges, dir)

PATHLIST *edges[];
int dir[], nedges;
{
    /*
     * Assign a direction, "dir[i]" to each of the "nedges" edges "edges[i]".
     * "Start" points to the first PATHLIST in the path list "edges".
     * Return 1 if all of the edges are horizontal or vertical, 0 otherwise.
     */

    int n;
    POINT *p, *q;

    for(n = 0; n < nedges; n++){
        /* note - path list should close on itself */
        p = &edges[n]->pathpoint;
        q = &edges[n]->pathlink->pathpoint;
        if(p->pointy == q->pointy){
            /* note - point may connect to itself here */
            dir[n] = HEDGE;
            continue;
        }
        if(p->pointx == q->pointx){
            if(p->pointy < q->pointy){
                dir[n] = REDGE;
                continue;
            }
            if(p->pointy > q->pointy){
                dir [n] = FEDGE;
                continue;
            }
            /* Point connects to itself */
            dir[n] = HEDGE;
            continue;
        }
        /* It's not manhattan folks. */
        return (0);
    }
    return (1);
}


int
cross(edge, dir, ybot, ytop)

PATHLIST *edge;
int dir;
int ybot, ytop;
{
    /*
     * return 1 if a horizontal line at height between ybot and ytop
     * (ybot < ytop) crosses the edge from edge->pathpoint to
     * edge->pathlink->pathpoint, 0 otherwise.  Dir is the direction
     * of the edge.
     */
    int ebot, etop;

    switch (dir){
    case REDGE:
        ebot = edge->pathpoint.pointy;
        etop = edge->pathlink->pathpoint.pointy;
        return (ebot <= ybot && etop >= ytop);

    case FEDGE:
        ebot = edge->pathlink->pathpoint.pointy;
        etop = edge->pathpoint.pointy;
        return (ebot <= ybot && etop >= ytop);

    default:
        return (0);
    }
}


RECT *
makerect(xbot, ybot, xtop, ytop, next)

int xbot, ybot, xtop, ytop;
RECT *next;
{
    /*
     * allocate space for a RECT structure and assign values to its
     * components from parameters.  Return pointer to structure or
     * null pointer if can't allocate space.
     */
    RECT *r;

    r = (RECT *) tmalloc (sizeof (RECT));
    if(r){
        r->left = xbot;
        r->bottom = ybot;
        r->right = xtop;
        r->top = ytop;
        r->r_next = next;
    }
    return (r);
}


void
freepath(pathheadpp)
PATHLIST **pathheadpp;
{
    PATHLIST *herep;
    PATHLIST *therep;

    for( herep = *pathheadpp ; herep != NULL ; herep = therep ){
        therep = herep -> pathlink;
        free( (char *) herep );
    }
    *pathheadpp = NULL;
}

/***** Memory Allocation ***********************************************/

char *
tmalloc(x)

unsigned x;
{
    char *c = (char*)malloc(x);

    if (c == NULL)
        err_fatal("Out of memory","");
    return (c);
}

/***** Etc ************************************************************/

#define Matching(string) (!strcmp(buf,string))

static void
file_open(name)

char *name;
{
    FILE *fp;
    char *cp, buf[80];

    fp = fopen(name,"r");
    if (fp == NULL) {
        sprintf(buf,"can't open file %s for reading",name);
        err_fatal("Error:",buf);
    }

    cp = get_token(fp);
    if (cp) {
        while (*cp) {
            if (*cp < '0' || *cp > '9')
                break;
            cp++;
        }
        rewind(fp);

        if (!*cp) {
            printf("Using layer table %s\n",name);
            read_layer_table(fp);
            return;
        }
        while ((cp = get_token(fp)) != NULL) {
            if (Matching("LAYERNAME") || Matching("LAYER")) {
                rewind(fp);
                printf("Using layers from tech file %s\n",name);
                read_tech_layers(fp);
                return;
            }
        }
    }
    sprintf(buf,"format error in %s",name);
    err_fatal("Error:",buf);
}


static char *
get_token(file)

FILE *file;
{
    char *cp;
    int c;
    static char keyword[40];

    /* look for first key word */
    while ( (c = getc(file)) < '0' ||
            (c > '9' && c < 'A') ||
            (c > 'Z' && c < 'a') ||
            (c > 'z') )
        if (c == EOF) return (NULL);

    /* scan to end of keyword and convert to upper case */
    cp = keyword;
    while (c != 040 && c != '\t' && c != 077 && c != '\n' && c != EOF) {
        if (c >= 'a' && c <= 'z') c -= 32;
        *cp++ = c;
        if (cp - keyword > 39)
            break;
        c = getc(file);
    }
    *cp = 0;
    /* scan to end of line */
    while (c != '\n' && c != EOF) {
        c = getc(file);
    }
    return (keyword);
}


FILE *OpenDevice() {return NULL;}

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
 
#define Allocate

#include "prefix.h"
#include "stream.h"
#include "kic.h"
#include <math.h>

/* Define to 1 if GDS symbol and lib names are to be converted to
 * upper case.  This was true in the previous versions of Kic.
 */
#define TOUPR 0

#define DATA_TYPE(a) (((a)<0||(a)>255)?0:(a))
#define LONGSCALE(s,x)  (int) (floor( (s * (double)(x)) + 0.5))

#define GDS_SUFFIX ".gds"

extern int  CDStatusInt;
extern char *CDStatusString;

struct f FB = { 0 };
int LayerNumbers[CDNUMLAYERS+1];
int DataTypeNumbers[CDNUMLAYERS+1];

int SelectiveSearch;    /* If set, only those symbols that exist in the
                           current working directory will be placed into
                           the output stream library file */

STRM_LIBRARY     STRMLibrary;
STRM_STRCT       STRMStructure;
STRM_BOUNDARY    STRMBoundary;
STRM_PATH        STRMPath;
STRM_TEXT        STRMText;
STRM_SREF        STRMSref;
STRM_AREF        STRMAref;
STRM_TRANSFORM   DefaultTransform;
STRM_PROPERTY    DefaultProperty;

int NumLayerTable;
struct kl LayerTable[CDNUMLAYERS+1];
struct kp Parameters;
struct cl ColorTable[12];
FILE   *StreamFile;

double Scale;

#if __STDC__
extern void help(void);
extern char *nextarg(void);
extern void GetLayerInfo(char*,int,int);
extern void SetLayerInfo(int,int);
extern int  GenStream(char*,struct s*,int*);
extern void finish_path(int);
extern void out_poly(struct p*,Poly*);
extern void break_poly(Poly*);
extern void convert_pathtype(int*,int*,int,int,int);
extern void MallocFailed(void);
extern void strm_beginrecord(int,int,int);
extern void strm_adddate(struct tm*);
extern void strm_entrasc_rec(char*,int,int);
extern void strm_entrasc(char*,int);
extern int  bgnlib(STRM_LIBRARY*);
extern int  endlib(void);
extern int  bgnstr(STRM_STRCT*);
extern int  endstr(void);
extern int  strm_endlmnt(STRM_PROPERTY*);
extern int  bndry(STRM_BOUNDARY*);
extern int  strm_strnsfm(STRM_TRANSFORM*);
extern int  path(STRM_PATH*);
extern int  sref(STRM_SREF*);
extern int  aref(STRM_AREF*);
extern int  text(STRM_TEXT*);
extern char *unalias(char*);
extern void strm_intcopy(int);
extern void strm_lngcopy(int);
extern void strm_dblcopy(double);
#else
extern void help();
extern char *nextarg();
extern void GetLayerInfo();
extern void SetLayerInfo();
extern int  GenStream();
extern void finish_path();
extern void out_poly();
extern void break_poly();
extern void convert_pathtype();
extern void MallocFailed();
extern void strm_beginrecord();
extern void strm_adddate();
extern void strm_entrasc_rec();
extern void strm_entrasc();
extern int  bgnlib();
extern int  endlib();
extern int  bgnstr();
extern int  endstr();
extern int  strm_endlmnt();
extern int  bndry();
extern int  strm_strnsfm();
extern int  path();
extern int  sref();
extern int  aref();
extern int  text();
extern void strm_intcopy();
extern void strm_lngcopy();
extern void strm_dblcopy();
extern char *unalias();
#endif

int   argc;
char  **argv;


int
main(ac, av)
char *av[];
{
    double Float1 = 1.0;
    double Float2;
    struct s *SymbolDesc;
    struct tm Now;
    struct prpty *PrptyDesc;
    char *cp;
    time_t tloc;
    int i = 0,k;
    int detail = 0;
    int symbolic = 0;
    int DBUpermic = 0;
    int SymbolNum = 0;
    int UseStreamNames = 0;
    int UseLayerTable = 0;
    char Root[81];
    char StreamFileName[81];
    char LayerFile[81];
    char Path[81];
    char *Tmp, *Tech;

    /* initialize */
    InitGlobal();
    SelectiveSearch              = False;
    tloc                         = time(NULL);
    Now                          = *localtime(&tloc);

    STRMLibrary.lib_name[0]      = '\0';
    STRMLibrary.lib_lib1[0]      = '\0';
    STRMLibrary.lib_lib2[44]     = '\0';
    STRMLibrary.lib_font0[0]     = '\0';
    STRMLibrary.lib_font1[0]     = '\0';
    STRMLibrary.lib_font2[0]     = '\0';
    STRMLibrary.lib_font3[0]     = '\0';
    STRMLibrary.lib_attr[0]      = '\0';
    STRMLibrary.lib_accessdate   = Now;
    STRMLibrary.lib_moddate      = Now;
    STRMLibrary.lib_uunit        = .01;
    STRMLibrary.lib_munit        = 1e-8;
    STRMLibrary.lib_gen          = 3;

    STRMStructure.str_moddate    = Now;
    STRMStructure.str_creatdate  = Now;

    DefaultTransform.trns_reflection = 0;
    DefaultTransform.trns_abs_mag    = 0;
    DefaultTransform.trns_abs_angle  = 0;
    DefaultTransform.trns_mag        = 1.0;
    DefaultTransform.trns_angle      = 0.0;
    DefaultProperty.prp_npropval     = 0;

    argc = ac;
    argv = av;
    Float1 = 1.0;
    *Root = '\0';
    *StreamFileName = '\0';

    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {

        case 'L':
        case 'l':
            sscanf(nextarg(), "%lg", &Float1);
            break;

        case 'Z':
        case 'z':
            /* extract STREAM filename */
            strcpy(STRMLibrary.lib_name,nextarg());
            break;

        case 'M':
        case 'm':
            ++DBUpermic;
            sscanf(nextarg(), "%lg", &Float2);
            break;

        case 'O':
        case 'o':
            strcpy(StreamFileName, nextarg());
            break;

        case 'S':
        case 's':
            symbolic++;
            break;

        case 'D':
        case 'd':
            detail++;
            break;

        case 'C':
        case 'c':
            SelectiveSearch++;
            break;

        case 'X':
        case 'x':
            strcpy(LayerFile, nextarg());
            UseLayerTable = 1;
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
            TECH_EXT = tmalloc(strlen(Tech) + 1);
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

    if (argc > 1)
        strcpy(Root,argv[1]);
    else {
        printf("Hierarchy's root cell? (hit return for help) ");
        Tmp = tmalloc(81);
        *Tmp = '\0';
        fgets(Tmp,81,stdin);
        if (sscanf(Tmp,"%s",Root) != 1)
            help();
        free(Tmp);
    }

    Path[0] = '\0';
    Tmp = strrchr(Root,DIRC);
    if (Tmp) {
        *Tmp = '\0';
        strcpy(Path,Root);
        for (i = 0, Tmp++; *Tmp; i++, Tmp++)
            Root[i] = *Tmp;
        Root[i] = '\0';
    }

    /*
     * Initializes CD package and reads in tech file so we
     * know the layer names.  Can't generate CIF without them.
     */
    ReadTechFile();

    if (!UseStreamNames) {
        if (UseLayerTable)
            printf("Using layer table file %s\n",LayerFile);
        else {
            printf("Using layers from %s\n",TECHFILE);
            strcpy(LayerFile,TECHFILE);
        }
    }
    else {
        printf("Using layer names for stream layer data\n");
        UseLayerTable = 0;
    }

    if (STRMLibrary.lib_name[0] == '\0')
        sprintf(STRMLibrary.lib_name,"KICTOSTREAM");

    printf("Microns per lambda: %15.8e\n",Float1);

    if (StreamFileName[0] == '\0') {
        cp = Root;
        i = 0;
        while (*cp != '.' And *cp != '\0') {
            StreamFileName[i] = *cp;
            ++i;
            ++cp;
        }
        StreamFileName[i] = '\0';
        strcat(StreamFileName, GDS_SUFFIX);
    }

    if (DBUpermic) {
        STRMLibrary.lib_uunit = 1.0/Float2;
        STRMLibrary.lib_munit = 1e-6 * STRMLibrary.lib_uunit;
        Float1 *= (.01/STRMLibrary.lib_uunit);
    }

    /* add the root path */
    if (*Path) {
        Tmp = PGetPath();
        strcat(Tmp,Path);
        printf("KIC search path: %s\n",Tmp);
    }

    for (k = 0; k < NumLayerTable; k++) {
        LayerNumbers[k] = LayerTable[k].klStreamNumber;
        if (LayerNumbers[k] <= 0)
            LayerNumbers[k] = k;
        DataTypeNumbers[k] = LayerTable[i].klStreamDataType;
    }

    GetLayerInfo(LayerFile,UseLayerTable,UseStreamNames);
    SetLayerInfo(detail,symbolic);

    if ((StreamFile=POpen(StreamFileName,"wb",
            (char *)NULL,(char **)NULL)) == NULL) {    
        fprintf(stderr,"Can't open STREAM file.");
        exit(1);
    }

    if (Not CDOpen(Root,&SymbolDesc,'r')) {
        fprintf(stderr,"Can't open root kic cell %s.\n",Root);
        fprintf(stderr,"Parse failed at around:  %s.\n",CDStatusString);
        exit(1);
    }
    if (CDStatusInt == CDNEWSYMBOL) {
        fprintf(stderr,"Can't find root kic cell %s.\n",Root);
        exit(1);
    }

    /* add STREAM-specific property list info */
    CDProperty(SymbolDesc,(struct o *)NULL,&PrptyDesc);
    while (PrptyDesc != NULL) {
        i = PrptyDesc->prpty_Value - PROPERTYOFFSET;
        if (i == HEADER) {
        /* version 3 is assumed */
        }
        else if (i == LIBNAME) {
            sscanf(PrptyDesc->prpty_String,"%s",STRMLibrary.lib_name);
        }
        else if (i == REFLIBS) {
            sscanf(PrptyDesc->prpty_String,"%s %s",STRMLibrary.lib_lib1,
            STRMLibrary.lib_lib2);
        }
        else if (i == FONTS) {
            sscanf(PrptyDesc->prpty_String,"%s %s %s %s",
            STRMLibrary.lib_font0,STRMLibrary.lib_font1,
            STRMLibrary.lib_font2,STRMLibrary.lib_font3);
        }
        else if (i == GENERATIONS) {
            sscanf(PrptyDesc->prpty_String,"%d",&STRMLibrary.lib_gen);
        }
        else if (i == ATTRTABLE) {
            sscanf(PrptyDesc->prpty_String,"%s",STRMLibrary.lib_attr);
        }
        PrptyDesc = PrptyDesc->prpty_Succ;
    }
#ifdef MSDOS
    readalias();
#endif

    bgnlib(&STRMLibrary);
    SymbolNum = 1;
    Scale = Float1;
    if (Not GenStream(Root,SymbolDesc,&SymbolNum)) {
        fprintf(stderr,"Unable to write stream file.\n");
        fprintf(stderr,"Status string: %s.\n",CDStatusString);
        exit(1);
    }
    endlib();
    fclose(StreamFile);
    printf("Translation of %s succeeded.\n",Root);
    return (0);
}


void
help()
{
    printf("\nkictostr-%s\n\n",VersionString);
    printf("Usage: kictostr [options] [root_kic_cell]\n\n");
    printf("options:\n");
    printf("  -C          convert only cells in current directory\n");
    printf("  -Zname      stream library name (default \"KICTOSTREAM\")\n");
    printf("  -Ostrname   stream file name to create (default is root kic\n");
    printf("              cell name with .str extension)\n");
    printf("  -S          convert symbolic layers only\n");
    printf("  -D          convert detail layers only\n");
    printf("  -Xfilename  filename = layer table reference file\n");
    printf("  -Text       use layers in tech.ext\n");
    printf("  -N          parse layer names for stream layers, kic layers\n");
    printf("              must be named \"NN NN\"\n");
    printf("  -Lmicprl    micron per lambda (default 1.0)\n");
    printf("  -Mupermic   units per micron (default 100.0)\n\n");
    printf("N supersedes X,T,  Default is layers from tech file.\n");
    printf("Prompts for kic cell name if not supplied.\n");
    exit(0);
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


void
GetLayerInfo(LayerFile,UseLayerTable,UseStreamNames)

char *LayerFile;
int UseLayerTable,UseStreamNames;
{
    FILE *ltable;
    int i,j,k,l,n;
    char buffer[80];
    char *emesg = "Incorrect layer table format in %s.\n";

    if (UseLayerTable) {
        if ((ltable = fopen(LayerFile,"r")) == NULL) {
            fprintf(stderr,"Can't open stream layer file %s.\n",LayerFile);
            exit(1);
        }
        fscanf(ltable,"%d",&n);
        if (n <= 0 || n > 127) {
            fprintf(stderr,emesg,LayerFile);
            exit(1);
        }
        for (i = 0; i < n; i++) {
            buffer[0] = buffer[1] = buffer[2] = buffer[3] = ' ';
            if (fscanf(ltable,"%s %d %d",buffer,&j,&k) < 3) {
                fprintf(stderr,emesg,LayerFile);
                exit(1);
            }
            for (l = 0; l < 4; l++) {
                if (buffer[l] < ' ')
                    buffer[l] = ' ';
            }
            for (l = 1; l <= CDNUMLAYERS; l++) {
                if (buffer[0] == CDLayer[l].lTechnology And
                    buffer[1] == CDLayer[l].lMask[0] And
                    buffer[2] == CDLayer[l].lMask[1] And
                    buffer[3] == CDLayer[l].lMask[2]) {

                    LayerNumbers[l] = j;
                    DataTypeNumbers[l] = k;
                }
            }
        }
        fclose(ltable);
        return;
    }

    if (UseStreamNames) {
        buffer[5] = '\0';;
        for (k = 0; k < CDNUMLAYERS; k++) {
            buffer[0] = CDLayer[k].lTechnology;
            buffer[1] = CDLayer[k].lMask[0];
            buffer[2] = ' ';
            buffer[3] = CDLayer[k].lMask[1];
            buffer[4] = CDLayer[k].lMask[2];
            if (sscanf(buffer,"%d %d",
                &LayerNumbers[k],&DataTypeNumbers[k]) < 2) {
                fprintf(stderr,
                    "Internal error: trouble reading stream layer %c%s.\n",
                    CDLayer[k].lTechnology,CDLayer[k].lMask);
                exit(1);
            }
        }
    }
}


void
SetLayerInfo(detail,symbolic)

int detail,symbolic;
{
    int Layers[CDNUMLAYERS];
    int Layer;

    if (!(detail ^ symbolic)) {
        detail = 0;
        symbolic = 0;
        printf("Converting all layers\n");
    }
    else if (detail)
        printf("Converting detail layers\n");
    else
        printf("Converting symbolic layers\n");


    for (Layer = 0; Layer < NumLayerTable; Layer++)
        Layers[Layer] = True;

    if (detail) {
        for (Layer = 0; Layer < NumLayerTable; Layer++)
            if (LayerTable[Layer+1].klAttributes & SYMBOLIC)
                Layers[Layer] = False;
    }
    else if (symbolic) {
        for (Layer = 0; Layer < NumLayerTable; Layer++)
            if (Not (LayerTable[Layer+1].klAttributes & SYMBOLIC))
               Layers[Layer] = False;
    }
    for (Layer = 0; Layer < NumLayerTable; Layer++)
        if (Layers[Layer])
            CDLayer[Layer].lCDFrom = True;
        else
            CDLayer[Layer].lCDFrom = False;
}


int
GenStream(MasterName,SymbolDesc,SymbolNum)

char *MasterName;
struct s *SymbolDesc;
int *SymbolNum;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct s *MasterDesc;
    struct p *Pair,*Path;
    struct t *TGen;
    struct prpty *PrptyDesc;
    char *SymbolName;
    char *Label;
    char Type,Xform;
    char garbage[120];
    int Info;
    int i;
    int Layer;
    int NumX,NumY;
    unsigned int size;
    int X,Y,Length,Width;
    int DX,DY;
    double a,b;

    printf("Converting: %s\n",MasterName);

    *SymbolNum += 1; 
    /*
     * Mark symbol associated with SymbolDesc as visited by storing
     * its symbol # in its info field.
     */
    CDSetInfo(SymbolDesc,(struct o *)NULL,*SymbolNum);

    /*
     * First write to the stream file any symbol definitions below
     * the symbol associated with SymbolDesc.
     */
    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
        return(CDError(CDMALLOCFAILED));
    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            break;
        CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);
        if (Not CDOpen(SymbolName,&MasterDesc,'w')) {
            fprintf(stderr,"Unable to open symbol %s.\n",SymbolName);
            fprintf(stderr,"Status string: %s.\n",CDStatusString);
            exit(1);
        }
        CDInfo(MasterDesc,(struct o *)NULL,&Info);
        if (Info == 0) {
            /* Write master's definition to stream file. */
            if (Not GenStream(SymbolName,MasterDesc,SymbolNum)) {
                fprintf(stderr,"Unable to write stream file.\n");
                fprintf(stderr,"Status string: %s.\n",CDStatusString);
                exit(1);
            }
        }
    }
    /*
     * If the SelectiveSearch integer is set, only those symbols which
     * exist in the current directory will be placed in the stream file.
     */
    if (SelectiveSearch) {
        if (access(MasterName,0) != 0) {
            fprintf(stderr,"Symbol %s not found in current directory.\n",
            MasterName);
            return(True);
        }
    }

    /*
     * Write to the stream file the definition of the symbol associated with
     * SymbolDesc.  Instance calls first--then geometries.
     */
#ifdef MSDOS
    strcpy(STRMStructure.str_name, unalias(MasterName));
#else
    strcpy(STRMStructure.str_name, MasterName);
#endif
    bgnstr(&STRMStructure);
    if (Not CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
        return(CDError(CDMALLOCFAILED));
    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL)
            break;
        CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);
        if (Not CDOpen(SymbolName,&MasterDesc,'w'))
            return(False);

        if (NumX > 1 || NumY > 1) {
            /* create an AREF */
 
#ifdef MSDOS
            strcpy(STRMAref.ar_name, unalias(SymbolName));
#else
            strcpy(STRMAref.ar_name, SymbolName);
#endif
            STRMAref.ar_transform = DefaultTransform;
            STRMAref.ar_col = NumX;
            STRMAref.ar_row = NumY;
            STRMAref.ar_xy[0] = 0;
            STRMAref.ar_xy[1] = 0;
 
            /* copy the property list */
            CDProperty(SymbolDesc,Pointer,&PrptyDesc);
            while (PrptyDesc != NULL) {
                if (STRMAref.ar_prop.prp_npropval >= 255) {
                    fprintf(stderr,"Too many properties for symbol %s.\n",
                        SymbolName);
                    STRMAref.ar_prop.prp_npropval = 255;
                }
                /* allow only valid STREAM property attributes */
                i = PrptyDesc->prpty_Value;
                if (i > 0 && i < 128) {
                    STRMAref.ar_prop.prp_propattr
                        [STRMAref.ar_prop.prp_npropval] = i;
                    size = strlen(PrptyDesc->prpty_String) + 2;
                    STRMAref.ar_prop.prp_propval
                        [STRMAref.ar_prop.prp_npropval] = tmalloc(size);
                    strcpy(STRMAref.ar_prop.prp_propval
                        [STRMAref.ar_prop.prp_npropval],
                        PrptyDesc->prpty_String);              
                    ++STRMAref.ar_prop.prp_npropval;
                }
                PrptyDesc = PrptyDesc->prpty_Succ;
            }
 
            STRMAref.ar_transform.trns_angle = 0;
            STRMAref.ar_transform.trns_reflection = 0;
            CDInitTGen(Pointer,&TGen);
            TPush();
            TIdentity();
            loop {
                CDTGen(&TGen,&Type,&X,&Y);
                if (TGen == NULL) break;
                elif (Type == CDROTATE) {
                    a = (double) X;
                    b = (double) Y;
                    STRMAref.ar_transform.trns_angle +=
                        (RADTODEG * atan2(b,a));
                    TRotate(X,Y);
                }
                elif (Type == CDTRANSLATE) {
                    TTranslate(LONGSCALE(Scale,X),LONGSCALE(Scale,Y));
                }
                elif (Type == CDMIRRORX) {
                    STRMAref.ar_transform.trns_reflection ^= 1;
                    STRMAref.ar_transform.trns_angle += 180.0;
                    TMX();
                }
                elif (Type == CDMIRRORY) {
                    STRMAref.ar_transform.trns_reflection ^= 1;
                    TMY();
                }
            }
            TPremultiply();
            STRMAref.ar_xy[0] = 0;
            STRMAref.ar_xy[1] = 0;
            STRMAref.ar_xy[2] = DX*NumX;
            STRMAref.ar_xy[3] = 0;
            STRMAref.ar_xy[4] = 0;
            STRMAref.ar_xy[5] = DY*NumY;
            TPoint(&STRMAref.ar_xy[0],&STRMAref.ar_xy[1]);
            TPoint(&STRMAref.ar_xy[2],&STRMAref.ar_xy[3]);
            TPoint(&STRMAref.ar_xy[4],&STRMAref.ar_xy[5]);
            TPop();
            aref(&STRMAref);
            
            /* free storage of property values */
            for (i = 0; i < STRMAref.ar_prop.prp_npropval; i++)
                free(STRMAref.ar_prop.prp_propval[i]);
            STRMAref.ar_prop.prp_npropval = 0;
        }
        else {
            /* create an SREF */

#ifdef MSDOS
            strcpy(STRMSref.sr_name, unalias(SymbolName));
#else
            strcpy(STRMSref.sr_name, SymbolName);
#endif
            STRMSref.sr_transform = DefaultTransform;
            STRMSref.sr_xy[0] = STRMSref.sr_xy[1] = 0;
            /* copy the property list */
            CDProperty(SymbolDesc,Pointer,&PrptyDesc);
            while (PrptyDesc != NULL) {
                if (STRMSref.sr_prop.prp_npropval >= 255) {
                    fprintf(stderr,"Too many properties for symbol %s.\n",
                        SymbolName);
                    STRMSref.sr_prop.prp_npropval = 255;
                }
                /* allow only valid STREAM property attributes */
                i = PrptyDesc->prpty_Value;
                if (i > 0 && i < 128) {
                    STRMSref.sr_prop.prp_propattr
                        [STRMSref.sr_prop.prp_npropval] = i;
                    size = strlen(PrptyDesc->prpty_String) + 2;
                    STRMSref.sr_prop.prp_propval
                        [STRMSref.sr_prop.prp_npropval] = tmalloc(size);
                    strcpy(STRMSref.sr_prop.prp_propval
                        [STRMSref.sr_prop.prp_npropval],
                        PrptyDesc->prpty_String);
                    ++STRMSref.sr_prop.prp_npropval;
                }
                PrptyDesc = PrptyDesc->prpty_Succ;
            }
            STRMSref.sr_transform.trns_angle = 0;
            STRMSref.sr_transform.trns_reflection = 0;
            CDInitTGen(Pointer,&TGen);
            loop {
                CDTGen(&TGen,&Type,&X,&Y);
                if (TGen == NULL) {
                    break;
                }
                elif (Type == CDROTATE) {
                    a = (double) X;
                    b = (double) Y;
                    STRMSref.sr_transform.trns_angle += 
                        (RADTODEG * atan2(b,a));
                }
                elif (Type == CDTRANSLATE) {
                    STRMSref.sr_xy[0]
                        = LONGSCALE(Scale,X);
                    STRMSref.sr_xy[1]
                        = LONGSCALE(Scale,Y);
                }
                elif (Type == CDMIRRORX) {
                    STRMSref.sr_transform.trns_reflection ^= 1;
                    STRMSref.sr_transform.trns_angle += 180.0;
                }
                elif (Type == CDMIRRORY)
                    STRMSref.sr_transform.trns_reflection ^= 1;
            }
            sref(&STRMSref);
            /* free storage of property values */
            for (i = 0; i < STRMSref.sr_prop.prp_npropval; ++i) {
                free(STRMSref.sr_prop.prp_propval[i]);
            }
            STRMSref.sr_prop.prp_npropval = 0;
        }
    }
    for (Layer = 1;Layer <= CDNUMLAYERS;++Layer) {
        if (Not CDInitGen(SymbolDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc))
            return(CDError(CDMALLOCFAILED));
        loop {
            CDGen(SymbolDesc,GenDesc,&Pointer);
            if (Pointer == NULL)
                break;
            /* copy the property list */
            CDProperty(SymbolDesc,Pointer,&PrptyDesc);
            while (PrptyDesc != NULL) {
                if (DefaultProperty.prp_npropval >= 255) {
                    fprintf(stderr,
                        "Too many properties for layer %d\n",Layer);
                    DefaultProperty.prp_npropval = 255;
                }
                i = PrptyDesc->prpty_Value;
                /* allow only valid STREAM property attributes */
                if (i > 0 && i < 128) {
                    DefaultProperty.prp_propattr
                        [DefaultProperty.prp_npropval] = i;
                    size = strlen(PrptyDesc->prpty_String) + 2;
                    DefaultProperty.prp_propval
                        [DefaultProperty.prp_npropval] = tmalloc(size);
                    strcpy(DefaultProperty.prp_propval
                        [DefaultProperty.prp_npropval],
                        PrptyDesc->prpty_String);
                    ++DefaultProperty.prp_npropval;
                }
                PrptyDesc = PrptyDesc->prpty_Succ;
            }
            CDType(Pointer,&Type);
            if (Type == CDBOX) {
                CDBox(Pointer,&Layer,&Length,&Width,&X,&Y);
                Length /= 2;
                Width /= 2;
                X = LONGSCALE(Scale,X);
                Y = LONGSCALE(Scale,Y);
                Length = LONGSCALE(Scale,Length);
                Width = LONGSCALE(Scale,Width);
                STRMBoundary.bnd_layer = LayerNumbers[Layer];
                STRMBoundary.bnd_datatype = DATA_TYPE(DataTypeNumbers[Layer]);
                STRMBoundary.bnd_prop = DefaultProperty;
                STRMBoundary.bnd_ncoord = 5;
                STRMBoundary.bnd_xy[0] = (X + Length);
                STRMBoundary.bnd_xy[1] = (Y + Width);
                STRMBoundary.bnd_xy[2] = (X + Length);
                STRMBoundary.bnd_xy[3] = (Y - Width);
                STRMBoundary.bnd_xy[4] = (X - Length);
                STRMBoundary.bnd_xy[5] = (Y - Width);
                STRMBoundary.bnd_xy[6] = (X - Length);
                STRMBoundary.bnd_xy[7] = (Y + Width);
                STRMBoundary.bnd_xy[8] = (X + Length);
                STRMBoundary.bnd_xy[9] = (Y + Width);
                bndry(&STRMBoundary);
            }
            elif (Type == CDWIRE) {
                CDWire(Pointer,&Layer,&Width,&Path);
                STRMPath.pth_layer = LayerNumbers[Layer];
                STRMPath.pth_datatype = DATA_TYPE(DataTypeNumbers[Layer]);
                STRMPath.pth_width = LONGSCALE(Scale,Width);
                STRMPath.pth_prop = DefaultProperty;
                STRMPath.pth_pathtype = 2;
                /*
                 * look at the property list for a STREAM pathtype definition
                 */
                CDProperty(SymbolDesc,Pointer,&PrptyDesc);
                while (PrptyDesc != NULL) {
                    i = PrptyDesc->prpty_Value - PROPERTYOFFSET;
                    if (i == PATHTYPE) {
                        sscanf(PrptyDesc->prpty_String,"%s %d",garbage,&i);
                        if (i >= 0 && i < 3)
                            STRMPath.pth_pathtype = i;
                        break;
                    }
                    PrptyDesc = PrptyDesc->prpty_Succ;
                }
                Pair = Path;
                i = 0;
                while (Pair != NULL) {
                    STRMPath.pth_xy[i++] = LONGSCALE(Scale,Pair->pX);
                    STRMPath.pth_xy[i++] = LONGSCALE(Scale,Pair->pY);
                    if (i == 2 && !Pair->pSucc) {
                        STRMPath.pth_xy[i++] = LONGSCALE(Scale,Pair->pX);
                        STRMPath.pth_xy[i++] = LONGSCALE(Scale,Pair->pY);
                    }
                    Pair = Pair->pSucc;
                    if (i == MAXSTRMCOORDS && Pair) {
                        printf("Breaking wire with too many vertices.\n");
                        finish_path(i);
                        i = 0;
                    }
                }
                finish_path(i);
            }
            elif (Type == CDPOLYGON) {
                CDPolygon(Pointer,&Layer,&Path);
                STRMBoundary.bnd_layer = LayerNumbers[Layer];
                STRMBoundary.bnd_datatype = DATA_TYPE(DataTypeNumbers[Layer]);
                STRMBoundary.bnd_prop = DefaultProperty;
                out_poly(Path, (Poly*)NULL);
            }
            elif (Type == CDLABEL) {
                CDLabel(Pointer,&Layer,&Label,&X,&Y,&Xform);
                STRMText.txt_layer = LayerNumbers[Layer];
                STRMText.txt_texttype = DATA_TYPE(DataTypeNumbers[Layer]);
                STRMText.txt_pathtype = 1;
                STRMText.txt_horizontal = 0;
                STRMText.txt_vertical = 0;
                STRMText.txt_font = 0;
                STRMText.txt_width = 0;
                STRMText.txt_xy[0] = LONGSCALE(Scale,X);
                STRMText.txt_xy[1] = LONGSCALE(Scale,Y);
                STRMText.txt_prop = DefaultProperty;
                STRMText.txt_transform = DefaultTransform;
                STRMSref.sr_xy[0] = STRMSref.sr_xy[1] = 0;
                memcpy(STRMText.txt_text,Label,44);
                STRMText.txt_text[44] = '\0';
                /* look at the property list for a STREAM text definition */
                CDProperty(SymbolDesc,Pointer,&PrptyDesc);
                while (PrptyDesc != NULL) {
                    int wdth;
                    int present,ptype,reflection;
                    double magn,rotn;
                    i = PrptyDesc->prpty_Value - PROPERTYOFFSET;
                    if (i == TEXT) {
                        sscanf(PrptyDesc->prpty_String,
                        "%s %d %s %d %s %d %s %lf %s %lf %s %d",
                        garbage,&wdth,garbage,&present,garbage,&ptype,
                        garbage,&magn,garbage,&rotn,garbage,&reflection);
                        STRMText.txt_pathtype = ptype;
                        STRMText.txt_horizontal = (present >> 8) & 3;
                        STRMText.txt_vertical = (present >> 10) & 3;
                        STRMText.txt_font = (present >> 12) & 3;
                        STRMText.txt_width = wdth;
                        STRMText.txt_transform.trns_reflection = reflection;
                        STRMText.txt_transform.trns_mag = magn;
                        STRMText.txt_transform.trns_angle = rotn;
                        break;
                    }
                    PrptyDesc = PrptyDesc->prpty_Succ;
                }
                text(&STRMText);
            }
            /* free storage of property values */
            for (i = 0; i < DefaultProperty.prp_npropval; ++i) {
                free(DefaultProperty.prp_propval[i]);
            }
            DefaultProperty.prp_npropval = 0;
        }
    }
    endstr();
    return(True);
}


void
finish_path(i)

int i;
{
    if (STRMPath.pth_pathtype == 0 && i > 3) {
        convert_pathtype(&STRMPath.pth_xy[0],&STRMPath.pth_xy[1],
        STRMPath.pth_xy[2],STRMPath.pth_xy[3],
        STRMPath.pth_width);
        convert_pathtype(&STRMPath.pth_xy[i-1],
        &STRMPath.pth_xy[i-2],STRMPath.pth_xy[i-3],
        STRMPath.pth_xy[i-4],STRMPath.pth_width);
    }
    STRMPath.pth_ncoord = i/2;
    path(&STRMPath);
}


void
out_poly(Path, po)

/* Output the poly data, or cut it into pieces if there are
 * too many vertices.
 */
struct p *Path;
Poly *po;
{
    int i;
    struct p *pp;
    Poly p1;
    int *xy, *bxy;

    if (!po) {
        for (i = 0, pp = Path; pp; i++,pp = pp->pSucc) ;
        p1.nvertices = i;
        xy = (int*) tmalloc(i*2*sizeof(int));
        p1.xy = xy;
        for (pp = Path; pp; pp = pp->pSucc) {
            *xy++ = pp->pX;
            *xy++ = pp->pY;
        }
    }
    else
        p1 = *po;

    if (p1.nvertices > MAXSTRMCOORDS/2) {
        printf("Breaking polygon with too many vertices.\n");
        break_poly(&p1);
    }
    else {
        xy = p1.xy;
        bxy = STRMBoundary.bnd_xy;
        for (i = 0; i < p1.nvertices; i++) {
            *bxy++ = LONGSCALE(Scale,*xy++);
            *bxy++ = LONGSCALE(Scale,*xy++);
        }
        if (i < 4) {
            /* close the triangle */
            *bxy++ = STRMBoundary.bnd_xy[0];
            *bxy++ = STRMBoundary.bnd_xy[1];
            i++;
        }
        STRMBoundary.bnd_ncoord = i;
        bndry(&STRMBoundary);
    }
    if (!po)
        free(p1.xy);
}


void
break_poly(p1)

/* Break a polygon across the midlle of its BB horizontally and
 * write out the resulting clipped polygons.
 */
Poly *p1;
{
    int i, x, y, RefY;
    struct  ka BB;
    Poly p2;
    int *xy;

    /* 4 * num vertices should be long enough */
    p2.xy = (int*) tmalloc(p1->nvertices*8*sizeof(int));

    BB.kaLeft = BB.kaRight = p1->xy[0];
    BB.kaBottom = BB.kaTop = p1->xy[1];

    xy = p1->xy;
    for (i = 0; i < p1->nvertices; i++) {
        x = *xy++;
        y = *xy++;
        if (x > BB.kaRight)  BB.kaRight = x;
        if (x < BB.kaLeft)   BB.kaLeft = x;
        if (y > BB.kaTop)    BB.kaTop = y;
        if (y < BB.kaBottom) BB.kaBottom = y;
    }
    RefY = (BB.kaBottom + BB.kaTop)/2;

    PolygonClip(p1,BB.kaLeft,BB.kaBottom,BB.kaRight,RefY);
    while (NewPolygon(&p2))
        out_poly((struct p*)NULL,&p2);

    PolygonClip(p1,BB.kaLeft,RefY,BB.kaRight,BB.kaTop);
    while (NewPolygon(&p2))
        out_poly((struct p*)NULL,&p2);
    free(p2.xy);
}


void
convert_pathtype(xe,ye,xb,yb,width)

int *xe,*ye;    /* coordinate of endpoint */
int xb,yb;      /* coordinate of previous or next point in path */
int width;      /* path width */
{
    double angle;
    double deltaX,deltaY;

    if (width == 0)
        return;
    else if (width < 0)
        width = -width;
    width /= 2;
    if (*xe == xb) {
        if (*ye > yb)
            *ye += width;
        else
            *ye -= width;
    }
    else if (*ye == yb) {
        if (*xe > xb)
            *xe += width;
        else
            *xe -= width;
    }
    else {
        deltaX = (double)(*xe - xb);
        deltaY = (double)(*ye - yb);
        angle = atan2(deltaY,deltaX);
        deltaX = (double)(width) * cos(angle);
        deltaY = (double)(width) * sin(angle);
        *xe += (int)(deltaX);
        *ye += (int)(deltaY);
    }
}


void
MallocFailed()

{
    if (CDStatusInt != CDMALLOCFAILED)
        return;
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


/*********************************************************************
 *
 * stream library
 *
 *********************************************************************/


struct STREAM_info streaminfo;

/***********************************************************************/
/* ******************************************************************* */
/* *                                                                 * */
/* *     The following functions will write a symbolic layout        * */
/* *     onto a disk file in STREAM format.  The information may     * */
/* *     then be transfered to a magnetic tape by using the 'dd'     * */
/* *     system command.                                             * */
/* *                                                                 * */
/* *     The order of the function calls is *not* arbitrary.         * */
/* *     The correct calling sequence is described below:            * */
/* *                                                                 * */
/* *      1) 'bgnlib' to begin library                               * */
/* *                                                                 * */
/* *      2) 'bgnstr' to begin structure definition                  * */
/* *                                                                 * */
/* *      3) 'bndry' or 'path' or 'sref' or 'aref' or 'text'         * */
/* *         or any combination thereof.                             * */
/* *                                                                 * */
/* *      4) 'endstr' to end structure definition                    * */
/* *                                                                 * */
/* *      5) 'endlib' to end library or 'bgnstr' to begin            * */
/* *         another structure definition                            * */
/* *                                                                 * */
/* ******************************************************************* */
/***********************************************************************/


/***********************************************************************/
/* function STRM_BEGINRECORD                                           */
/*      Function to begin record on StreamFile.                        */
/*      count     = the number of bytes that will be contained within  */
/*                  the record.                                        */
/*      type      = the STREAM record type.                            */
/*      datatype  = the type of data contained in the STREAM record.   */
/***********************************************************************/

void
strm_beginrecord(count, type, datatype)

int count;
int type;
int datatype;
{
    /*
     * count    = number of bytes in record
     * type     = type of record
     * datatype = type of data in data field
     */

    strm_intcopy(count);
    putc(type, StreamFile);
    putc(datatype, StreamFile);
    ++streaminfo.rec_count;
    streaminfo.byte_count += count;
}


/***********************************************************************/
/* function STRM_ADDATE                                                */
/*      Function to add date reference to the StreamFile.  The date    */
/*      is stored in a 'tm' structure that is used by CTIME(3).        */
/***********************************************************************/

void
strm_addate(datep)

struct tm *datep;
{
    /*
     * datep = pointer to the year spec. of the tm date structure.
     */

    strm_intcopy(datep->tm_year);
    strm_intcopy(datep->tm_mon);
    strm_intcopy(datep->tm_mday);
    strm_intcopy(datep->tm_hour);
    strm_intcopy(datep->tm_min);
    strm_intcopy(datep->tm_sec);
}


/***********************************************************************/
/* function STRM_ENTERASC_REC                                          */
/*      Function to enter a single ASCII record into StreamFile        */
/*      If 'conv' is set, conversion to upper case is performed.       */
/***********************************************************************/

void
strm_entrasc_rec(cp, type, conv)

char *cp;
int type;
int conv;
{
    int i = 0, j;
    char c;

    i = strlen(cp);
    if (i & 1) ++i;
    strm_beginrecord(i+4, type, 6);
    for (j = 0; j < i; ++j) {
        c = cp[j];
        if (conv) {
            if(c >= 'a' && c <= 'z')
                c -= 32;
        }
        putc(c, StreamFile);
    }
}


/***********************************************************************/
/* function STRM_ENTRASC                                               */
/*      Function to enter ascii string of 44 characters into the       */
/*      StreamFile.  If 'conv' is set, conversion to upper case is     */
/*      performed.                                                     */
/***********************************************************************/

void
strm_entrasc(cp, conv)

char *cp;
int conv;
{
    /* 
     * cp = pointer to character buffer.
     */
    int i;
    int n;
    char c;

    n = strlen(cp);
    for (i = 0; i < n; ++i) {
        c = *cp++;
        if (conv) {
            if (c >= 'a' && c <= 'z')
                c -= 32;
        }
        putc(c, StreamFile);
    }
    for (i = n; i < 44; ++i)
        putc('\0', StreamFile);
}


/***********************************************************************/
/* function BGNLIB                                                     */
/*      Function to begin new library on the StreamFile.               */
/***********************************************************************/

int
bgnlib(lib)

STRM_LIBRARY *lib;
{
    int version = 3;

    streaminfo.rec_count=0;            /* initialize for new library */
    streaminfo.struct_count=0;
    streaminfo.byte_count=0;
    streaminfo.level = 0;
    strm_beginrecord(6, HEADER, 2);
    strm_intcopy(version);
    strm_beginrecord(28, BGNLIB, 2);
    strm_addate(&(lib->lib_moddate));
    strm_addate(&(lib->lib_accessdate));
    strm_entrasc_rec(lib->lib_name, LIBNAME, TOUPR);
    if (lib->lib_lib1[0] != '\0' || lib->lib_lib2[0] != '\0') {
        strm_beginrecord(92, REFLIBS, 6);
        strm_entrasc(lib->lib_lib1, TOUPR);
        strm_entrasc(lib->lib_lib2, TOUPR);
    }
    if (lib->lib_font0[0] != '\0' || lib->lib_font1[0] != '\0'
        || lib->lib_font2[0] != '\0' || lib->lib_font3[0] != '\0') {
        strm_beginrecord(180, FONTS, 6);
        strm_entrasc(lib->lib_font0, TOUPR);
        strm_entrasc(lib->lib_font1, TOUPR);
        strm_entrasc(lib->lib_font2, TOUPR);
        strm_entrasc(lib->lib_font3, TOUPR);
    }
    if (lib->lib_attr[0] != '\0')
        strm_entrasc_rec(lib->lib_attr, ATTRTABLE, TOUPR);
    strm_beginrecord(6, GENERATIONS, 2);
    strm_intcopy(lib->lib_gen);
    strm_beginrecord(20, UNITS, 5);
    strm_dblcopy(lib->lib_uunit);
    strm_dblcopy(lib->lib_munit);
    return (0);
}


/***********************************************************************/
/* function ENDLIB                                                     */
/*      Function to end library on the StreamFile.                     */
/***********************************************************************/

int
endlib()

{
    int i, j;
    if(streaminfo.level != 0 && streaminfo.test) return(-1);
    strm_beginrecord(4, ENDLIB, 0);
    /* pad with nulls */
    i = (streaminfo.byte_count % 2048);
    for(j = 0; j < i; ++j)
    putc(0, StreamFile);
    return(0);
}


/***********************************************************************/
/* function BGNSTR                                                     */
/*      Function to structure definition in the StreamFile.            */
/***********************************************************************/

int
bgnstr(strp)

STRM_STRCT *strp;
{
    if(streaminfo.level != 0 && streaminfo.test) return(-1);
    streaminfo.level = 1;
    strm_beginrecord(28, BGNSTR, 2);
    strm_addate(&(strp->str_creatdate));
    strm_addate(&(strp->str_moddate));
    strm_entrasc_rec(strp->str_name, STRNAME, TOUPR);
    ++streaminfo.struct_count;
    return(0);
}


/***********************************************************************/
/* function ENDSTR                                                     */
/*      Function to end structure on the StreamFile.                   */
/***********************************************************************/

int
endstr()

{
    if(streaminfo.level != 1 && streaminfo.test) return(-1);
    streaminfo.level = 0;
    strm_beginrecord(4, ENDSTR, 0);
    return(0);
}


/***********************************************************************/
/* function STRM_ENDLMNT                                               */
/*      Function to end element stream on the StreamFile.              */
/***********************************************************************/

int
strm_endlmnt(propp)

STRM_PROPERTY *propp;
{
    int i,k;

    k = 0;
    if (streaminfo.level != 2 && streaminfo.test) return(-1);
    streaminfo.level = 1;
    i = propp->prp_npropval;
    while (i-- > 0) {
        strm_beginrecord(6, PROPATTR, 2);
        strm_intcopy(propp->prp_propattr[k]);
        strm_entrasc_rec(propp->prp_propval[k++], PROPVALUE, 0);
    }
    strm_beginrecord(4, ENDEL, 0);
    return (0);
}


/***********************************************************************/
/* function BNDRY                                                      */
/*      Function to enter boundary element into the StreamFile.        */
/***********************************************************************/

int
bndry(bndryp)

STRM_BOUNDARY *bndryp;
{
    int i;

    if (streaminfo.level != 1 && streaminfo.test) return (-1);
    streaminfo.level = 2;
    strm_beginrecord(4, BOUNDARY, 0);
    strm_beginrecord(6, LAYER, 2);
    strm_intcopy(bndryp->bnd_layer);
    strm_beginrecord(6, DATATYPE, 2);
    strm_intcopy(bndryp->bnd_datatype);
    if (bndryp->bnd_ncoord < 4) {
        fprintf(stderr,"less than four boundary coordinates\n");
        exit(1);
    }
    else {
        strm_beginrecord(8 * bndryp->bnd_ncoord + 4, XY, 3);
        for (i = 0; i < bndryp->bnd_ncoord + bndryp->bnd_ncoord; ++i) {
            strm_lngcopy(bndryp->bnd_xy[i]);
        }
    }
    i = strm_endlmnt(&(bndryp->bnd_prop));
    return (i);
}


/***********************************************************************/
/* function STRM_STRNSFM                                               */
/*      Function to enter structure transformation record              */
/***********************************************************************/

int
strm_strnsfm(trp)

STRM_TRANSFORM *trp;
{
    int i;

    i = 0;
    if (trp->trns_reflection == 1) i |= 0100000;
    if (trp->trns_abs_mag == 1) i |= 04;
    if (trp->trns_abs_angle == 1) i |= 02;
    strm_beginrecord(6, STRANS, 1);
    strm_intcopy(i);
    if (trp->trns_mag != 1) {
        strm_beginrecord(12, MAG, 5);
        strm_dblcopy(trp->trns_mag);
    }
    if (trp->trns_angle != 0) {
        strm_beginrecord(12, ANGLE, 5);
        strm_dblcopy(trp->trns_angle);
    }
    return (0);
}


/***********************************************************************/
/* function PATH                                                       */
/*      Function to enter path element into the StreamFile.            */
/***********************************************************************/

int
path(pathp)

STRM_PATH *pathp;
{
    int i;

    if (streaminfo.level != 1 && streaminfo.test) return(-1);
    streaminfo.level = 2;
    strm_beginrecord(4, PATH, 0);
    strm_beginrecord(6, LAYER, 2);
    strm_intcopy(pathp->pth_layer);
    strm_beginrecord(6, DATATYPE, 2);
    strm_intcopy(pathp->pth_datatype);
    strm_beginrecord(6, PATHTYPE, 2);
    strm_intcopy(pathp->pth_pathtype);
    strm_beginrecord(8, WIDTH, 3);
    strm_lngcopy(pathp->pth_width);
    if (pathp->pth_ncoord < 2) {
        fprintf(stderr,"less than two path points\n");
        exit(1);
    }
    else  {
        strm_beginrecord(8 * pathp->pth_ncoord + 4, XY, 3);
        for (i = 0; i < pathp->pth_ncoord + pathp->pth_ncoord; ++i) {
            strm_lngcopy(pathp->pth_xy[i]);
        }
    }
    i = strm_endlmnt(&(pathp->pth_prop));
    return (i);
}


/***********************************************************************/
/* function SREF                                                       */
/*      Function to enter structure reference to the StreamFile.       */
/***********************************************************************/

int
sref(srefp)

STRM_SREF *srefp;
{
    int i;

    if (streaminfo.level != 1 && streaminfo.test) return(-1);
    streaminfo.level = 2;
    strm_beginrecord(4, SREF, 0);                  /* enter SREF record */
    strm_entrasc_rec(srefp->sr_name, SNAME, TOUPR); /* enter SNAME record */
    strm_strnsfm(&(srefp->sr_transform));          /* enter STRANS record */
    strm_beginrecord(12, XY, 3);                   /* enter XY record */
    strm_lngcopy(srefp->sr_xy[0]);
    strm_lngcopy(srefp->sr_xy[1]);
    i = strm_endlmnt(&(srefp->sr_prop));
    return (i);
}


/***********************************************************************/
/* function AREF                                                       */
/*      Function to enter array reference element in the StreamFile.   */
/***********************************************************************/

int
aref(arefp)

STRM_AREF *arefp;
{
    int *ip;
    int i;

    if (streaminfo.level != 1 && streaminfo.test) return(-1);
    streaminfo.level = 2;
    strm_beginrecord(4, AREF, 0);                  /* enter AREF record */
    strm_entrasc_rec(arefp->ar_name, SNAME, TOUPR); /* enter SNAME record */
    strm_strnsfm(&(arefp->ar_transform));          /* enter STRANS record */
    strm_beginrecord(8, COLROW, 2);                /* enter COLROW record */
    strm_intcopy(arefp->ar_col);
    strm_intcopy(arefp->ar_row);
    strm_beginrecord(28, XY, 3);                   /* enter XY record */
    ip = arefp->ar_xy;
    for (i = 1; i <= 6; ++i) {
        strm_lngcopy(*ip);
        ++ip;
    }
    i = strm_endlmnt(&(arefp->ar_prop));
    return (i);
}


/***********************************************************************/
/* function TEXT                                                       */
/*      Function to enter text element into the StreamFile.            */
/***********************************************************************/

int
text(textp)

STRM_TEXT *textp;
{
    int i;

    if (streaminfo.level != 1 && streaminfo.test) return(-1);
    streaminfo.level = 2;
    strm_beginrecord(4, TEXT, 0);                  /* enter TEXT record */
    strm_beginrecord(6, LAYER, 2);                 /* enter LAYER record */
    strm_intcopy(textp->txt_layer);
    strm_beginrecord(6, TEXTTYPE, 2);              /* enter TEXTTYPE record */
    strm_intcopy(textp->txt_texttype);
    if (textp->txt_horizontal >= 0 && textp->txt_horizontal < 3) {
        if (textp->txt_vertical >= 0 && textp->txt_vertical < 3) {
            if (textp->txt_font >= 0 && textp->txt_font <= 3) {
                i = textp->txt_horizontal + (textp->txt_vertical << 2)
                    + (textp->txt_font << 4);
                strm_beginrecord(6, PRESENTATION, 1);
                strm_intcopy(i);
            }
        }
    }
    strm_beginrecord(6, PATHTYPE, 2);              /* enter PATHTYPE record */
    strm_intcopy(textp->txt_pathtype);
    strm_beginrecord(8, WIDTH, 3);
    strm_lngcopy(textp->txt_width);
    strm_strnsfm(&(textp->txt_transform));         /* enter STRANS record */
    strm_beginrecord(12, XY, 3);                   /* enter XY record */
    strm_lngcopy(textp->txt_xy[0]);
    strm_lngcopy(textp->txt_xy[1]);
    strm_entrasc_rec(textp->txt_text, STRING, 0);
    i = strm_endlmnt(&(textp->txt_prop));
    return (i);
}

/*
 * function STRM_INTCOPY
 *      Function to transfer the two bytes of a short integer to a
 *      Stream file.  The most significant byte is sent first.
 */

void
strm_intcopy(i)

int i;
{
    putc((char)((i >> 8) & 0xff),StreamFile);
    putc((char)(i & 0xff),StreamFile);
}


/*
 * function STRM_LNGCOPY
 *      Function to transfer the four bytes of a long integer to a
 *      Stream file.  The most significant byte is sent first.
 */

void
strm_lngcopy(i)

int i;
{
    putc((char)((i >> 24) & 0xff),StreamFile);
    putc((char)((i >> 16) & 0xff),StreamFile);
    putc((char)((i >> 8) & 0xff),StreamFile);
    putc((char)((i) & 0xff),StreamFile);
}


/*
 * function STRM_DBLCOPY
 *    Function to transfer double precision number to a character buffer.
 *    The first character of the buffer will contain the exponent field,
 *    the second character will contain the most significant byte of the
 *    mantissa field, etc.
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
 * This routine is really inefficient, but there are typically few
 * conversions required and portability is an issue.
 */
void
strm_dblcopy(r)

double r;
{
    int i, exp, sign;
    unsigned char b[8];

    if (r == 0.0) {
        for (i = 0; i < 8; i++)
            b[i] = 0;
        goto xfer;
    }
    sign = 0;
    if (r < 0) {
        sign = 1;
        r = -r;
    }

    /* normalize to 1/16 < r <= 1 */
    i = 0;
    if (r >= 1.0) {
        while (r >= 1.0) {
            r /= 16.0;
            i++;
        }
        if (i > 63) {
            /* overflow */
            for (i = 0; i < 8; i++)
                b[i] = 0xff;
            if (!sign)
                b[0] &= 0x7f;
            goto xfer;
        }
        exp = i + 64;
    }
    else if (r < 1/16.0) {
        while (r < 1/16.0 && i < 64) {
            r *= 16.0;
            i++;
        }
        if (i > 63) {
            /* underflow */
            for (i = 0; i < 8; i++)
                b[i] = 0;
            goto xfer;
        }
        exp = 64 - i;
    }
    else
        exp = 64;
    for (i = 1; i <= 7; i++) {
        r *= 256.0;
        b[i] = r;
        r -= b[i];
    }
    b[0] = exp;
    if (sign)
        b[0] |= 0x80;

xfer:
    /* transfer bytes to char buffer */
    putc(b[0], StreamFile);
    putc(b[1], StreamFile);
    putc(b[2], StreamFile);
    putc(b[3], StreamFile);
    putc(b[4], StreamFile);
    putc(b[5], StreamFile);
    putc(b[6], StreamFile);
    putc(b[7], StreamFile);
}


/*****  Alias For DOS **************************************************/

#ifdef MSDOS

/* Convert back to original stream structure names, if alias file is
 * present.
 */

struct aliastab  *aliasbase;

char *
unalias(dosname)

char *dosname;
{
    int i;
    struct aliastab *wl;

    for (i = 0,wl = aliasbase; wl; wl = wl->next) {
        if (!strcasecmp(dosname,wl->dosname))
            return (wl->strname);
    }
    return (dosname);
}


void
readalias()

{
    FILE *fp;
    char s[80], *c;
    struct aliastab *wl;

    if ((fp = fopen(ALIASFILE,"r")) == NULL) return;
    printf("Using alias file\n");
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

/***** Memory Allocation ***********************************************/

char *
tmalloc(x)

unsigned x;
{
    char *c = (char*)malloc(x);

    if (c == NULL)
        fprintf(stderr,"Out of memory");
    return (c);
}    

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
#include "stream.h"

/* Define to 1 if GDS symbol and lib names are to be converted to
 * upper case.  This was true in the previous versions of Kic.
 */
#define TOUPR 0

extern char *MenuHELP;
extern char *MenuCNVRT;
char *MenuTOGDS = "togds";
char *MenuTOCIF = "tocif";
char *MenuFRGDS = "frgds";
char *MenuFRCIF = "frcif";

static FILE *logfp;

#define Matching(string) (!strcmp(string,Parameters.kpCommand))
#define GDS_SUFFIX ".gds"


#if __STDC__
static int  clobber_check(int);
static int  gen_stream(char*,struct s*,int*);
static int  gen_stream_call(char*,struct s*);
static int  gen_stream_struc(struct s*);
static void finish_path(int);
static void out_poly(struct p*,Poly*);
static void break_poly(Poly*);
static void reset_info(struct s*);
static char *unalias(char*);
static void freealias(void);
static void strm_beginrecord(int,int,int);
static void strm_addate(struct tm*);
static void strm_entrasc_rec(char*,int,int);
static void strm_entrasc(char*,int);
static int  bgnlib(STRM_LIBRARY*);
static int  endlib(void);
static int  bgnstr(STRM_STRCT*);
static int  endstr(void);
static int  strm_endlmnt(STRM_PROPERTY*);
static int  bndry(STRM_BOUNDARY*);
static int  strm_strnsfm(STRM_TRANSFORM*);
static int  path(STRM_PATH*);
static int  sref(STRM_SREF*);
static int  aref(STRM_AREF*);
static int  text(STRM_TEXT*);
static void strm_intcopy(int);
static void strm_lngcopy(int);
static void strm_dblcopy(double);
static char get_cif_file_type(FILE*);
#else
static int  clobber_check();
static int  gen_stream();
static int  gen_stream_call();
static int  gen_stream_struc();
static void finish_path();
static void out_poly();
static void break_poly();
static void reset_info();
static char *unalias();
static void freealias();
static void strm_beginrecord();
static void strm_addate();
static void strm_entrasc_rec();
static void strm_entrasc();
static int  bgnlib();
static int  endlib();
static int  bgnstr();
static int  endstr();
static int  strm_endlmnt();
static int  bndry();
static int  strm_strnsfm();
static int  path();
static int  sref();
static int  aref();
static int  text();
static void strm_intcopy();
static void strm_lngcopy();
static void strm_dblcopy();
static char get_cif_file_type();
#endif

void
Convert()

{
    int dummy;
    char OldMenu;
    char *logfile = "convert.log";

    OldMenu = Parameters.kpMenu;
    Parameters.kpMenu = AMBIGUITYMENU;

    AmbiguityMenu[0].mEntry = MenuHELP;
    AmbiguityMenu[1].mEntry = MenuTOGDS;
    AmbiguityMenu[2].mEntry = MenuTOCIF;
    AmbiguityMenu[3].mEntry = MenuFRGDS;
    AmbiguityMenu[4].mEntry = MenuFRCIF;
    AmbiguityMenu[5].mEntry = NULL;
    FixMenuPrefix(AmbiguityMenu);
    ShowMenu(AmbiguityMenu);

    loop {
        switch (PointLoopSafe(&dummy)) {
        case PL_ESC:
            break;
        case PL_PCW:
            continue;
        case PL_CMD:
            if (Matching(MenuHELP))  {
                Help();
                continue;
            }
            if (Matching(MenuTOGDS)) {
                logfp = fopen(logfile,"w");
                ToGDSII();
                if (logfp) fclose(logfp);
                break;
            }
            if (Matching(MenuFRGDS)) {
                if (clobber_check(True)) break;
                logfp = fopen(logfile,"w");
                FromGDSII();
                if (logfp) fclose(logfp);
                break;
            }
            if (Matching(MenuTOCIF)) {
                logfp = fopen(logfile,"w");
                ToCIF();
                if (logfp) fclose(logfp);
                break;
            }
            if (Matching(MenuFRCIF)) {
                if (clobber_check(True)) break;
                logfp = fopen(logfile,"w");
                FromCIF();
                if (logfp) fclose(logfp);
                break;
            }
            continue;
        }
        break;
    }
    Parameters.kpMenu = OldMenu;
    ShowCommandMenu();
}


void
OutPrompt(s)

char *s;
{
    char c;

    if (logfp) (void)fprintf(logfp,"%s\n",s);
    if (strlen(s) > FB.fNumColumns - 8) {
        /* avoid "MORE" prompt */
        c = s[FB.fNumColumns - 8];
        s[FB.fNumColumns - 8] = '\0';
        ShowPrompt(s);
        s[FB.fNumColumns - 8] = c;
    }
    else
        ShowPrompt(s);

}


static int
clobber_check(dospeak)

int dospeak;
{
    char *TypeIn;

    ShowPrompt(
        "Warning, conversion may overwrite file(s).  OK to continue? (n)");
    TypeIn = FBEdit(NULL);
    if (TypeIn != NULL) {
        if (*TypeIn == 'y' || *TypeIn == 'Y')
            return (False);
    }
    if (dospeak)
        ShowPrompt(
            "Move to a clear directory before performing conversion.");
    return (True);
}



/*************************************************************************
 *
 * GDSII conversion (also see convert1.c).
 *
 *************************************************************************/

#define DATA_TYPE(a) (((a)<0||(a)>255)?0:(a))

/*
#define LONGSCALE(n)  (int) (floor( (ScaleFactor * (double)(n)) + 0.5))
*/
#define LONGSCALE(n) (n)

static STRM_LIBRARY     *STRMLibrary;
static STRM_STRCT       *STRMStructure;
static STRM_BOUNDARY    *STRMBoundary;
static STRM_PATH        *STRMPath;
static STRM_TEXT        *STRMText;
static STRM_SREF        *STRMSref;
static STRM_AREF        *STRMAref;
static STRM_TRANSFORM   *DefaultTransform;
static STRM_PROPERTY    *DefaultProperty;
static int              StrmNoGo;

static FILE *StreamFile;


void
ToGDSII()
{
    struct tm Now, *localtime();
    struct prpty *PDesc;
    time_t tloc;
    int SymbolNum = 0;
    char StreamFileName[81];
    char *s;

    MenuSelect(MenuTOGDS);

    STRMLibrary = (STRM_LIBRARY *)tmalloc(sizeof(STRM_LIBRARY));
    STRMStructure = (STRM_STRCT *)tmalloc(sizeof(STRM_STRCT));
    STRMBoundary = (STRM_BOUNDARY *)tmalloc(sizeof(STRM_BOUNDARY));
    STRMPath = (STRM_PATH *)tmalloc(sizeof(STRM_PATH));
    STRMText = (STRM_TEXT *)tmalloc(sizeof(STRM_TEXT));
    STRMSref = (STRM_SREF *)tmalloc(sizeof(STRM_SREF));
    STRMAref = (STRM_AREF *)tmalloc(sizeof(STRM_AREF));
    DefaultTransform = (STRM_TRANSFORM *)tmalloc(sizeof(STRM_TRANSFORM));
    DefaultProperty = (STRM_PROPERTY *)tmalloc(sizeof(STRM_PROPERTY));

    /* initialize */
    tloc                          = time(NULL);
    Now                           = *localtime(&tloc);
    Now.tm_mon++;

    StrmNoGo                      = False;
    STRMLibrary->lib_name[0]      = '\0';
    STRMLibrary->lib_lib1[0]      = '\0';
    STRMLibrary->lib_lib2[0]      = '\0';
    STRMLibrary->lib_font0[0]     = '\0';
    STRMLibrary->lib_font1[0]     = '\0';
    STRMLibrary->lib_font2[0]     = '\0';
    STRMLibrary->lib_font3[0]     = '\0';
    STRMLibrary->lib_attr[0]      = '\0';
    STRMLibrary->lib_accessdate   = Now;
    STRMLibrary->lib_moddate      = Now;
    STRMLibrary->lib_uunit        = .01;
    STRMLibrary->lib_munit        = 1e-8;
    STRMLibrary->lib_gen          = 3;

    STRMStructure->str_moddate    = Now;
    STRMStructure->str_creatdate  = Now;

    DefaultTransform->trns_reflection = 0;
    DefaultTransform->trns_abs_mag    = 0;
    DefaultTransform->trns_abs_angle  = 0;
    DefaultTransform->trns_mag        = 1.0;
    DefaultTransform->trns_angle      = 0.0;
    DefaultProperty->prp_npropval     = 0;

    ShowPrompt("Stream file name? ");
    strcpy(StreamFileName,Parameters.kpCellName);
    if ((s = strchr(StreamFileName,'.')) != NULL) *s = '\0';
    strcat(StreamFileName, GDS_SUFFIX);
    s = FBEdit(StreamFileName);
    if (s == NULL) goto quit;
    if (*s != '\0' && *s != '\n')
        strcpy(StreamFileName,s);

    strcpy(STRMLibrary->lib_name,"KICTOSTREAM");

    if (!access(StreamFileName,0))
        if (clobber_check(False))
            return;

    StreamFile = POpen(StreamFileName,"wb",(char *)NULL,(char **)NULL);
    if (StreamFile == NULL) {
        ShowPrompt("Can't open STREAM file.");
        goto quit;
    }

    readalias();

    /* add STREAM-specific property list info */
/*
 * We use the property list of KIC symbols to save the library information;
 * The value of the property is the numeric value of the STREAM record
 * type offset by 7000 (e.g. 7000 is the KIC property value describing the
 * STREAM version number, 7002 is the KIC property value describing the
 * STREAM library name, etc.)  The offset of 7000 was arbitrarily selected,
 * and care must be taken so that this value does not conflict with any
 * other convention.  The PROPERTYOFFSET define is for convenience.
 */
    for (PDesc = Parameters.kpCellDesc->sPrptyList; PDesc;
            PDesc = PDesc->prpty_Succ) {

        switch (PDesc->prpty_Value - PROPERTYOFFSET) {
        case HEADER:
            /* version 3 is assumed */
            break;
        case LIBNAME:
            sscanf(PDesc->prpty_String,"%s",STRMLibrary->lib_name);
            break;
        case REFLIBS:
            sscanf(PDesc->prpty_String,"%s %s",STRMLibrary->lib_lib1,
                STRMLibrary->lib_lib2);
            break;
        case FONTS:
            sscanf(PDesc->prpty_String,"%s %s %s %s",
                STRMLibrary->lib_font0,STRMLibrary->lib_font1,
                STRMLibrary->lib_font2,STRMLibrary->lib_font3);
            break;
        case GENERATIONS:
            sscanf(PDesc->prpty_String,"%d",&STRMLibrary->lib_gen);
            break;
        case ATTRTABLE:
            sscanf(PDesc->prpty_String,"%s",STRMLibrary->lib_attr);
            break;
        }
    }

    bgnlib(STRMLibrary);
    SymbolNum = 1;
    if (!gen_stream(Parameters.kpCellName,Parameters.kpCellDesc,&SymbolNum)) {
        ShowPrompt("Unable to write stream file.  MORE");
        (void) FBGetchar(ERASE);
        ShowPrompt(CDStatusString);
        fclose(StreamFile);
        goto quit;
    }
    endlib();
    fclose(StreamFile);
    if (StrmNoGo) {
        sprintf(TypeOut,"Translation of %s failed.",Parameters.kpCellName);
        unlink(StreamFileName);
    }
    else
        sprintf(TypeOut,"Translation of %s succeeded.",Parameters.kpCellName);
    OutPrompt(TypeOut);

quit:
    freealias();
    reset_info(Parameters.kpCellDesc);
    free(STRMLibrary);
    free(STRMStructure);
    free(STRMBoundary);
    free(STRMPath);
    free(STRMText);
    free(STRMSref);
    free(STRMAref);
    free(DefaultTransform);
    free(DefaultProperty);
    MenuDeselect(MenuTOGDS);
}


static int
gen_stream(MasterName,SymbolDesc,SymbolNum)

char *MasterName;
struct s *SymbolDesc;
int *SymbolNum;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct s *MasterDesc;
    char *SymbolName;

    sprintf(TypeOut,"Converting: %s",MasterName);
    OutPrompt(TypeOut);

    (*SymbolNum)++;
    /*
     * Mark symbol associated with SymbolDesc as visited by storing
     * its symbol # in its info field.
     */
    SymbolDesc->sInfo = *SymbolNum;

    /*
     * First write to the stream file any symbol definitions below
     * the symbol associated with SymbolDesc.
     */
    if (!CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return CDError(CDMALLOCFAILED);

    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL) break;
        SymbolName = ((struct c *)Pointer->oRep)->cMaster->mName;
        if (Not CDOpen(SymbolName,&MasterDesc,'w'))
            return False;
        if (MasterDesc->sInfo == 0) {
            /* Write master's definition to stream file. */
            if (!gen_stream(SymbolName,MasterDesc,SymbolNum))
                return False;
        }
    }
    if (!gen_stream_call(MasterName,SymbolDesc))
        return False;
    if (!gen_stream_struc(SymbolDesc))
        return False;
    return True;
}


static int
gen_stream_call(MasterName,SymbolDesc)

char *MasterName;
struct s *SymbolDesc;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct t *TGen;
    struct s *MasterDesc;
    struct prpty *PDesc;
    int X,Y;
    int DX,DY;
    int NumX,NumY;
    int i;
    char Type;
    char *SymbolName;
    double a,b;

    strcpy(STRMStructure->str_name,unalias(MasterName));
    bgnstr(STRMStructure);
    if (!CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return CDError(CDMALLOCFAILED);

    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL) break;
        CDCall(Pointer,&SymbolName,&NumX,&DX,&NumY,&DY);
        if (!CDOpen(SymbolName,&MasterDesc,'w'))
            return False;

        if (NumX > 1 || NumY > 1) {
            /* create an AREF */

            strcpy(STRMAref->ar_name,unalias(SymbolName));
            STRMAref->ar_transform = *DefaultTransform;
            STRMAref->ar_col = NumX;
            STRMAref->ar_row = NumY;
            STRMAref->ar_xy[0] = 0;
            STRMAref->ar_xy[1] = 0;

            /* copy the property list */
            for (PDesc = Pointer->oPrptyList; PDesc;
                    PDesc = PDesc->prpty_Succ) {

                /* allow only valid STREAM property attributes */
                i = PDesc->prpty_Value;
                if (i > 0 && i < 128) {
                    STRMAref->ar_prop.prp_propattr
                        [STRMAref->ar_prop.prp_npropval] = i;
                    STRMAref->ar_prop.prp_propval
                        [STRMAref->ar_prop.prp_npropval] =
                        CopyString(PDesc->prpty_String);
                    ++STRMAref->ar_prop.prp_npropval;
                    if (STRMAref->ar_prop.prp_npropval >= 255) break;
                }
            }

            STRMAref->ar_transform.trns_angle = 0;
            STRMAref->ar_transform.trns_reflection = 0;
            CDInitTGen(Pointer,&TGen);
            TPush();
            TIdentity();
            loop {
                CDTGen(&TGen,&Type,&X,&Y);
                if (TGen == NULL) break;
                elif (Type == CDROTATE) {
                    a = (double) X;
                    b = (double) Y;
                    STRMAref->ar_transform.trns_angle += 
                        (RADTODEG * atan2(b,a));
                    TRotate(X,Y);
                }
                elif (Type == CDTRANSLATE) {
                    TTranslate(X,Y);
                }
                elif (Type == CDMIRRORX) {
                    STRMAref->ar_transform.trns_reflection ^= 1;
                    STRMAref->ar_transform.trns_angle += 180.0;
                    TMX();
                }
                elif (Type == CDMIRRORY) {
                    STRMAref->ar_transform.trns_reflection ^= 1;
                    TMY();
                }
            }
            TPremultiply();
            STRMAref->ar_xy[0] = 0;
            STRMAref->ar_xy[1] = 0;
            STRMAref->ar_xy[2] = DX*NumX;
            STRMAref->ar_xy[3] = 0;
            STRMAref->ar_xy[4] = 0;
            STRMAref->ar_xy[5] = DY*NumY;
            TPoint(&STRMAref->ar_xy[0],&STRMAref->ar_xy[1]);
            TPoint(&STRMAref->ar_xy[2],&STRMAref->ar_xy[3]);
            TPoint(&STRMAref->ar_xy[4],&STRMAref->ar_xy[5]);
            TPop();
            aref(STRMAref);
            
            /* free storage of property values */
            for (i = 0; i < STRMAref->ar_prop.prp_npropval; i++)
                free(STRMAref->ar_prop.prp_propval[i]);
            STRMAref->ar_prop.prp_npropval = 0;
        }
        else {
            /* create an SREF */

            strcpy(STRMSref->sr_name,unalias(SymbolName));
            STRMSref->sr_transform = *DefaultTransform;
            STRMSref->sr_xy[0] = STRMSref->sr_xy[1] = 0;

            /* copy the property list */
            for (PDesc = Pointer->oPrptyList; PDesc;
                    PDesc = PDesc->prpty_Succ) {

                /* allow only valid STREAM property attributes */
                i = PDesc->prpty_Value;
                if (i > 0 && i < 128) {
                    STRMSref->sr_prop.prp_propattr
                        [STRMSref->sr_prop.prp_npropval] = i;
                    STRMSref->sr_prop.prp_propval
                        [STRMSref->sr_prop.prp_npropval] =
                        CopyString(PDesc->prpty_String);
                    ++STRMSref->sr_prop.prp_npropval;
                    if (STRMSref->sr_prop.prp_npropval >= 255) break;
                }
            }
            STRMSref->sr_transform.trns_angle = 0;
            STRMSref->sr_transform.trns_reflection = 0;
            CDInitTGen(Pointer,&TGen);
            loop {
                CDTGen(&TGen,&Type,&X,&Y);
                if (TGen == NULL) break;
                elif (Type == CDROTATE) {
                    a = (double) X;
                    b = (double) Y;
                    STRMSref->sr_transform.trns_angle += 
                        (RADTODEG * atan2(b,a));
                }
                elif (Type == CDTRANSLATE) {
                    STRMSref->sr_xy[0] = LONGSCALE(X);
                    STRMSref->sr_xy[1] = LONGSCALE(Y);
                }
                elif (Type == CDMIRRORX) {
                    STRMSref->sr_transform.trns_reflection ^= 1;
                    STRMSref->sr_transform.trns_angle += 180.0;
                }
                elif (Type == CDMIRRORY)
                    STRMSref->sr_transform.trns_reflection ^= 1;
            }
            sref(STRMSref);
            
            /* free storage of property values */
            for (i = 0; i < STRMSref->sr_prop.prp_npropval; i++)
                free(STRMSref->sr_prop.prp_propval[i]);
            STRMSref->sr_prop.prp_npropval = 0;
        }
    }
    return True;
}


static int
gen_stream_struc(SymbolDesc)

struct s *SymbolDesc;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct p *Pair,*Path;
    struct prpty *PDesc;
    int X,Y,Length,Width;
    int Layer,i;
    char Type,Xform;
    char garbage[80],*Label;

    for (Layer = 1; Layer <= NumLayerTable; Layer++) {
        if (!CDInitGen(SymbolDesc,Layer,-CDINFINITY,-CDINFINITY,
            CDINFINITY,CDINFINITY,&GenDesc)) return CDError(CDMALLOCFAILED);

        loop {
            CDGen(SymbolDesc,GenDesc,&Pointer);
            if (Pointer == NULL) break;

            /* copy the property list */
            for (PDesc = Pointer->oPrptyList; PDesc;
                    PDesc = PDesc->prpty_Succ) {
                i = PDesc->prpty_Value;
                /* allow only valid STREAM property attributes */
                if (i > 0 && i < 128) {
                    DefaultProperty->prp_propattr
                        [DefaultProperty->prp_npropval] = i;
                    DefaultProperty->prp_propval
                        [DefaultProperty->prp_npropval] =
                        CopyString(PDesc->prpty_String);
                    ++DefaultProperty->prp_npropval;
                    if (DefaultProperty->prp_npropval >= 255)
                        break;
                }
            }
            Type = Pointer->oType;
            if (Type == CDBOX) {
                CDBox(Pointer,&Layer,&Length,&Width,&X,&Y);
                Length /= 2;
                Width /= 2;
                X = LONGSCALE(X);
                Y = LONGSCALE(Y);
                Length = LONGSCALE(Length);
                Width = LONGSCALE(Width);
                STRMBoundary->bnd_layer = LayerTable[Layer].klStreamNumber;
                STRMBoundary->bnd_datatype =
                    DATA_TYPE(LayerTable[Layer].klStreamDataType);
                STRMBoundary->bnd_prop = *DefaultProperty;
                STRMBoundary->bnd_ncoord = 5;
                STRMBoundary->bnd_xy[0] = (X + Length);
                STRMBoundary->bnd_xy[1] = (Y + Width);
                STRMBoundary->bnd_xy[2] = (X + Length);
                STRMBoundary->bnd_xy[3] = (Y - Width);
                STRMBoundary->bnd_xy[4] = (X - Length);
                STRMBoundary->bnd_xy[5] = (Y - Width);
                STRMBoundary->bnd_xy[6] = (X - Length);
                STRMBoundary->bnd_xy[7] = (Y + Width);
                STRMBoundary->bnd_xy[8] = (X + Length);
                STRMBoundary->bnd_xy[9] = (Y + Width);
                bndry(STRMBoundary);
            }
            elif (Type == CDWIRE) {
                CDWire(Pointer,&Layer,&Width,&Path);
                STRMPath->pth_layer = LayerTable[Layer].klStreamNumber;
                STRMPath->pth_datatype =
                    DATA_TYPE(LayerTable[Layer].klStreamDataType);
                STRMPath->pth_width = LONGSCALE(Width);
                STRMPath->pth_prop = *DefaultProperty;
                STRMPath->pth_pathtype = 2;
                /* look at the property list for a STREAM pathtype definition */
                for (PDesc = Pointer->oPrptyList; PDesc;
                        PDesc = PDesc->prpty_Succ) {
                    if (PDesc->prpty_Value - PROPERTYOFFSET == PATHTYPE) {
                        sscanf(PDesc->prpty_String,"%s %d",garbage,&i);
                        if (i >= 0 && i < 3)
                            STRMPath->pth_pathtype = i;
                        break;
                    }
                }
                Pair = Path;
                i = 0;
                while (Pair != NULL) {
                    STRMPath->pth_xy[i++] = LONGSCALE(Pair->pX);
                    STRMPath->pth_xy[i++] = LONGSCALE(Pair->pY);
                    if (i == 2 && !Pair->pSucc) {
                        STRMPath->pth_xy[i++] = LONGSCALE(Pair->pX);
                        STRMPath->pth_xy[i++] = LONGSCALE(Pair->pY);
                    }
                    Pair = Pair->pSucc;
                    if (i == MAXSTRMCOORDS && Pair) {
                        OutPrompt("Breaking wire with too many vertices.");
                        finish_path(i);
                        i = 0;
                    }
                }
                finish_path(i);
            }
            elif (Type == CDPOLYGON) {
                CDPolygon(Pointer,&Layer,&Path);
                STRMBoundary->bnd_layer = LayerTable[Layer].klStreamNumber;
                STRMBoundary->bnd_datatype =
                    DATA_TYPE(LayerTable[Layer].klStreamDataType);
                STRMBoundary->bnd_prop = *DefaultProperty;
                out_poly(Path, (Poly*)NULL);
            }
            elif (Type == CDLABEL) {
                CDLabel(Pointer,&Layer,&Label,&X,&Y,&Xform);
                STRMText->txt_layer = LayerTable[Layer].klStreamNumber;
                STRMText->txt_texttype =
                    DATA_TYPE(LayerTable[Layer].klStreamDataType);
                STRMText->txt_pathtype = 1;
                STRMText->txt_horizontal = 0;
                STRMText->txt_vertical = 0;
                STRMText->txt_font = 0;
                STRMText->txt_width = 0;
                STRMText->txt_xy[0] = LONGSCALE(X);
                STRMText->txt_xy[1] = LONGSCALE(Y);
                STRMText->txt_prop = *DefaultProperty;
                STRMText->txt_transform = *DefaultTransform;
                STRMSref->sr_xy[0] = STRMSref->sr_xy[1] = 0;
                memcpy(STRMText->txt_text,Label,44);
                STRMText->txt_text[44] = '\0';
                /* look at the property list for a STREAM text definition */
                for (PDesc = Pointer->oPrptyList; PDesc;
                        PDesc = PDesc->prpty_Succ) {
                    int wdth;
                    int present,ptype,reflection;
                    double magn,rotn;
                    if (PDesc->prpty_Value - PROPERTYOFFSET == TEXT) {
                        sscanf(PDesc->prpty_String,
                        "%s %d %s %d %s %d %s %lf %s %lf %s %d",
                        garbage,&wdth,garbage,&present,garbage,&ptype,
                        garbage,&magn,garbage,&rotn,garbage,&reflection);
                        STRMText->txt_pathtype = ptype;
                        STRMText->txt_horizontal = (present >> 8) & 3;
                        STRMText->txt_vertical = (present >> 10) & 3;
                        STRMText->txt_font = (present >> 12) & 3;
                        STRMText->txt_width = wdth;
                        STRMText->txt_transform.trns_reflection = reflection;
                        STRMText->txt_transform.trns_mag = magn;
                        STRMText->txt_transform.trns_angle = rotn;
                        break;
                    }
                }
                text(STRMText);
            }
            /* free storage of property values */
            for (i = 0; i < DefaultProperty->prp_npropval; i++)
                free(DefaultProperty->prp_propval[i]);
            DefaultProperty->prp_npropval = 0;
        }
    }
    endstr();
    return True;
}


static void
finish_path(i)

int i;
{
    if (STRMPath->pth_pathtype == 0 && i > 3) {
        ConvertPathtype(&STRMPath->pth_xy[0],&STRMPath->pth_xy[1],
            STRMPath->pth_xy[2],STRMPath->pth_xy[3],
            STRMPath->pth_width,1);
        ConvertPathtype(&STRMPath->pth_xy[i-1],&STRMPath->pth_xy[i-2],
            STRMPath->pth_xy[i-3],STRMPath->pth_xy[i-4],
            STRMPath->pth_width,1);
    }
    STRMPath->pth_ncoord = i/2;
    path(STRMPath);
}


static void
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
        OutPrompt("Breaking polygon with too many vertices.");
        break_poly(&p1);
    }
    else {
        xy = p1.xy;
        bxy = STRMBoundary->bnd_xy;
        for (i = 0; i < p1.nvertices; i++) {
            *bxy++ = LONGSCALE(*xy++);
            *bxy++ = LONGSCALE(*xy++);
        }
        if (i < 4) {
            /* close the triangle */
            *bxy++ = STRMBoundary->bnd_xy[0];
            *bxy++ = STRMBoundary->bnd_xy[1];
            i++;
        }
        STRMBoundary->bnd_ncoord = i;
        bndry(STRMBoundary);
    }
    if (!po)
        free(p1.xy);
}


static void
break_poly(p1)

/* Break a polygon across the middle of its BB horizontally and
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
ConvertPathtype(xe,ye,xb,yb,width,togds)

int *xe,*ye;      /* coordinate of endpoint */
int xb,yb;        /* coordinate of previous or next point in path */
int width;        /* path width */
int togds;
{
    double angle;
    double deltaX,deltaY;

    if (width == 0) return;
    if (width < 0) width = -width;
    width /= 2;
    if (togds) {
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
    else {
        if (*xe == xb) {
            if (*ye > yb)
                *ye -= width;
            else
                *ye += width;
        }
        else if (*ye == yb) {
            if (*xe > xb)
                *xe -= width;
            else
                *xe += width;
        }
        else {
            deltaX = (double)(*xe - xb);
            deltaY = (double)(*ye - yb);
            angle = atan2(deltaY,deltaX);
            deltaX = (double)(width) * cos(angle);
            deltaY = (double)(width) * sin(angle);
            *xe -= (int)(deltaX);
            *ye -= (int)(deltaY);
        }
    }
}


static void
reset_info(SymbolDesc)

/* reset the info parameter to zero */
struct s *SymbolDesc;
{
    struct g *GenDesc;
    struct o *Pointer;
    struct s *MasterDesc;
    char *SymbolName;

    SymbolDesc->sInfo = 0;

    if (!CDInitGen(SymbolDesc,0,-CDINFINITY,-CDINFINITY,CDINFINITY,
        CDINFINITY,&GenDesc)) return;

    loop {
        CDGen(SymbolDesc,GenDesc,&Pointer);
        if (Pointer == NULL) break;
        SymbolName = ((struct c *)Pointer->oRep)->cMaster->mName;
        if (Not CDOpen(SymbolName,&MasterDesc,'w')) return;
        if (MasterDesc->sInfo != 0)
            reset_info(MasterDesc);
    }
}

#ifdef MSDOS
static struct aliastab *aliasbase;
#endif

static char *
unalias(dosname)

char *dosname;
{
#ifdef MSDOS
    int i;
    struct aliastab *wl;

    for (i = 0,wl = aliasbase; wl; wl = wl->next) {
        if (!stricmp(dosname,wl->dosname))
            return (wl->strname);
    }
#endif
    return (dosname);
}


static void
freealias()

{
#ifdef MSDOS
    struct aliastab *wl,*wn;

    for (wl = aliasbase; wl; wl = wn) {
        wn = wl->next;
        afree(wl,aliastab);
    }
    aliasbase = NULL;
#endif
}


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

static struct STREAM_info streaminfo;


/***********************************************************************/
/* function STRM_BEGINRECORD                                           */
/*      Function to begin record on StreamFile.                        */
/*      count     = the number of bytes that will be contained within  */
/*                  the record.                                        */
/*      type      = the STREAM record type.                            */
/*      datatype  = the type of data contained in the STREAM record.   */
/***********************************************************************/

static void
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
    putc((unsigned char)type, StreamFile);
    putc((unsigned char)datatype, StreamFile);
    ++streaminfo.rec_count;
    streaminfo.byte_count += count;
}


/***********************************************************************/
/* function STRM_ADDATE                                                */
/*      Function to add date reference to the StreamFile.  The date    */
/*      is stored in a 'tm' structure that is used by CTIME(3).        */
/***********************************************************************/

static void
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

static void
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
            if (c >= 'a' && c <= 'z')
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

static void
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

static int
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

static int
endlib()

{
    int i, j;

    if (streaminfo.level != 0 && streaminfo.test) return(-1);
    strm_beginrecord(4, ENDLIB, 0);
    /* pad with nulls */
    i = (streaminfo.byte_count % 2048);
    for (j = 0; j < i; ++j)
        putc(0, StreamFile);
    return (0);
}


/***********************************************************************/
/* function BGNSTR                                                     */
/*      Function to structure definition in the StreamFile.            */
/***********************************************************************/

static int
bgnstr(strp)

STRM_STRCT *strp;
{
    if (streaminfo.level != 0 && streaminfo.test) return(-1);
    streaminfo.level = 1;
    strm_beginrecord(28, BGNSTR, 2);
    strm_addate(&(strp->str_creatdate));
    strm_addate(&(strp->str_moddate));
    strm_entrasc_rec(strp->str_name, STRNAME, TOUPR);
    ++streaminfo.struct_count;
    return (0);
}


/***********************************************************************/
/* function ENDSTR                                                     */
/*      Function to end structure on the StreamFile.                     */
/***********************************************************************/

static int
endstr()

{
    if (streaminfo.level != 1 && streaminfo.test) return(-1);
    streaminfo.level = 0;
    strm_beginrecord(4, ENDSTR, 0);
    return (0);
}


/***********************************************************************/
/* function STRM_ENDLMNT                                               */
/*      Function to end element stream on the StreamFile.              */
/***********************************************************************/

static int
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

static int
bndry(bndryp)

STRM_BOUNDARY *bndryp;
{
    int i;

    if (streaminfo.level != 1 && streaminfo.test) return(-1);
    streaminfo.level = 2;
    strm_beginrecord(4, BOUNDARY, 0);
    strm_beginrecord(6, LAYER, 2);
    strm_intcopy(bndryp->bnd_layer);
    strm_beginrecord(6, DATATYPE, 2);
    strm_intcopy(bndryp->bnd_datatype);
    if (bndryp->bnd_ncoord < 4) {
        OutPrompt("Error: less than four boundary coordinates");
        StrmNoGo = True;
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

static int
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

static int
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
        OutPrompt("Error: less than two path points");
        StrmNoGo = True;
    }
    else {
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

static int
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

static int
aref(arefp)

STRM_AREF *arefp;
{
    int *ip;
    int i;

    if (streaminfo.level != 1 && streaminfo.test) return (-1);
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

static int
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

static void
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

static void
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
 *    Function to transfer double precision number to a Stream file.
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
static void
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


/*************************************************************************
 *
 * CIF conversion.
 *
 *************************************************************************/


void
ToCIF()

{
    char *s;
    char CIFFile[81];
    int Layer;
    int Layers[CDNUMLAYERS+1];

    MenuSelect(MenuTOCIF);

    ShowPrompt("CIF file name? ");
    strcpy(CIFFile,Parameters.kpCellName);
    if ((s = strchr(CIFFile,'.')) != NULL) *s = '\0';
    strcat(CIFFile,".cif");
    s = FBEdit(CIFFile);
    if (s != NULL) {
        if (*s != '\0' && *s != '\n')
            strcpy(CIFFile,s);

        if (!access(CIFFile,0))
            if (clobber_check(False))
                return;

        for (Layer = 0; Layer <= NumLayerTable; Layer++)
            Layers[Layer] = True;

        if (!CDFrom(Parameters.kpCellName,CIFFile,RESOLUTION,RESOLUTION,
            Layers,NumLayerTable+1,'k')) {
            sprintf(TypeOut,"Translation of %s failed.  MORE",
                Parameters.kpCellName);
            OutPrompt(TypeOut);
            (void)FBGetchar(ERASE);
            OutPrompt(CDStatusString);
        }
        else {
            sprintf(TypeOut,"Translation of %s succeeded.",
            Parameters.kpCellName);
            OutPrompt(TypeOut);
        }
    }
    MenuDeselect(MenuTOCIF);
}


void
FromCIF()

{
    char *s,CIFFile[81],type;
    FILE *fp;

    MenuSelect(MenuFRCIF);

    ShowPrompt("CIF file name? ");
    s = FBEdit(NULL);
    if (s != NULL && *s != '\0' && *s != '\n') {
        strcpy(CIFFile,s);
        if ((fp = POpen(CIFFile,"r",(char *)NULL,(char **)NULL)) == NULL) {
            sprintf(TypeOut,"CIF file %s not found.",CIFFile);
            ShowPrompt(TypeOut);
            MenuDeselect(MenuFRCIF);
            return;
        }
        type = get_cif_file_type(fp);
        fclose(fp);
        sprintf(TypeOut,"CIF file type: %c",type);
        OutPrompt(TypeOut);

        if (!CDTo(CIFFile,"Root",RESOLUTION,RESOLUTION,type)) {
            sprintf(TypeOut,"Translation of %s failed.  MORE",CIFFile);
            OutPrompt(TypeOut);
            (void)FBGetchar(ERASE);
            OutPrompt(CDStatusString);
        }
        else {
            sprintf(TypeOut,"Translation of %s succeeded.",CIFFile);
            OutPrompt(TypeOut);
        }
    }
    MenuDeselect(MenuFRCIF);
}


static char
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


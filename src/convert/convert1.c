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

/* This converts all structure names and references to lower case */


#include "prefix.h"
#include "kic.h"
#include "stream.h"
#include <setjmp.h>
#include <ctype.h>

/* Define to 1 to convert GDS symbol names to lower case.  This was true in
 * previous versions of Kic.
 */
#define TOLWR 0

#define    LONGSCALE(n)  (int)(floor(ScaleFactor * ((double)(n)) + 0.5))

static int     ByteSwap;
static int     CurrentLayer;
static int     CurrentAttribute;
static int     CurrentDataType;
static int     NumSymbols;
static int     CurrentSize;
/* static int     NumArefs; */
static int     CurrentOffset;
static int     *XYbuf;
static FILE    *SymDesc;
static char    **SymbolNames;
static char    CurrentSymbol[45];
static double  ScaleFactor;
static FILE    *StreamFile;
static jmp_buf jbuf;

extern char *MenuFRGDS;

struct headerlist {
    int hd_RecordType;
    char *hd_Text;
    char *hd_Comment;
    struct headerlist *hd_Succ;
};
typedef struct headerlist HEADLIST;


#if __STDC__
static HEADLIST* newhead(HEADLIST**,int);
static void symdef(char*,HEADLIST*,int*);
static void s_bndry(char*);
static void s_path(char*);
static void s_sref(char*);
static void s_aref(char*);
static void s_text(char*);
static int  get_record(char*);
static int  struct_index(char*);
static void print_layer(int,int);
static FILE *open_symbol(char*);
static void new_symbol(char*);
static void set_property_value(char*,int);
static void set_xy(char*,int*,int);
static void path_to_rect(int*);
static void set_path(char*,int,int*);
static void set_instance(int,int,int,int,double,double);
static void set_array(int,int,int,int,double,int*,double);
static int  set_angle(double,int*,int*);
static void err_fatal_1(char*,char*);
static void err_fatal_2(char*,int);
static void err_warn_1(char*,char*,char*);
static void err_warn_2(char*,int,char*);
static int  strm_ival(char*);
static int  strm_longval(char*);
static double strm_doubleval(char*);
static char *alias(char*);
static void dumpalias(void);
#else
static HEADLIST* newhead();
static void symdef();
static void s_bndry();
static void s_path();
static void s_sref();
static void s_aref();
static void s_text();
static int  get_record();
static int  struct_index();
static void print_layer();
static FILE *open_symbol();
static void new_symbol();
static void set_property_value();
static void set_xy();
static void path_to_rect();
static void set_path();
static void set_instance();
static void set_array();
static int  set_angle();
static void err_fatal_1();
static void err_fatal_2();
static void err_warn_1();
static void err_warn_2();
static int  strm_ival();
static int  strm_longval();
static double strm_doubleval();
static char *alias();
static void dumpalias();
#endif


void
FromGDSII()

{

    HEADLIST *HeaderCopy;
    HEADLIST *SymbolProps;
    int i;
    int type;
    int struct_dates[12];
    char *cp;
    char cbuf[MAXRECSIZE + 4];

    MenuSelect(MenuFRGDS);

    ShowPrompt("Stream file name? ");
    cp = FBEdit(NULL);
    if (cp == NULL || *cp == '\0' || *cp == '\n') {
        MenuDeselect(MenuFRGDS);
        return;
    }

    SymbolNames = (char **) tmalloc(MAXSYMBOLS*sizeof(char *));
    XYbuf = (int *) tmalloc(MAXSTRMCOORDS*sizeof(int));

    readalias();
    SymbolProps = NULL;
    NumSymbols  = 0;

    StreamFile = fopen(cp,"rb");
    if (StreamFile == NULL) {
        sprintf(TypeOut,"Can't open stream file %s.",cp);
        ShowPrompt(TypeOut);
        goto quit;
    }

    /* byte swap test */
    ByteSwap = False;
    if ((i = getc(StreamFile)) != 0)
        ByteSwap = True;
    ungetc(i,StreamFile);

    if (setjmp(jbuf) == 1) {
        sprintf(TypeOut,"Translation failed.");
        OutPrompt(TypeOut);
        goto quit;
    }

    /* build the symbol table */
    sprintf(TypeOut,"Building symbol table.");
    OutPrompt(TypeOut);
    while ((type = get_record(cbuf)) != 4)
        if (type == 6) {
            if (TOLWR)
                to_lower_case(cbuf);
            new_symbol(cbuf);
        }
    rewind(StreamFile);

    while ((type = get_record(cbuf))!= ENDLIB) {
        switch(type) {

        case HEADER:
            HeaderCopy = newhead(&SymbolProps,HEADER);
            sprintf(TypeOut,"%d",strm_ival(cbuf));
            HeaderCopy->hd_Text = CopyString(TypeOut);
            sprintf(TypeOut,"( VERSION %d )",strm_ival(cbuf));
            HeaderCopy->hd_Comment = CopyString(TypeOut);
            break;

        case BGNLIB:
            break;

        case LIBNAME:
            HeaderCopy = newhead(&SymbolProps,LIBNAME);
            HeaderCopy->hd_Text = CopyString(cbuf);
            sprintf(TypeOut,"( LIBNAME %s )",cbuf);
            HeaderCopy->hd_Comment = CopyString(TypeOut);
            break;

        case UNITS:
            ScaleFactor = (1e8 * strm_doubleval(cbuf+8));
            break;

        case BGNSTR:
            for (i = 0; i < 12; i++)
                struct_dates[i] = strm_ival(cbuf + (i << 1));
            break;

        case STRNAME:
            symdef(cbuf,SymbolProps,struct_dates);
            fprintf(SymDesc,"DF;\n");
            fprintf(SymDesc,"E\n");
            fclose(SymDesc);
            break;

        case REFLIBS:
            HeaderCopy = newhead(&SymbolProps,REFLIBS);
            sprintf(TypeOut,"%s %s",cbuf,cbuf+44);
            HeaderCopy->hd_Text = CopyString(TypeOut);
            HeaderCopy->hd_Comment = CopyString("( REFLIBS )");
            break;

        case FONTS:
            HeaderCopy = newhead(&SymbolProps,FONTS);
            sprintf(TypeOut,"%s %s %s %s",cbuf,cbuf+44,cbuf+88,cbuf+132);
            HeaderCopy->hd_Text = CopyString(TypeOut);
            HeaderCopy->hd_Comment = CopyString("( FONTS )");
            break;

        case GENERATIONS:
            HeaderCopy = newhead(&SymbolProps,GENERATIONS);
            sprintf(TypeOut,"%d",strm_ival(cbuf));
            HeaderCopy->hd_Text = CopyString(TypeOut);
            sprintf(TypeOut,"( GENERATIONS %d )",strm_ival(cbuf));
            HeaderCopy->hd_Comment = CopyString(TypeOut);
            break;

        case ATTRTABLE:
            HeaderCopy = newhead(&SymbolProps,ATTRTABLE);
            HeaderCopy->hd_Text = CopyString(cbuf);
            sprintf(TypeOut,"( ATTRIBUTE TABLE %s )",cbuf);
            HeaderCopy->hd_Comment = CopyString(TypeOut);
            break;

        default:
            /*
            err_fatal_2("Illegal record type",type);
            */
            sprintf(TypeOut,"Warning: unknown record type %d",type);
            OutPrompt(TypeOut);
        }
    }

quit:
    dumpalias();
    for (HeaderCopy = SymbolProps; HeaderCopy; HeaderCopy = SymbolProps) {
        SymbolProps = HeaderCopy->hd_Succ;
        free(HeaderCopy->hd_Text);
        if (HeaderCopy->hd_Comment)
            free(HeaderCopy->hd_Comment);
        free(HeaderCopy);
    }
    for (i = 0; i < NumSymbols; i++)
        free(SymbolNames[i]);
    free(SymbolNames);
    free(XYbuf);
    sprintf(TypeOut,"Translation complete.");
    OutPrompt(TypeOut);
    MenuDeselect(MenuFRGDS);
}


static HEADLIST *
newhead(sprops,which)

HEADLIST **sprops;
int which;
{
    HEADLIST *Hc,*Hn;

    Hc = (HEADLIST *) tmalloc(sizeof(HEADLIST));
    Hc->hd_RecordType = PROPERTYOFFSET + which;
    Hc->hd_Succ = NULL;
    if (*sprops == NULL) *sprops = Hc;
    else {
        for (Hn = *sprops; Hn->hd_Succ; Hn = Hn->hd_Succ) ;
        Hn->hd_Succ = Hc;
    }
    return (Hc);
}


static void
symdef(cbuf,sprops,struct_dates)

char *cbuf;
HEADLIST *sprops;
int *struct_dates;
{
/*
 * BEGIN SYMBOL DEFINITION
 */
    int i;
    int index;
    int type;
    char *c;

    if (TOLWR)
        to_lower_case(cbuf);
    CurrentLayer = CurrentDataType = CurrentAttribute = -1;
    /* search for symbol number */
    index = struct_index(cbuf);
    strcpy(CurrentSymbol,cbuf);

    SymDesc = open_symbol(c = alias(cbuf));
    if (!strcmp(c,cbuf))
        sprintf(TypeOut,"Converting: %s",cbuf);
    else
        sprintf(TypeOut,"Converting: %-30s(new name: %s)",cbuf,c);
    OutPrompt(TypeOut);

    /* add the symbol property list */
    for ( ; sprops != NULL; sprops = sprops->hd_Succ) {
        if (sprops->hd_Comment)
            fprintf(SymDesc,"%s;\n",sprops->hd_Comment);
        fprintf(SymDesc,"5 %d %s;\n",sprops->hd_RecordType,sprops->hd_Text);
    }
    fprintf(SymDesc,"( CREATION DATE ");
    for (i = 0; i < 6; i++)
        fprintf(SymDesc,"%d ",struct_dates[i]);
    fprintf(SymDesc,": MOD DATE ");
    for (i = 0; i < 6; i++)
        fprintf(SymDesc,"%d ",struct_dates[i+6]);
    fprintf(SymDesc,");\n");
    fprintf(SymDesc,"( Symbol %s );\n",c);
    fprintf(SymDesc,"9 %s;\n",c);
    fprintf(SymDesc,"DS %d 1 1;\n",index);

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


static void
s_bndry(cbuf)

char *cbuf;
{
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
    print_layer(layer,dtype);
    set_xy(cbuf,XYbuf,ncoords);
}


static void
s_path(cbuf)

char *cbuf;
{
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
    print_layer(layer,dtype);
    if (ptype == 0 && ncoords > 1) {
        /*
         * subtract a half width from the endpoints.
         */
        ConvertPathtype(&XYbuf[0],&XYbuf[1],XYbuf[2],XYbuf[3],pathwidth,0);
        i = ncoords+ncoords-2;
        ConvertPathtype(&XYbuf[i],&XYbuf[i+1],XYbuf[i-2],XYbuf[i-1],pathwidth,0);
    }
    if (ptype != 2)
        fprintf(SymDesc,"5 %d PATHTYPE %d;\n",PROPERTYOFFSET + PATHTYPE,ptype);

    sprintf(cbuf,"W %d",pathwidth);
    set_path(cbuf,ncoords,XYbuf);
}


static void
s_sref(cbuf)

char *cbuf;
{
    double magn = 1,rotn = 0;
    int cx = 0,cy = 0;
    int reflection = 0;
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
    set_instance(index,reflection,cx,cy,rotn,magn);

#ifdef notdef
    else {
        err_warn_1("Magnification not unity",
            SymbolNames[index],CurrentSymbol);

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
            fprintf(afile,"DS %d %ld %d;\n",NumSymbols,lone,j);
            fprintf(afile,"9 %s;\n",SymbolNames[index]);
            fprintf(afile,"C %d;\n",index);
            fprintf(afile,"DF;\n");
            fprintf(afile,"E\n");
            fclose(afile);
            set_instance(NumSymbols,reflection,cx,cy,rotn);
        }
        else
            err_warn_1("Magnification too small",SymbolNames[index],
                CurrentSymbol);
    }
#endif
}


static void
s_aref(cbuf)

char *cbuf;
{
    double magn = 1,rotn = 0;
    int reflection = 0;
    int nx = 0,ny = 0;
    int i;
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
    set_array(index,reflection,nx,ny,rotn,XYbuf,magn);

#ifdef notdef

    else {
        err_warn_1("Magnification not unity",
            SymbolNames[index],CurrentSymbol);
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
            fprintf(afile,"DS %d %ld %d;\n",NumSymbols,lone,j);
            fprintf(afile,"9 %s;\n",SymbolNames[NumSymbols-2]);
            fprintf(afile,"C %d;\n",index);
            fprintf(afile,"DF;\n");
            fprintf(afile,"E\n");
            fclose(afile);
            set_array(NumSymbols,reflection,nx,ny,rotn,XYbuf);
        }
        else
            err_warn_1("Magnification too small",SymbolNames[index],
                CurrentSymbol);
    }
#endif
}


static void
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
    print_layer(layer,dtype);
    fprintf(SymDesc,
        "5 %d WIDTH %d PRESENT %d PTYPE %d MAG %f ANGLE %f REFLECT %d;\n",
        PROPERTYOFFSET + TEXT,width,present,ptype,magn,rotn,reflection);
    fprintf(SymDesc,"94 %s %d %d 0;\n",string,cx,cy);
}


static int
get_record(cbuf)

char *cbuf;
{
    int i;
    int size;
    unsigned byte0;
    unsigned byte1;
    unsigned rtype;

    CurrentOffset = ftell(StreamFile);
    if (!ByteSwap) {
        byte0 = getc(StreamFile);
        byte1 = getc(StreamFile);
        rtype = getc(StreamFile);
        (void)  getc(StreamFile);
    }
    else {
        byte1 = getc(StreamFile);
        byte0 = getc(StreamFile);
        (void)  getc(StreamFile);
        rtype = getc(StreamFile);
    }
    size  = byte0*256 + byte1 - 4;
    if (size < 0) size = 0;

    if (size > MAXRECSIZE)
        err_fatal_2("Oversized record, size",size);
    CurrentSize = size;
    size >>= 1;

    if (size && fread(cbuf,(size_t)size,(size_t)2,StreamFile) != 2)
        err_fatal_1("Read error on input","");

    size <<= 1;
    if (CurrentSize != size) {
        cbuf[size] = getc(StreamFile);
        cbuf[size+1] = getc(StreamFile);
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
        ungetc(cbuf[size+1],StreamFile);
    cbuf[CurrentSize] = '\0';
    return (rtype);
}


static int
struct_index(cbuf)

char *cbuf;
{
    int i;

    /* the symbol table should be complete at this point */
    for (i = 0; i < NumSymbols; i++)
        if (!strcmp(SymbolNames[i],alias(cbuf))) return (i);

    sprintf(TypeOut,"Warning: Undefined symbol %s.",cbuf);
    OutPrompt(TypeOut);
    new_symbol(cbuf);
    return (NumSymbols-1);
}


static void
print_layer(layer,datatype)

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
    for (l = 1; l <= NumLayerTable; l++) {

        if (LayerTable[l].klStreamNumber == layer &&
                (LayerTable[l].klStreamDataType < 0 ||
                LayerTable[l].klStreamDataType == datatype)) {

            if (layer == CurrentLayer && LayerTable[l].klStreamDataType < 0)
                return;
            buf[0] = LayerTable[l].klTechnology;
            buf[1] = LayerTable[l].klMask[0];
            buf[2] = LayerTable[l].klMask[1];
            buf[3] = LayerTable[l].klMask[2];
            if (isspace(buf[1])) buf[1] = '\0';
            else if (isspace(buf[2])) buf[2] = '\0';
            else if (isspace(buf[3])) buf[3] = '\0';
            else buf[4] = '\0';
            fprintf(SymDesc,"L %s;\n",buf);
            break;
        }
        else if (l == NumLayerTable) {
            sprintf(buf,"Undefined layer %d datatype",layer);
            err_warn_2(buf,datatype,CurrentSymbol);
            fprintf(SymDesc,"L %02d\n",layer);
        }
    }
    CurrentLayer = layer;
    CurrentDataType = datatype;
}


static FILE *
open_symbol(name)

char *name;
{
    FILE *fp;

    if ((fp = fopen(name,"w")) == 0)
        err_fatal_1("Can't open symbol",name);
    return (fp);
}


static void
new_symbol(name)

char *name;
{
    name = alias(name);
    if (NumSymbols >= MAXSYMBOLS)
        err_fatal_2("Exceeded maximum symbol count of",MAXSYMBOLS);
    SymbolNames[NumSymbols] = CopyString(name);
    NumSymbols++;
}


static void
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


static void
set_xy(s,xy,n)

/* save it as a box if rectangular, otherwise create a polygon */
char *s;
int *xy;
int n;
{
    s[0] = 'P';
    s[1] = '\0';
    if (n != 5) {
        set_path(s,n,xy);
        return;
    }
    if (xy[0] == xy[2] && xy[3] == xy[5] &&
            xy[4] == xy[6] && xy[7] == xy[1]) {
        path_to_rect(xy);
        return;
    }
    if (xy[1] == xy[3] && xy[2] == xy[4] &&
            xy[5] == xy[7] && xy[6] == xy[0]) {
        path_to_rect(xy);
        return;
    }
    set_path(s,n,xy);
}


static void
path_to_rect(xy)

int *xy;
{
    int cx,cy,length,width;

    cx = (xy[0] + xy[4])/2;
    cy = (xy[1] + xy[5])/2;
    length = xy[4] - xy[0];
    width = xy[5] - xy[1];
    if (width < 0) width = -width;
    if (length < 0) length = -length;
    fprintf(SymDesc,"B %d %d %d %d;\n",length,width,cx,cy);
}


static void
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


static void
set_instance(index,reflection,cx,cy,rotn,magn)

int index, reflection;
int cx,cy;
double rotn;
double magn;
{
    int i, j;

    fprintf(SymDesc,"9 %s;\n",SymbolNames[index]);
    if (magn != 1.0) {
        err_warn_1("Magnification not unity",
            SymbolNames[index],CurrentSymbol);
        fprintf(SymDesc,"( Scale %g );\n",magn);
    }
    fprintf(SymDesc,"C %d",index);
    if (reflection)    fprintf(SymDesc," MY");
    if (set_angle(rotn,&i,&j))
        fprintf(SymDesc," R %d %d",i,j);
    fprintf(SymDesc," T %d %d;\n",cx,cy);
}


static void
set_array(index,reflection,nx,ny,rotn,xy,magn)

int index, reflection;
int nx,ny;
double rotn;
int *xy;
double magn;
{
    int i, j, k;
    int dx,dy;

    fprintf(SymDesc,"9 %s;\n",SymbolNames[index]);
    if (magn != 1.0) {
        err_warn_1("Magnification not unity",
            SymbolNames[index],CurrentSymbol);
        fprintf(SymDesc,"( Scale %g );\n",magn);
    }
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


static int
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
        return (0);

    if (*i == 0 && *j == 1)     /* 90 */
        return (1);

    if (*i == -1 && *j == 0)    /* 180 */
        return (2);

    if (*i == 0 && *j == -1)    /* 270 */
        return (3);

    err_warn_1("Nonorthogonal rotation","",CurrentSymbol);
    fprintf(SymDesc,"( Rotate %g );\n",rotn);
    *i = *j = 0;
    return (0);
}


static void
err_fatal_1(str,what)

char *str,*what;
{
    sprintf(TypeOut,"Error: %s %s at offset %d.",str,what,CurrentOffset);
    OutPrompt(TypeOut);
    longjmp(jbuf,1);
}


static void
err_fatal_2(str,type)

char *str;
int type;
{
    sprintf(TypeOut,"Error: %s %d at offset %d.",str,type,CurrentOffset);
    OutPrompt(TypeOut);
    longjmp(jbuf,1);
}


static void
err_warn_1(str,which,what)

char *str,*which,*what;
{
    sprintf(TypeOut,"Warning: %s for instance %s in symbol %s at offset %d.",
        str,which,what,CurrentOffset);
    OutPrompt(TypeOut);
}


static void
err_warn_2(str,type,what)

char *str,*what;
int type;
{
    sprintf(TypeOut,"Warning: %s %d in symbol %s at offset %d.",
        str,type,what,CurrentOffset);
    OutPrompt(TypeOut);
}


/***********************************************************************
 * Stream number conversion.
 ***********************************************************************/


/***********************************************************************/
/* function STRM_IVAL                                                  */
/*      Function to evaluate STREAM short integer.                     */
/***********************************************************************/

static int
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

static int
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
static double
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


/************************************************************************
 * Map the symbol names into a DOS compatible file name, i.e., 8
 * characters. The first 6 are mapped directly, last 2 are index digits.
 ************************************************************************/

#ifdef MSDOS

static struct aliastab *aliasbase;

static char *
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
            wl->next = alloc(aliastab);
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
    wl = alloc(aliastab);
    aliasbase = wl;
    strcpy(wl->strname,strname);
    strncpy(wl->dosname,dosname,7);
    wl->next = NULL;
    wl->dosname[6] = '0' + i/10;
    wl->dosname[7] = '0' + i%10;
    wl->dosname[8] = '\0';
    return (wl->dosname);
}


static void
dumpalias()

{
    FILE *fp;
    struct aliastab *wl,*wn;

    if (aliasbase == NULL) return;
    fp = fopen(ALIASFILE,"w");
    if (fp == NULL) return;
    for (wl = aliasbase; wl; wl = wn) {
        wn = wl->next;
        fprintf(fp,"%-10s%s\n",wl->dosname,wl->strname);
        afree(wl,aliastab);
    }
    fclose(fp);
    aliasbase = NULL;
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
            aliasbase = alloc(aliastab);
            wl = aliasbase;
        }
        else {
            wl->next = alloc(aliastab);
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

#else
/* assume UNIX, strip any bad characters */

static char *
alias(strname)
char *strname;
{
    char *s;

    for (s = strname; *s; s++) {
        if (*s == '/')
            *s = '_';
    }
    return (strname);
}

void
readalias() {}

static void
dumpalias() {}

#endif


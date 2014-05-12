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
#include <time.h>

/* if the target machine uses IEEE floating point format, */
#define IEEE
/* otherwise a DEC VAX format is assumed */

/*
 * We use the property list of KIC symbols to save the library information;
 * The value of the property is the numeric value of the STREAM record
 * type offset by 7000 (e.g. 7000 is the KIC property value describing the
 * STREAM version number, 7002 is the KIC property value describing the
 * STREAM library name, etc.)  The offset of 7000 was arbitrarily selected,
 * and care must be taken so that this value does not conflict with any
 * other convention.  The PROPERTYOFFSET define is for convenience.
 *
 * The STREAM-specific property list is attributed to every KIC symbol
 * by 'strmtokic', and 'kictostrm' will look for this information.
 */
#define PROPERTYOFFSET 7000

#define MAXSTRMCOORDS   500
#define MAXSYMBOLS      2000
#define MAXRECSIZE      2048
#define RADTODEG        57.29577951

typedef struct rct1{
    int left;
    int bottom;
    int right;
    int top;
    struct rct1 *r_next;
}
    RECT;

typedef struct{
    int pointx;
    int pointy;
}
    POINT;

struct pathlist{
    POINT pathpoint;
    struct pathlist *pathlink;
};

typedef struct pathlist PATHLIST;

struct STREAM_info {
    int byte_count;       /* counter of the number of bytes     */
                           /* currently in the streamfile        */
    int rec_count;        /* number of records in streamfile    */
    int struct_count;     /* number of structures in streamfile */
    int level;             /* current level in STREAM library    */
    int test;              /* activates STREAM level test        */
    };

extern FILE *STREAMFILE;   /* STREAM output file                 */

struct strans  {           /* structure with strm_strnsfm info.  */
    int trns_reflection;   /* x-axis reflection flag             */
    int trns_abs_mag;      /* absolute magnification flag        */
    int trns_abs_angle;    /* absolute angle flag                */
    double trns_mag;       /* magnification factor               */
    double trns_angle;     /* angular rotation factor            */
    };                                                     

typedef struct strans STRM_TRANSFORM;

struct property_element {
    int prp_npropval;             /* number of property-attribute pairs */
    int prp_propattr[256];
    char *prp_propval[256];       /* pointers to the property strings   */
    };

typedef struct property_element STRM_PROPERTY;

struct library_information {
    struct tm lib_moddate;        /* modification date structure     */
    struct tm lib_accessdate;     /* access date structure           */
    int lib_gen;                  /* number of generations           */
    double lib_uunit, lib_munit;  /* length of database unit         */
    char lib_name[45];            /* pointer to library name         */
    char lib_lib1[45];            /* pointers to reflib names        */
    char lib_lib2[45];
    char lib_font0[45];           /* pointers to font names          */
    char lib_font1[45];
    char lib_font2[45];
    char lib_font3[45];
    char lib_attr[45];            /* attribute filename              */
    };

typedef struct library_information STRM_LIBRARY;

struct stream_strct_data {
    struct tm str_moddate;
    struct tm str_creatdate;
    char str_name[45];
    };

typedef struct stream_strct_data STRM_STRCT;

struct text_element {
    int txt_layer;
    int txt_texttype;
    int txt_horizontal;       /* 0=left justified,  1=center,  2=right */
    int txt_vertical;         /* 0=top justified,  1=middle,  2=bottom */
    int txt_pathtype;
    int txt_font;
    int txt_width;
    int txt_xy[2];
    struct strans txt_transform;
    struct property_element txt_prop;
    char txt_text[45];
    };

typedef struct text_element STRM_TEXT;

struct aref_element {
    int ar_col;
    int ar_row;
    int ar_xy[6];
    struct strans ar_transform;
    struct property_element ar_prop;
    char ar_name[45];
    };

typedef struct aref_element STRM_AREF;

struct sref_element {
    int sr_xy[2];
    struct strans sr_transform;
    struct property_element sr_prop;
    char sr_name[45];
    };

typedef struct sref_element STRM_SREF;

struct path_element {
    int pth_layer;
    int pth_datatype;
    int pth_pathtype;
    int pth_ncoord;
    int pth_width;
    int pth_xy[MAXSTRMCOORDS];
    struct property_element pth_prop;
    };

typedef struct path_element STRM_PATH;

struct bndry_element {
    int bnd_layer;
    int bnd_datatype;
    int bnd_ncoord;
    int bnd_xy[MAXSTRMCOORDS];
    struct property_element bnd_prop;
    };

typedef struct bndry_element STRM_BOUNDARY;


/*    The following are definitions of CALMA Stream elements    */
/*    that would otherwise be noted by number.                  */

#define HEADER        0
#define BGNLIB        1
#define LIBNAME       2
#define UNITS         3
#define ENDLIB        4
#define BGNSTR        5
#define STRNAME       6
#define ENDSTR        7
#define BOUNDARY      8
#define PATH          9
#define SREF         10
#define AREF         11
#define TEXT         12
#define LAYER        13
#define DATATYPE     14
#define WIDTH        15
#define XY           16
#define ENDEL        17
#define SNAME        18
#define COLROW       19
#define TEXTNODE     20
#define SNAPNODE     21
#define TEXTTYPE     22
#define PRESENTATION 23
#define SPACING      24
#define STRING       25
#define STRANS       26
#define MAG          27
#define ANGLE        28
#define UINTEGER     29
#define USTRING      30
#define REFLIBS      31
#define FONTS        32
#define PATHTYPE     33
#define GENERATIONS  34
#define ATTRTABLE    35
#define STYPTABLE    36
#define STRTYPE      37
#define ELFLAGS      38
#define ELKEY        39
#define LINKTYPE     40
#define LINKKEYS     41
#define NODETYPE     42
#define PROPATTR     43
#define PROPVALUE    44


/* DOS filename mapping */
#define ALIASFILE "dos__str.als"

struct aliastab {
    char strname[48];
    char dosname[10];
    struct aliastab *next;
};

#ifdef __STDC__
extern void readalias(void);
#else
extern void readalias();
#endif

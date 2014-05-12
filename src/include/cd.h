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
 * CD package data structures.
 *  
 * Giles Billingsley
 * Ken Keller
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "nmalloc.h"
#include "macros.h"

#ifdef vms
#define void int
#endif

#define CDDelete CDDeleteObjectDesc

#define FILENAMESIZE      32    /* maximum size of a file name */

/*
 * Values routines return in StatusInt of CDOpen, CDBeginMakeCall,
 * CDTo, CDFrom, or CDParseCIF.
 */
#define CDPARSEFAILED      1    /* (FATAL) parse failed */
#define CDOLDSYMBOL        2    /* symbol already exists in database */
#define CDNEWSYMBOL        3    /* (empty) symbol not in search path */
#define CDSUCCEEDED        4    /* new symbol(s) found in search path */

/*
 * Valid arguments to CDError().
 */
#define CDMALLOCFAILED    11    /* (FATAL) out of memory */
#define CDBADBOX          12    /* zero width or length box */
#define CDXFORMSTACKFULL  13    /* transform stack overflow */
#define CDBADPATH         14    /* bad directory name in search path */

/*
 * Types of geometries
 */
#define CDSYMBOLCALL      'c'
#define CDPOLYGON         'p'
#define CDROUNDFLASH      'r'
#define CDLABEL           'l'
#define CDWIRE            'w'
#define CDBOX             'b'

/*
 * Types of transformations
 */
#define CDMIRRORX         'x'    /* mirror in the direction of x */
#define CDMIRRORY         'y'    /* mirror in the direction of y */
#define CDROTATE          'r'    /* rotate by vector X,Y */
#define CDTRANSLATE       't'    /* translate to X,Y */

/*
 * CD Control flags; See struct d below.
 */
#define DCONTROLCDOPEN    'o'
#define DCONTROLPCIF      'p'
#define DCONTROLCDTO      't'
#define DCONTROLVANILLA   'v'

/*
 * Coordinate system with 1 micron features and 1 cm dice.
 * Remember that there are 100 CIF units per micron.
 */
#define CDINFINITY         100000000L

/*
 * These are the numbers that CD uses to determine which bin an object
 * resides in.  They should reflect the average size of a layout being
 * edited by KIC.  KIC will not fail if the numbers are too small.
 * Anything outside of this window is placed in the residual bin.
 * If these numbers become too large, CDIntersect() must use floating
 * point calculations.
 */
#define CDBINMAXX        500000L
#define CDBINMAXY        500000L
#define CDBINMINX        (-CDBINMAXX)
#define CDBINMINY        (-CDBINMAXY)

/*
 * PLEASE NOTE
 * ^^^^^^^^^^^
 * Because a char is used as the layer fields, the absolute maximum
 * number of layers is 127.  The may be increase by recompiling
 * KIC and CD with the Layer typed as ints.
 */
#define CDNUMBINS        10
#define CDNUMLAYERS      70

/*
 * Number of symbols stored in the symbol table for any given CIF file.
 */
#define CDNUMREMEMBER    3000

/*
 * Storage for diagnostics of CDError().
 */
extern char *CDStatusString;
extern int  CDStatusInt;

/*
 * Relax polygon min vertex requirement
 */
extern int  CDBogusPoly;

/*
 * Master list desc.
 */
struct m {
    int mLeft,mBottom,mRight,mTop; 
    char *mName;
    struct m *mPred,*mSucc;
    int mReferenceCount;
};

/*
 * Propety List desc.
 * SRW ** added Info field.
 * CD itself doesn't use it.
 */
struct prpty {
    char *prpty_String;
    char *prpty_Info;
    struct prpty *prpty_Succ;
    int prpty_Value;
};

/*
 * Symbol desc.
 */
struct s {
    int sLeft,sBottom,sRight,sTop; 
    int sA,sB;
    char *sName;
    /*
     * One bin foe each layer.  Layer 0 is for call descs. 
     * Each bin points to a double linked list of object descs. 
     * Bin[.][0][0] are the RESIDUAL bins--Bin[.][0][1] and Bin[.][1][0]
     * are unused TUNABLE.  NumBins should be as big as it can be.
     * For 20 layers and 100 bins per layer,
     * the data structure becomes 2000 words. 
     */
    struct o ***sBin[CDNUMLAYERS+1];
    struct m *sMasterList;
    struct prpty *sPrptyList;
    short sInfo;
    short sBBValid;
};

/*
 * Object desc.
 */
struct o {
    int oLeft,oBottom,oRight,oTop;
    struct o *oRep;
    struct o *oPred,*oSucc;
    struct prpty *oPrptyList;
    short oInfo;
    char oType;
    char oLayer;
};

/*
 * Polygon desc.
 */
struct po {
    struct p *poPath; 
};

/*
 * Round flash desc.
 */
struct r {
    int rWidth,rX,rY;
};

/*
 * Wire desc.
 */
struct w {
    int wWidth;
    struct p *wPath;
}; 

/*
 * Call desc.
 */
struct c {
    int cDX,cDY;        /* center to center array spacing */
    struct t *cT;        /* Pointer to transformation descriptor. */
    struct m *cMaster;   /* Pointer to master list descriptor. */
    short cNumX,cNumY;   /* Array parameters. */
    int cNum;
};

/*
 * Transform desc.
 * If MX, tType == CDMIRRORX.
 * If MY, tType == CDMIRRORX.
 * If R,  tType == CDROTATE, tX == XDirection, tY == YDirection.
 * If T,  tType == CDTRANSLATE, tX == TX, tY = TY;
 */
struct t {
    int tX,tY;
    struct t *tSucc;
    char tType;
};

/*
 * Label desc.
 */
struct la {
    int laX,laY;
    char *laLabel;
    char laXform;
    /* laXform bits:
     * 0-1, 0-no rotation, 1-90, 2-180, 3-270.
     * 2, mirror y
     * 3, mirror x
     */
};
 

/*
 * Linked path structure
 */
struct p {
    int pX,pY;
    struct p *pSucc;
};

/*
 * Generator desc.
 * Search Bin[Layer][0][0] first.
 * Then Bin[Layer][BeginX..EndX][BeginY..EndY].
 * Bin[Layer][X][Y] is the current bin. 
 * Pointer points to the current desc in the current bin. 
 */
struct g {
    int  gLeft,gBottom,gRight,gTop;
    int  gBeginX,gX,gEndX,gBeginY,gY,gEndY;
    struct o *gPointer;
    int gLayer;
};

/*
 * CD's current parameter struct
 */
struct d {

    /* Current parameters for symbol being parsed in CDOpen. */
    int  dNumX,dNumY;
    int dDX,dDY;

    /* Scale factors for CDTo and CDFrom. */
    int dA,dB;

    /* Symbol scale factors. */
    int dDSA,dDSB;

    struct o *dPointer;
    struct s *dSymbolDesc;
    struct s *dRootCellDesc;

    FILE *dSymbolFileDesc;

    /*
     * Fields used in CDTo follow.
     */

    /* True if parsing root symbol. */
    int dRoot;

    /* Root's file desc. */
    FILE *dRootFileDesc;

    /* Current property list for symbol being parsed */
    struct prpty *dPrptyList;

    /*
     * Symbol name table.
     * Big arrays are allocated in CDInit().
     */
    char (*dSymTabNames)[FILENAMESIZE]; 
    int  *dSymTabNumbers; 
    int  dNumSymbolTable;

    /*
     * Because CIF files may have FORWARD references, CDTo must pass
     * over the CIF file TWICE. 
     * On the first pass, it just fills up the symbol name table.
     * On the second pass, it does the translation to KIC format.
     */
    int dFirstPass;

    /*
     * True if debugging.
     */
    int dDebug;
    int dNumSymbolsAllocated;

    /*
     * DCONTROLCDOPEN  => CD is in CDOpen
     * DCONTROLPCIF    => CD is in CDOpen and parsing CIF rather than kic
     * DCONTROLCDTO    => CD is in CDTo
     * DCONTROLVANILLA => CD is in none of the above
     */
    char dControl;

    /*
     * dProgram == 'h' if IGS gened it.
     *          == 'i' if Icarus gened it.
     *          == 's' if Sif gened it.
     *          == 'n' if none of the above.
     */
    char dProgram;
    char dSymbolName[FILENAMESIZE];
};
extern struct d CDDesc;

/*
 * CD layer table
 */
struct l {
    char lTechnology;
    char lMask[3];
    /*True if CDFrom should output layer.*/
    char lCDFrom;
};
extern struct l CDLayer[CDNUMLAYERS+1];

/*
 * Hash table of symbol descs keyed on symbol's name.
 */
struct bu {
    struct s *buSymbolDesc;
    struct bu *buPred;
    struct bu *buSucc;
};
extern struct bu *CDSymbolTable[CDNUMLAYERS+1];

#include "cdext.h"

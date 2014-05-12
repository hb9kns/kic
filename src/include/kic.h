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
 * KIC data structures.
 * 
 */

#include <ctype.h>
#include <setjmp.h>
#include "cd.h"
#include "fb.h"
#include "coords.h"

/*
 * Size of memory blocks to be managed by nmalloc
 */
#define NMALLOC_BLOCK_SIZE    4096

/*
 * Viewport control flags
 */
#define SPLITSCREEN           'b'
#define FULLSCREEN            'o'
#define FINEVIEWPORTONLY      'f'
#define COARSEVIEWPORTONLY    'c'

/*
 * Menu names
 */
#define BASICMENU             'b'
#define DEBUGMENU             'd'
#define ATTRIBUTESMENU        'a'
#define PROPERTYMENU          'p'
#define AMBIGUITYMENU         'A'

/* 
 * Returns from PointLoop() function.
 */
#define PL_ESC 1
#define PL_UND 2
#define PL_CMD 3
#define PL_PCW 4
#define PL_PLT 5

/* character that terminates most commands */
#define ESCAPE 27

struct kl {
      /*
       * The following info is read from the .KIC file.
       */
      int  klMinDimensions;          /* minimum lambda*RESOLUTION   */
      int  klWireWidth;              /* wire width >= mindimensions */
      struct eparms *klElectrical;    /* electrical parameters       */
      short klR,klG,klB;              /* RGB color                   */
      unsigned char klStyle[8];       /* bit array for fill pattern  */
      unsigned char klAltStyle[8];    /* array for alt fill pattern  */
      short klStyleID;                /* style ID                    */
      short klAltStyleID;             /* alternate style ID          */
      short klAttributes;             /* presentation attributes     */
      short klStreamNumber;           /* layer number for Stream     */
      short klStreamDataType;         /* datatype for Stream         */
#define SYMBOLIC       0x1            /* layer is symbolic           */
#define BLINK          0x2            /* blinking layer              */ 
#define VISIBLE        0x4            /* layer is visible            */
#define FINE_FILL      0x8            /* show fill in fine viewport  */                         
#define COARSE_FILL    0x10           /* show fill in coarse viewport*/       
#define OUTLINED       0x20           /* outline figure              */ 
#define FILLED         0x40           /* layer is filled             */
#define ALT_FILLED     0x80           /* alternate pattern filled    */
#define ALT_OUTLINED   0x100          /* alternate pattern outlined  */
#define ALT_VISIBLE    0x200          /* alternate visiblilty        */
      char klTechnology,klMask[5];    /* layer name                  */
};


struct cl {
    short Ent,R,G,B;
};
extern struct cl ColorTable[];

#define HighlightingColor     1
#define MenuPromptColor       2
#define MenuTextColor         3
#define MenuHighlightingColor 4
#define MenuSelectColor       5
#define MoreTextColor         6
#define InstanceBBColor       7
#define InstanceNameColor     8
#define InstanceSizeColor     9
#define CoarseGridColor       10
#define FineGridColor         11


struct eparms {
    int e_type;
    double e_parms[8];
};
/* defines for e_type */
#define ERESIS 1
#define ECAP   2
#define ETRANS 3

extern char TypeOut[200];
extern int NumLayerTable;
extern struct kl LayerTable[CDNUMLAYERS+1];

/*
 * Cursor desc.
 */
struct kc {
    int kcPredX,kcPredY,kcX,kcY;
    int kcRawX,kcRawY;
    int kcDX,kcDY;
    int  kcInFine;
};
extern struct kc KicCursor;

/*
 * Area structure.
 */
struct ka {
    int kaLeft,kaBottom,kaRight,kaTop;
    int kaX,kaY;
    double kaWidth,kaHeight;
};

/*
 * KIC text viewports.
 */
extern struct ka MenuViewport;
extern struct ka LayerTableViewport;
extern struct ka ParameterViewport;

/*
 * Structure to keep current area of interest.
 */
struct a {
    int aLF,aBF,aRF,aTF;
    int aLC,aBC,aRC,aTC;
    int  aInFine;
    int  aInCoarse;
};
extern struct a CurrentAOI;

/*
 * Structure used to save windows in window stack.
 */
struct kw {
    int kwLastWindowX;
    int kwLastWindowY;
    int kwLastWindowWidth;
    int kwLastFineWindowX;
    int kwLastFineWindowY;
    int kwLastFineWindowWidth;
    struct kw *kwNext;
    char kwExpand;
    char kwExpandFineOnly;
    char kwName[8];
};

/*
 * Layout windows and the viewports they map to.
 */
struct kv {
    struct ka *kvFineWindow;
    struct ka *kvFineViewport;
    struct ka *kvCoarseWindow;
    struct ka *kvCoarseViewport;
    struct ka *kvLargeCoarseViewport;
    struct ka *kvSmallCoarseViewport;
    /* viewport/window */
    double kvFineRatio;
    double kvCoarseRatio;
    int kvFineViewportOnBottom;
    char kvControl; 
};
extern struct kv *View;


/*
 * Parameters that control KIC.
 */
struct kp {
    /* Symbol desc for current cell */
    struct s *kpCellDesc;

    /* Symbol name for current cell */
    char *kpCellName;

    /* Symbol desc for top level cell */
    struct s *kpTopDesc;

    /* Symbol name for top level cell */
    char *kpTopName;

    /* Command selected if any from command menu */
    char *kpCommand;

    /* Linked list of saved views */
    struct kw *kpWindowStack;

    /* current instance array parameters */
    int kpNumX;
    int kpNumY;
    double kpDX;
    double kpDY;

    /* Number of RESOLUTION*lambda between grid points. */
    int kpGrid;

    /* Debug parameters */
    int kpNumGeometries;

    /*
     * PixToLambdaSnapping is the number of points between grid lines
     * to which a cursor input point is snapped.
     */
    int kpPixToLambdaSnapping;

    /* True if layers and screen attributes have combined color map */
    short kpMergeColors;

    /* True if instances should be expanded */
    short kpExpandInstances;

    /* True if context is shown in subedit */
    short kpShowContext;

    /* True if instance is expanded in fine viewport only */
    short kpExpandFineViewportOnly;

    /* If False then the SelectQ is never redisplayed (for speed) */
    short kpEnableSelectQRedisplay;

    /* If True, user pointed inside layer table and Command[0] == EOS */
    short kpPointLayerTable;

    /* If True, user pointed inside coarse viewport and Command[0] == EOS */
    short kpPointCoarseWindow;

    /* True if all selection commands (SELec and Area) are LayerSpecific */
    short kpLayerSpecificSelection;

    /* If True, polygon vertices are clipped to the nearest grid point */
    short kpClipVerticesToGrid;

    /* If True, display grid */
    short kpGridDisplayed;

    /* If True, put grid below layout geometries */
    short kpGridOnTop;

    /* If True, grid will be shown in large viewport */
    short kpShowGridInLargeViewport;

    /* If True, then show redisplay bandwidth */
    short kpShowBandwidth;

    /* If True, user has just pressed the interrupt key */
    short kpSIGINTERRUPT;

    /* If True, all text is displayed */
    short kpDisplayAllLabels;

    /* If True, all instances will be labeled in the viewport */
    short kpLabelAllInstances;

    /* If True, instances will be marked when selected */
    short kpShowInstanceMarkers;

    /* True if wires and polygons should be constrained to 45s */
    short kp45s;

    /* If true, use lower left corner when placing cells */
    short kpSubrefLowerLeft;

    /* True if making hard copy */
    short kpDoingHardcopy;

    /* If True, show grid in hardcopy */
    short kpHardcopyGrid;

    /* True if current cell has been modified */
    short kpModified;

    /* Line style of grid lines (0 for point grid) */
    short kpGridLineStyle;

    /* Current layer */
    short kpLayer;

    /* Control of the Layer Menu */
    short kpNumLayerMenuRows;
    short kpLayersPerMenuRow;
    short kpVisibleLayerMenuRow;

    /* Number of sides for round flashes */
    short kpNumRoundFlashSides;

    /* Parameters for modifying geometries with stretch command */
    short kpStretchType;
#define STR_TBRL 0
#define STR_TB 1
#define STR_RL 2

    /* Which types of object will be selected */
    char kpSelectTypes[8];

    /*
     * PointingThreshold is the minimum value of ViewportWidth/WindowWidth
     * such that it is still comfortable to point with lambda precision.
     */
    short kpPointingThreshold;

    /* Current transform */
    short kpRotationAngle;    /* 0, 90, 180, or 270 */
    short kpMX;
    short kpMY;

    /* At what level in the hierarchy are we?  See Redisplay */
    short kpHierarchyLevel;

    /* Factor for scaling text labels for hard copy */
    short kpHardcopyTextScale;

    /* Device driver name for hardcopy support */
    char *kpHardcopyDevice;

    /* Format code for hardcopy support */
    char kpHardcopyFormat[2];

    /* Hardcopy resolution */
    short kpHardcopyResolution;

    /*
     * == COARSEVIEWPORTONLY if coarse window-viewport to be redisplayed
     * == FINEVIEWPORTONLY if fine window-viewport should be redisplayed
     * == SPLITSCREEN if both should be redisplayed
     */
    char kpRedisplayControl;

    /*
     * Current command menu
     *     == INSTANCEMENU denotes instance menu
     *     == ATTRIBUTESMENU denotes attribute menu
     *     == PROPERTYMENU denotes property menu
     *     == BASICMENU denotes basic menu
     *     == SELECTIONMENU denotes selection menu
     *     == DEBUGMENU denotes debug menu
     *     == AMBIGUITYMENU denotes ambiguity menu
     */
    char kpMenu;

    /* Type of cursor chosen */
    short kpCursorShape;

    /* True if full screen cursor in use */
    short kpFullScreenCursor;

    /* Beep volume */
    short kpPointBeepVolume;

    /* Name of X font used */
    char kpFontName[81];
};
extern struct kp Parameters;

/*
 * Selection queue.
 *
 *
 * The select Q is a single-linked list of select Q descriptors.
 * Each desc points to the object descriptor for a selected object. 
 * SelectQHead is the head of the list.
 * SelectQBB is the BB of ALL of the selected objects.
 * Move and delete are easily implemented as operations on the Q.
 */

struct ks {
    struct ks *ksSucc;
    struct o *ksPointer;
};

extern struct ks *SelectQHead;
extern struct ka SelectQBB;

/* Values of Info parameter used to manage objects in SelectQ */

/* Previous object not in Q */
#define SQ_OLD     0

/* Previous object in Q, selected */
#define SQ_OLDSEL  1

/* Conditionally deleted object */
#define SQ_GONE    2

/* Newly created object, not selected */
#define SQ_NEW     3

/* Newly created object, selected */
#define SQ_NEWSEL  4

/* Partially created object */
#define SQ_INCMPLT 5


/*
 * KIC menus.
 */

struct menu {
    char *mEntry;
    short mActive;
    char mPrefix[6];
};
typedef struct menu MENU;

extern MENU *BasicMenu;
extern MENU *AttributeMenu;
extern MENU *DebugMenu;
extern MENU *PropertyMenu;
extern MENU AmbiguityMenu[];

#include "kicext.h"

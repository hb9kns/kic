/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Giles C. Billingsley
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
 * Main procedure for KIC.
 */

#ifdef WIN32
#include <windows.h>
#endif
#include "prefix.h"
#include "kic.h"
#include "hlpdefs.h"
#include <signal.h>


#define MAXCELLS  80        /* max number of cell names on the arg list  */

static char *CellNames[MAXCELLS];    /* Cell names in argumnet list      */
static int CurrentCell;              /* Current cell in argument list    */
static int NumCells;                 /* Number of cells in argument list */

#define Matching(string) !strcmp(Parameters.kpCommand,string)

/***********************************************************************
 *
 * Menu definition package;
 *
 ***********************************************************************/

#ifdef __STDC__
static void init_basic_menu(void);
static void init_attribute_menu(void);
static void init_debug_menu(void);
static void init_property_menu(void);
#else
static void init_basic_menu();
static void init_attribute_menu();
static void init_debug_menu();
static void init_property_menu();
#endif

MENU *BasicMenu;
MENU *AttributeMenu;
MENU *DebugMenu;
MENU *PropertyMenu;
MENU AmbiguityMenu[81];

char *Menu0;                    
char *Menu90;                
char *Menu180;            
char *Menu270;            
char *MenuMINSB;    
char *MenuMINSG;    
char *MenuMINSR;    
char *MenuPLUSB;    
char *MenuPLUSG;    
char *MenuPLUSR;    
char *Menu45S;            
char *MenuADLYR;    
char *MenuADPRP;    
char *MenuALLOC;    
char *MenuARC;            
char *MenuAREA;        
char *MenuARRAY;        
char *MenuATTRI;    
char *MenuBASIC;    
char *MenuBLINK;    
char *MenuBOXES;    
char *MenuBREAK;    
char *MenuBW;               
char *MenuCHLYR;    
char *MenuCNAMS;    
char *MenuCNTXT;
char *MenuCNVRT;
char *MenuCOLOR;       
char *MenuCOPY;       
char *MenuCRSYM;    
char *MenuCURSR;    
char *MenuDEBUG;    
char *MenuDELET;    
char *MenuDESEL;    
char *MenuDIMEN;    
char *MenuDIR;            
char *MenuDONUT;    
char *MenuEDIT;        
char *MenuERASE;        
char *MenuEXIT;    
char *MenuEXPND;    
char *MenuFILL;    
char *MenuFLASH;    
char *MenuFLATN;    
char *MenuFONT;    
char *MenuGRID;        
char *MenuHCOPY;        
char *MenuHELP;        
char *MenuLABEL;    
char *MenuLABLS;    
char *MenuLAST;        
char *MenuLAYER;
char *MenuLLREF;
char *MenuLOGO;    
char *MenuMARK;        
char *MenuMOVE;        
char *MenuMX;               
char *MenuMY;                
char *MenuPAN;            
char *MenuPEEK;        
char *MenuPLACE;        
char *MenuPOLYG;    
char *MenuPOP;            
char *MenuPRPTY;    
char *MenuPUSH;        
char *MenuRDRAW;    
char *MenuRGB;            
char *MenuRL;
char *MenuRMOVE;    
char *MenuRMPRP;    
char *MenuSAVE;        
char *MenuSELEC;    
char *MenuSHOW;        
char *MenuSIDES;    
char *MenuSNAP;        
char *MenuSTBOX;    
char *MenuSTRCH;    
char *MenuTB;            
char *MenuTBRL;            
char *MenuUNDO;        
char *MenuUPDAT;    
char *MenuVIEW;        
char *MenuVISIB;    
char *MenuWIDTH;    
char *MenuWINDO;    
char *MenuWIRES;    
char *MenuXOR;    
char *MenuZOOM;
static char *MenuSpace;

void
InitMenus()
{

    Menu0        = "0    ";
    Menu90       = "90   ";
    Menu180      = "180  ";
    Menu270      = "270  ";
    MenuMINSB    = "-b   ";
    MenuMINSG    = "-g   ";
    MenuMINSR    = "-r   ";
    MenuPLUSB    = "+b   ";
    MenuPLUSG    = "+g   ";
    MenuPLUSR    = "+r   ";
    Menu45S      = "45s  ";
    MenuADLYR    = "adlyr";
    MenuADPRP    = "adprp";
    MenuALLOC    = "alloc";
    MenuARC      = "arc  ";
    MenuAREA     = "area ";
    MenuARRAY    = "array";
    MenuATTRI    = "attri";
    MenuBASIC    = "basic";
    MenuBLINK    = "blink";
    MenuBOXES    = "boxes";
    MenuBREAK    = "break";
    MenuBW       = "bw   ";
    MenuCHLYR    = "chlyr";
    MenuCNAMS    = "cnams";
    MenuCNTXT    = "cntxt";
    MenuCNVRT    = "cnvrt";
    MenuCOLOR    = "color";
    MenuCOPY     = "copy ";
    MenuCRSYM    = "crsym";
    MenuCURSR    = "cursr";
    MenuDEBUG    = "debug";
    MenuDELET    = "delet";
    MenuDESEL    = "desel";
    MenuDIMEN    = "dimen";
    MenuDIR      = "dir  ";
    MenuDONUT    = "donut";
    MenuEDIT     = "edit ";
    MenuEXIT     = "quit ";
    MenuERASE    = "erase";
    MenuEXPND    = "expnd";
    MenuFILL     = "fill ";
    MenuFLASH    = "flash";
    MenuFLATN    = "flatn";
    MenuFONT     = "font ";
    MenuGRID     = "grid ";
    MenuHCOPY    = "hcopy";
    MenuHELP     = "help ";
    MenuLABEL    = "label";
    MenuLABLS    = "labls";
    MenuLAST     = "last ";
    MenuLAYER    = "layer";
    MenuLLREF    = "llref";
    MenuLOGO     = "logo ";
    MenuMARK     = "mark ";
    MenuMOVE     = "move ";
    MenuMX       = "mx   ";
    MenuMY       = "my   ";
    MenuPAN      = "pan  ";
    MenuPEEK     = "peek ";
    MenuPLACE    = "place";
    MenuPOLYG    = "polyg";
    MenuPOP      = "pop  ";
    MenuPRPTY    = "prpty";
    MenuPUSH     = "push ";
    MenuRDRAW    = "rdraw";
    MenuRGB      = "rgb  ";
    MenuRL       = "rl   ";
    MenuRMOVE    = "rmove";
    MenuRMPRP    = "rmprp";
    MenuSAVE     = "save ";
    MenuSELEC    = "selec";
    MenuSHOW     = "show ";
    MenuSIDES    = "sides";
    MenuSNAP     = "snap ";
    MenuSTRCH    = "strch";
    MenuTB       = "tb   ";
    MenuTBRL     = "tbrl ";
    MenuUNDO     = "undo ";
    MenuUPDAT    = "updat";
    MenuVIEW     = "view ";
    MenuVISIB    = "visib";
    MenuWIDTH    = "width";
    MenuWINDO    = "windo";
    MenuWIRES    = "wires";
    MenuXOR      = "xor  ";
    MenuZOOM     = "zoom ";
    MenuSpace    = "     ";


    init_basic_menu();
    init_attribute_menu();
    init_debug_menu();
    init_property_menu();
}

int NumBasicMenu = 60;

static void
init_basic_menu()

{
    int i = 0;

    if (BasicMenu == NULL)
        BasicMenu = (MENU *) tmalloc(NumBasicMenu*sizeof(MENU));
    memset(BasicMenu,0,NumBasicMenu*sizeof(MENU));

    BasicMenu[i++].mEntry = MenuHELP;
    BasicMenu[i++].mEntry = MenuATTRI;
    BasicMenu[i++].mEntry = MenuEDIT;
    BasicMenu[i++].mEntry = MenuDIR;
    BasicMenu[i++].mEntry = MenuSAVE;

    BasicMenu[i++].mEntry = MenuCNVRT;
    BasicMenu[i++].mEntry = MenuPRPTY;
    BasicMenu[i++].mEntry = MenuDEBUG;
    BasicMenu[i++].mEntry = MenuEXIT;
    BasicMenu[i++].mEntry = MenuSpace;

    BasicMenu[i++].mEntry = MenuEXPND;
    BasicMenu[i++].mEntry = MenuPEEK;
    BasicMenu[i++].mEntry = MenuPAN;
    BasicMenu[i++].mEntry = MenuZOOM;
    BasicMenu[i++].mEntry = MenuWINDO;

    BasicMenu[i++].mEntry = MenuVIEW;
    BasicMenu[i++].mEntry = MenuLAST;
    BasicMenu[i++].mEntry = MenuRDRAW;
    BasicMenu[i++].mEntry = MenuGRID;
    BasicMenu[i++].mEntry = MenuSNAP;

    BasicMenu[i++].mEntry = MenuSpace;
    BasicMenu[i++].mEntry = MenuBOXES;
    BasicMenu[i++].mEntry = MenuLABEL;
    BasicMenu[i++].mEntry = MenuWIRES;
    BasicMenu[i++].mEntry = MenuWIDTH;

    BasicMenu[i++].mEntry = Menu45S;
    BasicMenu[i++].mEntry = MenuPOLYG;
    BasicMenu[i++].mEntry = MenuDONUT;
    BasicMenu[i++].mEntry = MenuFLASH;
    BasicMenu[i++].mEntry = MenuARC;

    BasicMenu[i++].mEntry = MenuSpace;
    BasicMenu[i++].mEntry = MenuMOVE;
    BasicMenu[i++].mEntry = MenuCOPY;
    BasicMenu[i++].mEntry = MenuUNDO;
    BasicMenu[i++].mEntry = MenuSpace;

    BasicMenu[i++].mEntry = MenuLAYER;
    BasicMenu[i++].mEntry = MenuAREA;
    BasicMenu[i++].mEntry = MenuSELEC;
    BasicMenu[i++].mEntry = MenuDESEL;
    BasicMenu[i++].mEntry = MenuCHLYR;

    BasicMenu[i++].mEntry = MenuDELET;
    BasicMenu[i++].mEntry = MenuSTRCH;
    BasicMenu[i++].mEntry = MenuTBRL;
    BasicMenu[i++].mEntry = MenuERASE;
    BasicMenu[i++].mEntry = MenuXOR;

    BasicMenu[i++].mEntry = MenuBREAK;
    BasicMenu[i++].mEntry = MenuMX;
    BasicMenu[i++].mEntry = MenuMY;
    BasicMenu[i++].mEntry = Menu0;
    BasicMenu[i++].mEntry = MenuSpace;

    BasicMenu[i++].mEntry = MenuPLACE;
    BasicMenu[i++].mEntry = MenuLLREF;
    BasicMenu[i++].mEntry = MenuARRAY;
    BasicMenu[i++].mEntry = MenuCRSYM;
    BasicMenu[i++].mEntry = MenuFLATN;

    BasicMenu[i++].mEntry = MenuPUSH;
    BasicMenu[i++].mEntry = MenuCNTXT;
    BasicMenu[i++].mEntry = MenuPOP;
    BasicMenu[i++].mEntry = MenuLOGO;
    BasicMenu[i++].mEntry = NULL;
}


static void
init_attribute_menu()

{
    int i = 0;
    int NumAttributeMenu = 33;
 
    if (AttributeMenu == NULL)
        AttributeMenu = (MENU *) tmalloc(NumAttributeMenu*sizeof(MENU));
    memset(AttributeMenu,0,NumAttributeMenu*sizeof(MENU));
 
    AttributeMenu[i++].mEntry = MenuHELP;
    AttributeMenu[i++].mEntry = MenuBASIC;
    AttributeMenu[i++].mEntry = MenuSpace;
    AttributeMenu[i++].mEntry = MenuPAN;
    AttributeMenu[i++].mEntry = MenuZOOM;

    AttributeMenu[i++].mEntry = MenuRDRAW;
    AttributeMenu[i++].mEntry = MenuSpace;
    AttributeMenu[i++].mEntry = MenuLABLS;
    AttributeMenu[i++].mEntry = MenuCNAMS;
    AttributeMenu[i++].mEntry = MenuMARK;

    AttributeMenu[i++].mEntry = MenuGRID;
    AttributeMenu[i++].mEntry = MenuSIDES;

    AttributeMenu[i++].mEntry = MenuFONT;
    AttributeMenu[i++].mEntry = MenuCURSR;

    AttributeMenu[i++].mEntry = MenuDIMEN;

    AttributeMenu[i++].mEntry = MenuVISIB;
    AttributeMenu[i++].mEntry = MenuADLYR;
    AttributeMenu[i++].mEntry = MenuRMOVE;
    AttributeMenu[i++].mEntry = MenuSpace;
    AttributeMenu[i++].mEntry = MenuUPDAT;

    AttributeMenu[i++].mEntry = MenuHCOPY;
    AttributeMenu[i++].mEntry = MenuSpace;
    AttributeMenu[i++].mEntry = MenuFILL;
    AttributeMenu[i++].mEntry = MenuCOLOR;
    AttributeMenu[i++].mEntry = MenuSpace;

    AttributeMenu[i++].mEntry = MenuPLUSR;
    AttributeMenu[i++].mEntry = MenuMINSR;
    AttributeMenu[i++].mEntry = MenuPLUSG;
    AttributeMenu[i++].mEntry = MenuMINSG;
    AttributeMenu[i++].mEntry = MenuPLUSB;

    AttributeMenu[i++].mEntry = MenuMINSB;
    AttributeMenu[i++].mEntry = MenuRGB;
    AttributeMenu[i++].mEntry = NULL;
}


static void
init_debug_menu()

{
    int i = 0;
    int NumDebugMenu = 7;
 
    if (DebugMenu == NULL)
        DebugMenu = (MENU *) tmalloc(NumDebugMenu*sizeof(MENU));
    memset(DebugMenu,0,NumDebugMenu*sizeof(MENU));
 
    DebugMenu[i++].mEntry = MenuHELP;
    DebugMenu[i++].mEntry = MenuSpace;
    DebugMenu[i++].mEntry = MenuBASIC;
    DebugMenu[i++].mEntry = MenuSpace;
    DebugMenu[i++].mEntry = MenuBW;
 
    DebugMenu[i++].mEntry = MenuALLOC;
    DebugMenu[i++].mEntry = NULL;
}
 

static void
init_property_menu()

{
    int i = 0;
    int NumPropertyMenu = 13;
 
    if (PropertyMenu == NULL)
        PropertyMenu = (MENU *) tmalloc(NumPropertyMenu*sizeof(MENU));
    memset(PropertyMenu,0,NumPropertyMenu*sizeof(MENU));
 
    PropertyMenu[i++].mEntry = MenuHELP;
    PropertyMenu[i++].mEntry = MenuSpace;
    PropertyMenu[i++].mEntry = MenuBASIC;
    PropertyMenu[i++].mEntry = MenuSpace;
    PropertyMenu[i++].mEntry = MenuLAYER;
 
    PropertyMenu[i++].mEntry = MenuAREA;
    PropertyMenu[i++].mEntry = MenuSELEC;
    PropertyMenu[i++].mEntry = MenuDESEL;
    PropertyMenu[i++].mEntry = MenuSpace;
    PropertyMenu[i++].mEntry = MenuSHOW;
 
    PropertyMenu[i++].mEntry = MenuADPRP;
    PropertyMenu[i++].mEntry = MenuRMPRP;
    PropertyMenu[i++].mEntry = NULL;
}


int
main(argc,argv)

int argc;
char *argv[];
{
    int Int1;
    char *Tech;

    CurrentCell = 0;
    NumCells = 0;
    /* partial initialization */
    Parameters.kpMenu = BASICMENU;
#ifndef USE_OLD_MALLOC
    nm_set_block_size(NMALLOC_BLOCK_SIZE);
#endif
#ifdef WIN32
    {
        // If started from an icon, ditch the console
        char *t;
        STARTUPINFO si;
        GetStartupInfo(&si);
        if (!si.lpTitle ||
                ((t = strrchr(si.lpTitle, '.')) != 0 && !stricmp(t, ".lnk")))
            FreeConsole();
    }
#endif
    FB.fInitialized = False;
    FB.fDisplay = CopyString(getenv("DISPLAY"));
    for (Int1 = 1; Int1 < argc; ++Int1) {
        if (*argv[Int1] == '-' && *(argv[Int1]+1) == 'd') {
            ++argv[Int1];
            if (*(++argv[Int1]) == '\0')
                FB.fDisplay = argv[++Int1];
            else
                FB.fDisplay = argv[Int1];
        }
        else if (*argv[Int1] == '-' && *(argv[Int1]+1) == 't') {
            if (*(argv[Int1]+2) == '\0')
                Tech = argv[++Int1];
            else
                Tech = argv[Int1]+2;
#ifdef MSDOS
            if (strlen(Tech) > 3)
                Tech[3] = '\0';
#endif
            TECH_EXT = malloc(strlen(Tech) + 1);
            if (TECH_EXT == NULL)
                fatal_error("Memory allocation failure.");
            strcpy(TECH_EXT,Tech);
        }
        else
            CellNames[NumCells++] = argv[Int1];
        if (NumCells >= MAXCELLS) {
            fprintf(stderr,"Too many file names.\n");
            NumCells = MAXCELLS - 1;
#ifdef WIN32
            Sleep(3000);
#else
            sleep(3);
#endif
            break;
        }
    }

    CellNames[NumCells] = NULL;
    InitGlobal();
    Init();

    /* Edit default cell name if no command line cell */
    if (!NumCells) {
        NumCells++;
        CellNames[0] = DEFAULT_EDIT_FILE;
        CellNames[1] = NULL;
    }

    if (NumCells) strcpy(Parameters.kpCellName,CellNames[CurrentCell]);
    InitMenus();
    KICMain();
    return (0);
}


void
KICMain()

{
    int LookedAhead = False;
    char *TypeIn;

    InitVLT();
    FBForeground(ERASE,0);
    FBFlood();
    Basic((int*)&TypeIn); /* dummy variable */
    ShowLayerTable();
    Edit(True,True,False);

    loop {
        InitSignals();
        if (Not LookedAhead)
            Point();
        else
            LookedAhead = False;
        if (Parameters.kpCellName[0] == EOS) {
            /*
             * The CD package doesn't check the integrity of symbol
             * descriptors and now Parameters.kpCellDesc == NULL. 
             * Except for a few commands, each command invokes the
             * CD package.  If such a command is invoked now, the
             * CD package will crash and thus KIC will.  So, force user
             * to select a cell to edit before invoking any such command.
             */
            if (Not (Matching(MenuEDIT) Or Matching(MenuDEBUG) Or
                Matching(MenuDIR) Or  Matching(MenuEXIT) Or
                Matching(MenuHELP))) {
                ShowPrompt("You haven't selected a cell to edit.");
                continue;
            }
        }
        if (SafeCmds(&LookedAhead)) continue;

        if (Matching(MenuEXIT))  { AbortKIC();                 continue; }
        if (Matching(MenuADLYR)) { AddLayer();                 continue; }
        if (Matching(MenuADPRP)) { AddProperty();              continue; }
        if (Matching(MenuARC))   { Arcs(&LookedAhead);         continue; }
        if (Matching(MenuAREA))  { Area(&LookedAhead);         continue; }
        if (Matching(MenuATTRI)) { Attri(&LookedAhead);        continue; }
        if (Matching(MenuBASIC)) { Basic(&LookedAhead);        continue; }
        if (Matching(MenuBOXES)) { Boxes(&LookedAhead);        continue; }
        if (Matching(MenuBREAK)) { Break(&LookedAhead);        continue; }
        if (Matching(MenuCHLYR)) { ChangeLayer(&LookedAhead);  continue; }
        if (Matching(MenuCOPY))  { Copy(&LookedAhead);         continue; }
        if (Matching(MenuCRSYM)) { NewSymbol();                continue; }
        if (Matching(MenuDELET)) { Del(&LookedAhead);          continue; }
        if (Matching(MenuDESEL)) { Desel();                    continue; }
        if (Matching(MenuDONUT)) { Doughnut(&LookedAhead);     continue; }
        if (Matching(MenuEDIT))  { Edit(False,True,False);     continue; }
        if (Matching(MenuERASE)) { Erase(&LookedAhead);        continue; }
        if (Matching(MenuFLASH)) { Flash(&LookedAhead);        continue; }
        if (Matching(MenuFLATN)) { Flatten(&LookedAhead);      continue; }
        if (Matching(MenuLABEL)) { Label(&LookedAhead);        continue; }
        if (Matching(MenuLOGO))  { Logo(&LookedAhead);         continue; }
        if (Matching(MenuMOVE))  { Move(&LookedAhead);         continue; }
        if (Matching(MenuPLACE)) { Place(&LookedAhead);        continue; }
        if (Matching(MenuPOLYG)) { Polygons(&LookedAhead);     continue; }
        if (Matching(MenuPOP))   { Pop();                      continue; }
        if (Matching(MenuPRPTY)) { Properties(&LookedAhead);   continue; }
        if (Matching(MenuPUSH))  { Push(&LookedAhead);         continue; }
        if (Matching(MenuRMOVE)) { RemoveLayer(&LookedAhead);  continue; }
        if (Matching(MenuRMPRP)) { RemoveProperty();           continue; }
        if (Matching(MenuSAVE))  { WriteCell();                continue; }
        if (Matching(MenuSELEC)) { Sel(&LookedAhead);          continue; }
        if (Matching(MenuSHOW))  { DoShowProperties();         continue; }
        if (Matching(MenuSTRCH)) { Stretch(&LookedAhead);      continue; }
        if (Matching(MenuUNDO))  { Undo();                     continue; }
        if (Matching(MenuUPDAT)) { Updat();                    continue; }
        if (Matching(MenuWIDTH)) { Width(&LookedAhead);        continue; }
        if (Matching(MenuWIRES)) { Wires(&LookedAhead);        continue; }
        if (Matching(MenuXOR))   { XORbox(&LookedAhead);       continue; }
    }
}


int
SafeCmds(LookedAhead)

int *LookedAhead;
{

    if (Matching(Menu0))     { Rotat0();                   return True; }
    if (Matching(Menu90))    { Rotat90();                  return True; }
    if (Matching(Menu180))   { Rotat180();                 return True; }
    if (Matching(Menu270))   { Rotat270();                 return True; }
    if (Matching(Menu45S))   { DoSet45();                  return True; }
    if (Matching(MenuALLOC)) { DoAlloc();                  return True; }
    if (Matching(MenuARRAY)) { GetArraySpec();             return True; }
    if (Matching(MenuBLINK)) { Blink(LookedAhead);         return True; }
    if (Matching(MenuBW))    { DoBW();                     return True; }
    if (Matching(MenuCNTXT)) { ShowContext();              return True; }
    if (Matching(MenuCNVRT)) { Convert();                  return True; }
    if (Matching(MenuCNAMS)) { LabelInstances();           return True; }
    if (Matching(MenuCOLOR)) { AttribColor(LookedAhead);   return True; }
    if (Matching(MenuCURSR)) { SelectKicCursor();          return True; }
    if (Matching(MenuDEBUG)) { Debug(LookedAhead);         return True; }
    if (Matching(MenuDIMEN)) { Dimen(LookedAhead);         return True; }
    if (Matching(MenuDIR))   { Dir();                      return True; }
    if (Matching(MenuEXPND)) { Expand();                   return True; }
    if (Matching(MenuFILL))  { Fill(LookedAhead);          return True; }
    if (Matching(MenuFONT))  { SelectKicFont();            return True; }
    if (Matching(MenuGRID))  { SetGrid(LookedAhead);       return True; }
    if (Matching(MenuHCOPY)) { Hcopy();                    return True; }
    if (Matching(MenuHELP))  { Help();                     return True; }
    if (Matching(MenuLABLS)) { DisplayLabels();            return True; }
    if (Matching(MenuLAST))  { LastView();                 return True; }
    if (Matching(MenuLAYER)) { Layer();                    return True; }
    if (Matching(MenuLLREF)) { Handle();                   return True; }
    if (Matching(MenuMARK))  { Mark();                     return True; }
    if (Matching(MenuMINSB)) { SetColor('b','-');          return True; }
    if (Matching(MenuMINSG)) { SetColor('g','-');          return True; }
    if (Matching(MenuMINSR)) { SetColor('r','-');          return True; }
    if (Matching(MenuMX))    { MX();                       return True; }
    if (Matching(MenuMY))    { MY();                       return True; }
    if (Matching(MenuPAN))   { Pan(LookedAhead);           return True; }
    if (Matching(MenuPEEK))  { Peek();                     return True; }
    if (Matching(MenuPLUSB)) { SetColor('b','+');          return True; }
    if (Matching(MenuPLUSG)) { SetColor('g','+');          return True; }
    if (Matching(MenuPLUSR)) { SetColor('r','+');          return True; }
    if (Matching(MenuRDRAW)) { Rdraw();                    return True; }
    if (Matching(MenuRGB))   { ShowRGB();                  return True; }
    if (Matching(MenuRL))    { SetStretchMode();           return True; }
    if (Matching(MenuSIDES)) { Sides();                    return True; }
    if (Matching(MenuSNAP))  { Snap();                     return True; }
    if (Matching(MenuTB))    { SetStretchMode();           return True; }
    if (Matching(MenuTBRL))  { SetStretchMode();           return True; }
    if (Matching(MenuVIEW))  { ShowFull();                 return True; }
    if (Matching(MenuVISIB)) { Visib(LookedAhead);         return True; }
    if (Matching(MenuWINDO)) { Windo(LookedAhead);         return True; }
    if (Matching(MenuZOOM))  { Zoom(LookedAhead);          return True; }

    return False;
}


char 
*NextCellName()

{
    if (++CurrentCell >= NumCells) {
        if (NumCells > 0) {
            --CurrentCell;
            ShowPromptAndWait("No more cells to edit.");
        }
        else
            ShowPromptAndWait("No current cell name.");
        return NULL;
    }
    else
        return CellNames[CurrentCell];
}


void
SaveTechFile()

{
    char String[161];
    int Layer,i;
    FILE *TechFileDesc;
    struct eparms *e;
#ifdef CIFPLOT
    char *patbuf;
    FILE *cifplot;
    FILE *patterns;
    int n;
#endif


    if (TECH_EXT)
        sprintf(String,"%s.%s",TECHNAME,TECH_EXT);
    else
        strcpy(String,TECHNAME);

    if ((TechFileDesc = fopen(String,"w")) == NULL) {
        sprintf(TypeOut,"Can't write %s file.",String);
        ShowPrompt(TypeOut);
        return;
    }

    /* print path */
    fprintf(TechFileDesc,"Path? ( %s ) \n",PGetPath());
    fprintf(TechFileDesc,"\n");
    for (Layer = 1; Layer <= NumLayerTable; ++Layer) {
        /*
         * LayerName?
         */
        fprintf(TechFileDesc,"LayerName? %c%c%c%c\n",
            LayerTable[Layer].klTechnology ,
            LayerTable[Layer].klMask[0] ,
            LayerTable[Layer].klMask[1] ,
            LayerTable[Layer].klMask[2] );
        /*
         * RGB?
         */
        fprintf(TechFileDesc,"RGB? %d %d %d\n",LayerTable[Layer].klR,
            LayerTable[Layer].klG,LayerTable[Layer].klB);
        /*
         * Symbolic?
         */
        if (LayerTable[Layer].klAttributes & SYMBOLIC)
            fprintf(TechFileDesc,"Symbolic? y\n");
        else
            fprintf(TechFileDesc,"Symbolic? n\n");

        /*
         * Filled?
         */
        fprintf(TechFileDesc,"Filled?");
        if (!(LayerTable[Layer].klAttributes & FILLED))
            fprintf(TechFileDesc," n\n");
        else {
            if (LayerTable[Layer].klStyle[0] == 0 And
                 LayerTable[Layer].klStyle[1] == 0 And
                 LayerTable[Layer].klStyle[2] == 0 And
                 LayerTable[Layer].klStyle[3] == 0 And
                 LayerTable[Layer].klStyle[4] == 0 And
                 LayerTable[Layer].klStyle[5] == 0 And
                 LayerTable[Layer].klStyle[6] == 0 And
                 LayerTable[Layer].klStyle[7] == 0 ) {
                 fprintf(TechFileDesc," y\n");
             }
             else {
                for (i=0; i<8; ++i)
                    fprintf(TechFileDesc," %02x",
                        LayerTable[Layer].klStyle[i]);
                if (LayerTable[Layer].klAttributes & OUTLINED)
                        fprintf(TechFileDesc," outline\n");
                else
                    fprintf(TechFileDesc,"\n");
            }
        }
        /*
         * AltFilled?
         */
        fprintf(TechFileDesc,"AltFilled?");
        if (!(LayerTable[Layer].klAttributes & ALT_FILLED))
            fprintf(TechFileDesc," n\n");
        else {
            if (LayerTable[Layer].klAltStyle[0] == 0 And
                 LayerTable[Layer].klAltStyle[1] == 0 And
                 LayerTable[Layer].klAltStyle[2] == 0 And
                 LayerTable[Layer].klAltStyle[3] == 0 And
                 LayerTable[Layer].klAltStyle[4] == 0 And
                 LayerTable[Layer].klAltStyle[5] == 0 And
                 LayerTable[Layer].klAltStyle[6] == 0 And
                 LayerTable[Layer].klAltStyle[7] == 0 ) {
                 fprintf(TechFileDesc," y\n");
             }
             else {
                for (i=0; i<8; ++i)
                    fprintf(TechFileDesc," %02x",
                        LayerTable[Layer].klAltStyle[i]);
                if (LayerTable[Layer].klAttributes & ALT_OUTLINED)
                        fprintf(TechFileDesc," outline\n");
                else
                    fprintf(TechFileDesc,"\n");
            }
        }

        /*
         * Invisible?
         */
        if (!(LayerTable[Layer].klAttributes & VISIBLE))
            fprintf(TechFileDesc,"Invisible\n");

        /*
         * AltInvisible?
         */
        if (!(LayerTable[Layer].klAttributes & ALT_VISIBLE))
            fprintf(TechFileDesc,"AltInvisible\n");

        /*
         * Blinkers?
         */
        if (LayerTable[Layer].klAttributes & BLINK)
            fprintf(TechFileDesc,"Blink\n");

        /*
         * MinDimensions?
         */
        fprintf(TechFileDesc,"MinDimensions %g %g\n",
            (double)LayerTable[Layer].klMinDimensions/RESOLUTION,
            (double)LayerTable[Layer].klWireWidth/RESOLUTION);

        /*
         * StreamData?
         */
        fprintf(TechFileDesc,"StreamData? %d %d\n",
            LayerTable[Layer].klStreamNumber,
            LayerTable[Layer].klStreamDataType);

        /*
         * Electrical info?
         */
        if ((e = LayerTable[Layer].klElectrical) != NULL) {
            switch (e->e_type) {
                case ERESIS:
                    fprintf(TechFileDesc,
                        "resistance %g\n",*e->e_parms);
                    break;
                case ECAP:
                    fprintf(TechFileDesc,
                        "capacitance %g\n",*e->e_parms);
                    break;
                case ETRANS:
                    fprintf(TechFileDesc,
                        "tranline %g %g %g %g %g %g\n",
                        e->e_parms[0],e->e_parms[1],e->e_parms[2],
                        e->e_parms[3],e->e_parms[4],e->e_parms[5]);
                    break;
            }
        }
        fprintf(TechFileDesc,"\n");
    }
    fprintf(TechFileDesc,"Highlighting %d %d %d\n",
        ColorTable[HighlightingColor].R,
        ColorTable[HighlightingColor].G,
        ColorTable[HighlightingColor].B);
    fprintf(TechFileDesc,"Background %d %d %d\n",
        ColorTable[0].R,
        ColorTable[0].G,
        ColorTable[0].B);
    if (Parameters.kpMergeColors) {
        fprintf(TechFileDesc,"MenuText %d\n",
            ColorTable[MenuTextColor].Ent);
        fprintf(TechFileDesc,"MenuSelect %d\n",
            ColorTable[MenuSelectColor].Ent);
        fprintf(TechFileDesc,"MenuHighlighting %d\n",
            ColorTable[MenuHighlightingColor].Ent);
        fprintf(TechFileDesc,"MenuPrompt %d\n",
            ColorTable[MenuPromptColor].Ent);
        fprintf(TechFileDesc,"MoreText %d\n",
            ColorTable[MoreTextColor].Ent);
        fprintf(TechFileDesc,"FineGrid %d\n",
            ColorTable[FineGridColor].Ent);
        fprintf(TechFileDesc,"CoarseGrid %d\n",
            ColorTable[CoarseGridColor].Ent);
        fprintf(TechFileDesc,"InstanceBox %d\n",
            ColorTable[InstanceBBColor].Ent);
        fprintf(TechFileDesc,"InstanceName %d\n",
            ColorTable[InstanceNameColor].Ent);
        fprintf(TechFileDesc,"InstanceSize %d\n",
            ColorTable[InstanceSizeColor].Ent);
    }
    else {
        fprintf(TechFileDesc,"MenuText %d %d %d\n",
            ColorTable[MenuTextColor].R,
            ColorTable[MenuTextColor].G,
            ColorTable[MenuTextColor].B);
        fprintf(TechFileDesc,"MenuSelect %d %d %d\n",
            ColorTable[MenuSelectColor].R,
            ColorTable[MenuSelectColor].G,
            ColorTable[MenuSelectColor].B);
        fprintf(TechFileDesc,"MenuHighlighting %d %d %d\n",
            ColorTable[MenuHighlightingColor].R,
            ColorTable[MenuHighlightingColor].G,
            ColorTable[MenuHighlightingColor].B);
        fprintf(TechFileDesc,"MenuPrompt %d %d %d\n",
            ColorTable[MenuPromptColor].R,
            ColorTable[MenuPromptColor].G,
            ColorTable[MenuPromptColor].B);
        fprintf(TechFileDesc,"MoreText %d %d %d\n",
            ColorTable[MoreTextColor].R,
            ColorTable[MoreTextColor].G,
            ColorTable[MoreTextColor].B);
        fprintf(TechFileDesc,"FineGrid %d %d %d\n",
            ColorTable[FineGridColor].R,
            ColorTable[FineGridColor].G,
            ColorTable[FineGridColor].B);
        fprintf(TechFileDesc,"CoarseGrid %d %d %d\n",
            ColorTable[CoarseGridColor].R,
            ColorTable[CoarseGridColor].G,
            ColorTable[CoarseGridColor].B);
        fprintf(TechFileDesc,"InstanceBox %d %d %d\n",
            ColorTable[InstanceBBColor].R,
            ColorTable[InstanceBBColor].G,
            ColorTable[InstanceBBColor].B);
        fprintf(TechFileDesc,"InstanceName %d %d %d\n",
            ColorTable[InstanceNameColor].R,
            ColorTable[InstanceNameColor].G,
            ColorTable[InstanceNameColor].B);
        fprintf(TechFileDesc,"InstanceSize %d %d %d\n",
            ColorTable[InstanceSizeColor].R,
            ColorTable[InstanceSizeColor].G,
            ColorTable[InstanceSizeColor].B);
    }
    fprintf(TechFileDesc,"GridSpacing %g\n",
        (double)Parameters.kpGrid/RESOLUTION);
    fprintf(TechFileDesc,"GridStyle %d\n",
        Parameters.kpGridLineStyle);
    if (Not Parameters.kpGridOnTop)
        fprintf(TechFileDesc,"GridOnBottom\n");
    if (Parameters.kpShowGridInLargeViewport)
        fprintf(TechFileDesc,"ShowGrid\n");
    if (Parameters.kpHardcopyGrid)
        fprintf(TechFileDesc,"AltShowGrid\n");
    if (Parameters.kpDisplayAllLabels)
        fprintf(TechFileDesc,"DisplayAllText\n");
    if (Parameters.kpLabelAllInstances)
        fprintf(TechFileDesc,"LabelAllInstances\n");
    fprintf(TechFileDesc,"RoundFlashSides %d\n",
            Parameters.kpNumRoundFlashSides);
    if (Parameters.kpHardcopyDevice)
        fprintf(TechFileDesc,"AltDevice %s\n",
            Parameters.kpHardcopyDevice);
    fprintf(TechFileDesc,"AltResolution %d\n",
        Parameters.kpHardcopyResolution);
    fprintf(TechFileDesc,"AltFormat %c\n",
        *Parameters.kpHardcopyFormat);

    fprintf(TechFileDesc,"Snapping %d\n",
        Parameters.kpPixToLambdaSnapping);
    if (View->kvFineViewportOnBottom == False)
        fprintf(TechFileDesc,"FineViewportOnSide\n");
    if (View->kvControl == SPLITSCREEN)
        fprintf(TechFileDesc,"SplitScreen\n");
    if (Parameters.kpFontName != NULL)
        fprintf(TechFileDesc,"FontName %s\n",
            Parameters.kpFontName);
    if (Parameters.kpCursorShape >= 0)
        fprintf(TechFileDesc,"CursorShape %d\n",
            Parameters.kpCursorShape);
    if (Parameters.kpFullScreenCursor == True)
        fprintf(TechFileDesc,"FullScreenCursor\n");
    fprintf(TechFileDesc,"BeepVolume %d\n",
        Parameters.kpPointBeepVolume);

    fclose(TechFileDesc);

    sprintf(TypeOut,"Current attributes updated in %s file",String);
    ShowPrompt(TypeOut);

    /*
     * cifplot pattern file
     */
#ifdef CIFPLOT
    patbuf = String;
    if ((patterns = POpen(CIFPLOT_PATTERNS, "r", (char *)NULL, (char **)NULL))
        == NULL) {
        ShowPrompt("Can't open cifplot pattern file.\n");
        return;
    }
    if ((cifplot = fopen("cifplot","w")) == NULL) {
        ShowPrompt("Can't create cifplot pattern file.\n");
        return;
    }
    fprintf(cifplot, "\n");
    Layer = 0;
    while (++Layer <= NumLayerTable) {
        if (fscanf(patterns,"%s",patbuf) <= 0) {
            rewind(patterns);
            --Layer;
            continue;
        }
        if (LayerTable[Layer].klTechnology != ' ') {
            fprintf(cifplot, "\"%c", LayerTable[Layer].klTechnology);
            for (i=0; i<3; ++i) {
                if (LayerTable[Layer].klMask[i] == ' ')
                    break;
                else
                    fprintf(cifplot, "%c", LayerTable[Layer].klMask[i]);
            }
            fprintf(cifplot, "\"%s\n", patbuf);
        }
    }
    fprintf(cifplot, "\n");
    fclose(patterns);
    fclose(cifplot);
#endif
}

/* Interrupt handling */

#if __STDC__
static void segv_err_handler();
#ifdef SIGBUS
static void bus_err_handler();
#endif
static void ill_err_handler();
static void fpe_err_handler();
static void int_handler();
#else
static void segv_err_handler();
static void bus_err_handler();
static void ill_err_handler();
static void fpe_err_handler();
static void int_handler();
#endif


void
InitSignals()

{
    Parameters.kpSIGINTERRUPT = False;
    (void) signal(SIGSEGV, segv_err_handler);
#ifdef SIGBUS
    (void) signal(SIGBUS, bus_err_handler);
#endif
    (void) signal(SIGILL, ill_err_handler);
    (void) signal(SIGFPE, fpe_err_handler);
    (void) signal(SIGINT, int_handler);
}


static void
segv_err_handler()

{
    (void) signal(SIGSEGV,SIG_DFL);
    ShowPromptAndWait("Fatal internal error: segmentation violation.");
    AbortKIC();
    FBEnd();
    exit(0);
}


#ifdef SIGBUS
static void
bus_err_handler()

{
    (void) signal(SIGBUS,SIG_DFL);
    ShowPromptAndWait("Fatal internal error: bus error.");
    AbortKIC();
    FBEnd();
    exit(0);
}
#endif


static void
ill_err_handler()

{
    (void) signal(SIGILL,SIG_DFL);
    ShowPromptAndWait("Fatal internal error: illegal instruction.");
    AbortKIC();
    FBEnd();
    exit(0);
}


static void
fpe_err_handler()

{
    (void) signal(SIGFPE,SIG_DFL);
    ShowPromptAndWait("Fatal internal error: floating point exception.");
    AbortKIC();
    FBEnd();
    exit(0);
}


static void
int_handler()

{
    Parameters.kpSIGINTERRUPT = True;
}


char *
CopyString(s)

char *s;
{
    char *t;

    if (s) {
        t = tmalloc(strlen(s)+1);
        strcpy(t,s);
        return t;
    }
    return s;
}


char *
tmalloc(x)

unsigned x;
{
    char *c = (char*)malloc(x);
    if (c == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    memset(c,0,x);
    return c;
}


void
MallocFailed()

{
    char *cp, buf[256];
    char tn[32];
    if (CDStatusInt != CDMALLOCFAILED)
        return;
    strcpy(tn, "KcXXXXXX");
    cp = mktemp(tn);
    FBEnd();
    strcpy(buf, "Memory allocation error.  ");

    if (Parameters.kpCellName[0] != EOS) {
        if (Not CDUpdate(Parameters.kpCellDesc,cp))
            strcat(buf,"Sorry, couldn't save current cell.");
        else {
            strcat(buf,"Current cell saved in ");
            strcat(buf,cp);
            strcat(buf,".");
        }
    }
    fatal_error(buf);
}


/* unused features for CD */

/* Called from CDEndMakeCall() */
void
UpdateProperties() {}

/* Called from POpen() */
FILE *
OpenDevice()
{ return NULL; }



/***********************************************************************
 *
 * Help interface.
 * 
 *
 ***********************************************************************/


extern FILE *cp_in,*cp_out,*cp_err;
extern char *MenuHELP;
extern char *MenuPLUSR,*MenuMINSR;
extern char *MenuPLUSG,*MenuMINSG;
extern char *MenuPLUSB,*MenuMINSB;
extern char *Menu0,*Menu90,*Menu180,*Menu270;
extern char *MenuMX,*MenuMY;
extern char *MenuTB,*MenuRL;
extern char *Menu45S;

/* PATH_TO_HELP should have been defined */

extern char *HELPPATH;


void
Help()

{
    int dummy;
    char cmd[32], *c;

    MenuSelect(MenuHELP);
    *TypeOut = '\0';
    HELPPATH = PATH_TO_HELP;
    cp_in = stdin;
    cp_err = stderr;
#ifdef MSDOS
    cp_out = fopen("NUL","w");
    c = getenv("SRWDRV");
    if (c && isalpha(*c) && (*c = tolower(*c)) > 'b')
        *HELPPATH = *c;
#else
#ifdef WIN32
    cp_out = fopen("NUL","w");
#else
    cp_out = fopen("/dev/null","w");
#endif
#endif
    ShowPrompt("Point to menu item for help (ESC to exit help).");

    loop {
        switch (PointLoopSafe(&dummy)) {
        case PL_ESC:
            fclose(cp_out);
            MenuDeselect(MenuHELP);
            ErasePrompt();
            EnableMore(False);
            FullRedisplay();
            return;
        case PL_PCW:
            *TypeOut = '\0';
            continue;
        case PL_CMD:
            if (Matching(MenuMINSR) ||
                Matching(MenuPLUSR) ||
                Matching(MenuMINSG) ||
                Matching(MenuPLUSG) ||
                Matching(MenuMINSB) ||
                Matching(MenuPLUSB))
                strcpy(Parameters.kpCommand,"plusretc");

            else

            if (Matching(Menu0)   ||
                Matching(Menu90)  ||
                Matching(Menu180) ||
                Matching(Menu270))
                strcpy(Parameters.kpCommand,"rotate");

            else

            if (Matching(MenuMX) ||
                Matching(MenuMY))
                strcpy(Parameters.kpCommand,"mirror");

            else

            if (Matching(MenuTB) ||
                Matching(MenuRL))
                strcpy(Parameters.kpCommand,"tbrl");

            else

            if (Matching(Menu45S))
                strcpy(Parameters.kpCommand,"a45s");

            sprintf(cmd,"%s",Parameters.kpCommand);
            strlwr(cmd);
            if ((c = strchr(cmd,' ')) != NULL) *c = '\0';
            if (!cmd[0])
                continue;
            EnableMore(False);
            EnableMore(True);
            hlp_main(HELPPATH,cmd);
            MorePageDisplay();
        }
    }
}


/* for help */
void
out_init() {}


void
PutString(string)

char *string;
{
    if (strlen(TypeOut) + strlen(string) < 200)
        strcat(TypeOut,string);
    else
        strcat(TypeOut,"\n");
    if (strchr(TypeOut,'\n') == NULL) return;
    if (MoreLine(TypeOut))
        EnableMore(False);
    *TypeOut = '\0';
}


void
PutErrorString(string)

char *string;
{
    PutString(string);
}


void
PutBoldString(string)

char *string;
{
    int tmp;

    tmp = ColorTable[MoreTextColor].Ent;
    ColorTable[MoreTextColor].Ent = ColorTable[HighlightingColor].Ent;
    PutString(string);
    ColorTable[MoreTextColor].Ent = tmp;
}


char *
GetString(s,n,fp,prompt)

char *s;
int n;
FILE *fp;
char *prompt;
{
    char *c;

    if (!prompt) {
        ErasePrompt();
        return NULL;
    }
    ShowPrompt(prompt);
    MorePageDisplay();
    c = FBEdit(NULL);
    ErasePrompt();
    if (c == NULL || *c == '\0' || *c == '\n') return NULL;
    strcpy(s,c);
    return s;
}


void
RepaintWindow(i)

int i;
{
    if (i < 0) return;
    RedisplayKIC();
}

void
SetDisplayWindow() {}

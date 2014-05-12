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
#include "kic.h"


int NumLayerTable;
struct kl LayerTable[CDNUMLAYERS+1];
struct kp Parameters;
struct cl ColorTable[12];
struct f FB = { 0 };

int  argc;
char **argv;

#if __STDC__
extern void help(void);
extern char *nextarg(void);
#else
extern void help();
extern char *nextarg();
#endif


int
main(ac, av)

int ac;
char *av[];
{
    float Float1 = 1.0;
    char *cp;
    int i;
    int Layer;
    int Layers[CDNUMLAYERS];
    int detail = 0;
    int symbolic = 0;
    char Root[81];
    char Path[81];
    char CIFFile[81];
    char Program;
    char *Tmp, *Tech;

    argc = ac;
    argv = av;
    Float1 = 1.0;
    Program = 'k';
    InitGlobal();
    Root[0] = '\0';
    CIFFile[0] = '\0';
    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {

        case 'L':
        case 'l':
            sscanf(nextarg(), "%f", &Float1);
            break;

        case 'O':
        case 'o':
            strcpy(CIFFile, nextarg());
            break;

        case 'S':
        case 's':
            symbolic++;
            break;

        case 'D':
        case 'd':
            detail++;
            break;

        case 'P':
        case 'p':
            Program = *nextarg();
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
    if (argc > 1)
        strcpy(Root,argv[1]);
    else {
        printf("Hierarchy's root cell? (hit return for help) ");
        Tmp = malloc(81);
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
    printf("Microns per lambda: %15.8e\n",Float1);
    if (CIFFile[0] == '\0') {
        cp = Root;
        i = 0;
        while (*cp != '.' And *cp != '\0') {
            CIFFile[i] = *cp;
            ++i;
            ++cp;
        }
        CIFFile[i] = '\0';
        strcat(CIFFile, ".cif");
    }

   /*
    * Initializes CD package and reads in tech file so we
    * know the layer names.  Can't generate CIF without them.
    */
    ReadTechFile();

    /* add the root path */
    if (*Path) {
        Tmp = PGetPath();
        strcat(Tmp,Path);
        printf("KIC search path: %s\n",Tmp);
    }
    if (!(detail ^ symbolic)) {
        detail = 0;
        symbolic = 0;
        printf("Converting all layers\n");
    }
    else if (detail)
        printf("Converting detail layers\n");
    else
        printf("Converting symbolic layers\n");

    for (Layer = 1; Layer <= NumLayerTable; ++Layer)
        Layers[Layer-1] = True;

    if (detail) {
        for (Layer = 1; Layer <= NumLayerTable; ++Layer)
            if (LayerTable[Layer].klAttributes & SYMBOLIC)
                Layers[Layer-1] = False;
    }
    else if(symbolic) {
        for (Layer = 1;Layer <= NumLayerTable;++Layer)
            if (Not (LayerTable[Layer].klAttributes & SYMBOLIC))
                Layers[Layer-1] = False;
    }

    if (Not CDFrom(Root,CIFFile,(int)(Float1*RESOLUTION),RESOLUTION,
            Layers,NumLayerTable,Program))
        printf("Translation of %s failed.\n%s\n", Root, CDStatusString);
    else
        printf("Translation of %s succeeded.\n", Root);
    return (0);
}


void
help()
{
    printf("\nkictocif-%s\n\n",VersionString);
    printf("Usage: kictocif [options] [root_kic_cell]\n\n");
    printf("options:\n");
    printf("  -Ocifname   cif file name to create\n");
    printf("  -S          convert symbolic layers only\n");
    printf("  -D          convert detail layers only\n");
    printf("  -Pc         c = program prefix (a,b NCA/Stanford, i Icarus, s SIF)\n");
    printf("  -Text       use %s.ext file for layers\n",TECHNAME);
    printf("  -Lmicprl    micron per lambda (default 1.0)\n\n");
    exit(0);
}


char *
nextarg()

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

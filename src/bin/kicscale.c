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
 * KIC scaling program
 * Calling sequence
 * 
 *      kicscale [-a numerator] [-b denominator] [-t ext] [rootfile]
 * 
 * where numerator and denominator are ints and equal to unity by default,
 * and ext implies calling .KIC.ext for layer definitions.
 * 
 * Giles Billingsley
 */

#define Allocate

#include "prefix.h"
#include "kic.h"

int NumLayerTable;
struct kp Parameters;
struct kl LayerTable[CDNUMLAYERS+1];
struct cl ColorTable[12];
struct f FB = { 0 };

int    argc;
char   **argv;

#if __STDC__
extern char *nextarg(void);
extern void help(void);
#else
extern char *nextarg();
extern void help();
#endif


int
main(ac, av)
char *av[];
{
    int Layer;
    int Layers[CDNUMLAYERS];
    int ScaleA = 1;
    int ScaleB = 1;
    char *tmp;
    char Root[81];
    char TmpFile[81];
    char JnkFile[81];
    char *Tech;
    char tf1[32], tf2[32];

    argc = ac;
    argv = av;
    Root[0] = '\0';
    InitGlobal();
    while (argc > 1 && argv[1][0] == '-') {
        switch(argv[1][1]) {

        case 'a':
        case 'A':
            sscanf(nextarg(), "%d", &ScaleA);
            if(ScaleA < 1){
                fprintf(stderr,"Invalid scaling factor A = %d.\n",ScaleA);
                fprintf(stderr,"Must be positive integer.\n");
                exit(1);
            }
            break;
        
        case 'b':
        case 'B':
            sscanf(nextarg(), "%d", &ScaleB);
            if(ScaleB < 1){
                fprintf(stderr,"Invalid scaling factor B = %d.\n",ScaleB);
                fprintf(stderr,"Must be positive integer.\n");
                exit(1);
            }
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
    printf("WARNING!!! This program will overwrite the specified KIC\n");
    printf("file and all of the subcell files.");
    printf("Enter y to continue: ");
    if (getchar() != 'y')
        exit(0);

    if (argc > 1)
        strcpy(Root, argv[1]);
    else {
        printf("Hierarchy's root cell? (hit return for help) ");
        tmp = malloc(81);
        *tmp = '\0';
        fgets(tmp,81,stdin);
        if (sscanf(tmp,"%s",Root) != 1)
            help();
        free(tmp);
    }

    strcpy(tf1,"SCLXXXXXX");
    tmp = mktemp(tf1);
    sprintf(TmpFile,"%s.CIF",tmp);

    strcpy(tf2,"SCLXXXXXX");
    tmp = mktemp(tf2);
    sprintf(JnkFile,"%s.KIC",tmp);

   /*
    * Initializes CD package and reads in tech file so we know
    * the layer names.  Can't generate CIF without them.
    */
    ReadTechFile();
    for (Layer = 1; Layer <= NumLayerTable; ++Layer)
        Layers[Layer-1] = True;

    if (Not CDFrom(Root,TmpFile,ScaleA*RESOLUTION,RESOLUTION,
        Layers,NumLayerTable,'k')){
        fprintf(stderr,
            "Translation of %s failed.\n%s\n", Root, CDStatusString);
        exit(1);
    }
    if (Not CDTo(TmpFile,JnkFile,ScaleB*RESOLUTION,RESOLUTION,'k')) {
        printf("Translation of %s failed.\n%s\n",TmpFile,CDStatusString);
        exit(1);
    }

#ifdef vms
    sprintf(Root,"DEL %s;*",TmpFile);
    system(Root);
    sprintf(Root,"DEL %s;*",JnkFile);
    system(Root);
#else
    unlink(TmpFile);
    unlink(JnkFile);
#endif
    return (0);
}


char *
nextarg()

{
    if(argv[1][2] != '\0')
        return(&argv[1][2]);
    argv++;
    argc--;
    return (argv[1]);
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


void
help()
{
    printf("\nUsage: scale [options] [root_kic_cell]\n\n");
    printf("options (case insensitive):\n");
    printf("  -A numer    numerator, positive >= 1\n");
    printf("  -B denom    denominator, positive >= 1\n");
    printf("  -Text       use %s.ext file for layers\n\n",TECHNAME);
    printf("Multiplies cell coordinates by numer/denom, and\n");
    printf("OVERWRITES THE CELLS REFERENCED!!!\n");
    exit(0);
}

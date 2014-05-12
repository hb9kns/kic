/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Giles C. Billingsley, Ed Gould
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
 * Here is the format.
 * 
 * Path? (~user/KIC)
 * 
 * LayerName? nd
 * Color'sName? green
 * RGB? 0 255 0
 * Filled? y
 * (Invisible)
 * (Blink)
 * MinDimensions? 2
 * Symbolic? n
 * (tran ...)
 * (resis ...)
 * (cap ...)
 * 
 * ...
 * () = Optional
 */

#include "prefix.h"
#include "kic.h"

/* used by InitParameters() routine in init.c */
int FineVPonBottom = True;
char InitScreenMode = FULLSCREEN;

#define curlayer  LayerTable[layer]
#define Matching(string) (!strcmp(cp,string))


#ifdef __STDC__
static char *get_keyword(FILE*,char*);
static int  get_int(char*);
static void get_color(char*,int);
#else
static char *get_keyword();
static int  get_int();
static void get_color();
#endif


void
ReadTechFile()
{
    int layer = 0;
    int i,j;
    int rgb[3];
    int fp[8];
    char c,*cp;
    char str1[512];
    char inbuf[512];       /* 512 character input buffer */
    FILE *techfile;
    double d;

    /*
     * Initialize for default values
     */
    Parameters.kpGridDisplayed           = True;
    Parameters.kpGridOnTop               = True;
    Parameters.kpShowGridInLargeViewport = False;
    Parameters.kpDisplayAllLabels        = False;
    Parameters.kpLabelAllInstances       = False;
    Parameters.kpGridLineStyle           = 0xcc;
    Parameters.kpHardcopyResolution      = 150;   /* 75,100,150,300 */
    *Parameters.kpHardcopyFormat         = POSTSC;
#ifdef MSDOS
    Parameters.kpHardcopyDevice          = "prn";
#else
    Parameters.kpHardcopyDevice          = "lpr";
#endif
    Parameters.kpNumRoundFlashSides      = 20;
    Parameters.kpPixToLambdaSnapping     = 1;

    *Parameters.kpFontName               = '\0';
    Parameters.kpCursorShape             = -1;
    Parameters.kpFullScreenCursor        = False;
    Parameters.kpPointBeepVolume         = 0;

    /*
     * In lambda * RESOLUTION units
     */
    Parameters.kpGrid = RESOLUTION;

    LayerTable[0].klAttributes = FILLED | VISIBLE;
    LayerTable[0].klStyleID = 0;


    /*
     * CDInit must be invoked only once,
     * else the layer table will be trashed
     */
    if (CDInit())
        MallocFailed();

    /* exit on error */
    if ((techfile = OpenTechFile()) == NULL)
        fatal_error("Can not open startup file.");

    while ((cp = get_keyword(techfile,inbuf)) != NULL) {
        if (Matching("PATH")) {
            for (i = 0; inbuf[i] != '(' && inbuf[i]; i++) ;
            if (inbuf[i] == '\0') continue;
            i++;
            while (isspace(inbuf[i])) i++;
            for (j = 0; (str1[j] = inbuf[i]) != ')' && inbuf[i]; i++,j++) ;
            if (inbuf[i] == '\0') continue;
            j--;
            while (isspace(str1[j])) j--;
            str1[j+1] = '\0';
            CDPath(str1);
            continue;
        }
        if (Matching("LAYERNAME") || Matching("LAYER")) {
            if (++layer > CDNUMLAYERS)
                fatal_error("There are too many layers in your tech file.");
            curlayer.klTechnology  =
                curlayer.klMask[0] =
                curlayer.klMask[1] =
                curlayer.klMask[2] = ' ';
            curlayer.klAttributes |= VISIBLE | ALT_VISIBLE;

            for (i = 0; (c = inbuf[i]) != 0; i++) {
                if ((c > 057 && c < 072) || (c > 0100 && c < 0133)
                    || (c > 0140 && c < 0173))
                    break;
            }
            if (c == '\0') continue;
            curlayer.klTechnology = c;
            if (((c = inbuf[++i]) > 057 && c < 072) ||
                (c > 0100 && c < 0133) || (c > 0140 && c < 0173)) {
                curlayer.klMask[0] = c;
                if (((c = inbuf[++i]) > 057 && c < 072) ||
                    (c > 0100 && c < 0133) || (c > 0140 && c < 0173)) {
                    curlayer.klMask[1] = c;
                    if (((c = inbuf[++i]) > 057 && c < 072) ||
                        (c > 0100 && c < 0133) || (c > 0140 && c < 0173)) {
                        curlayer.klMask[2] = c;
                    }
                }
            }
            CDSetLayer(layer, curlayer.klTechnology,curlayer.klMask);
            continue;
        }

        if (Matching("RGB")) {
            rgb[0] = rgb[1] = rgb[2] = 0;
            sscanf(inbuf,"%d %d %d",&rgb[0],&rgb[1],&rgb[2]);
            curlayer.klR = max(min(rgb[0],255),0);
            curlayer.klG = max(min(rgb[1],255),0);
            curlayer.klB = max(min(rgb[2],255),0);
            continue;
        }
        if (Matching("SYMBOLIC")) {
            i = 0;
            while (isspace(inbuf[i])) i++;
            if (inbuf[i] == '\0') continue;
            if (inbuf[i] == 'y' || inbuf[i] == 'Y')
                curlayer.klAttributes |= SYMBOLIC;
            continue;
        }
        if (Matching("FILLED")) {
            i = 0;
            while (isspace(inbuf[i])) i++;
            if (inbuf[i] == '\0') continue;
            if (inbuf[i] == 'y' || inbuf[i] == 'Y')
                curlayer.klAttributes |= FILLED;
            else if (inbuf[i] == 'n' || inbuf[i] == 'N')
                curlayer.klAttributes &= ~FILLED;
            else {
                *str1 = '\0';
                fp[0] = 0;
                fp[1] = 0;
                fp[2] = 0;
                fp[3] = 0;
                fp[4] = 0;
                fp[5] = 0;
                fp[6] = 0;
                fp[7] = 0;
                curlayer.klAttributes |= FILLED;
                sscanf(inbuf+i, "%x %x %x %x %x %x %x %x %s",
                    fp,fp+1,fp+2,fp+3,fp+4,fp+5,fp+6,fp+7,str1);
                curlayer.klStyle[0] = fp[0];
                curlayer.klStyle[1] = fp[1];
                curlayer.klStyle[2] = fp[2];
                curlayer.klStyle[3] = fp[3];
                curlayer.klStyle[4] = fp[4];
                curlayer.klStyle[5] = fp[5];
                curlayer.klStyle[6] = fp[6];
                curlayer.klStyle[7] = fp[7];
                if (*str1 == 'y' || *str1 == 'Y' ||
                    *str1 == 'o' || *str1 == 'O')
                    curlayer.klAttributes |= OUTLINED;
            }
            continue;
        }
        if (Matching("ALTFILLED")) {
            i = 0;
            while (isspace(inbuf[i])) i++;
            if (inbuf[i] == '\0') continue;
            if (inbuf[i] == 'y' || inbuf[i] == 'Y')
                curlayer.klAttributes |= ALT_FILLED;
            else if (inbuf[i] == 'n' || inbuf[i] == 'N')
                curlayer.klAttributes &= ~ALT_FILLED;
            else {
                *str1 = '\0';
                fp[0] = 0;
                fp[1] = 0;
                fp[2] = 0;
                fp[3] = 0;
                fp[4] = 0;
                fp[5] = 0;
                fp[6] = 0;
                fp[7] = 0;
                curlayer.klAttributes |= ALT_FILLED;
                sscanf(inbuf+i, "%x %x %x %x %x %x %x %x %s",
                    fp,fp+1,fp+2,fp+3,fp+4,fp+5,fp+6,fp+7,str1);
                curlayer.klAltStyle[0] = fp[0];
                curlayer.klAltStyle[1] = fp[1];
                curlayer.klAltStyle[2] = fp[2];
                curlayer.klAltStyle[3] = fp[3];
                curlayer.klAltStyle[4] = fp[4];
                curlayer.klAltStyle[5] = fp[5];
                curlayer.klAltStyle[6] = fp[6];
                curlayer.klAltStyle[7] = fp[7];
                if (*str1 == 'y' || *str1 == 'Y' ||
                    *str1 == 'o' || *str1 == 'O')
                    curlayer.klAttributes |= ALT_OUTLINED;
            }
            continue;
        }
        if (Matching("INVISIBLE")) {
            curlayer.klAttributes &= ~VISIBLE;
            continue;
        }
        if (Matching("ALTINVISIBLE")) {
            curlayer.klAttributes &= ~ALT_VISIBLE;
            continue;
        }
        if (Matching("BLINK")) {
            curlayer.klAttributes &= BLINK;
            continue;
        }
        if (Matching("MINDIMENSIONS")) {
            double ww;

            d = ww = 0;
            i = sscanf(inbuf,"%lf %lf", &d, &ww);
            if (i > 0) {
                if (d < 0)
                    d = 0;
                if (ww < d)
                    ww = d;
                curlayer.klMinDimensions = d*RESOLUTION;
                curlayer.klWireWidth = ww*RESOLUTION;
            }
            continue;
        }
        if (Matching("STREAMDATA")) {
            i = j = 0;
            sscanf(inbuf,"%d %d",&i,&j);
            if (i < 0 || i > 255)
                i = 0;
            if (j < -1 || j > 255)
                j = -1;
            curlayer.klStreamNumber = i;
            curlayer.klStreamDataType = j;
            continue;
        }
        if (strncmp(cp,"RESIS",5) == 0) {
            if (sscanf(inbuf,"%lf",&d) == 1 && d >= 0) {
                curlayer.klElectrical = alloc(eparms);
                curlayer.klElectrical->e_type = ERESIS;
                *curlayer.klElectrical->e_parms = d;
            }
            continue;
        }
        if (strncmp(cp,"CAP",3) == 0) {
            if (sscanf(inbuf,"%lf",&d) == 1 && d >= 0) {
                curlayer.klElectrical = alloc(eparms);
                curlayer.klElectrical->e_type = ECAP;
                *curlayer.klElectrical->e_parms = d;
            }
            continue;
        }
        if (strncmp(cp,"TRAN",4) == 0) {
            curlayer.klElectrical = alloc(eparms);
            curlayer.klElectrical->e_type = ETRANS;
            sscanf(inbuf,"%lf %lf %lf %lf %lf %lf",
                curlayer.klElectrical->e_parms,
                curlayer.klElectrical->e_parms+1,
                curlayer.klElectrical->e_parms+2,
                curlayer.klElectrical->e_parms+3,
                curlayer.klElectrical->e_parms+4,
                curlayer.klElectrical->e_parms+5);
            continue;
        }
        if (Matching("GRIDSPACING")) {
            if (sscanf(inbuf,"%lf",&d) == 1 && d*RESOLUTION > 0)
                Parameters.kpGrid  = d*RESOLUTION;
            continue;
        }
        if (Matching("GRIDSTYLE")) {
            Parameters.kpGridLineStyle = get_int(inbuf);
            continue;
        }
        if (Matching("ROUNDFLASHSIDES")) {
            Parameters.kpNumRoundFlashSides = get_int(inbuf);
            continue;
        }
        if (Matching("SHOWGRID")) {
            Parameters.kpShowGridInLargeViewport = True;
            continue;
        }
        if (Matching("ALTSHOWGRID")) {
            Parameters.kpHardcopyGrid = True;
            continue;
        }
        if (Matching("ALTDEVICE")) {
            i = 0;
            while (isspace(inbuf[i])) i++;
            Parameters.kpHardcopyDevice = malloc(strlen(inbuf) + 1);
            strcpy(Parameters.kpHardcopyDevice,inbuf + i);
            continue;
        }
        if (Matching("ALTRESOLUTION")) {
            Parameters.kpHardcopyResolution = get_int(inbuf);
            continue;
        }
        if (Matching("ALTFORMAT")) {
            i = 0;
            while (isspace(inbuf[i])) i++;
            if (inbuf[i] == 'p' || inbuf[i] == 'P')
                *Parameters.kpHardcopyFormat = POSTSC;
            else if (inbuf[i] == 'h' || inbuf[i] == 'H')
                *Parameters.kpHardcopyFormat = HPLASER;
            continue;
        }
        if (Matching("GRIDONBOTTOM")) {
            Parameters.kpGridOnTop = False;
            continue;
        }
        if (Matching("DISPLAYALLTEXT")) {
            Parameters.kpDisplayAllLabels = True;
            continue;
        }
        if (Matching("LABELALLINSTANCES")) {
            Parameters.kpLabelAllInstances = True;
            continue;
        }
        if (Matching("HIGHLIGHTING")) {
            rgb[0] = rgb[1] = rgb[2] = 0;
            sscanf(inbuf,"%d %d %d",&rgb[0],&rgb[1],&rgb[2]);
            ColorTable[HighlightingColor].R = min(max(rgb[0],0),255);
            ColorTable[HighlightingColor].G = min(max(rgb[1],0),255);
            ColorTable[HighlightingColor].B = min(max(rgb[2],0),255);
            continue;
        }
        if (Matching("BACKGROUND")) {
            rgb[0] = rgb[1] = rgb[2] = 0;
            sscanf(inbuf,"%d %d %d",&rgb[0],&rgb[1],&rgb[2]);
            ColorTable[0].R = min(max(rgb[0],0),255);
            ColorTable[0].G = min(max(rgb[1],0),255);
            ColorTable[0].B = min(max(rgb[2],0),255);
            continue;
        }
        if (Matching("MENUTEXT")) {
            get_color(inbuf,MenuTextColor);
            continue;
        }
        if (Matching("MENUPROMPT")) {
            get_color(inbuf,MenuPromptColor);
            continue;
        }
        if (Matching("MORETEXT")) {
            get_color(inbuf,MoreTextColor);
            continue;
        }
        if (Matching("MENUHIGHLIGHTING")) {
            get_color(inbuf,MenuHighlightingColor);
            continue;
        }
        if (Matching("MENUSELECT")) {
            get_color(inbuf,MenuSelectColor);
            continue;
        }
        if (Matching("COARSEGRID")) {
            get_color(inbuf,CoarseGridColor);
            continue;
        }
        if (Matching("FINEGRID")) {
            get_color(inbuf,FineGridColor);
            continue;
        }
        if (Matching("INSTANCEBOX")) {
            get_color(inbuf,InstanceBBColor);
            continue;
        }
        if (Matching("INSTANCENAME")) {
            get_color(inbuf,InstanceSizeColor);
            continue;
        }
        if (Matching("INSTANCESIZE")) {
            get_color(inbuf,InstanceNameColor);
            continue;
        }
        if (Matching("COLOR'SNAME") || Matching("COLOR"))
            /* color name is not used except by people */
            continue;
        if (Matching("LRC") || Matching("LRCRULES"))
            /* don't use these anymore */
            continue;
        if (Matching("SPLITSCREEN")) {
            InitScreenMode = SPLITSCREEN;
            continue;
        }

        /* These were used in XKIC */

        if (Matching("NO_OUTLINE")) {
            /* obsolete, not used */
            continue;
        }
        if (Matching("SNAPPING")) {
            Parameters.kpPixToLambdaSnapping = get_int(inbuf);
            continue;
        }
        if (Matching("FONTNAME")) {
            char *ip = inbuf;
            while (isspace(*ip)) ++ip;
            strncpy(Parameters.kpFontName, ip, 80);
            Parameters.kpFontName[80] = EOS;
            continue;
        }
        if (Matching("CURSORSHAPE")) {
            Parameters.kpCursorShape = get_int(inbuf);
            continue;
        }
        if (Matching("FULLSCREENCURSOR")) {
            Parameters.kpFullScreenCursor = True;
            continue;
        }
        if (Matching("BEEPVOLUME")) {
            Parameters.kpPointBeepVolume = get_int(inbuf);
            continue;
        }
        if (Matching("FINEVIEWPORTONSIDE")) {
            FineVPonBottom = False;
            continue;
        }

        printf("Warning: unknown keyword \"%s\" in %s file\n",cp,TECHFILE);
    }
    NumLayerTable = layer;
    fclose(techfile);
}


static char
*get_keyword(file,inbuf)

FILE *file;
char *inbuf;
{
    char *cp;
    int c;
    static char keyword[40];

    /* look for first key word */
    while (((c = getc(file)) < 0101 || c > 0132) && (c < 0141 || c > 0172))
        if (c == EOF) return (NULL);
    /* scan to end of keyword and convert to upper case */
    cp = keyword;
    while (c != 040 && c != '\r' && c != '\t' && c != 077 && c != '\n' &&
            c != EOF) {
        if (c > 0140 && c < 0173) c -= 32;
        *cp++ = c;
        c = getc(file);
    }
    *cp = 0;
    /* scan to end of line and put args in inbuf */
    while (c != '\n' && c != EOF) {
        if (c == '\t' || c == 077) c = 040;
        *inbuf++ = c;
        c = getc(file);
    }
    *inbuf = 0;
    return (keyword);
}


static int
get_int(buf)

char *buf;
{
    int i = 0;

    sscanf(buf, "%d", &i);
    if (i < 0)
        i = 0;
    return (i);
}


static void
get_color(buf,what)

char *buf;
int what;
{
    int i, rgb[3];

    rgb[0] = rgb[1] = rgb[2] = 0;
    i = sscanf(buf,"%d %d %d",&rgb[0],&rgb[1],&rgb[2]);
    if (i == 1) {
        ColorTable[what].Ent = rgb[0];
    }
    else {
        ColorTable[what].R = min(max(rgb[0],0),255);
        ColorTable[what].G = min(max(rgb[1],0),255);
        ColorTable[what].B = min(max(rgb[2],0),255);
    }
}



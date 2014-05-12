/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

/*
 * KIC environmental data
 * 
 */
#include <stdio.h>

#ifndef HAVE_STRLWR
#ifdef __STDC__
extern char *strlwr(char*);
#else
extern char *strlwr();
#endif
#endif

#ifndef HAVE_STRICMP
#ifdef __STDC__
extern int stricmp(char*, char*);
extern int strnicmp(char*, char*, int);
#else
extern int stricmp();
extern int strnicmp();
#endif
#endif

/* default basename for layer description file */
#define TECHNAME "kic_tech"

/* where file locations are set */
#if __STDC__
extern void InitGlobal(void);
extern FILE *OpenTechFile(void);
#else
extern void InitGlobal();
extern FILE *OpenTechFile();
#endif

/* version string, defined in makefile */
extern char *VersionString;

/* name of the layer attributes file */
extern char *TECHFILE;

/* extension of the layer attributes file */
extern char *TECH_EXT;

/* full path to help database */
extern char *PATH_TO_HELP;

/* full path to default layer table (stand-alone stream converter only) */
extern char *DEFAULTLTAB;

/* full path to mfb.rc file (DOS MFB only) */
extern char *MFBRCD;

/* name of default file to edit */
#define DEFAULT_EDIT_FILE  "noname"


/* separator character in path string (used in paths.c) */
#ifdef MSDOS
#define DIRC '\\'
#else
#define DIRC '/'
#endif

/*
 * There are 100 data base units per micron
 */

#define RESOLUTION        100
#define HALFRESOLUTION    50


/* debugging stuff for cd */

/* print trace from actions.c
#define TRACEPARSER
*/

/* print trace from parser.c
#define TRACE
*/

/* debugging from cd.c
#define DEBUGREFLECT
#define DEBUGGEN
#define DEBUG_CDUNMARK
*/

/* for CDIntersect()
#define FLOAT
*/

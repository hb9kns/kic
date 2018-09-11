/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

#ifdef WIN32
#include <windows.h>
#endif
#include "prefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

char *VersionString = VERSION_STR;
char *TECHFILE;
char *TECH_EXT;
char *PATH_TO_HELP;
char *DEFAULTLTAB;
char *MFBRCD;

#ifdef __STDC__
static char *_copy_str(char*);
#else
static char *_copy_str();
#endif


#ifdef WIN32

/* The following stuff determines the installation location. */

/*
 Advance past the terminating quote character, copying into bf if
 given.  If inclq, include the quotes in bf.  The character
 referenced by s should be a single or double quote (no checking
 here).  This keeps track of nesting.  Note that bf is *not* given a
 terminating 0.
 */
void
advq(char **s, char **bf, int inclq)
{
    int bs = 0;
    char quotechar = **s;
    if (bf && inclq)
        *(*bf)++ = quotechar;
    (*s)++;
    while (**s && (**s != quotechar || bs)) {
        if (**s == '\\') {
            bs = 1;
            if (bf)
                *(*bf)++ = **s;
            (*s)++;
        }
        else if ((**s == '"' || **s == '\'') && !bs)
            advq(s, bf, 1);
        else {
            if (bf) {
                if (**s == quotechar && bs && !inclq)
                    (*bf)--;
                *(*bf)++ = **s;
            }
            bs = 0;
            (*s)++;
        }
    }
    if (**s == quotechar) {
        if (bf && inclq)
            *(*bf)++ = **s;
        (*s)++;
    }
}


/*
 As for gettok(), but handle single and double quoted substrings. 
 The outermost quotes are stripped, and the enclosing characters are
 added to adjacent tokens, if any.  Alternate nested quoting is
 preserved.  The backslash can be used to hide the quote marks.

 Unlike gettok(), this can return an empty string.
*/
char *
getqtok(char **s)
{
    char *st, *cbuf, *c;
    int bs = 0;
    if (s == 0 || *s == 0)
        return (0);
    while (isspace(**s))
        (*s)++;
    if (!**s)
        return (0);
    st = *s;
    while (**s && !isspace(**s)) {
        if (**s == '\\') {
            bs = 1;
            (*s)++;
        }
        else if ((**s == '"' || **s == '\'') && !bs)
            advq(s, 0, 0);
        else {
            bs = 0;
            (*s)++;
        }
    }
    cbuf = (char*)malloc(*s - st + 1);
    c = cbuf;
    while (st < *s) {
        if (*st == '\\') {
            bs = 1;
            *c++ = *st++;
        }
        else if ((*st == '"' || *st == '\'') && !bs)
            advq(&st, &c, 0);
        else {
            if ((*st == '"' || *st == '\'') && bs)
                c--;
            bs = 0;
            *c++ = *st++;
        }
    }
    *c = 0;
    while (isspace(**s))
        (*s)++;
    return (cbuf);
}


/*
 Convert to UNIX style path
*/
void
unix_path(char *path)
{
    if (path) {
        char *s;
        for (s = path; *s; s++) {
            if (*s == '\\')
                *s = '/';
        }
    }
}


/*
 The inno installer places an item in the registry which gives the
 location of the uninstall directory.  Return the full path to the
 directory containing this directory.  The returned path uses '/' as
 the separator character.

 This assumes admin install only.
*/
static char *
get_inno_uninst(const char *program)
{
    /* note that "program" is the name used by the installer */
    char buf[1024], *s, *p;
    DWORD len, type;
    HKEY key;
    long ret;

    sprintf(buf,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s_is1",
        program);

    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_READ, &key);
    if (ret != ERROR_SUCCESS)
        return (0);
    len = 1024;
    ret = RegQueryValueEx(key, "UninstallString", 0, &type, (BYTE*)buf, &len);
    RegCloseKey(key);

    /* The string should contain the full path to the uninstall program, */
    if (ret != ERROR_SUCCESS || type != REG_SZ || len < 20)
        return (0);

    s = buf;
    p = getqtok(&s);
    s = strrchr(p, '\\');
    if (!s) {
        free(p);
        return (0);
    }
    *s = 0;
    s = strrchr(p, '\\');
    if (!s) {
        free(p);
        return (0);
    }
    *s = 0;

    unix_path(p);
    return (p);
}


/*
 Return a pointer to the last directory separator character found
 in string
*/
char *
strrdirsep(char *string)
{
    char *s;

    if (!string)
        return (0);
    for (s = string + strlen(string) - 1; s >= string; s--) {
        if (*s == '/' || *s == '\\')
            return (s);
    }
    return (0);
}


/*
 The Ghost Installer places an item in the registry which gives the
 location of the uninstall.log file.  Return the full path to the
 directory containing this file.  The returned path uses '/' as the
 separator character
*/
static char *
get_gins_uninst(const char *program)
{
    /* note that "program" is the name used by the installer */
    char buf[1024], *s, *t, *dir;
    DWORD len, type;
    HKEY key;
    long ret;
    int ok;

    sprintf(buf, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s",
        program);

    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_READ, &key);
    if (ret != ERROR_SUCCESS)
        return (0);
    len = 1024;
    ret = RegQueryValueEx(key, "UninstallString", 0, &type, (BYTE*)buf, &len);
    RegCloseKey(key);

    /*
     The string should contain the full path to the uninstall program,
     followed by the path to the uninstall log file, possibly quoted.
     If the string length is too short to make sense, abort
    */
    if (ret != ERROR_SUCCESS || type != REG_SZ || len < 20)
        return (0);

    s = _copy_str(buf);
    /*
     Remove any quotes, and peel off the argument of the uninstall
     command, which is the full path to install.log
    */
    t = s + strlen(s) - 1;
    while (t >= s && *t == '"')
        *t-- = '\0';
    while (t > s && *t != '"' && (!isalpha(*t) || *(t+1) != ':' ||
            (*(t+2) != '/' && *(t+2) != '\\')))
        t--;
    dir = 0;
    ok = 0;
    if (t > s && *t != '"') { 
        /* found the start of the path */
        dir = _copy_str(t);
        t = strrdirsep(dir);
        if (t) {
            *t = 0;  /* stripped "/uninstall.log"; */
            ok = 1;
        }
    }
    free(s);
    if (!ok) {
        free(dir);
        dir = 0;
    }
    unix_path(dir);
    return (dir);
}


/*
 Return the path to the uninstall data.
*/
static char *
get_from_registry()
{
    char *dir = get_inno_uninst("Kic");
    if (!dir)
        dir = get_gins_uninst("Kic");
    return (dir);
}

#endif


void
fatal_error(const char *msg)
{
    if (!msg)
        msg = "Unknown error.";
#ifdef WIN32
    MessageBox(0, msg, "KIC Fatal Error", MB_ICONSTOP);
#else
    fprintf(stderr, "Fatal Error: %s\n", msg);
#endif
    exit(1);
}


void
InitGlobal()
{
    char *startupdir, *s;

    startupdir = getenv("KIC_LIB_DIR");
#ifdef WIN32
    if (startupdir == NULL)
        startupdir = get_from_registry();
#endif
    if (startupdir == NULL)
        startupdir = KIC_LIB_DIR;

    s = malloc(strlen(startupdir) + 1);
    if (s == NULL)
        goto bad;
    (void)strcpy(s,startupdir);
    startupdir = s;

    MFBRCD = startupdir;
    DEFAULTLTAB = startupdir;
    PATH_TO_HELP = startupdir;
    return;

bad:
    fatal_error("Memory allocation failure.");

}

/* subsidiary basename for layer description file */
#ifdef MSDOS
#define TECHNAMETOO "dotkic"
#else
#define TECHNAMETOO ".KIC"
#endif


static char *
_copy_str(src)

char *src;
{
    char *dst = (char*)malloc(strlen(src)+1);

    if (!dst)
        fatal_error("Memory allocation failure.");
    strcpy(dst,src);
    return (dst);
}


FILE *
OpenTechFile()

{
    FILE *fp;
    char buf[256];

    if (TECH_EXT)
        sprintf(buf,"%s.%s",TECHNAME,TECH_EXT);
    else
        strcpy(buf,TECHNAME);
    fp = fopen(buf, "r");
    if (fp) {
        TECHFILE = _copy_str(buf);
        return (fp);
    }

    if (TECH_EXT)
        sprintf(buf,"%s.%s",TECHNAMETOO,TECH_EXT);
    else
        strcpy(buf,TECHNAMETOO);
    fp = fopen(buf, "r");
    if (fp) {
        TECHFILE = _copy_str(buf);
        return (fp);
    }

    if (TECH_EXT)
        sprintf(buf,"%s%c%s.%s",DEFAULTLTAB,DIRC,TECHNAME,TECH_EXT);
    else
        sprintf(buf,"%s%c%s",DEFAULTLTAB,DIRC,TECHNAME);
    fp = fopen(buf,"r");
    if (fp) {
        TECHFILE = _copy_str(buf);
        return (fp);
    }

    if (TECH_EXT)
        sprintf(buf,"%s%c%s.%s",DEFAULTLTAB,DIRC,TECHNAMETOO,TECH_EXT);
    else
        sprintf(buf,"%s%c%s",DEFAULTLTAB,DIRC,TECHNAMETOO);
    fp = fopen(buf,"r");
    if (fp) {
        TECHFILE = _copy_str(buf);
        return (fp);
    }
    return (NULL);
}

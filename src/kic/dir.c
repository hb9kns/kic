/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/
/*************************************************************************
 *
 * Dir command.
 * Show a directory listing of symbol files.
 *
 *************************************************************************/

#include "prefix.h"
#include "kic.h"
#if __NDPC__
#include <direct.h>
typedef	int	*DIR;
struct direct {
	char	d_name[20];
	};
#if __STDC__
extern DIR *opendir(char *);
extern struct direct *readdir(DIR *);
extern void closedir(DIR *);
#else
extern DIR *opendir( );
extern struct direct *readdir( );
extern void closedir( );
#endif
#else
#ifdef HPUX
/* more foulness from HP */
typedef unsigned int ino_t;
#endif
#include <sys/types.h>
#include <dirent.h>
#ifdef direct
#undef direct
#endif
#define direct dirent
#endif


extern char *MenuDIR;

struct list {
    char *l_word;
    struct list *l_next;
};


#ifdef __STDC__
static void format_lines(struct list*);
static void display_lines(struct list*);
static void list_free(struct list*);
static void list_sort(struct list*);
static int  lcomp(const void*,const void*);
static int  is_symfile(char*);
static struct list *symfiles(char*);
#else
static void format_lines();
static void display_lines();
static void list_free();
static void list_sort();
static int  lcomp();
static int  is_symfile();
static struct list *symfiles();
#endif

/* external */
extern int  MoreLine();
extern void EnableMore();
extern char *CopyString();
extern char *tmalloc();


void
Dir()

{
    char *path;
    char *c,*s;
    struct list *wl,*wx = NULL,*wx0 = NULL;

    MenuSelect(MenuDIR);
    path = PGetPath();

    for (s = path;;) {

        while (isspace(*s)) s++;
        if (*s == '\0')
            break;

        c = TypeOut;
        while (!isspace(*s) && *s != '\0')
            *c++ = *s++;
        *c = '\0';
        /* TypeOut contains the directory path */

        wl = symfiles(TypeOut);

        list_sort(wl);
        format_lines(wl);
        if (wx0 == NULL)
            wx = wx0 = alloc(list);
        else {
            wx->l_next = alloc(list);
            wx = wx->l_next;
        }
        if (wx == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        strcat(TypeOut," :");
        wx->l_word = CopyString(TypeOut);
        wx->l_next = wl;
        while (wx->l_next) wx = wx->l_next;
    }
    display_lines(wx0);
    list_free(wx0);
    FullRedisplay();
    MenuDeselect(MenuDIR);
}


static void
format_lines(li)

/* combine the words into columns */
struct list *li;
{
    struct list *wl;
    int i,width,colw;
    char *buf,*b;

    if (li == NULL) return;
    width = View->kvLargeCoarseViewport->kaWidth - 8;
    width /= FB.fFontWidth;
    buf = tmalloc(width+1);

    colw = 0;
    for (wl = li; wl; wl = wl->l_next) {
        i = strlen(wl->l_word);
        if (i > colw) colw = i;
    }
    colw += 2;
    width /= colw;

    b = buf;
    i = 0;
    for (wl = li; wl; wl = wl->l_next) {
        sprintf(b,"%-*s",colw,wl->l_word);
        b = buf + strlen(buf);
        i++;
        if (i == width) {
            free(li->l_word);
            li->l_word = CopyString(buf);
            li = li->l_next;
            b = buf;
            i = 0;
        }
    }
    if (i) {
        free(li->l_word);
        li->l_word = CopyString(buf);
    }
    if (li->l_next) list_free(li->l_next);
    li->l_next = alloc(list);
    li = li->l_next;
    if (li == NULL) {
        CDStatusInt = CDMALLOCFAILED;
        MallocFailed();
    }
    /* add a blank line */
    li->l_word = CopyString(" ");
    li->l_next = NULL;
    free(buf);
}


static void
display_lines(wl)

/* print the list, using MORE mode */
struct list *wl;
{
    EnableMore(True);
    for (; wl; wl = wl->l_next)
        if (MoreLine(wl->l_word)) break;

    MorePageDisplay();
    ShowPrompt("Hit any key to continue.");
    EnableMore(False);
    (void)FBGetchar(ERASE);
}


static void
list_free(wl)

struct list *wl;
{
    struct list *nw;

    for (; wl; wl = nw) {
        nw = wl->l_next;
        free(wl->l_word);
        afree(wl,list);
    }
    return;
}


static void
list_sort(wl)

/* alphabetically sort the list */
struct list *wl;
{
    int i = 0;
    struct list *ww;
    char **stuff;

    for (i = 0, ww = wl; ww; i++, ww = ww->l_next) ;
    if (i < 2)
        return;
    stuff = (char **) tmalloc(i * sizeof (char *));
    for (i = 0, ww = wl; ww; i++, ww = ww->l_next)
        stuff[i] = ww->l_word;
    qsort((char *) stuff, i, sizeof (char *), lcomp);
    for (i = 0, ww = wl; ww; i++, ww = ww->l_next)
        ww->l_word = stuff[i];
    free(stuff);
    return;
}


static int
lcomp(s, t)

#ifdef __STDC__
const void *s;
const void *t;
#else
char **s, **t;
#endif
{
    return (strcmp(*(char**)s,*(char**)t));
}


static int
is_symfile(buf)

/* return True if a symbol file */
char *buf;
{
    if (buf[0] != '(') return (False);
    if ((buf[1] != ' ') && (buf[1] != 'S')) return (False);
    if ((buf[2] != 'S') && (buf[2] != 'y') && (buf[2] != 'V')) return (False);
    if ((buf[3] != 'y') && (buf[3] != 'm') && (buf[3] != 'E')) return (False);
    if ((buf[4] != 'm') && (buf[4] != 'b') && (buf[4] != 'R')) return (False);
    if ((buf[5] != 'b') && (buf[5] != 'o') && (buf[5] != 'S')) return (False);
    if ((buf[6] != 'o') && (buf[6] != 'l') && (buf[6] != 'I')) return (False);
    return (True);
}

#ifdef MSDOS
#define DIR_TERM '\\'
#else
#define DIR_TERM '/'
#endif


static struct list *
symfiles(dir)

char *dir;
{
    DIR *wdir;
    struct direct *de;
    struct list *wl = NULL, *wl0 = NULL;
    char buf[256];
    FILE *fp;
    int i;

    if (!(wdir = opendir(dir)))
        return (NULL);

    while ((de = readdir(wdir)) != (struct direct *)NULL) {

        sprintf(buf,"%s%c%s",dir,DIR_TERM,de->d_name);
        fp = fopen(buf,"r");
        if (fp == NULL)
            continue;

        /* is it a symbol file? */

        for (i = 0; i <7; i++)
            buf[i] = getc(fp);
        buf[i] = '\0';
        fclose(fp);

        if (!is_symfile(buf))
            continue;

        if (wl0 == NULL) {
            wl = wl0 = alloc(list);
        }
        else {
            wl->l_next = alloc(list);
            wl = wl->l_next;
        }
        wl->l_word = CopyString(de->d_name);
        wl->l_next = NULL;
    }
    closedir(wdir);
    return (wl0);
}

#if __NDPC__ 


DIR *
opendir(dir)

char *dir;
{
    struct find_t *d;
    char buf[128];

    strcpy(buf,dir);
    if (*buf == '\\' && *(buf+1) == '\0')
        strcat(buf,"*.*");
    else
        strcat(buf,"\\*.*");

    d = (struct find_t *) malloc(sizeof(struct find_t));
    if (d == NULL) return NULL;
    if (findfirst(buf,d,0x1f)) {
        free(d);
        return (NULL);
    }
    return ((DIR *) d);
}


struct direct *
readdir(d)

DIR *d;
{
    char *c;
    static struct direct dd;
    int i;

    if (d == NULL) return NULL;
    strcpy(dd.d_name,((struct find_t *)d)->name);

    for (c = dd.d_name; *c; c++)
        if (isupper(*c)) *c = tolower(*c);

    if (findnext((struct find_t *)d))
        return (NULL);
    return (&dd);
}



void
closedir(d)

DIR *d;
{
    if (d)
        free(d);
}

#endif

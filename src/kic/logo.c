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

#include "prefix.h"
#include "kic.h"

#define FONT_FILE "logofont.kic"
/* a WordPerfect font file */

struct cdat {
	unsigned char *headr;
	unsigned char *data;
	int size;
};

struct s_rlist {
    int xl, yl, xr, yu;
    struct s_rlist *next;
};

#if __STDC__
static void fixname(char*);
static struct cdat *findchar(int,FILE*);
static void addchar(struct cdat*,int,int,int,FILE*);
static void coalesce(struct  s_rlist**,int,int);
static void swaptest(void);
static void fixint(unsigned char*);
#else
static void fixname();
static struct cdat *findchar();
static void addchar();
static void coalesce();
static void swaptest();
static void fixint();
#endif

extern char *MenuLOGO;
extern char *MenuPLACE;

#if __NDPC__
#if __STDC__
static FILE *my_fopen(char*,char*);
#else
static FILE *my_fopen();
#endif
#define fopen my_fopen
#endif


void
Logo(LookedAhead)

int *LookedAhead;
{
    FILE *fp,*kf;
    struct cdat *c;
    int i,x,x1,y1,psize;
    char *s, buf[80];
    char name[12];

    swaptest();
    MenuSelect(MenuLOGO);
    fp = fopen(FONT_FILE,"rb");
    if (fp == NULL) {
        sprintf(TypeOut,"%s%c%s",PATH_TO_HELP,DIRC,FONT_FILE);
        fp = fopen(TypeOut,"rb");
        if (fp == NULL) {
            ShowPrompt("Can't open font file.");
            MenuDeselect(MenuLOGO);
            return;
        }
    }
    ShowPrompt("Enter text: ");
    s = FBEdit(NULL);
    if (s == NULL || !*s) {
        ErasePrompt();
        MenuDeselect(MenuLOGO);
        fclose(fp);
        return;
    }
    strncpy(buf,s,80);
    buf[79] = '\0';
    strncpy(name,s,8);
    name[8] = '\0';
    fixname(name);
    i = 1;
    s = strchr(name,'\0');
    while (!access(name,0)) {
        sprintf(s,".%d",i);
        i++;
    }

    ShowPrompt("Enter pixel size: ");
    s = FBEdit(NULL);
    if (s == NULL || !*s) {
        ErasePrompt();
        MenuDeselect(MenuLOGO);
        fclose(fp);
        return;
    }
    psize = 100.0*atof(s);

    kf = fopen(name,"w");
    if (kf == NULL) {
        ShowPrompt("Can't open output file.\n");
        MenuDeselect(MenuLOGO);
        fclose(fp);
        return;
    }
    sprintf(TypeOut,"Symbol %s",name);
    GenComment(kf,TypeOut);
    fprintf(kf,"9 %s;\n",name);
    GenBeginSymbol(kf,0,1,1);
    GenLayer(kf,LayerTable[Parameters.kpLayer].klTechnology,
        LayerTable[Parameters.kpLayer].klMask);

    s = buf;
    x = 0;
    while (*s) {
        c = findchar(*s,fp);
        if (c == NULL) {
            x += 20;
            rewind(fp);
            s++;
            continue;
        }
        fixint(c->headr+6);
        fixint(c->headr+8);
        fixint(c->headr+10);
        x1 = x + (*(short*)(c->headr+6));
        y1 = (*(short*)(c->headr+8));
        addchar(c,x1,y1,psize,kf);
        x += (*(short*)(c->headr+10));
        free(c->headr);
        free(c);
        rewind(fp);
        s++;
    }
    GenEndSymbol(kf);
    GenEnd(kf);
    fclose(kf);
    fclose(fp);
    sprintf(TypeOut,"New symbol %s created, point to place.",name);
    ShowPrompt(TypeOut);
    MenuSelect(MenuPLACE);
    MenuDeselect(MenuLOGO);
    MakeInstance(LookedAhead,name);
    MenuDeselect(MenuPLACE);
}


static void
addchar(cd,x0,y0,psize,kf)

struct cdat *cd;
int x0,y0,psize;
FILE *kf;
{
    int i,j,k,bwidth,lines,x,y,xl = 0;
    unsigned char c,*s;
    int inbox;
    struct s_rlist **rlist, *list = NULL;

    if (cd == NULL) return;

    bwidth = (*(short*) (cd->headr+10) + 7)/8;
    lines = cd->size/bwidth;
    s = cd->data;

    rlist = (struct s_rlist**)tmalloc(lines*sizeof(struct s_rlist*));

    y = y0*psize;
    inbox = 0;
    for (i = 0; i < lines; i++) {
        rlist[i] = NULL;
        x = x0*psize;
        for (j = 0; j < bwidth; j++) {
            c = *s++;
            for (k = 0; k < 8; k++) {
                if (c & 0x80) {
                    if (!inbox) {
                        xl = x;
                        inbox = 1;
                    }
                }
                else {
                    if (inbox) {
                        if (!rlist[i])
                            rlist[i] = list = alloc(s_rlist);
                        else {
                            list->next = alloc(s_rlist);
                            list = list->next;
                        }
                        list->xl = xl;
                        list->yl = y - psize;
                        list->xr = x;
                        list->yu = y;
                        list->next = NULL;
                        inbox = 0;
                    }
                }
                c <<= 1;
                x += psize;
            }
        }
        if (inbox) {
            if (!rlist[i])
                rlist[i] = list = alloc(s_rlist);
            else {
                list->next = alloc(s_rlist);
                list = list->next;
            }
            list->xl = xl;
            list->yl = y - psize;
            list->xr = x;
            list->yu = y;
            list->next = NULL;
            inbox = 0;
        }
        y -= psize;
    }

    coalesce(rlist,lines,psize);

    for (i = 0; i < lines; i++) {
        for (list = rlist[i]; list; list = rlist[i]) {
            rlist[i] = list->next;
            GenBox(kf,list->xr - list->xl,list->yu - list->yl,
                list->xl + (list->xr - list->xl)/2,
                list->yl + (list->yu - list->yl)/2,1,0);
            free(list);
        }
    }
    free(rlist);
}


static void
coalesce(rlist,lines,psize)

/* merge rectangles */
struct s_rlist **rlist;
int lines, psize;
{
    int i;
    struct s_rlist *listL, *listU, *listp;

    for (i = 1; i < lines; i++) {

        for (listL = rlist[i]; listL; listL = listL->next) {
            listp = NULL;
            for (listU = rlist[i-1]; listU;
                    listp = listU, listU = listU->next) {
                if (listL->xl == listU->xl && listL->xr == listU->xr) {
                    listL->yu = listU->yu;
                    if (listp == NULL) {
                        rlist[i-1] = listU->next;
                    }
                    else {
                        listp->next = listU->next;
                    }
                    free(listU);
                    break;
                }
            }
        }
    }
}


static void
fixname(s)

char *s;
{
    while (*s) {
        if (!isalpha(*s) && !isdigit(*s))
            *s = '_';
        s++;
    }
}


static struct cdat *
findchar(char_id,fp)

int char_id;
FILE *fp;
{
    unsigned char buf[64],*s;
    unsigned int c;
    int size;
    struct cdat *cd;

    for (;;) {
        if (feof(fp)) break;
        c = getc(fp);
        if (c != 0x1b) continue;
        c = getc(fp);
        if (c != '*') continue;
        c = getc(fp);
        if (c != 'c') continue;
        c = getc(fp);
        s =  buf;
        while (isdigit(c)) {
            *s++ = c;
            c  = getc(fp);
        }
        *s = '\0';
        if (char_id != atoi((char*)buf)) continue;
        if (c != 'E') continue;
        c = getc(fp);
        if (c != 0x1b) continue;
        c = getc(fp);
        if (c != '(') continue;
        c = getc(fp);
        if (c != 's') continue;

        c = getc(fp);
        s =  buf;
        while (isdigit(c)) {
            *s++ = c;
            c  = getc(fp);
        }
        *s = '\0';
        if (c != 'W') continue;

        size = atoi((char*)buf);
        cd = (struct cdat*)tmalloc(sizeof(struct cdat));
        cd->size = size - 16;
        cd->headr = (unsigned char*)tmalloc(size);
        fread(cd->headr,size,1,fp);
        cd->data = cd->headr + 16;
        return (cd);
    }
    return (NULL);
}


static int swap_bytes;


static void
swaptest()

{
    union {unsigned short i; unsigned char c[2];} u;

    u.i = 1;
    if (u.c[0] == 1)
        swap_bytes = True;
    else
        swap_bytes = False;
}


static void
fixint(i)

unsigned char *i;
{
    unsigned char c;

    if (swap_bytes) {
        c = *i;
        *i = *(i+1);
        *(i+1) = c;
    }
}


#if __NDPC__
#undef fopen
extern int _pmode;

static FILE*
my_fopen(what,how)

/* support for NDPC's silly pmode */
char *what, *how;
{
    FILE *fp;
    char *c, buf[8];
    char *index();

    strncpy(buf,how,8);
    buf[7] = '\0';
    if ((c = index(buf,'b')) != NULL) {
        _pmode = 0x8000;
        *c = '\0';
    }
    fp = fopen(what,buf);
    _pmode = 0x4000;
    return (fp);
}

#endif

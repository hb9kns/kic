/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright (c) 1986 Wayne A. Christopher, U. C. Berkeley CAD Group 
 *     faustus@cad.berkeley.edu, ucbvax!faustus
 * Permission is granted to modify and re-distribute this code in any manner
 * as long as this notice is preserved.  All standard disclaimers apply.
 *
 *************************************************************************/

#include "prefix.h"
#include "hlpdefs.h"
#include <stdlib.h>

#define prefix(x,y) ciprefix(x,y)

struct sHlpEnt {
    int offset;
    char *keyword;
    char *title;
    struct sHlpEnt *next;
};

static struct sHlpEnt *HelpBase;


#if __STDC__
static void sortlist(toplink**);
static int  sortcmp(toplink**,toplink**);
static void tlfree(toplink*);
static FILE *db_open(char*,struct sHlpEnt**);
static void read_db(FILE*);
static struct sHlpEnt *find_entry(char*);
static char *my_fgets(char*,int,FILE*);
#else
static void sortlist();
static int  sortcmp();
static void tlfree();
static FILE *db_open();
static void read_db();
static struct sHlpEnt *find_entry();
static char *my_fgets();
#endif

static topic *alltopics = NULL;

#define HLP_KEYWORD   "!!KEYWORD"
#define HLP_TITLE     "!!TITLE"
#define HLP_TEXT      "!!TEXT"
#define HLP_SEEALSO   "!!SEEALSO"
#define HLP_SUBTOPICS "!!SUBTOPICS"


topic *
hlp_read(word)

char *word;
{
    char buf[BSIZE], *s, *t;
    int i, seealso, subtopics;
    FILE *fp;
    topic *top;
    toplink *tl = NULL;
    struct sHlpEnt *bb;
    wordlist *wl = NULL;

    if (!(fp = db_open(word,&bb))) {
        err_printf("Error: no title for topic %s.\n", word);
        return (NULL);
    }
    top = alloc(topic);
    top->keyword = copy(word);
    top->title = copy(bb->title);
    top->subtopics = NULL;
    top->seealso = NULL;
    top->text = NULL;
    top->maxcols = 0;
    seealso = false;
    subtopics = false;
        
    while ((s = my_fgets(buf,BSIZE,fp)) != NULL) {

        if (prefix(HLP_KEYWORD,s))
            break;

        if (prefix(HLP_SEEALSO,s)) {
            seealso = true;
            subtopics = false;
            tl = NULL;
            continue;
        }
        if (prefix(HLP_SUBTOPICS,s)) {
            subtopics = true;
            seealso = false;
            tl = NULL;
            continue;
        }

        if (seealso || subtopics) {

            if (*s == '#' || *s == '*')
		continue;
            while (isspace(*s))
                s++;
            if (!*s)
                continue;

            t = s + strlen(s)-1;
	    while (*t <= ' ')
		*t-- = '\0';

            if (tl) {
                tl->next = alloc(toplink);
                tl = tl->next;
            }
            else
                tl = alloc(toplink);
            tl->keyword = copy(s);
            tl->next = NULL;
            bb = find_entry(s);
            if (bb)
                tl->description = copy(bb->title);
            else
                tl->description = copy("<unknown>");

            if (seealso && !top->seealso)
                top->seealso = tl;
            else if (subtopics && !top->subtopics)
                top->subtopics = tl;
            continue;
        }
        if ((s = strchr(buf,'\n')) != NULL)
            *s = '\0';
        if (wl) {
            wl->wl_next = alloc(wordlist);
            wl->wl_next->wl_prev = wl;
            wl = wl->wl_next;
        }
        else {
            wl = alloc(wordlist);
            wl->wl_prev = NULL;
        }
        wl->wl_next = NULL;
        wl->wl_word = copy(buf);

        if (!top->text)
            top->text = wl;

        top->numlines++;
        if ((i = strlen(buf)) > top->maxcols)
            top->maxcols = i;
    }
    (void) fclose(fp);

    sortlist(&top->seealso);
    sortlist(&top->subtopics);
    
    top->readlink = alltopics;
    alltopics = top;

    return (top);
}


static void
sortlist(tlp)

toplink **tlp;
{
    toplink **vec, *tl;
    int num, i;

    for (num = 0,tl = *tlp; tl; tl = tl->next) num++;

    if (!num) return;

    vec = (toplink **) TMALLOC(sizeof (toplink *) * num);
    for (tl = *tlp, i = 0; tl; tl = tl->next, i++)
        vec[i] = tl;
    (void) qsort((char *) vec, num, sizeof (toplink *),
#if __STDC__
        (int(*)(const void*,const void*))sortcmp);
#else
        sortcmp);
#endif
    *tlp = vec[0];
    for (i = 0; i < num - 1; i++)
        vec[i]->next = vec[i + 1];
    vec[i]->next = NULL;
    free(vec);
    return;
}


static int
sortcmp(tlp1, tlp2)

toplink **tlp1, **tlp2;
{
    return (strcmp((*tlp1)->description, (*tlp2)->description));
}


void
hlp_free()

{
    topic *top, *nt = NULL;

    for (top = alltopics; top; top = nt) {
        nt = top->readlink;
        free(top->title);
        free(top->keyword);
        wl_free(top->text);
        tlfree(top->subtopics);
        tlfree(top->seealso);
        free(top);
    }
    alltopics = NULL;
    return;
}


static void
tlfree(tl)

toplink *tl;
{
    toplink *nt = NULL;

    while (tl) {
        free(tl->description);
        free(tl->keyword);
        /* Don't free the button stuff... */
        nt = tl->next;
        free(tl);
        tl = nt;
    }
    return;
}


static FILE *
db_open(word,pb)

char *word;
struct sHlpEnt **pb;
{
    char buf[BSIZE];
    FILE *fp;
    struct sHlpEnt *bb;

    (void) sprintf(buf,"%s/%s",HELPPATH,DBFILE);
    hlp_pathfix(buf);
    /* open in binary mode, so ftell/fseek work in DOS */
    fp = fopen(buf,"rb");
    if (!fp) {
        perror(buf);
        return (NULL);
    }
    if (!HelpBase)
        read_db(fp);

    if (!HelpBase)
        return (NULL);

    bb = find_entry(word);
    if (bb) {
        *pb = bb;
        fseek(fp,bb->offset,0);
    }
    else {
        fclose(fp);
        fp = NULL;
    }
    return (fp);
}


static void
read_db(fp)

FILE *fp;
{
    char buf[BSIZE], *s, *t, *kw = NULL, *ti = NULL;
    struct sHlpEnt *bb = NULL;
    int keyword = 0, title = 0;

    while ((s = my_fgets(buf, BSIZE, fp)) != NULL) {

        if (prefix(HLP_KEYWORD,buf)) {
            keyword = true;
            title = false;
            kw = NULL;
            continue;
        }
        if (prefix(HLP_TITLE,buf)) {
            title = true;
            keyword = false;
            ti = NULL;
            continue;
        }
        if (prefix(HLP_TEXT,buf)) {
            if (ti && kw) {

                if (HelpBase == NULL) {
                    HelpBase = alloc(sHlpEnt);
                    bb = HelpBase;
                }
                else {
                    bb->next = alloc(sHlpEnt);
                    bb = bb->next;
                }
                bb->next = NULL;
                bb->keyword = kw;
                bb->title = ti;
                bb->offset = ftell(fp);
                ti = NULL;
                kw = NULL;
                keyword = false;
                title = false;
            }
            continue;
        }

        if (keyword || title) {

            while (isspace(*s))
                s++;
            if (!*s)
                continue;

	    t = s + strlen(s)-1;
	    while (*t <= ' ')
		*t-- = '\0';

            if (keyword)
                kw = copy(s);
            else
                ti = copy(s);

        }
    }
}


static struct sHlpEnt *
find_entry(word)

char *word;
{
    struct sHlpEnt *bb;

    if (!word)
        return (HelpBase);

    for (bb = HelpBase; bb; bb = bb->next) {
        if (!strcmp(word,bb->keyword))
            return (bb);
    }
    return (NULL);
}


static char *
my_fgets(buf,size,fp)

char *buf;
int size;
FILE *fp;
{
    char *s;
    int i, c;
    /* works with DOS or UNIX */

    for (s = buf, i = size; i; s++, i--) {
        c = getc(fp);
        if (c == '\r')
            c = getc(fp);
        if (c == EOF) {
            *s = '\0';
            if (s == buf)
                return (NULL);
            return (buf);
        }
        *s = c;
        if (c == '\n') {
            *++s = '\0';
            return (buf);
        }
    }
    buf[size-1] = '\0';
    return (buf);
}

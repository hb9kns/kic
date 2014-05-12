/*************************************************************************
 Graphics and miscellaneous library
 Copyright (c) Stephen R. Whiteley 1994
 Author: Stephen R. Whiteley
 *************************************************************************/

#include "prefix.h"
#include "kic.h"

/* A new polygon clipping routine.  Unlike the old routine, this one
 * keeps track of severed polygons by means of a linked list.
 * First call PolygonClip(), which will perform clipping and
 * create a list of polygons.  The polygons are then extracted with
 * NewPolygon() until 0 is returned.  A suitably large xy buffer
 * must be supplied in the Poly structure passed to NewPolygon().
 */

/* Undefine DO_FLOAT below if there is no chance of integer overflow,
 * avoiding floating point for efficiency.
 */
#define DO_FLOAT

/* We represent individual polygons with a circular doubly linked list */
struct pair {
    int pa_x,pa_y;
    struct pair *pa_next;
    struct pair *pa_prev;
};

/* List of circularly linked polygons */
struct plist {
    struct pair *pl_pair;
    struct pair *pl_start;
};

static struct plist *Polybuf;            /* head of polygon list       */
static struct plist *Newpoly;            /* free structure to allocate */
static struct plist *Polycurrent;        /* current polygon to return  */
static int          Numpolys;            /* size of poly storage       */

static struct pair  *Linkbuf;            /* polygon point storage      */
static struct pair  *Newlink;            /* free structure to allocate */
static int          Numlinks;            /* size of link storage       */


#ifdef __STDC__
static void clip_right(int);
static void clip_left(int);
static void clip_bottom(int);
static void clip_top(int);
static void linkpolys(struct plist*,int);
static int  polycmpX(const void*,const void*);
static int  polycmpY(const void*,const void*);
static void newlink(void);
static void newpoly(void);
#else
static void clip_right();
static void clip_left();
static void clip_bottom();
static void clip_top();
static void linkpolys();
static int  polycmpX();
static int  polycmpY();
static void newlink();
static void newpoly();
#endif


void
PolygonClip(poly,left,bottom,right,top)

Poly *poly;
int left, bottom, right, top;
{
    struct pair *pa;
    int i, n;
    int tmp;

    if (left > right) {
        tmp = left;
        left = right;
        right = tmp;
    }
    if (bottom > top) {
        tmp = bottom;
        bottom = top;
        top = tmp;
    }

    if (Linkbuf == NULL)
        newlink();
    if (Polybuf == NULL)
        newpoly();

    Newlink = Linkbuf;
    Newpoly = Polybuf;

    n = (poly->nvertices << 1) - 2;
    for (i = 0; i < n; i += 2) {
        Newlink->pa_x = poly->xy[i];
        Newlink->pa_y = poly->xy[i+1];
        Newlink->pa_prev = Newlink-1;
        Newlink->pa_next = Newlink+1;
        newlink();
    }
    pa = Linkbuf;

    if (pa->pa_x != poly->xy[i] || pa->pa_y != poly->xy[i+1]) {
        Newlink->pa_x = poly->xy[i];
        Newlink->pa_y = poly->xy[i+1];
        Newlink->pa_prev = Newlink-1;
        Newlink->pa_next = Newlink+1;
        newlink();
    }
    Newlink--;
    Newlink->pa_next = pa;
    pa->pa_prev = Newlink;
    Newlink++;

    Newpoly->pl_pair = pa;
    newpoly();

    clip_left(left);
    clip_bottom(bottom);
    clip_right(right);
    clip_top(top);

    Polycurrent = Polybuf;
}


int
NewPolygon(p)

Poly *p;
{
    int *xy;
    int i;
    struct pair *pp, *pa;

    for (;;) {
        xy = (int*)p->xy;
        i = 1;
        if (Polycurrent == Newpoly) {
            Newlink = Linkbuf;
            Newpoly = Polybuf;
            Polycurrent = Newpoly;
            return (0);
        }
        pp = pa = Polycurrent->pl_pair;
        if (pp) {
            *xy++ = pp->pa_x;
            *xy++ = pp->pa_y;

            for (pp = pp->pa_next; ; pp = pp->pa_next) {
                i++;
                *xy++ = pp->pa_x;
                *xy++ = pp->pa_y;
                if (pp == pa) break;
            }
        }

        Polycurrent++;
        if (i >= 4)
            break;
    }
    p->nvertices = i;
    return (1);
}


static void
clip_right(right)
 
int right;
{
    struct pair *pa, *pp, *pfirst, *plast, *pnext;
    struct plist *list, *end, *pointer;
    int x, y, xnext, ynext;
 
    end = Newpoly;
    for (list = Polybuf; list < end; list++) {
        pointer = Newpoly;
        pa = list->pl_pair;
        /*
         * First, find a vertex outside of the clipping
         * region.
         */

        for (pp = pa;;) {
            if (pp->pa_x > right)
                break;
            pp = pp->pa_next;
            if (pp == pa) {
                /* no clipping needed */
                Newpoly->pl_pair = pa;
                newpoly();
                goto next;
            }
        }
        /* start with an "outside" point */
 
        for (pa = pp;;) {
            x = pp->pa_x;
            y = pp->pa_y;
            pnext = pp->pa_next;
            xnext = pnext->pa_x;
            ynext = pnext->pa_y;

            if (xnext <= right && x > right) {
                /* poly edge crosses bounding clip line */
#ifdef DO_FLOAT
                y += (ynext - y)*(((double)(right - x))/(xnext - x));
#else
                y += ((right-x)*(ynext-y))/(xnext-x);
#endif
                x = right;
 
                if (x != xnext || y != ynext) {
                    pfirst = Newlink;
                    pfirst->pa_x = x;
                    pfirst->pa_y = y;
                    pfirst->pa_prev = NULL;
                    newlink();
                    pp = pnext;
                    pfirst->pa_next = pp;
                    pp->pa_prev = pfirst;
                }
                else {
                    pp = pnext;
                    pfirst = pnext;
                    pfirst->pa_prev = NULL;
                }
 
                /* walk along inside points, find the last one */
                for (;;) {
                    if (pp->pa_x > right) {
                        pnext = pp;
                        pp = pp->pa_prev;
                        break;
                    }
                    pp = pp->pa_next;
                }
                x = pp->pa_x;
                y = pp->pa_y;
                xnext = pnext->pa_x;
                ynext = pnext->pa_y;
#ifdef DO_FLOAT
                y += (ynext - y)*(((double)(right - x))/(xnext - x));
#else
                y += ((right-x)*(ynext-y))/(xnext-x);
#endif
                x = right;
          
                if (x != xnext || y != ynext) {
                    plast = Newlink;
                    plast->pa_x = x;
                    plast->pa_y = y;
                    pp->pa_next = plast;
                    plast->pa_prev = pp;
                    plast->pa_next = NULL;
                    newlink();
                }
                else {
                    plast = pp;
                    plast->pa_next = NULL;
                }
                if (pfirst->pa_next != plast) {

                    /* note that we are saving poly fragments */
                    Newpoly->pl_pair = pfirst;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                    Newpoly->pl_pair = plast;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                }
            }
            pp = pnext;
            if (pp == pa)
                break;
        }
        if (pointer < Newpoly)
            linkpolys(pointer,'Y');
next:
        ;
    }
    for (list = end, end = Polybuf; list < Newpoly; list++) {
        if (list->pl_pair) {
            end->pl_pair = list->pl_pair;
            end->pl_start = NULL;
            end++;
        }
    }
    Newpoly = end;
}


static void
clip_left(left)
 
int left;
{
    struct pair *pa, *pp, *pfirst, *plast, *pnext;
    struct plist *list, *end, *pointer;
    int x, y, xnext, ynext;
 
    end = Newpoly;
    for (list = Polybuf; list < end; list++) {
        pointer = Newpoly;
        pa = list->pl_pair;
        /*
         * First, find a vertex outside of the clipping
         * region.
         */

        for (pp = pa;;) {
            if (pp->pa_x < left)
                break;
            pp = pp->pa_next;
            if (pp == pa) {
                /* no clipping needed */
                Newpoly->pl_pair = pa;
                newpoly();
                goto next;
            }
        }
        /* start with an "outside" point */
 
        for (pa = pp;;) {
            x = pp->pa_x;
            y = pp->pa_y;
            pnext = pp->pa_next;
            xnext = pnext->pa_x;
            ynext = pnext->pa_y;

            if (xnext >= left && x < left) {
                /* poly edge crosses bounding clip line */
#ifdef DO_FLOAT
                y += (ynext - y)*(((double)(left - x))/(xnext - x));
#else
                y += ((left - x) * (ynext - y))/(xnext - x);
#endif
                x = left;
 
                if (x != xnext || y != ynext) {
                    pfirst = Newlink;
                    pfirst->pa_x = x;
                    pfirst->pa_y = y;
                    pfirst->pa_prev = NULL;
                    newlink();
                    pp = pnext;
                    pfirst->pa_next = pp;
                    pp->pa_prev = pfirst;
                }
                else {
                    pp = pnext;
                    pfirst = pnext;
                    pfirst->pa_prev = NULL;
                }
 
                /* walk along inside points, find the last one */
                for (;;) {
                    if (pp->pa_x < left) {
                        pnext = pp;
                        pp = pp->pa_prev;
                        break;
                    }
                    pp = pp->pa_next;
                }
                x = pp->pa_x;
                y = pp->pa_y;
                xnext = pnext->pa_x;
                ynext = pnext->pa_y;
#ifdef DO_FLOAT
                y += (ynext - y)*(((double)(left - x))/(xnext - x));
#else
                y += ((left - x) * (ynext - y))/(xnext - x);
#endif
                x = left;
          
                if (x != xnext || y != ynext) {
                    plast = Newlink;
                    plast->pa_x = x;
                    plast->pa_y = y;
                    pp->pa_next = plast;
                    plast->pa_prev = pp;
                    plast->pa_next = NULL;
                    newlink();
                }
                else {
                    plast = pp;
                    plast->pa_next = NULL;
                }
                if (pfirst->pa_next != plast) {

                    /* note that we are saving poly fragments */
                    Newpoly->pl_pair = pfirst;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                    Newpoly->pl_pair = plast;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                }
            }
            pp = pnext;
            if (pp == pa)
                break;
        }
        if (pointer < Newpoly)
            linkpolys(pointer,'Y');
next:
        ;
    }
    for (list = end, end = Polybuf; list < Newpoly; list++) {
        if (list->pl_pair) {
            end->pl_pair = list->pl_pair;
            end->pl_start = NULL;
            end++;
        }
    }
    Newpoly = end;
}


static void
clip_bottom(bottom)
 
int bottom;
{
    struct pair *pa, *pp, *pfirst, *plast, *pnext;
    struct plist *list, *end, *pointer;
    int x, y, xnext, ynext;
 
    end = Newpoly;
    for (list = Polybuf; list < end; list++) {
        pointer = Newpoly;
        pa = list->pl_pair;
        /*
         * First, find a vertex outside of the clipping
         * region.
         */

        for (pp = pa;;) {
            if (pp->pa_y < bottom)
                break;
            pp = pp->pa_next;
            if (pp == pa) {
                /* no clipping needed */
                Newpoly->pl_pair = pa;
                newpoly();
                goto next;
            }
        }
        /* start with an "outside" point */
 
        for (pa = pp;;) {
            x = pp->pa_x;
            y = pp->pa_y;
            pnext = pp->pa_next;
            xnext = pnext->pa_x;
            ynext = pnext->pa_y;

            if (ynext >= bottom && y < bottom) {
                /* poly edge crosses bounding clip line */
#ifdef DO_FLOAT
                x += (xnext - x)*(((double)(bottom - y))/(ynext - y));
#else
                x += ((bottom - y) * (xnext - x))/(ynext - y);
#endif
                y = bottom;
 
                if (x != xnext || y != ynext) {
                    pfirst = Newlink;
                    pfirst->pa_x = x;
                    pfirst->pa_y = y;
                    pfirst->pa_prev = NULL;
                    newlink();
                    pp = pnext;
                    pfirst->pa_next = pp;
                    pp->pa_prev = pfirst;
                }
                else {
                    pp = pnext;
                    pfirst = pnext;
                    pfirst->pa_prev = NULL;
                }
 
                /* walk along inside points, find the last one */
                for (;;) {
                    if (pp->pa_y < bottom) {
                        pnext = pp;
                        pp = pp->pa_prev;
                        break;
                    }
                    pp = pp->pa_next;
                }
                x = pp->pa_x;
                y = pp->pa_y;
                xnext = pnext->pa_x;
                ynext = pnext->pa_y;
#ifdef DO_FLOAT
                x += (xnext - x)*(((double)(bottom - y))/(ynext - y));
#else
                x += ((bottom - y) * (xnext - x))/(ynext - y);
#endif
                y = bottom;
          
                if (x != xnext || y != ynext) {
                    plast = Newlink;
                    plast->pa_x = x;
                    plast->pa_y = y;
                    pp->pa_next = plast;
                    plast->pa_prev = pp;
                    plast->pa_next = NULL;
                    newlink();
                }
                else {
                    plast = pp;
                    plast->pa_next = NULL;
                }
                if (pfirst->pa_next != plast) {

                    /* note that we are saving poly fragments */
                    Newpoly->pl_pair = pfirst;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                    Newpoly->pl_pair = plast;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                }
            }
            pp = pnext;
            if (pp == pa)
                break;
        }
        if (pointer < Newpoly)
            linkpolys(pointer,'X');
next:
        ;
    }
    for (list = end, end = Polybuf; list < Newpoly; list++) {
        if (list->pl_pair) {
            end->pl_pair = list->pl_pair;
            end->pl_start = NULL;
            end++;
        }
    }
    Newpoly = end;
}


static void
clip_top(top)
 
int top;
{
    struct pair *pa, *pp, *pfirst, *plast, *pnext;
    struct plist *list, *end, *pointer;
    int x, y, xnext, ynext;
 
    end = Newpoly;
    for (list = Polybuf; list < end; list++) {
        pointer = Newpoly;
        pa = list->pl_pair;
        /*
         * First, find a vertex outside of the clipping
         * region.
         */

        for (pp = pa;;) {
            if (pp->pa_y > top)
                break;
            pp = pp->pa_next;
            if (pp == pa) {
                /* no clipping needed */
                Newpoly->pl_pair = pa;
                newpoly();
                goto next;
            }
        }
        /* start with an "outside" point */
 
        for (pa = pp;;) {
            x = pp->pa_x;
            y = pp->pa_y;
            pnext = pp->pa_next;
            xnext = pnext->pa_x;
            ynext = pnext->pa_y;

            if (ynext <= top && y > top) {
                /* poly edge crosses bounding clip line */
#ifdef DO_FLOAT
                x += (xnext - x)*(((double)(top - y))/(ynext - y));
#else
                x += ((top - y) * (xnext - x))/(ynext - y);
#endif
                y = top;
 
                if (x != xnext || y != ynext) {
                    pfirst = Newlink;
                    pfirst->pa_x = x;
                    pfirst->pa_y = y;
                    pfirst->pa_prev = NULL;
                    newlink();
                    pp = pnext;
                    pfirst->pa_next = pp;
                    pp->pa_prev = pfirst;
                }
                else {
                    pp = pnext;
                    pfirst = pnext;
                    pfirst->pa_prev = NULL;
                }
 
                /* walk along inside points, find the last one */
                for (;;) {
                    if (pp->pa_y > top) {
                        pnext = pp;
                        pp = pp->pa_prev;
                        break;
                    }
                    pp = pp->pa_next;
                }
                x = pp->pa_x;
                y = pp->pa_y;
                xnext = pnext->pa_x;
                ynext = pnext->pa_y;
#ifdef DO_FLOAT
                x += (xnext - x)*(((double)(top - y))/(ynext - y));
#else
                x += ((top - y) * (xnext - x))/(ynext - y);
#endif
                y = top;
          
                if (x != xnext || y != ynext) {
                    plast = Newlink;
                    plast->pa_x = x;
                    plast->pa_y = y;
                    pp->pa_next = plast;
                    plast->pa_prev = pp;
                    plast->pa_next = NULL;
                    newlink();
                }
                else {
                    plast = pp;
                    plast->pa_next = NULL;
                }
                if (pfirst->pa_next != plast) {

                    /* note that we are saving poly fragments */
                    Newpoly->pl_pair = pfirst;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                    Newpoly->pl_pair = plast;
                    Newpoly->pl_start = pfirst;
                    newpoly();
                }
            }
            pp = pnext;
            if (pp == pa)
                break;
        }
        if (pointer < Newpoly)
            linkpolys(pointer,'X');
next:
        ;
    }
    for (list = end, end = Polybuf; list < Newpoly; list++) {
        if (list->pl_pair) {
            end->pl_pair = list->pl_pair;
            end->pl_start = NULL;
            end++;
        }
    }
    Newpoly = end;
}


static void
linkpolys(base,XorY)

struct plist *base;
int XorY;
{
    struct plist tmp, *b, *bb;

    if (XorY == 'X')
        qsort(base,Newpoly-base,sizeof(struct plist),polycmpX);
    else
        qsort(base,Newpoly-base,sizeof(struct plist),polycmpY);

    for (b = base; b < Newpoly; b += 2) {
        /* link adjacent vertices */
        if (b->pl_pair == b->pl_start) {
            if ((b+1)->pl_pair == (b+1)->pl_start) {
                tmp = *(b+1);
                *(b+1) = *(b+2);
                *(b+2) = tmp;
            }
            b->pl_pair->pa_prev  = (b+1)->pl_pair;
            (b+1)->pl_pair->pa_next = b->pl_pair;
            (b+1)->pl_pair = NULL;
            if (b->pl_start != (b+1)->pl_start) {
                for (bb = b+2; bb < Newpoly; bb++) {
                    if (bb->pl_start == b->pl_start) {
                        bb->pl_start = (b+1)->pl_start;
                        break;
                    }
                }
                b->pl_pair = NULL;
            }
        }
        else {
            if ((b+1)->pl_pair != (b+1)->pl_start) {
                tmp = *(b+1);
                *(b+1) = *(b+2);
                *(b+2) = tmp;
            }
            b->pl_pair->pa_next  = (b+1)->pl_pair;
            (b+1)->pl_pair->pa_prev = b->pl_pair;
            (b+1)->pl_pair = NULL;
            if (b->pl_start != (b+1)->pl_start) {
                for (bb = b+2; bb < Newpoly; bb++) {
                    if (bb->pl_start == (b+1)->pl_start) {
                        bb->pl_start = b->pl_start;
                        break;
                    }
                }
                b->pl_pair = NULL;
            }
        }
    }
}


static int
polycmpX(p1,p2)

#ifdef __STDC__
const void *p1, *p2;
#else
char *p1, *p2;
#endif
{
    return (((struct plist *)p1)->pl_pair->pa_x -
        ((struct plist *)p2)->pl_pair->pa_x);
}


static int
polycmpY(p1,p2)

#ifdef __STDC__
const void *p1, *p2;
#else
char *p1, *p2;
#endif
{
    return (((struct plist *)p1)->pl_pair->pa_y -
        ((struct plist *)p2)->pl_pair->pa_y);
}


/* The next two routines support dynamic memory.  Structures are expanded
 * as necessary but never freed.
 */

static void
newlink()

{
    struct pair *oldbuf;
    int i, num;

    if (Linkbuf == NULL) {
        Linkbuf = (struct pair *)malloc(400*sizeof(struct pair));
        if (Linkbuf == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        Newlink = Linkbuf;
        Numlinks = 400;
        return;
    }

    Newlink++;
    num = Newlink - Linkbuf;
    if (num > Numlinks) {
        Numlinks = num + 200;
        oldbuf = Linkbuf;
        Linkbuf = (struct pair *)
            realloc((char*)Linkbuf,Numlinks*sizeof(struct pair));
        if (Linkbuf == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        Newlink = Linkbuf + num;
        for (i = 0; i < num; i++) {
            (Linkbuf+i)->pa_next = ((Linkbuf+i)->pa_next - oldbuf) + Linkbuf;
            (Linkbuf+i)->pa_prev = ((Linkbuf+i)->pa_prev - oldbuf) + Linkbuf;
        }
        num = Newpoly - Polybuf;
        for (i = 0; i < num; i++) {
            (Polybuf+i)->pl_pair = ((Polybuf+i)->pl_pair - oldbuf) + Linkbuf;
        }
    }
}


static void
newpoly()

{
    int num;

    if (Polybuf == NULL) {
        Polybuf = (struct plist *)malloc(100*sizeof(struct plist));
        if (Polybuf == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        Newpoly = Polybuf;
        Numpolys = 100;
        return;
    }

    Newpoly++;
    num = Newpoly - Polybuf;
    if (num > Numpolys) {
        Numpolys = num + 100;
        Polybuf = (struct plist *)
            realloc((char*)Polybuf,Numpolys*sizeof(struct plist));
        if (Polybuf == NULL) {
            CDStatusInt = CDMALLOCFAILED;
            MallocFailed();
        }
        Newpoly = Polybuf + num;
    }
}

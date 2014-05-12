/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Peter P. Moore, Giles C. Billingsley
 *
 *     CD is a CIF database package that was developed by the integrated
 * circuits group of the Electronics Research Laboratory and the
 * Department of Electrical Engineering and Computer Sciences at
 * the University of California, Berkeley, California.  The programs in
 * CD are available free of charge to any interested party.
 * The sale, resale, or use of these programs for profit without the
 * express written consent of the Department of Electrical Engineering
 * and Computer Sciences, University of California, Berkeley, California,
 * is forbidden.
 *
 *************************************************************************/

/*
 * nmalloc.c
 *
 *  A new malloc/free package for virtual memory machines. 
 *
 *                A seperate freelist is kept for each object of size 
 *  sizeof(NM_ALIGN) to NM_MAX_INDEX*sizeof(NM_ALIGN), where NM_ALIGN is
 *  the smallest-sized type having the most stringent alignment 
 *  requirement on the particular machine. ( i.e. an int on the VAX ). 
 *  Any objects larger than NM_MAX_INDEX*sizeof(NM_ALIGN) is allocated
 *  and free'd by using the old malloc and free respectively. The space
 *  is allocated in blocks of size BLOCK_SIZE, but this can be changed
 *  by calling nm_set_block_size(new_size). The relevant routines are :
 *
 *            nm_malloc(size) : same syntax and usage as the old malloc.
 *
 *            nm_free(ptr, size) : free the object pointed to by ptr and
 *                                 of size 'size'.
 *            
 *            In order to allow compatibility with the old malloc and free
 *  three macros are defined in nmalloc.h : malloc(size), free(ptr), and
 *  alloc(type). Malloc and free are the same as of old, alloc returns 
 *  a properly cast pointer to an object of type 'type' ( i.e. it will
 *  keep lint quiet ). So to replace the old malloc/free, simply put
 *  #include "nmalloc.h" in any file that does malloc or free and 
 *  recompile with nmalloc.o.
 *
 *        Keep the following dire warnings in mind :
 *
 *            You may use the old and new mallocs in the same program,
 *            but DO NOT use my free on something the old malloc allocated
 *            or vice-versa.
 *
 *            If you use the free(ptr) macro, sizeof(*ptr) better be
 *            the same as the size you used in the malloc, i.e. 
 *                    
 *                    int *ptr;
 *                    ptr = (int *)malloc(40);
 *                    free(ptr);
 *
 *            will cause the allocated object of size 40 to be placed
 *            in the freelist for objects of size 4 ( sizeof(int) = 4 
 *            on a VAX). This could cause extreme difficulties. You
 *            don`t have to worry about this if all your allocations
 *            are of the form :
 *                
 *                    thing *ptr;
 *                    ptr = (thing *) malloc(sizeof(thing))
 *                    (* usage of ptr *)
 *                    free(ptr);
 *
 *            or better yet :
 *                    
 *                    thing *ptr;
 *                    ptr = alloc(thing);
 *                    (* usage of pointers *)
 *                    free(ptr);
 *
 *            But if you are doing a lot of type casting of pointers or 
 *            other jockish things, then explicit use of nm_free(ptr,size)
 *            is suggested.
 *
 *
 *        The *( (char **) ptr) constructs are used to recast portions
 *        of the objects as linking pointers to chain them to the 
 *        freelist.
 */

#include "prefix.h"
#include "nmalloc.h"

#define BLOCK_SIZE 2048              /* the amount of space requested in
                                        each block allocation */
static int nm_block_size = BLOCK_SIZE;


#ifndef USE_OLD_MALLOC

#define NM_MAX_INDEX 20              /* see above description */
#define NM_ALIGN int                /* see above description */

#define nm_align(size)\
    /* round size to next largest multiple of sizeof(NM_ALIGN) */\
    (( int )(( (int) size + sizeof(NM_ALIGN) - 1)/sizeof(NM_ALIGN)))

#ifdef __STDC__
static char *nm_block_alloc(int);
#else
static char *nm_block_alloc();
#endif

static char *nm_freelist[NM_MAX_INDEX + 1];


char *
nm_malloc(size)

unsigned int size;
{
    /* 
     *  Return a pointer to an object of size 'size',
     *  allocating a new block of free space if necessary
     */
    char *temp;
    int index;

    index = nm_align(size);
    if (index > NM_MAX_INDEX)  /* too large, use old malloc */
        return malloc(size);

    else if (nm_freelist[index] == (char *) 0) {
        /* out of free space, allocate a new block */
        return nm_block_alloc(index);
    }
    else {
        /* return the top item on the freelist */
        temp = nm_freelist[index];
        nm_freelist[index] = *(char **) temp;
        *(char **) temp = (char *) 0;
        return temp;
    }
}


void
nm_free(ptr,size)

char *ptr;
int size;
{
    /* 
     *  Free the object pointed to by ptr of size 'size'
     */
    int index;

    index = nm_align(size);
    if (index > NM_MAX_INDEX)   /* too large, use old free */
        free(ptr);

    else {
        /* link to the top of the freelist */
        *(char **) ptr = nm_freelist[index];
        nm_freelist[index] = (char *) ptr;
    }
}


static char *
nm_block_alloc(index)

int index;
{
    /*
     * Allocate a block of size nm_block_size, aligned to the
     * sizeof(NM_ALIGN)
     */
    char *ptr;
    char *current_pos, *start_of_block, *end_of_block;
    int size = sizeof(NM_ALIGN)*index;

#ifdef VMS
    if ((start_of_block = (char *) malloc(nm_block_size)) == (char *) 0)
        return (char *) 0;
#else

    current_pos = sbrk(0);
    start_of_block = (char *) (nm_align(current_pos)*sizeof(NM_ALIGN));

#endif

    end_of_block = start_of_block + nm_block_size;
#ifndef VMS
    if ((int)sbrk((int)(end_of_block - current_pos)) ==  -1L)
        /* out of memory */
        return( (char *) 0);
#endif
    end_of_block -= 2*size;
    for(ptr = start_of_block; ptr <= end_of_block; ptr += size) {
        /* link all the objects in the new block together */
        *(char **) ptr = ptr + size;
    }
    *(char **) ptr = (char *) 0;
    nm_freelist[index] = start_of_block + size;
    return start_of_block;
}

#endif /* USE_OLD_MALLOC */

void
nm_set_block_size(size)

int size;
{
    nm_block_size = size;
}


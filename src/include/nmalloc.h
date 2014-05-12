/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************
 * Original header:
 *
 * Copyright -C- 1981 Peter P. Moore, Giles C. Billingsley
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
 * for nonvirtual UNIX versions, use the old malloc by defining
 * USE_OLD_MALLOC.
 *
 * #ifdef mc500
 * #define USE_OLD_MALLOC
 * #endif
 */

#ifdef USE_OLD_MALLOC

#define alloc(type) (struct type *) malloc(sizeof(struct type))
#define afree(ptr,type) free((char *) ptr)

#else

#if __STDC__
extern char *nm_malloc(unsigned); 
extern void nm_free(char*,int);
extern void nm_set_block_size(int);
#else
extern char *nm_malloc(); 
extern void nm_free();
extern void nm_set_block_size();
#endif

#define alloc(type) (struct type *) nm_malloc(sizeof(struct type))
#define afree(ptr,type) nm_free((char *) ptr, sizeof(struct type))

#endif


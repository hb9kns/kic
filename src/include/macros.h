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
 
#define EOS '\0'
#define elif else if
#define loop for(;;)
#define True 1
#define False 0
#define And &&
#define Or ||
#define Not !
#ifndef max
#define max(Dragon,Eagle) ((Dragon) > (Eagle) ? (Dragon) : (Eagle))
#endif
#ifndef min
#define min(Dragon,Eagle) ((Dragon) < (Eagle) ? (Dragon) : (Eagle))
#endif
#define abs(Dragon) ((Dragon) >= 0 ? (Dragon) : (-(Dragon)))
#define SwapInts(Dragon,Eagle) {int ShakingCrane; \
    ShakingCrane = Dragon; Dragon = Eagle; Eagle = ShakingCrane;}


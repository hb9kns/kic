/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

/* Superconducting Microstripline Model */

#ifndef SLINE_H
#define SLINE_H

struct params {
    double lthick;
    double ldepth;
    double gpthick;
    double gpdepth;
    double dthick;
    double dielcon;
    double lwidth;
    double llength;
};

struct output {
    double L;
    double C;
    double Z;
    double T;
};

void sline(struct params*, struct output*);

#endif


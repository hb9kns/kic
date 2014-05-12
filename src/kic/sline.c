/*************************************************************************
 *
 * Enhanced KIC layout editor - Stephen R. Whiteley, 1992
 *
 *************************************************************************/

/* Superconducting Microstripline Model */

#include <stdio.h>
#include <math.h>

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

#define EP 8.85416e-6
#define MU 1.256637
#ifndef PI
#define PI 3.1415926
#endif
#define atanh(x) log(((x)+1)/(1-(x)))/2
#define MAX(x,y) (((x)>(y)) ? (x) : (y))

void
sline(tl, out)
struct params *tl;
struct output *out;
{
    double p, pp, eta, lnra, rb, rb0;
    double cap, zz, gind, kappa, aa;

    p    = 1 + tl->lthick/tl->dthick;
    p    = 2*p*p - 1;
    p   += sqrt(p*p - 1);
    pp   = sqrt(p);
    aa   = (p+1)/(2*pp);

    eta = aa * (1 + log(4/(p-1))) - 2*atanh(1/pp);
    eta = pp * (PI*tl->lwidth/(2*tl->dthick) + eta);

    rb0  = eta + aa * log(MAX(p,eta));

    if (tl->lwidth/tl->dthick >= 5)
        rb = rb0;
    else {
        rb  = -sqrt((rb0-1)*(rb0-p));
        rb += (p+1)* atanh(sqrt((rb0-p)/(rb0-1)));
        rb -= 2*pp * atanh(sqrt((rb0-p)/p/(rb0-1)));
        rb += rb0 + pp*(PI*tl->lwidth)/2/tl->dthick;
        /*
        if (tl->lwidth/tl->dthick < 1) help(3);
        */
    }

    lnra    = -2*aa * atanh(1/pp);
    lnra   -= 1 + (PI*tl->lwidth)/2/tl->dthick + log((p-1)/4/p);

    kappa   = 2*tl->dthick * (log(2*rb) - lnra)/tl->lwidth/PI;

    cap   = kappa*tl->lwidth*EP*tl->dielcon / tl->dthick;

    gind  = tl->ldepth * (1/tanh(tl->lthick/tl->ldepth) +
        2*pp/rb /sinh(tl->lthick/tl->ldepth));
    gind += tl->gpdepth / tanh(tl->gpthick/tl->gpdepth);
    gind  = MU/tl->lwidth/kappa * (tl->dthick + gind);

    zz    = sqrt(gind / cap);

    out->L = gind*tl->llength;
    out->C = cap*tl->llength;
    out->Z = zz;
    out->T = sqrt(gind*cap)*tl->llength;
}

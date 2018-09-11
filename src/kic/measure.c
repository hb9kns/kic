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

/*
 * Simple measurement package for KIC.
 */

#include "prefix.h"
#include "kic.h"
#ifdef MSDOS
#include <sys/types.h>
#include <time.h>
#else
#include <time.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#endif
#endif

time_t Realti,Realtf,Userti,Usertf,Systemti,Systemtf;

#ifdef vms
struct tbuffer {
    int proc_user_time;
    int proc_system_time;
    int child_user_time;
    int child_system_time;
};
struct tbuffer buffer;
#else
#if !defined(MDOS) && !defined(WIN32)
struct tms buffer;
#endif
#endif


void
StartTiming()

{
    time((time_t *)&Realti);
#if !defined(MDOS) && !defined(WIN32)
    times(&buffer);
#ifdef vms
    Userti = buffer.proc_user_time/60;
    Systemti = buffer.proc_system_time/60;
#else
    Userti = buffer.tms_utime/60;
    Systemti = buffer.tms_stime/60;
#endif
#endif
}


void
StopTiming()

{
    time((time_t *)&Realtf);
#if !defined(MDOS) && !defined(WIN32)
    times(&buffer);
#ifdef vms
    Usertf = buffer.proc_user_time/60;
    Systemtf = buffer.proc_system_time/60;
#else
    Usertf = buffer.tms_utime/60;
    Systemtf = buffer.tms_stime/60;
#endif
#endif
}


int
ElapsedRealTime()

{
    return ((int)(Realtf-Realti));
}


int
ElapsedUserTime()

{
    return ((int)(Usertf-Userti));
}


int
ElapsedSystemTime()

{
    return ((int)(Systemtf-Systemti));
}


void
ShowRatio(VariablesName,VariablesValue,PerUnitName,PerUnitValue)

char *VariablesName;
int VariablesValue;
char *PerUnitName;
int PerUnitValue;
{
    if(PerUnitValue != 0){
        sprintf(TypeOut,"%d %s; %d %s; %d %s/%s MORE",VariablesValue,
            VariablesName,PerUnitValue,PerUnitName,VariablesValue/PerUnitValue,
            VariablesName,PerUnitName);
    }
    else{
        sprintf(TypeOut,"%d %s; %d %s MORE",VariablesValue,VariablesName,
            PerUnitValue,PerUnitName);
    }
    ShowPrompt(TypeOut);
    (void)FBGetchar(ERASE);
    ErasePrompt();
}


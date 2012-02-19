#ifndef _PSTAT_H_
#define _PSTAT_H_

#include "param.h"

struct pstat {
    int inuse[NPROCS]; // whether this slot of the process process table is in use (1 or 0)
    int pid[NPROCS];   // the PID of each process
    int ticks[NPROCS]; // the number of ticks each process has accumulated
};


#endif // _PSTAT_H_

#ifndef MYTHREADING
#define MYTHREADING

#include <windows.h>

#include "types.h"

extern int threadCount;
extern int threadNumber;
extern int threadsStarted;
extern int threadsStopped;

int notMyJob(ULONG cutCounter, int threadId);

void doThreadedWork(LPTHREAD_START_ROUTINE worker);

#endif
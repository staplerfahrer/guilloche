#ifndef MYTHREADING
#define MYTHREADING

#include <windows.h>

#include "types.h"

int notMyJob(ULONG cutCounter, int threadId);

void doThreadedWork(LPTHREAD_START_ROUTINE worker);

#endif
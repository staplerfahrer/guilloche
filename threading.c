#include <windows.h>
#include <stdio.h>

#include "threading.h"
#include "types.h"

int threadCount    = 1;
int threadNumber   = 0;
int threadsStarted = 0;
int threadsStopped = 0;

#pragma GCC push_options
#pragma GCC optimize("O2")
int notMyJob(ULONG cutCounter, int threadId)
{
	return cutCounter % threadCount != threadId;
}
#pragma GCC pop_options

void doThreadedWork(LPTHREAD_START_ROUTINE worker)
{
	threadsStarted = 0;
	threadsStopped = 0;
	for (threadNumber = 0; threadNumber < threadCount; threadNumber++)
	{
		// create x threads
		HANDLE thread = CreateThread(NULL, 0, worker, NULL, 0, NULL);
		if (thread)
		{
			while (threadsStarted <= threadNumber)
			{
				// wait until thread has assigned threadNumber id
				printf("\%");
			}
			printf(" New thread %i\n", threadNumber);
		}
		else
		{
			printf("No thread %i\n", threadNumber);
			return;
		}
	}
	printf("\n");
	printf("%i threads started.\n", threadsStarted);

	while (threadsStopped < threadCount)
	{
		// TODO won't quit if a thread (handle) wasn't created above
		printf("\%");
		Sleep(1);
	}
	printf("\n");
	printf("%i threads stopped.\n", threadsStopped);
}
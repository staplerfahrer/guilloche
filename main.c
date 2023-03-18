#include <stdio.h>
#include <unistd.h>
#include <windows.h>

#include "types.h"
#include "patterns.h"
#include "fileio.h"
#include "drawing.h"

ParameterSet pSet;

USHORT threadCount = 16;
int threadNumber;
int threadsStarted;
int threadsStopped;

#pragma GCC push_options
#pragma GCC optimize("O2")
BOOL notMyJob(ULONG cutCounter, int threadId)
{
	return cutCounter % threadCount != threadId;
}
#pragma GCC pop_options

DWORD WINAPI threadWork(void *data) 
{
	// https://stackoverflow.com/questions/1981459/using-threads
	//     -in-c-on-windows-simple-example
	// Do stuff. This will be the first function called
	// on the new thread. When this function returns,
	// the thread goes away. See MSDN for more details.

	int threadId = threadNumber;
	printf("Starting thread %i\n", threadId);
	threadsStarted++;

	if      (strcmp(algorithm, "customParameterDrawing") == 0)
		;// customParameterDrawing(threadId);
	else if (strcmp(algorithm, "sunburstAndCircles") == 0)
		;// sunburstAndCircles(threadId);
	else
	{
		printf("Unknown algorithm \"%s\"", algorithm);
	}

	// drawCone(threadId);

	printf("Stopping thread %i\n", threadId);
	threadsStopped++;
	return 0;
}

void doThreadedWork()
{
	threadsStarted = 0;
	threadsStopped = 0;
	for (threadNumber = 0; threadNumber < threadCount; threadNumber++)
	{
		// create x threads
		HANDLE thread = CreateThread(NULL, 0, threadWork, NULL, 0, NULL);
		if (thread)
		{
			while (threadsStarted <= threadNumber)
			{
				// wait until thread has assigned threadNumber id
				printf("\%\n");
			}
			printf("New thread %i\n", threadNumber);
		}
		else
		{
			printf("No thread %i\n", threadNumber);
			return;
		}
	}

	printf("%i threads started.\n", threadsStarted);

	while (threadsStopped < threadCount)
	{
		// TODO won't quit if a thread (handle) wasn't created above
		Sleep(1);
	}

	printf("%i threads stopped.\n", threadsStopped);
}

void nonUi(int argc, char *argv[])
{
	if (argc < 14)
		return;

	pSet.waves                = atof(argv[1]);
	pSet.spiral               = atof(argv[2]);
	pSet.depthA               = atof(argv[3]);
	pSet.depthB               = atof(argv[4]);
	pSet.wheel1SizeA          = atof(argv[5]);
	pSet.wheel1SizeB          = atof(argv[6]);
	pSet.wheelCenterOffset    = atof(argv[7]);
	pSet.wheelCount           = atol(argv[8]);
	pSet.teethDensityRelative = atof(argv[9]);
	pSet.teethCountFixed      = atol(argv[10]);

	// output resolution & downsampling
	if (strcmp(argv[11], "1k") == 0)
	{
		imageSize          = 1024;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x20449A;
		strcpy(tifFormatFile, "1kx1kx1x16b.tif");
	}
	else if (strcmp(argv[11], "4k") == 0)
	{
		imageSize          = 4096;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x200449A;
		strcpy(tifFormatFile, "4kx4kx1x16b.tif");
	}
	else
	{
		return;
	}

	// tool
	// if (strcmp(argv[12], "cone63") == 0)
	// halfToolSize = (float)(toolSize / 2);

	// drawing algorithm
	strcpy(algorithm, argv[13]);

	wipe(image, imageSize * imageSize);

	//doThreadedWork();

	//finish();
}

void stdioLoop()
{
	enum
	{
		maxArgs = 20,
		linelen = 80
	};
	char commandLine[linelen] = "";
	char ch;
	char pos;

	while (1)
	{
		// repeat execution until a blank command line is fed
		pos = 0;
		while (1)
		{
			// read stdin until \n
			if (read(STDIN_FILENO, &ch, 1) > 0)
			{
				if (ch == '\n')
					break;
				commandLine[pos] = ch;
				pos++;
			}
		}
		commandLine[pos] = '\0';
		// exit on blank line
		if (strlen(commandLine) == 0)
			break;

		// parse stdin line into argc and argv[]
		int argc = 0;
		char *argv[maxArgs];
		char *p2 = strtok(commandLine, " ");
		while (p2 && argc < maxArgs - 1)
		{
			argv[argc++] = p2;
			p2 = strtok(0, " ");
		}
		argv[argc] = 0;

		// print the arguments
		for (char i = 0; i < argc; i++)
			puts(argv[i]);

		nonUi(argc, argv);
	}
}

int main(int argc, char *argv[])
{
	stdioLoop();
	return 0;
}

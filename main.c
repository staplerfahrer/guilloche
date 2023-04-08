#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "types.h"
#include "patterns.h"
#include "fileio.h"
#include "drawing.h"
#include "threading.h"

ParameterSet pSet;

extern int threadCount;
extern int threadNumber;
extern int threadsStarted;
extern int threadsStopped;

DWORD WINAPI workerThread(void *data) 
{
	// https://stackoverflow.com/questions/1981459/using-threads
	//     -in-c-on-windows-simple-example
	// Do stuff. This will be the first function called
	// on the new thread. When this function returns,
	// the thread goes away. See MSDN for more details.

	int threadId = threadNumber;
	threadsStarted++;
	printf("Started worker thread %i\n", threadId);

	if      (strcmp(algorithm, "customParameterDrawing") == 0)
		;// customParameterDrawing(threadId);
	else if (strcmp(algorithm, "sunburstAndCircles")     == 0)
		//sunburstAndCircles(threadId);
		//testDrawing(threadId);
		wavyCircles(threadId);
	else if (strcmp(algorithm, "slinky")                 == 0)
		slinky(threadId);
	else if (strcmp(algorithm, "drawCone")               == 0)
		drawCone(threadId);
	else
		printf("Unknown algorithm \"%s\"", algorithm);


	threadsStopped++;
	printf("Stopped worker thread %i\n", threadId);
	return 0;
}

void nonUi(int argc, char *argv[])
{
	if (argc < 1)
		return;

	int parameterIndex = 0;
	pSet.waves                = atof(argv[parameterIndex++]);
	pSet.spiral               = atof(argv[parameterIndex++]);
	pSet.depthA               = atof(argv[parameterIndex++]);
	pSet.depthB               = atof(argv[parameterIndex++]);
	pSet.wheel1SizeA          = atof(argv[parameterIndex++]);
	pSet.wheel1SizeB          = atof(argv[parameterIndex++]);
	pSet.wheelCenterOffset    = atol(argv[parameterIndex++]);
	pSet.wheelCount           = atof(argv[parameterIndex++]);
	pSet.teethDensityRelative = atol(argv[parameterIndex++]);
	pSet.teethCountFixed      = atol(argv[parameterIndex++]);
	pSet.toolWidth            = atof(argv[parameterIndex++]);

	// output resolution & downsampling
	if      (strcmp(argv[parameterIndex], "1k") == 0)
	{
		imageSize          = 1024;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 0x2E;
		imageFooterAddress = 0x449A;
		strcpy(tifFormatFile, "tools and templates\\1kx1kx1x16b.dat");
	}
	else if (strcmp(argv[parameterIndex], "4k") == 0)
	{
		imageSize          = 4096;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 0x2E;
		imageFooterAddress = 0x449A;
		strcpy(tifFormatFile, "tools and templates\\4kx4kx1x16b.dat");
	}
	else if (strcmp(argv[parameterIndex], "8k") == 0)
	{
		imageSize          = 8192;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 0x2E;
		imageFooterAddress = 0x449A;
		strcpy(tifFormatFile, "tools and templates\\8kx8kx1x16b.dat");
	}
	else if (strcmp(argv[parameterIndex], "16k") == 0)
	{
		imageSize          = 16384;
		imageHeaderSize    = 0x449C;
		imageFooterSize    = 46;
		imageFooterAddress = 0x449C;
		strcpy(tifFormatFile, "tools and templates\\16kx16kx1x16b.dat");
	}
	else
	{
		return;
	}
	parameterIndex++;

	// wiping and loading
	wipe(image, imageSize * imageSize);

	// set the drawing algorithm
	strcpy(algorithm, argv[parameterIndex++]);

	doThreadedWork(workerThread);

	// maximize();

	finish();
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

		clock_t start = clock();
		nonUi(argc, argv);
		double clockTimeUsed = (double)(clock() - start) / CLOCKS_PER_SEC;
		printf("\nTime passed: %.1f seconds.\n", clockTimeUsed);

		printf("stdioLoop() repeat...\n");
	}
}

int main(int argc, char *argv[])
{
	stdioLoop();
	return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "types.h"
#include "patterns.h"
#include "fileio.h"
#include "drawing.h"
#include "threading.h"

ParameterSet pSet;

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
	else if (strcmp(algorithm, "sunburstAndCircles") == 0)
		//sunburstAndCircles(threadId);
		testDrawing(threadId);
	else
		printf("Unknown algorithm \"%s\"", algorithm);

	// drawCone(threadId);

	threadsStopped++;
	printf("Stopped worker thread %i\n", threadId);
	return 0;
}

void nonUi(int argc, char *argv[])
{
	if (argc < 12)
		return;

	pSet.waves                = atof(argv[0]);
	pSet.spiral               = atof(argv[1]);
	pSet.depthA               = atof(argv[2]);
	pSet.depthB               = atof(argv[3]);
	pSet.wheel1SizeA          = atof(argv[4]);
	pSet.wheel1SizeB          = atof(argv[5]);
	pSet.wheelCenterOffset    = atof(argv[6]);
	pSet.wheelCount           = atol(argv[7]);
	pSet.teethDensityRelative = atof(argv[8]);
	pSet.teethCountFixed      = atol(argv[9]);

	// output resolution & downsampling
	if      (strcmp(argv[10], "1k") == 0)
	{
		imageSize          = 1024;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x20449A;
		toolSize           = 5040;
		strcpy(tifFormatFile, "tools and templates\\1kx1kx1x16b.tif");
	}
	else if (strcmp(argv[10], "4k") == 0)
	{
		imageSize          = 4096;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x200449A;
		toolSize           = 5040;
		strcpy(tifFormatFile, "tools and templates\\4kx4kx1x16b.tif");
	}
	else
	{
		return;
	}

	// wiping and loading
	wipe(image, imageSize * imageSize);
	wipe(samplingTool, imageSize * imageSize);

	// tool
	loadSamplingTool("tools and templates\\cone_5040x5040_16b.raw");

	// set the drawing algorithm
	strcpy(algorithm, argv[11]);

	doThreadedWork(workerThread);

	maximize();

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

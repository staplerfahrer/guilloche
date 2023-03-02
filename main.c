#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <windows.h>
#include <time.h>

//--------------------------------------------------------------------
#define PI     3.1415926535897932384626433832795
#define TWOPI  6.283185307179586476925286766559
#define USHORT unsigned short
#define ULONG  unsigned long

//--------------------------------------------------------------------
#define THREADCOUNT 1

//--------------------------------------------------------------------
USHORT width;
USHORT height;
USHORT resampleDivisor;
USHORT imageHeaderSize;
USHORT imageFooterSize;
ULONG imageFooterAddress;
char tifFormatFile[20];
USHORT toolSize;

//--------------------------------------------------------------------
// Keep global, or else it won't fit on the stack.
// Alternatively, this:
// https://stackoverflow.com/questions/22945647/why-does-a-
//		large-local-array-crash-my-program-but-a-global-one-doesnt
char imageHeader[1048576];
char imageFooter[1048576];

USHORT image[16384 * 16384];      // 16k x 16k x 2 bytes per pixel = 512 MB
USHORT imageFinal[16384 * 16384]; // 16k x 16k x 2 bytes per pixel = 512 MB
USHORT tool[1023*1023];

ULONG pixelInteractions = 0;

//--------------------------------------------------------------------
BOOL running = TRUE;
int threadNumber;
int threadsStarted;
int threadsStopped;

typedef struct Parameters
{
	float waves;
	float spiral;
	float depthA;		// ax
	float depthB;		//    + b
	float wheel1SizeA;	// ax
	float wheel1SizeB;	//    + b
	float wheelCenterOffset;
	float wheelCount;
	float teethDensityRelative;
	int teethCountFixed;
} ParameterSet;

ParameterSet pSet =
{
	.waves = 0,
	.spiral = 0,
	.depthA = 7.0/8,
	.depthB = 1.0/8,
	.wheel1SizeA = 0.9,
	.wheel1SizeB = 0.0,
	.wheelCenterOffset = 0.4,
	.wheelCount = 72,
	.teethDensityRelative = 0.1,
	.teethCountFixed = 0
};

void wipe(USHORT *img, ULONG elements)
{
	for (ULONG i = 0; i < elements; i++) img[i] = -1;
}

void loadTool(char *name, ULONG headerSize)
{
	FILE *ptr;
	ptr = fopen(name, "rb");
	fseek(ptr, headerSize, SEEK_SET);
	fread(tool, toolSize*toolSize*2, 1, ptr); // W x H x 2 bytes per pixel
	fclose(ptr);
}

void save(char *name, USHORT *img)
{
	FILE *ptr;
	ptr = fopen(tifFormatFile, "rb");
	fread(imageHeader, imageHeaderSize, 1, ptr);
	fseek(ptr, imageFooterAddress, SEEK_SET);
	fread(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);

	ptr = fopen(name, "wb");
	fwrite(imageHeader, imageHeaderSize, 1, ptr);
	fwrite(img, width*height*2, 1, ptr); // 2 bytes per pixel
	fwrite(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);
}

int inputInt(char *text, int value)
{
	printf("%s [%d]: ", text, value);
	char inp[20];
	// TODO handle CTRL+C
	fgets(inp, 20, stdin);
	if (strlen(inp) > 1) //??
		{return atol(inp);}
	else
		{return value;}
}

float inputFloat(char *text, float value)
{
	printf("%s [%f]: ", text, value);
	char inp[20];
	// TODO handle CTRL+C
	fgets(inp, 20, stdin);
	if (strlen(inp) > 1) //??
		{return atof(inp);}
	else
		{return value;}
}

USHORT getPixel(USHORT *img, int w, int x, int y)
{
	return img[w * y + x];
}

void setPixel(USHORT *img, int w, int x, int y, USHORT p)
{
	img[w * y + x] = p;
}

void resample(USHORT *img, USHORT *imgResized)
{
	int targetHeight = height / resampleDivisor;
	int targetWidth = width / resampleDivisor;
	ULONG p;
	int x, y, n, o;
	for (y = 0; y < targetHeight; y++)
	{
		for (x = 0; x < targetWidth; x++)
		{
			p = 0;
			for (o = 0; o < resampleDivisor; o++)
			{
				for (n = 0; n < resampleDivisor; n++)
				{
					p += getPixel(img, width, x * resampleDivisor + n, y * resampleDivisor + o);
				}
			}
			p /= resampleDivisor * resampleDivisor;
			imgResized[targetWidth * y + x] = p;
		}
	}
}

USHORT minimum(unsigned int a, unsigned int b)
{
	return a < b ? a : b;
}

float limit(float a)
{
	return a < 0 ? 0 : a > 1 ? 1 : a;
}

void cut(USHORT *img, USHORT *tool, int x, int y, USHORT depth)
{
	int toolX, toolY;
	int imageX, imageY;
	USHORT pImage = 0;
	USHORT pTool = 0;
	USHORT pFinal = 0;
	for (toolY = 0; toolY < toolSize; toolY++)
	{
		for (toolX = 0; toolX < toolSize; toolX++)
		{
			imageX = x - ((toolSize - 1) / 2) + toolX;
			imageY = y - ((toolSize - 1) / 2) + toolY;

			if (imageX < 0) continue;
			if (imageX >= width) continue;
			if (imageY < 0) continue;
			if (imageY >= height) continue;

			pTool = getPixel(tool, toolSize, toolX, toolY);
			pImage = getPixel(img, width, imageX, imageY);
			pFinal = minimum(pTool + depth, pImage); // + causes unsigned int?
			setPixel(img, width, imageX, imageY, pFinal);
		}
	}
	pixelInteractions += pow(toolSize, 2) * 3;
}

void cutFloat(USHORT *img, USHORT *tool, float x, float y, float depth)
{
	int halfX = width / 2;
	int halfY = height / 2;
	int imageX = halfX + (x * (float) width / 2);
	int imageY = height - (halfY + (y * (float) height / 2));
	USHORT depthInt = 0xFFFF - (limit(depth) * 0xFFFF);
	cut(img, tool, imageX, imageY, depthInt);
}

void finish()
{
	if (resampleDivisor > 1)
	{
		printf("resampling...\n");
		resample(image, imageFinal);
		printf("saving...\n");
		save("C:\\Temp\\out.temp", imageFinal);
	}
	else
	{
		printf("saving...\n");
		save("C:\\Temp\\out.temp", image);
	}
	remove("C:\\Temp\\out.tif");
	rename("C:\\Temp\\out.temp", "C:\\Temp\\out.tif");
	//system("C:\\Temp\\out.tif"); // open the .tif
	printf("done\n");
}

BOOL WINAPI ctrlCHandler(DWORD signal)
{
	printf("signal %d ", signal);
    if (signal == CTRL_C_EVENT)
	{
        printf("Ctrl-C handled\n");
		running = FALSE;
	}
    return TRUE;
}

BOOL notThisThread(int threadId, ULONG cutCounter)
{
	// Perfectly divide the cutting
	// tasks over the number of threads (THREADCOUNT).
	// cutCounter % THREADCOUNT = 0..11
	//printf("cutCounter %i THREADCOUNT %i threadId %i", cutCounter, THREADCOUNT, threadId);
	return cutCounter % THREADCOUNT != threadId;
}

void parameterUi()
{
	pSet.waves                = inputFloat("1/10  waves", pSet.waves);
	pSet.spiral               = inputFloat("2/10  spiral", pSet.spiral);
	pSet.depthA               = inputFloat("3/10  depthA", pSet.depthA);
	pSet.depthB               = inputFloat("4/10  depthB", pSet.depthB);
	pSet.wheel1SizeA          = inputFloat("5/10  wheel1SizeA", pSet.wheel1SizeA);
	pSet.wheel1SizeB          = inputFloat("6/10  wheel1SizeB", pSet.wheel1SizeB);
	pSet.wheelCenterOffset    = inputFloat("7/10  wheelCenterOffset", pSet.wheelCenterOffset);
	pSet.wheelCount           = inputFloat("8/10  wheelCount", pSet.wheelCount);
	pSet.teethDensityRelative = inputFloat("9/10  teethDensityRelative", pSet.teethDensityRelative);
	pSet.teethCountFixed      = inputInt(  "10/10 teethCountFixed", pSet.teethCountFixed);
}

void concentricWobbleSpiral(int threadId)
{
	ULONG cutCounter = 0;
	float wheel1Rotation;
	float wheel1Size;
	float teethDensityRelative;
	int wheel1Teeth;
	float wheel1Tooth;
	float wheel1SizeMax = 0.9;
	float wheelCount;
	float waves;
	float spiral;
	int wheelNumber;

	wheelCount = 32;
	waves = 24;
	teethDensityRelative = 0.1;
	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	{
		printf("wheel %d... ", wheelNumber);
		wheel1Size = 1.0 / wheelCount * wheelNumber;
		wheel1Teeth = PI * width * wheel1Size * teethDensityRelative;
		wheel1Tooth = TWOPI / wheel1Teeth;
		spiral = 0-wheelNumber/5.0;
		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
		{
			cutCounter++;
			if (cutCounter % 1000 == 0) printf(".");
			cutFloat(image, tool,
					wheel1SizeMax*wheel1Size*cos(wheel1Rotation),
					wheel1SizeMax*wheel1Size*sin(wheel1Rotation),
					((7.0/8)
						+(1.0/8)*cos((wheel1Rotation+spiral)*waves))
						*wheelNumber/wheelCount);
			if (!running) break;
		}
		printf(" ");
		if (!running) break;
	}
}

void sunburst(int threadId)
{
	ULONG cutCounter = 0;
	float waves = 0;
	float spiral = 0;
	float depthA = 0.0/8; // depth = ax + b
	float depthB = 2.0/8;
	float wheel1Rotation;
	float wheel1Size;
	float wheel1SizeMax = 0.9;
	int wheel1Teeth;
	float wheel1Tooth;
	float wheelCount = 200;
	float teethDensityRelative = 0.2;
	int teethCountFixed = 0;
	int wheelNumber;
	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	{
		printf("wheel %d...", wheelNumber);
		wheel1Size = wheel1SizeMax / wheelCount * wheelNumber;
		wheel1Teeth = teethCountFixed > 0
			? teethCountFixed
			: PI * width * wheel1Size * teethDensityRelative;
		wheel1Tooth = TWOPI / wheel1Teeth;
		float spiralTurn = 0-wheelNumber/wheelCount*spiral*TWOPI;
		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
		{
			cutCounter++;
			if (cutCounter % THREADCOUNT != threadId) continue;
			if (cutCounter % 1000 == 0) printf(".");
			cutFloat(image, tool,
					wheel1Size*cos(wheel1Rotation+spiralTurn),
					wheel1Size*sin(wheel1Rotation+spiralTurn),
					depthA*(0)+depthB);
			if (!running) break;
		}
		printf(" ");
		if (!running) break;
	}
}

void overlappingCircles(int threadId)
{
	ULONG cutCounter = 0;
	float waves = 0;
	float spiral = 0;
	float depthA = 7.0/8; // depth = ax
	float depthB = 1.0/8; //         + b
	float wheel1Rotation;
	float wheel1Size;
	float wheel1SizeMax = 0.9;
	int wheel1Teeth;
	float wheel1Tooth;
	float wheelCount = 72;
	float teethDensityRelative = 0.5;
	int teethCountFixed = 0;
	int wheelNumber;
	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	{
		printf("%d: wheel %d...", threadId, wheelNumber);
		wheel1Size = 0.4; //wheel1SizeMax / wheelCount * wheelNumber;
		wheel1Teeth = teethCountFixed > 0
			? teethCountFixed
			: PI * width * wheel1Size * teethDensityRelative;
		wheel1Tooth = TWOPI / wheel1Teeth;
		float spiralTurn = 0-wheelNumber/wheelCount*spiral*TWOPI;
		float wheelCenterOffset = 0.4;
		float wheelCenterX = cos(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
		float wheelCenterY = sin(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
		{
			cutCounter++;
			if (cutCounter % THREADCOUNT != threadId) continue; // not this thread
			if (cutCounter % 1000 == 0) printf(".");
			float cutX = wheelCenterX+wheel1Size*cos(wheel1Rotation+spiralTurn);
			float cutY = wheelCenterY+wheel1Size*sin(wheel1Rotation+spiralTurn);
			float depthX = sqrt(pow(fabs(cutX),2)+pow(fabs(cutY),2));
			cutFloat(image, tool,
					cutX,
					cutY,
					depthA*depthX+depthB);
			if (!running) break;
		}
		printf(" ");
		if (!running) break;
	}
}

void customParameterDrawing(int threadId)
{
	float waves                = pSet.waves;
	float spiral               = pSet.spiral;
	float depthA               = pSet.depthA;
	float depthB               = pSet.depthB;
	float wheel1SizeA          = pSet.wheel1SizeA;
	float wheel1SizeB          = pSet.wheel1SizeB;
	float wheelCenterOffset    = pSet.wheelCenterOffset;
	float wheelCount           = pSet.wheelCount;
	float teethDensityRelative = pSet.teethDensityRelative;
	int   teethCountFixed      = pSet.teethCountFixed;

	ULONG cutCounter = 0;
	float wheel1Rotation;
	float wheel1Size;
	int   wheel1Teeth;
	float wheel1Tooth;
	int   wheelNumber;
	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	{
		printf("t %d w %d... ", threadId, wheelNumber);
		wheel1Size  = wheel1SizeA*(wheelNumber/wheelCount)+wheel1SizeB;
		wheel1Teeth = teethCountFixed > 0
			? teethCountFixed
			: PI * width * wheel1Size * teethDensityRelative;
		wheel1Tooth = TWOPI / wheel1Teeth;
		float spiralTurn = 0-wheelNumber/wheelCount*spiral*TWOPI;
		float wheelCenterX = cos(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
		float wheelCenterY = sin(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
		{
			cutCounter++;
			if (notThisThread(threadId, cutCounter)) 
			{
					printf("not ");

				continue;
			}
			float cutX = wheelCenterX+wheel1Size*cos(wheel1Rotation+spiralTurn);
			float cutY = wheelCenterY+wheel1Size*sin(wheel1Rotation+spiralTurn);
			float depthX = sqrt(pow(fabs(cutX),2)+pow(fabs(cutY),2));
			cutFloat(image, tool, cutX, cutY, depthA*depthX+depthB);
			if (!running) break;
		}
		printf("%s ", running ? "running" : "not running");
		if (!running) break;
	}
}

// void drawCone(int threadId)
// {
// 	float waves = 0;
// 	float spiral = 0;
// 	float depthA = 0;
// 	float depthB = 0;
// 	float wheel1SizeA = 1;
// 	float wheel1SizeB = 0;
// 	float wheelCenterOffset = 0;
// 	float wheelCount = 16384;
// 	float teethDensityRelative = 16;
// 	int teethCountFixed = 0;

// 	ULONG cutCounter = 0;
// 	float wheel1Rotation;
// 	float wheel1Size;
// 	int wheel1Teeth;
// 	float wheel1Tooth;
// 	int wheelNumber;

// 	USHORT halfX;
// 	USHORT halfY;
// 	USHORT imageX;
// 	USHORT imageY;
// 	halfX = (USHORT) (width / 2);
// 	halfY = (USHORT) (height / 2);

// 	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
// 	{
// 		//printf("%d: wheel %d...", threadId, wheelNumber);
// 		wheel1Size = wheel1SizeA*(wheelNumber/wheelCount)+wheel1SizeB;
// 		wheel1Teeth = teethCountFixed > 0
// 			? teethCountFixed
// 			: PI * width * wheel1Size * teethDensityRelative;
// 		wheel1Tooth = TWOPI / wheel1Teeth;
// 		float spiralTurn = 0-wheelNumber/wheelCount*spiral*TWOPI;
// 		float wheelCenterX = cos(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
// 		float wheelCenterY = sin(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
// 		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
// 		{
// 			cutCounter++;
// 			if (notThisThread(threadId, cutCounter)) continue;
// 			float cutX = wheelCenterX+wheel1Size*cos(wheel1Rotation+spiralTurn);
// 			float cutY = wheelCenterY+wheel1Size*sin(wheel1Rotation+spiralTurn);
// 			imageX = halfX + (USHORT) (cutX * (float) width / 2);
// 			imageY = height - (halfY + (USHORT) (cutY * (float) height / 2));
// 			USHORT p = (USHORT) (0xFFFF*(wheelNumber-1)/wheelCount);
// 			setPixel(image, width, imageX, imageY, p);
// 			if (!running) break;
// 		}
// 		//printf(" ");
// 		//if (!running) break;
// 	}
// }

DWORD WINAPI ThreadFunc(void *data)
{
	// https://stackoverflow.com/questions/1981459/using-threads
	//     -in-c-on-windows-simple-example
	// Do stuff. This will be the first function called
	// on the new thread. When this function returns,
	// the thread goes away. See MSDN for more details.

	int threadId = threadNumber;
	printf(" starting thread %i", threadId);
	threadsStarted++;
	//inputFloat("thread paused", 0);
	customParameterDrawing(threadId);
	// drawCone(threadId);

	printf(" stopping thread %i", threadId);
	threadsStopped++;
	return 0;
}

void uiLoop()
{
	SetConsoleCtrlHandler(ctrlCHandler, TRUE);

	while (running)
	{
		width              = 4096;
		height             = 4096;
		resampleDivisor    = 1;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x200449A;
		toolSize           = 255;
		loadTool("cone255.tif", 0x4768);
		strcpy(tifFormatFile, "4kx4kx1x16b.tif");

		wipe(image, width * height);

		// present UI
		parameterUi();

		clock_t start = clock();

		threadsStarted = 0;
		threadsStopped = 0;
		for (threadNumber = 0; threadNumber < THREADCOUNT; threadNumber++)
		{
			// create x threads
			HANDLE thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
			if (thread)
			{
				while (threadsStarted <= threadNumber)
				{
					// wait until thread has assigned threadNumber id
					printf("\%");
				}
				printf("have thread %i ", threadNumber);
			}
			else
			{
				printf("no thread %i ", threadNumber);
				return;
			}
		}

		printf("%i threads started. ", threadsStarted);

		while (threadsStopped < THREADCOUNT)
		{
			// TODO won't quit if a thread (handle) wasn't created above
			Sleep(10);
			if (!running) break;
		}

		double cpuTimeUsed = (double)(clock() - start)/CLOCKS_PER_SEC;
		printf("\nPixel interactions: %.3f M pixels in %.3f seconds, %.3f G pix/sec.\n",
			(double)pixelInteractions/1000000,
			cpuTimeUsed,
			(double)pixelInteractions/cpuTimeUsed/1000000000);

		finish();
	}
}

void nonUi(int argc, char *argv[])
{
	if (argc < 11) return;
	pSet.waves                = atof(argv[1]);
	pSet.spiral               = atof(argv[2]);
	pSet.depthA               = atof(argv[3]);
	pSet.depthB               = atof(argv[4]);
	pSet.wheel1SizeA          = atof(argv[5]);
	pSet.wheel1SizeB          = atof(argv[6]);
	pSet.wheelCenterOffset    = atof(argv[7]);
	pSet.wheelCount           = atof(argv[8]);
	pSet.teethDensityRelative = atof(argv[9]);
	pSet.teethCountFixed      = atoi(argv[10]);

	if (strcmp(argv[11], "1k") == 0)
	{
		width              = 1024;
		height             = 1024;
		resampleDivisor    = 1;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x20449A;
		toolSize           = 63;
		loadTool("cone63.tif", 0x444C);
		strcpy(tifFormatFile, "1kx1kx1x16b.tif");
	}

	if (strcmp(argv[11], "4k") == 0)
	{
		width              = 4096;
		height             = 4096;
		resampleDivisor    = 1;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x200449A;
		toolSize           = 255;
		loadTool("cone255.tif", 0x4768);
		strcpy(tifFormatFile, "4kx4kx1x16b.tif");
	}

	if (strcmp(argv[11], "downsampled_4k") == 0)
	{
		width              = 16384;
		height             = 16384;
		resampleDivisor    = 4;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x200449A;
		toolSize           = 255;
		loadTool("cone1023.tif", 0x48da);
		strcpy(tifFormatFile, "4kx4kx1x16b.tif");
	}

	wipe(image, width * height);
	// loadTool("cone511d.tif", 0x48ac);
	// loadTool("cone511d-softer.tif", 0x48fa);
	// loadTool("cone255-maxed.tif", 0x48d2);
	// loadTool("smooth2550.tif", 0x477a0);
	// loadTool("cone1023.tif", 0x48da);

	threadsStarted = 0;
	threadsStopped = 0;
	for (threadNumber = 0; threadNumber < THREADCOUNT; threadNumber++)
	{
		// create x threads
		HANDLE thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
		if (thread)
		{
			// got a handle
			while (threadsStarted < threadNumber)
			{
				printf("?");
				// wait until thread has presented itself as alive
				Sleep(1);
			}
		}
		else
		{
			printf("no thread %i", threadNumber);
		}
	}

	printf("all threads active");

	while (threadsStopped < threadsStarted)
	{
		printf(".");
		Sleep(1);
		if (!running) break;
	}

	finish();
}

void stdioLoop()
{
	enum { maxArgs = 13, linelen = 80 };
	char commandLine[linelen] = "";
	char ch;
	char pos;

	while (TRUE)
	{
		// repeat nonUi() until blank command line
		pos = 0;
		while(TRUE)
		{
			// read stdin until \n
			if (read(STDIN_FILENO, &ch, 1) > 0)
			{
				if (ch == '\n') break;
				commandLine[pos] = ch;
				pos++;
			}
		}
		commandLine[pos] = '\0';
		if (strlen(commandLine) == 0) break;

		// parse stdin line into argc and argv[]
		int argc = 0;
		char *argv[maxArgs];
		char *p2 = strtok(commandLine, " ");
		while (p2 && argc < maxArgs-1)
		{
			argv[argc++] = p2;
			p2 = strtok(0, " ");
		}
		argv[argc] = 0;
		printf("command line: %s %s %s %s %s %s %s %s %s %s %s %s\n",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],argv[8],argv[9],argv[10],argv[11]);
		nonUi(argc, argv);
	}
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		uiLoop();
	}
	else
	{
		stdioLoop();
	}
	return 0;
}

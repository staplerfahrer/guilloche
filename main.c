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
#define THREADCOUNT 16

//--------------------------------------------------------------------
USHORT width;
USHORT height;
USHORT resampleDivisor;
USHORT imageHeaderSize;
USHORT imageFooterSize;
ULONG  imageFooterAddress;
USHORT toolSize;
char   tifFormatFile[20];

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
int  threadNumber;
int  threadsStarted;
int  threadsStopped;

char algorithm[80];

typedef struct Parameters
{
	float waves;
	float spiral;
	float depthA;		// ax
	float depthB;		//    + b
	float wheel1SizeA;	// ax
	float wheel1SizeB;	//    + b
	float wheelCenterOffset;
	int   wheelCount;
	float teethDensityRelative;
	int   teethCountFixed;
} ParameterSet;

ParameterSet pSet =
{
	.waves                = 0.0,
	.spiral               = 0.0,
	.depthA               = 1.0,
	.depthB               = 1.0,
	.wheel1SizeA          = 0.9,
	.wheel1SizeB          = 0.0,
	.wheelCenterOffset    = 0.0,
	.wheelCount           = 10,
	.teethDensityRelative = 0.01,
	.teethCountFixed      = 0
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
	// 2 bytes per pixel
	ULONG outputBytes = width*height*2/resampleDivisor/resampleDivisor;

	FILE *ptr;
	ptr = fopen(tifFormatFile, "rb");
	fread(imageHeader, imageHeaderSize, 1, ptr);
	fseek(ptr, imageFooterAddress, SEEK_SET);
	fread(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);

	ptr = fopen(name, "wb");
	fwrite(imageHeader, imageHeaderSize, 1, ptr);
	fwrite(img, outputBytes, 1, ptr);
	fwrite(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);
}

int inputInt(char *text, int value)
{
	printf("%s [%d]: ", text, value);
	char inp[20];
	fgets(inp, 20, stdin);
	if (!running) return 0;
	if (strlen(inp) > 1) //??
		return atol(inp);
	else
		return value;
}

float inputFloat(char *text, float value)
{
	printf("%s [%f]: ", text, value);
	char inp[20];
	fgets(inp, 20, stdin);
	if (!running) return 0.0;
	if (strlen(inp) > 1) //??
		return atof(inp);
	else
		return value;
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

USHORT minimum(USHORT a, USHORT b)
{
	return a < b ? a : b;
}

float limit(float a)
{
	return a < 0 ? 0 : a > 1 ? 1 : a;
}

void cutPixelAa(float x, float y, float depth)
{
	// x, y are pixel coordinates from 0 to 4095 or some such
	// depth is 0 to 1, 1 being black, 0 being transparent

	// ex.
	// x = 1.4, y = 1.2
	// depth = 1
	int   x_whole     = x;                                           // 1
	int   y_whole     = y;                                           // 1
	float x_fraction  = x - x_whole;                                 // 0.4
	float y_fraction  = y - y_whole;                                 // 0.2
	float current     = depth * (1.0-x_fraction) * (1.0-y_fraction); // 1 * (1.0-0.4) * (1.0-0.2)
	float right       = depth * (x_fraction)     * (1.0-y_fraction); // 1 * 0.4 * (1.0-0.2)
	float below       = depth * (1.0-x_fraction) * (y_fraction);     // 1 * (1.0-0.4) * 0.2
	float below_right = depth * (x_fraction)     * (y_fraction);     // 1 * 0.4 * 0.2
	
	USHORT int_current     = minimum(getPixel(image, width, x_whole,   y_whole  ), (USHORT) (65535.0 - 65535.0*current));
	USHORT int_right       = minimum(getPixel(image, width, x_whole+1, y_whole  ), (USHORT) (65535.0 - 65535.0*right));
	USHORT int_below       = minimum(getPixel(image, width, x_whole,   y_whole+1), (USHORT) (65535.0 - 65535.0*below));
	USHORT int_below_right = minimum(getPixel(image, width, x_whole+1, y_whole+1), (USHORT) (65535.0 - 65535.0*below_right));

	setPixel(image, width, x_whole,   y_whole,   int_current);
	setPixel(image, width, x_whole+1, y_whole,   int_right);
	setPixel(image, width, x_whole,   y_whole+1, int_below);
	setPixel(image, width, x_whole+1, y_whole+1, int_below_right);
}

void cut(USHORT *img, int x, int y, USHORT depth)
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
			pFinal = minimum(pTool + depth, pImage); // "+" causes unsigned int?
			setPixel(img, width, imageX, imageY, pFinal);
		}
	}
	pixelInteractions += pow(toolSize, 2) * 3;
}

void cutFloat(USHORT *img, float x, float y, float depth)
{
	int halfX = width / 2;
	int halfY = height / 2;
	int imageX = halfX + (x * (float) width / 2);
	int imageY = height - (halfY + (y * (float) height / 2));
	USHORT depthInt = 0xFFFF - (limit(depth) * 0xFFFF);
	cut(img, imageX, imageY, depthInt);
}

void finish()
{
	USHORT *output = image;

	if (resampleDivisor > 1)
	{
		printf("resampling...\n");
		resample(image, imageFinal);
		output = imageFinal;
	}

	printf("saving...\n");
	save("C:\\Temp\\out.temp", output);

	remove("C:\\Temp\\out.tif");
	rename("C:\\Temp\\out.temp", "C:\\Temp\\out.tif");

	printf("Image saved to C:\\Temp\\out.tif\n");
	printf("Done.\n");
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

void parameterUi()
{
	pSet.waves                = inputFloat("1/10  waves                ", pSet.waves);
	if (!running) return;
	pSet.spiral               = inputFloat("2/10  spiral               ", pSet.spiral);
	if (!running) return;
	pSet.depthA               = inputFloat("3/10  depthA               ", pSet.depthA);
	if (!running) return;
	pSet.depthB               = inputFloat("4/10  depthB               ", pSet.depthB);
	if (!running) return;
	pSet.wheel1SizeA          = inputFloat("5/10  wheel1SizeA          ", pSet.wheel1SizeA);
	if (!running) return;
	pSet.wheel1SizeB          = inputFloat("6/10  wheel1SizeB          ", pSet.wheel1SizeB);
	if (!running) return;
	pSet.wheelCenterOffset    = inputFloat("7/10  wheelCenterOffset    ", pSet.wheelCenterOffset);
	if (!running) return;
	pSet.wheelCount           = inputInt(  "8/10  wheelCount           ", pSet.wheelCount);
	if (!running) return;
	pSet.teethDensityRelative = inputFloat("9/10  teethDensityRelative ", pSet.teethDensityRelative);
	if (!running) return;
	pSet.teethCountFixed      = inputInt(  "10/10 teethCountFixed      ", pSet.teethCountFixed);
	if (!running) return;
}

#pragma GCC push_options
#pragma GCC optimize ("O2")
BOOL notMyJob(ULONG cutCounter, int threadId)
{
	return cutCounter % THREADCOUNT != threadId;
}
#pragma GCC pop_options

void customParameterDrawing(int threadId)
{
	float  waves                = pSet.waves;
	float  spiral               = pSet.spiral;
	float  depthA               = pSet.depthA;
	float  depthB               = pSet.depthB;
	float  wheel1SizeA          = pSet.wheel1SizeA;
	float  wheel1SizeB          = pSet.wheel1SizeB;
	float  wheelCenterOffset    = pSet.wheelCenterOffset;
	float  wheelCount           = pSet.wheelCount; // float for correct division
	float  teethDensityRelative = pSet.teethDensityRelative;
	USHORT teethCountFixed      = pSet.teethCountFixed;

	ULONG  cutCounter = 0;
	float  wheel1Rotation;
	float  wheel1Size;
	USHORT wheel1Teeth;
	float  wheel1Tooth;
	USHORT wheelNumber;
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
			if (notMyJob(cutCounter, threadId))	continue;
			float cutX = wheelCenterX+wheel1Size*cos(wheel1Rotation+spiralTurn);
			float cutY = wheelCenterY+wheel1Size*sin(wheel1Rotation+spiralTurn);
			float depthX = sqrt(pow(fabs(cutX),2)+pow(fabs(cutY),2));
			cutFloat(image, cutX, cutY, depthA*depthX+depthB);
			if (!running) break;
		}
		if (!running) break;
	}
}

void sunburstAndCircles(int threadId)
{
	float  waves                = pSet.waves;
	float  spiral               = pSet.spiral;
	float  depthA               = pSet.depthA;
	float  depthB               = pSet.depthB;
	float  wheel1SizeA          = pSet.wheel1SizeA;
	float  wheel1SizeB          = pSet.wheel1SizeB;
	float  wheelCenterOffset    = pSet.wheelCenterOffset;
	float  wheelCount           = pSet.wheelCount; // float for correct division
	USHORT teethCountFixed      = pSet.teethCountFixed;

	int   wheelNumber;
	float wheel1Rotation;
	float wheel1Tooth;
	float wheel1Size;
	float wheel1SizeDivision;
	float depth = 1;

	char  stat = '.';

	wheel1Tooth = TWOPI / teethCountFixed;

	if (wheelCount == 1)
		wheel1SizeDivision = 0;
	else
		wheel1SizeDivision = (wheel1SizeA - wheel1SizeB) / (wheelCount - 1);

	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	{
		write(STDOUT_FILENO, &stat, 1);
		wheel1Size = wheel1SizeA - (wheel1SizeDivision * (wheelNumber - 1));
		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
		{
			float x = wheel1Size * cos(wheel1Rotation);
			float y = wheel1Size * sin(wheel1Rotation);
			cutFloat(image, x, y, depth);
		}
	}
}

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
		customParameterDrawing(threadId);
	else if (strcmp(algorithm, "sunburstAndCircles")     == 0)
		sunburstAndCircles(threadId);
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
	clock_t start = clock();

	threadsStarted = 0;
	threadsStopped = 0;
	for (threadNumber = 0; threadNumber < THREADCOUNT; threadNumber++)
	{
		// create x threads
		HANDLE thread = CreateThread(NULL, 0, threadWork, NULL, 0, NULL);
		if (thread)
		{
			while (threadsStarted <= threadNumber)
			{
				// wait until thread has assigned threadNumber id
				printf("\%");
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

	while (threadsStopped < THREADCOUNT)
	{
		// TODO won't quit if a thread (handle) wasn't created above
		Sleep(1);
		if (!running) break;
	}

	printf("%i threads stopped.\n", threadsStopped);

	double cpuTimeUsed = (double)(clock() - start)/CLOCKS_PER_SEC;
	printf("\nPixel interactions: %.3f M pixels in %.3f seconds, %.3f G pix/sec.\n",
		(double)pixelInteractions/1000000,
		cpuTimeUsed,
		(double)pixelInteractions/cpuTimeUsed/1000000000);
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
		if (!running) return;

		doThreadedWork("customParameterDrawing");

		finish();
	}
}

void nonUi(int argc, char *argv[])
{
	if (argc < 14) return;

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


	// TODO? fix resolution to 4095, or 1023 so we have a middle pixel!
	// 512/512 or 2048/2048 is currently "the middle"


	// output resolution & downsampling
	if      (strcmp(argv[11], "1k")             == 0)
	{
		width              = 1024;
		height             = 1024;
		resampleDivisor    = 1;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x20449A;
		strcpy(tifFormatFile, "1kx1kx1x16b.tif");
	}
	else if (strcmp(argv[11], "4k")             == 0)
	{
		width              = 4096;
		height             = 4096;
		resampleDivisor    = 1;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x200449A;
		strcpy(tifFormatFile, "4kx4kx1x16b.tif");
	}
	else if (strcmp(argv[11], "downsampled_4k") == 0)
	{
		width              = 16384;
		height             = 16384;
		resampleDivisor    = 4;
		imageHeaderSize    = 0x449A;
		imageFooterSize    = 46;
		imageFooterAddress = 0x200449A;
		strcpy(tifFormatFile, "4kx4kx1x16b.tif");
	}

	// tool
	// loadTool("cone511d.tif", 0x48ac);
	// loadTool("cone511d-softer.tif", 0x48fa);
	// loadTool("cone255-maxed.tif", 0x48d2);
	// loadTool("smooth2550.tif", 0x477a0);
	// loadTool("cone1023.tif", 0x48da);
	if      (strcmp(argv[12], "cone63")   == 0)
	{
		toolSize = 63;
		loadTool("cone63.tif", 0x444C);
	}
	else if (strcmp(argv[12], "cone255")  == 0)
	{
		toolSize = 255;
		loadTool("cone255.tif", 0x4768);
	}
	else if (strcmp(argv[12], "cone1023") == 0)
	{
		toolSize = 1023;
		loadTool("cone1023.tif", 0x48da);
	}

	strcpy(algorithm, argv[13]);

	wipe(image, width * height);

	doThreadedWork();

	cutPixelAa(1.0, 1.0, 1.0);

	cutPixelAa(1.01 + 3, 1.0, 1.0);

	cutPixelAa(1.0 + 6, 1.01, 1.0);

	cutPixelAa(1.01 + 9, 1.01, 1.0);

	cutPixelAa(1.99 + 12, 1.0, 1.0);

	cutPixelAa(1.0 + 15, 1.99, 1.0);

	cutPixelAa(1.99 + 18, 1.99, 1.0);

	finish();
}

void stdioLoop()
{
	enum { maxArgs = 20, linelen = 80 };
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
		int  argc = 0;
		char *argv[maxArgs];
		char *p2 = strtok(commandLine, " ");
		while (p2 && argc < maxArgs-1)
		{
			argv[argc++] = p2;
			p2 = strtok(0, " ");
		}
		argv[argc] = 0;

		char i = 0;
		for (i = 0; i < argc; i++)
			puts(argv[i]);

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

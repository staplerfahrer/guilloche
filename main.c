#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <time.h>

#define WIDTH 8192
#define HEIGHT 8192
#define BDEPTH unsigned short
// #define TOOLSIZE 255
#define TOOLSIZE 1023
#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559
#define THREADCOUNT 12

// Keep global, or else it won't fit on the stack.
// Alternatively, this:
// https://stackoverflow.com/questions/22945647/why-does-a-
//		large-local-array-crash-my-program-but-a-global-one-doesnt
char imageHeader[0x449A]; // same 4k or 8k
char imageFooter[46]; // 47 for 8k?
long int imageFooterAddress = 0x200449A; // 200449A for 4k, 800449A for 8k
BDEPTH image[WIDTH * HEIGHT];
BDEPTH imageQtr[WIDTH * HEIGHT / 4];
BDEPTH tool[TOOLSIZE * TOOLSIZE];

BOOL running = TRUE;
BOOL threads[THREADCOUNT];
int threadNumber;
unsigned long pixelInteractions = 0;

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
	.teethDensityRelative = 0.5,
	.teethCountFixed = 0
};

void wipe(BDEPTH *img, unsigned long elements)
{
	for (unsigned long i = 0; i < elements; i++) img[i] = -1;
}

void load(char *name, unsigned long headerSize, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen(name, "rb");
	fseek(ptr, headerSize, SEEK_SET);
	fread(img, size, 1, ptr);
	fclose(ptr);
}

void save(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen("4kx4kx1x16b.tif", "rb");
	fread(imageHeader, sizeof(imageHeader), 1, ptr);
	fseek(ptr, imageFooterAddress, SEEK_SET);
	fread(imageFooter, sizeof(imageFooter), 1, ptr);
	fclose(ptr);

	ptr = fopen(name, "wb");
	fwrite(imageHeader, sizeof(imageHeader), 1, ptr);
	fwrite(img, size, 1, ptr);
	fwrite(imageFooter, sizeof(imageFooter), 1, ptr);
	fclose(ptr);
}

int inputInt(char *text, int value)
{
	printf("%s (%d): ", text, value);
	char inp[20];
	fgets(inp, 20, stdin);
	return atol(inp);
}

float inputFloat(char *text, float value)
{
	printf("%s (%f): ", text, value);
	char inp[20];
	fgets(inp, 20, stdin);
	return atof(inp);
}

BDEPTH getPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y)
{
	return img[w * y + x];
}

void setPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y, BDEPTH p)
{
	img[w * y + x] = p;
}

void resample(BDEPTH *img, BDEPTH *imgQtr)
{
	int divisor = 2;
	int targetHeight = HEIGHT / divisor;
	int targetWidth = WIDTH / divisor;
	unsigned long p;
	int x, y, n, o;
	for (y = 0; y < targetHeight; y++)
	{
		for (x = 0; x < targetWidth; x++)
		{
			p = 0;
			for (o = 0; o < divisor; o++)
			{
				for (n = 0; n < divisor; n++)
				{
					p += getPixel(img, WIDTH, x * divisor + n, y * divisor + o);
				}
			}
			p /= divisor * divisor;
			imgQtr[targetWidth * y + x] = p;
		}
	}
}

BDEPTH minimum(unsigned int a, unsigned int b)
{
	return a < b ? a : b;
}

float limit(float a)
{
	return a < 0 ? 0 : a > 1 ? 1 : a;
}

void cut(BDEPTH *img, BDEPTH *tool, BDEPTH x, BDEPTH y, BDEPTH depth)
{
	BDEPTH toolX, toolY;
	int imageX, imageY;
	BDEPTH pImage = 0;
	BDEPTH pTool = 0;
	BDEPTH pFinal = 0;
	for (toolY = 0; toolY < TOOLSIZE; toolY++)
	{
		for (toolX = 0; toolX < TOOLSIZE; toolX++)
		{
			imageX = x - ((TOOLSIZE - 1) / 2) + toolX;
			imageY = y - ((TOOLSIZE - 1) / 2) + toolY;

			if (imageX < 0) continue;
			if (imageX >= WIDTH) continue;
			if (imageY < 0) continue;
			if (imageY >= HEIGHT) continue;

			pTool = getPixel(tool, TOOLSIZE, toolX, toolY);
			pImage = getPixel(img, WIDTH, imageX, imageY);
			pFinal = (BDEPTH) minimum(pTool + depth, pImage);
			setPixel(img, WIDTH, imageX, imageY, pFinal);
		}
	}
	pixelInteractions += pow(TOOLSIZE, 2) * 3;
}

void cutFloat(BDEPTH *img, BDEPTH *tool, float x, float y, float depth)
{
	BDEPTH halfX = (BDEPTH) (WIDTH / 2);
	BDEPTH halfY = (BDEPTH) (HEIGHT / 2);
	BDEPTH imageX = halfX + (BDEPTH) (x * (float) WIDTH / 2);
	BDEPTH imageY = HEIGHT - (halfY + (BDEPTH) (y * (float) HEIGHT / 2));
	BDEPTH depthInt = 0xFFFF - (limit(depth) * 0xFFFF);
	cut(img, tool, imageX, imageY, depthInt);
}

void finish()
{
	printf("resampling...\n");
	resample(image, imageQtr);
	printf("saving...\n");
	save("out.tif", imageQtr, sizeof(imageQtr));
	system("out.tif");
	printf("done\n");
}

BOOL WINAPI ctrlCHandler(DWORD signal)
{
	printf("%d", signal);
    if (signal == CTRL_C_EVENT)
	{
        printf("\nCtrl-C handled\n");
		running = FALSE;
	}
    return TRUE;
}

BOOL notThisThread(int threadId, unsigned long cutCounter)
{
	// Perfectly divide the cutting
	// tasks over the number of threads (12).
	// cutCounter % THREADCOUNT = 0..11
	return cutCounter % THREADCOUNT != threadId;
}

void inputParameters()
{
	pSet.waves = inputFloat("waves", pSet.waves);
	pSet.spiral = inputFloat("spiral", pSet.spiral);
	pSet.depthA = inputFloat("depthA", pSet.depthA);
	pSet.depthB = inputFloat("depthB", pSet.depthB);
	pSet.wheel1SizeA = inputFloat("wheel1SizeA", pSet.wheel1SizeA);
	pSet.wheel1SizeB = inputFloat("wheel1SizeB", pSet.wheel1SizeB);
	pSet.wheelCenterOffset = inputFloat("wheelCenterOffset", pSet.wheelCenterOffset);
	pSet.wheelCount = inputFloat("wheelCount", pSet.wheelCount);
	pSet.teethDensityRelative = inputFloat("teethDensityRelative", pSet.teethDensityRelative);
	pSet.teethCountFixed = inputInt("teethCountFixed", pSet.teethCountFixed);
}

void concentricWobbleSpiral(int threadId)
{
	unsigned long cutCounter = 0;
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
		printf("wheel %d...", wheelNumber);
		wheel1Size = 1.0 / wheelCount * wheelNumber;
		wheel1Teeth = PI * WIDTH * wheel1Size * teethDensityRelative;
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
	unsigned long cutCounter = 0;
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
			: PI * WIDTH * wheel1Size * teethDensityRelative;
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
	unsigned long cutCounter = 0;
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
			: PI * WIDTH * wheel1Size * teethDensityRelative;
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
	float waves = pSet.waves;
	float spiral = pSet.spiral;
	float depthA = pSet.depthA;
	float depthB = pSet.depthB;
	float wheel1SizeA = pSet.wheel1SizeA;
	float wheel1SizeB = pSet.wheel1SizeB;
	float wheelCenterOffset = pSet.wheelCenterOffset;
	float wheelCount = pSet.wheelCount;
	float teethDensityRelative = pSet.teethDensityRelative;
	int teethCountFixed = pSet.teethCountFixed;

	unsigned long cutCounter = 0;
	float wheel1Rotation;
	float wheel1Size;
	int wheel1Teeth;
	float wheel1Tooth;
	int wheelNumber;
	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	{
		printf("%d: wheel %d...", threadId, wheelNumber);
		wheel1Size = wheel1SizeA*(wheelNumber/wheelCount)+wheel1SizeB;
		wheel1Teeth = teethCountFixed > 0
			? teethCountFixed
			: PI * WIDTH * wheel1Size * teethDensityRelative;
		wheel1Tooth = TWOPI / wheel1Teeth;
		float spiralTurn = 0-wheelNumber/wheelCount*spiral*TWOPI;
		float wheelCenterX = cos(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
		float wheelCenterY = sin(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
		{
			cutCounter++;
			if (notThisThread(threadId, cutCounter)) continue;
			float cutX = wheelCenterX+wheel1Size*cos(wheel1Rotation+spiralTurn);
			float cutY = wheelCenterY+wheel1Size*sin(wheel1Rotation+spiralTurn);
			float depthX = sqrt(pow(fabs(cutX),2)+pow(fabs(cutY),2));
			cutFloat(image, tool, cutX, cutY, depthA*depthX+depthB);
			if (!running) break;
		}
		printf(" ");
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

// 	unsigned long cutCounter = 0;
// 	float wheel1Rotation;
// 	float wheel1Size;
// 	int wheel1Teeth;
// 	float wheel1Tooth;
// 	int wheelNumber;

// 	BDEPTH halfX;
// 	BDEPTH halfY;
// 	BDEPTH imageX;
// 	BDEPTH imageY;
// 	halfX = (BDEPTH) (WIDTH / 2);
// 	halfY = (BDEPTH) (HEIGHT / 2);

// 	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
// 	{
// 		//printf("%d: wheel %d...", threadId, wheelNumber);
// 		wheel1Size = wheel1SizeA*(wheelNumber/wheelCount)+wheel1SizeB;
// 		wheel1Teeth = teethCountFixed > 0
// 			? teethCountFixed
// 			: PI * WIDTH * wheel1Size * teethDensityRelative;
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
// 			imageX = halfX + (BDEPTH) (cutX * (float) WIDTH / 2);
// 			imageY = HEIGHT - (halfY + (BDEPTH) (cutY * (float) HEIGHT / 2));
// 			BDEPTH p = (BDEPTH) (0xFFFF*(wheelNumber-1)/wheelCount);
// 			setPixel(image, WIDTH, imageX, imageY, p);
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
	threads[threadId] = TRUE;
	customParameterDrawing(threadId);
	// drawCone(threadId);

	threads[threadId] = FALSE;
	return 0;
}

int main()
{
	SetConsoleCtrlHandler(ctrlCHandler, TRUE);

	while (running)
	{
		inputParameters();

		wipe(image, WIDTH * HEIGHT);
		// load("cone255.tif", 0x4768, tool, sizeof(tool));
		// load("cone511d.tif", 0x48ac, tool, sizeof(tool));
		// load("cone511d-softer.tif", 0x48fa, tool, sizeof(tool));
		// load("cone255-maxed.tif", 0x48d2, tool, sizeof(tool));
		// load("smooth2550.tif", 0x477a0, tool, sizeof(tool));
		load("cone1023.tif", 0x48da, tool, sizeof(tool));


		clock_t start = clock();

		for (threadNumber = 0; threadNumber < THREADCOUNT; threadNumber++)
		{
			HANDLE thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
			if (thread) 
			{
				while (!threads[threadNumber])
				{
					Sleep(1);
				}
			}
		}

		BOOL threadsActive = TRUE;
		while (threadsActive)
		{
			Sleep(1);
			threadsActive = FALSE;
			for (threadNumber = 0; threadNumber < THREADCOUNT; threadNumber++)
			{
				threadsActive |= threads[threadNumber];
			}
			if (!running) break;
		}

		double cpuTimeUsed = (double)(clock() - start)/CLOCKS_PER_SEC;
		printf("Pixel interactions: %lu pixels in %f seconds.\n", pixelInteractions, cpuTimeUsed);

		finish();
	}
	return 0;
}

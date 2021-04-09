#include <stdio.h>
#include <math.h>
#include <windows.h>

#define WIDTH 8192
#define HEIGHT 8192
#define BDEPTH unsigned short
#define TOOLSIZE 255
#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559
#define THREADCOUNT 12

// Keep global, or else it won't fit on the stack.
// Alternatively, this:
// https://stackoverflow.com/questions/22945647/why-does-a-
//		large-local-array-crash-my-program-but-a-global-one-doesnt
char imageHeader[0x449A];
char imageFooter[0x2F];
long int imageFooterAddress = 0x800449A;
BDEPTH image[WIDTH * HEIGHT];
BDEPTH tool[TOOLSIZE * TOOLSIZE];

BOOL running = TRUE;
BOOL threads[THREADCOUNT];
int threadNumber;

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
	ptr = fopen("8kx8kx1x16b.tif", "rb");
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

BDEPTH getPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y)
{
	return img[w * y + x];
}

void setPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y, BDEPTH p)
{
	img[w * y + x] = p;
}

BDEPTH minimum(unsigned int a, unsigned int b)
{
	return a < b ? a : b;
}

float limit(float a)
{
	return a < 0 ? 0 : a > 1 ? 1 : a;
}

float square(float a)
{
	return a * a;
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
	printf("saving...\n");
	save("out.tif", image, sizeof(image));
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
		float spiralAdd = 0-wheelNumber/wheelCount*spiral*TWOPI;
		for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
		{
			cutCounter++;
			if (cutCounter % THREADCOUNT != threadId) continue;
			if (cutCounter % 1000 == 0) printf(".");
			cutFloat(image, tool,
					wheel1Size*cos(wheel1Rotation+spiralAdd),
					wheel1Size*sin(wheel1Rotation+spiralAdd),
					depthA*(0)+depthB);
			if (!running) break;
		}
		printf(" ");
		if (!running) break;
	}
}

DWORD WINAPI ThreadFunc(void *data)
{
	// https://stackoverflow.com/questions/1981459/using-threads
	//     -in-c-on-windows-simple-example
	// Do stuff. This will be the first function called
	// on the new thread. When this function returns,
	// the thread goes away. See MSDN for more details.

	int threadId = threadNumber;
	threads[threadId] = TRUE;
	//concentricWobbleSpiral(threadId);
	sunburst(threadId);

	threads[threadId] = FALSE;
	return 0;
}

int main()
{
	SetConsoleCtrlHandler(ctrlCHandler, TRUE);
	wipe(image, WIDTH * HEIGHT);
	load("cone255.tif", 0x4768, tool, sizeof(tool));

	for (threadNumber = 0; threadNumber < THREADCOUNT; threadNumber++)
	{
		HANDLE thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
		if (thread) 
		{
			while (!threads[threadNumber])
			{
				Sleep(0.1);
			}
		}
	}

	BOOL threadsActive = TRUE;
	while (threadsActive)
	{
		Sleep(1000);
		threadsActive = FALSE;
		for (threadNumber = 0; threadNumber < THREADCOUNT; threadNumber++)
		{
			threadsActive |= threads[threadNumber];
		}
		if (!running) break;
	}

	finish();
}

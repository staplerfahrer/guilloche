#include <stdio.h>
#include <math.h>
#include <windows.h> 

#define WIDTH 8192
#define HEIGHT 8192
#define BDEPTH unsigned short
#define TOOLSIZE 255
#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559

BOOL running = TRUE;

// Keep global, or else it won't fit on the stack.
// Alternatively, this:
// https://stackoverflow.com/questions/22945647/why-does-a-
//		large-local-array-crash-my-program-but-a-global-one-doesnt
char imageHeader[0x449A];
char imageFooter[0x2F];
long int imageFooterAddress = 0x800449A;
BDEPTH image[WIDTH * HEIGHT];
BDEPTH tool[TOOLSIZE * TOOLSIZE];

void blank(BDEPTH *img, BDEPTH w, BDEPTH h)
{
	unsigned long i;
	for (i = 0; i < w * h; i++)
	{
		img[i] = -1;
	}
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
	return a < 0 
			? 0
			: a > 1 
				? 1
				: a;
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

BOOL WINAPI consoleHandler(DWORD signal) 
{
	printf("%d", signal);
    if (signal == CTRL_C_EVENT)
	{
        printf("\nCtrl-C handled\n");
		running = FALSE;
	}
    return TRUE;
}

int main() 
{
    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) 
	{
        printf("\nERROR: Could not set control handler\n"); 
        return 1;
    }

	blank(image, WIDTH, HEIGHT);
	load("cone255.tif", 0x4768, tool, sizeof(tool));

	int cutCounter = 0;
	float wheel1;
	float wheel1Size;
	float oversampling;
	int wheel1Teeth;
	float wheel1Tooth;
	float wheelSizeMax = 0.9;
	float spin;
	int wheelNumber;

	// // g spiral.tif
	// for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	// {
	// 	printf("wheel %d...", wheelNumber);
	// 	oversampling = 0.1;
	// 	wheel1Size = 1.0 / wheelCount * wheelNumber;
	// 	wheel1Teeth = PI * WIDTH * wheel1Size * oversampling;
	// 	wheel1Tooth = TWOPI / wheel1Teeth;
	// 	spin = 0-wheelNumber/5.0;
	// 	for (wheel1 = 0; wheel1 < TWOPI; wheel1 += wheel1Tooth)
	// 	{
	// 		cutCounter++;
	// 		if (cutCounter % 1000 == 0) printf(".");
	// 		cutFloat(image, tool, 
	// 				wheelSizeMax*wheel1Size*cos(wheel1), 
	// 				wheelSizeMax*wheel1Size*sin(wheel1), 
	// 				((7.0/8)
	// 					+(1.0/8)*cos((wheel1+spin)*waves))
	// 					*wheelNumber/wheelCount);
	// 		if (!running) break;
	// 	}
	// 	printf(" ");
	// 	if (!running) break;
	// }

	float wheelCount = 32;
	float waves = 24;

	for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	{
		printf("wheel %d...", wheelNumber);
		oversampling = 0.1;
		wheel1Size = 1.0 / wheelCount * wheelNumber;
		wheel1Teeth = PI * WIDTH * wheel1Size * oversampling;
		wheel1Tooth = TWOPI / wheel1Teeth;
		spin = 0-wheelNumber/5.0;
		for (wheel1 = 0; wheel1 < TWOPI; wheel1 += wheel1Tooth)
		{
			cutCounter++;
			if (cutCounter % 1000 == 0) printf(".");
			cutFloat(image, tool, 
					wheelSizeMax*wheel1Size*cos(wheel1), 
					wheelSizeMax*wheel1Size*sin(wheel1), 
					((7.0/8)
						+(1.0/8)*cos((wheel1+spin)*waves))
						*wheelNumber/wheelCount);
			if (!running) break;
		}
		printf(" ");
		if (!running) break;
	}

	finish();
}

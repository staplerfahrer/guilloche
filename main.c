#include <stdio.h>
#include <math.h>

#define WIDTH 8192
#define HEIGHT 8192
#define BDEPTH unsigned short
#define TOOLSIZE 255
#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559

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

BDEPTH min(unsigned int a, unsigned int b)
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
			pFinal = (BDEPTH) min(pTool + depth, pImage);
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

int main() 
{
	blank(image, WIDTH, HEIGHT);
	load("cone255.tif", 0x4768, tool, sizeof(tool));

	double wheel1;
	double wheel1Size;
	double oversampling;
	int wheel1Teeth;
	double wheel1Tooth;

	int wheelCount;
	for (wheelCount = 1; wheelCount <= 32; wheelCount++)
	{
		printf("wheel %d... ", wheelCount);
		wheel1Size = 1.0 / 32 * wheelCount;
		oversampling = 0.1;
		wheel1Teeth = PI * WIDTH * wheel1Size * oversampling;
		wheel1Tooth = TWOPI / wheel1Teeth;
		for (wheel1 = 0; wheel1 < TWOPI; wheel1 += wheel1Tooth)
		{
			cutFloat(image, tool, 
					wheel1Size*cos(wheel1), 
					wheel1Size*sin(wheel1), 
					2.0/8+cos(wheel1*24)/8);
		}
	}

	save("out.tif", image, sizeof(image));
	printf("done\n");
}

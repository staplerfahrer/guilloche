#include <stdio.h>
#include <math.h>

#define WIDTH 1024
#define HEIGHT 1024
#define CHANNELS 3
#define BDEPTH unsigned short
#define TOOLSIZE 31
#define PI 3.141592653589793

struct Pixel {
	BDEPTH r;
	BDEPTH g;
	BDEPTH b;
};

// Keep global, or else it won't fit on the stack.
// Alternatively, this:
// https://stackoverflow.com/questions/22945647/why-does-a-
//		large-local-array-crash-my-program-but-a-global-one-doesnt
char header[0x5310];
char footer[0x2D];
long int footerAddress = 0x605310;
BDEPTH image[WIDTH * HEIGHT * CHANNELS];
BDEPTH tool[TOOLSIZE * TOOLSIZE * CHANNELS];

void load(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen(name, "rb");  // r for read, b for binary
	fread(img, size, 1, ptr);
	fclose(ptr);
}

void save(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen("1024x1024x16b.tif", "rb");  // r for read, b for binary
	fread(header, sizeof(header), 1, ptr);
	fseek(ptr, footerAddress, SEEK_SET);
	fread(footer, sizeof(footer), 1, ptr);
	fclose(ptr);

	ptr = fopen(name, "wb");  // w for write, b for binary
	fwrite(header, sizeof(header), 1, ptr);
	fwrite(img, size, 1, ptr);
	fwrite(footer, sizeof(footer), 1, ptr);
	fclose(ptr);
}

void getPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	p->r = img[(CHANNELS * w * y) + (CHANNELS * x)];
	p->g = img[(CHANNELS * w * y) + (CHANNELS * x) + 1];
	p->b = img[(CHANNELS * w * y) + (CHANNELS * x) + 2];
}

void setPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	img[(CHANNELS * w * y) + (CHANNELS * x)] = p->r;
	img[(CHANNELS * w * y) + (CHANNELS * x) + 1] = p->g;
	img[(CHANNELS * w * y) + (CHANNELS * x) + 2] = p->b;
}

BDEPTH min(unsigned int a, unsigned int b)
{
	return a < b ? a : b;
}

void cut(BDEPTH *img, BDEPTH *tool, BDEPTH x, BDEPTH y, BDEPTH depth)
{
	BDEPTH toolX, toolY; 
	int imageX, imageY;
	struct Pixel pImage = {.r = 0, .g = 0, .b = 0};
	struct Pixel pTool = {.r = 0, .g = 0, .b = 0};
	struct Pixel pFinal = {.r = 0, .g = 0, .b = 0};
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

			getPixel(tool, TOOLSIZE, toolX, toolY, &pTool);
			getPixel(img, WIDTH, imageX, imageY, &pImage);
			pFinal.r = (BDEPTH) min(pTool.r + depth, pImage.r);
			pFinal.g = (BDEPTH) min(pTool.g + depth, pImage.g);
			pFinal.b = (BDEPTH) min(pTool.b + depth, pImage.b);
			setPixel(img, WIDTH, imageX, imageY, &pFinal);
		}
	}
}

void cutFloat(BDEPTH *img, BDEPTH *tool, float x, float y, BDEPTH depth)
{
	BDEPTH halfX = (BDEPTH) (WIDTH / 2); // 512
	BDEPTH halfY = (BDEPTH) (HEIGHT / 2);
	BDEPTH imageX = halfX + (BDEPTH) (x * (float) WIDTH / 2); // 512 + (1 * 1024 / 2) or 512 + (-1 * 1024 / 2)
	BDEPTH imageY = HEIGHT - (halfY + (BDEPTH) (y * (float) HEIGHT / 2));
	cut(img, tool, imageX, imageY, depth);
}

int main() 
{
	size_t imgSize = sizeof(image);
	size_t brsSize = sizeof(tool);
	load("1024x1024x16b.raw", image, imgSize);
	load("hardGradient.raw", tool, brsSize);

	int i;
	int steps = 10000;
	double angle;
	double twoPi = PI * 2;
	double step = twoPi / steps;
	float loopX, loopY;
	BDEPTH depth = 32757;
	for (i = 0; i < steps; i++)
	{
		angle = twoPi / steps * i;
		loopX = (float) cos(angle);
		loopY = (float) sin(angle);
		cutFloat(image, tool, loopX, loopY, depth);
	}

	save("out.tif", image, imgSize);
	printf("done\n");
}

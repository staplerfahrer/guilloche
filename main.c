#include <stdio.h>
#include <math.h>

#define WIDTH 1024
#define HEIGHT 1024
#define CHANNELS 3
#define BDEPTH unsigned short
#define TOOLSIZE 31
#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559

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
	ptr = fopen(name, "rb");
	fread(img, size, 1, ptr);
	fclose(ptr);
}

void save(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen("1024x1024x16b.tif", "rb");
	fread(header, sizeof(header), 1, ptr);
	fseek(ptr, footerAddress, SEEK_SET);
	fread(footer, sizeof(footer), 1, ptr);
	fclose(ptr);

	ptr = fopen(name, "wb");
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
	size_t imgSize = sizeof(image);
	size_t brsSize = sizeof(tool);
	load("1024x1024x16b.raw", image, imgSize);
	load("cone.raw", tool, brsSize);

	double wheel1;
	double wheel1Size;
	double oversampling;
	int wheel1Teeth;
	double wheel1Tooth;

	// wheel1Size = 0.9;
	// oversampling = 1;
	// wheel1Teeth = PI * WIDTH * wheel1Size * oversampling;
	// wheel1Tooth = TWOPI / wheel1Teeth;
	// for (wheel1 = 0; wheel1 < TWOPI; wheel1 += wheel1Tooth)
	// {
	// 	cutFloat(image, tool, 
	// 			wheel1Size*cos(wheel1), 
	// 			wheel1Size*sin(wheel1), 
	// 			7.0/8.0+cos(wheel1*24)/8.0);
	// }

	int wheelCount;
	for (wheelCount = 1; wheelCount <= 32; wheelCount++)
	{
		wheel1Size = 1.0 / 32 * wheelCount;
		oversampling = 1;
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

	save("out.tif", image, imgSize);
	printf("done\n");
}

#include <stdio.h>
#include <math.h>

#define WIDTH 1024
#define HEIGHT 1024
#define CHANNELS 3
#define BDEPTH unsigned short
#define BRUSHSIZE 31
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
BDEPTH image[WIDTH * HEIGHT * CHANNELS];
BDEPTH brush[BRUSHSIZE * BRUSHSIZE * CHANNELS];

void getImage(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen(name, "rb");  // r for read, b for binary
	fread(img, size, 1, ptr);
	fclose(ptr);
}

void setImage(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen(name, "wb");  // r for read, b for binary
	fwrite(img, size, 1, ptr);
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

BDEPTH min(BDEPTH a, BDEPTH b)
{
	return a < b ? a : b;
}

void paint(BDEPTH *img, BDEPTH *brush, BDEPTH x, BDEPTH y)
{
	BDEPTH brushX, brushY; 
	int imageX, imageY;
	struct Pixel pImage = {.r = 0, .g = 0, .b = 0};
	struct Pixel pBrush = {.r = 0, .g = 0, .b = 0};
	struct Pixel pFinal = {.r = 0, .g = 0, .b = 0};
	for (brushY = 0; brushY < BRUSHSIZE; brushY++)
	{
		for (brushX = 0; brushX < BRUSHSIZE; brushX++)
		{
			imageX = x - ((BRUSHSIZE - 1) / 2) + brushX;
			imageY = y - ((BRUSHSIZE - 1) / 2) + brushY;

			if (imageX < 0) continue;
			if (imageX >= WIDTH) continue;
			if (imageY < 0) continue;
			if (imageY >= HEIGHT) continue;

			getPixel(brush, BRUSHSIZE, brushX, brushY, &pBrush);
			getPixel(img, WIDTH, imageX, imageY, &pImage);
			pFinal.r = min(pBrush.r, pImage.r);
			pFinal.g = min(pBrush.g, pImage.g);
			pFinal.b = min(pBrush.b, pImage.b);
			setPixel(img, WIDTH, imageX, imageY, &pFinal);
		}
	}
}

void paintFloat(BDEPTH *img, BDEPTH *brush, float x, float y)
{
	BDEPTH halfX = (BDEPTH) (WIDTH / 2); // 512
	BDEPTH halfY = (BDEPTH) (HEIGHT / 2);
	BDEPTH imageX = halfX + (BDEPTH) (x * (float) WIDTH / 2); // 512 + (1 * 1024 / 2) or 512 + (-1 * 1024 / 2)
	BDEPTH imageY = HEIGHT - (halfY + (BDEPTH) (y * (float) HEIGHT / 2));
	paint(img, brush, imageX, imageY);
}

int main() 
{
	size_t imgSize = sizeof(image);
	size_t brsSize = sizeof(brush);
	getImage("1024x1024x16b copy.raw", image, imgSize);
	getImage("hardGradient.raw", brush, brsSize);

	int i;
	int steps = 1000;
	double angle;
	double twoPi = PI * 2;
	double step = twoPi / steps;
	float loopX, loopY;
	for (i = 0; i < steps; i++)
	{
		angle = twoPi / steps * i;
		loopX = (float) cos(angle);
		loopY = (float) sin(angle);
		paintFloat(image, brush, loopX, loopY);
	}

	setImage("out.raw", image, imgSize);
	printf("done\n");
}

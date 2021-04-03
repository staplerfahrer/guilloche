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
// https://stackoverflow.com/questions/22945647/why-does-a-large-local
// -array-crash-my-program-but-a-global-one-doesnt
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

void paint(BDEPTH *img, BDEPTH *brush, BDEPTH x, BDEPTH y)
{
	BDEPTH brushX, brushY; 
	int imageX, imageY;
	struct Pixel *p;
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

			getPixel(brush, BRUSHSIZE, brushX, brushY, p);
			setPixel(img, WIDTH, imageX, imageY, p);
		}
	}
}

void paintFloat(BDEPTH *img, BDEPTH *brush, float x, float y)
{
	BDEPTH halfX = (BDEPTH) (WIDTH / 2); // 512
	BDEPTH halfY = (BDEPTH) (HEIGHT / 2);
	BDEPTH pixelX = halfX + (BDEPTH) (x * (float) WIDTH / 2); // 512 + (1 * 1024 / 2) or 512 + (-1 * 1024 / 2)
	BDEPTH pixelY = HEIGHT - (halfY + (BDEPTH) (y * (float) HEIGHT / 2));
	paint(img, brush, pixelX, pixelY);
}

int main() {
	size_t imgSize = sizeof(image);
	size_t brsSize = sizeof(brush);
	getImage("1024x1024x16b copy.raw", image, imgSize);
	getImage("brush.raw", brush, brsSize);

	// int i;
	// for (i = 0; i < 100; i++)
	// {
	// 	paint(image, brush, i, 0);
	// }

	// float angle;
	// int steps = 10;
	// for (angle = 0; angle <= (PI * 2); angle += (PI / steps))
	// {
	// 	printf("angle %f\n", angle);
	// 	float x = (float) cos(angle);
	// 	float y = (float) sin(angle);
	// 	printf("cos: %5.5f\n", x);
	// 	printf("sin: %5.5f\n", y);
	// 	paintFloat(image, brush, x, y);
	// }

	paintFloat(image, brush, 1.0, 1.0);

	setImage("out.raw", image, imgSize);
	printf("done\n");
}

#include <stdio.h>

#define WIDTH 1024
#define HEIGHT 1024
#define CHANNELS 3
#define BDEPTH unsigned short
#define BRUSHSIZE 31

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
	fread(img, size, 1, ptr); // read all bytes to our image
	fclose(ptr);
	printf("read\n");
}

void setImage(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen(name, "wb");  // r for read, b for binary
	fwrite(img, size, 1, ptr); // write all bytes to the file
	fclose(ptr);
	printf("written\n");
}

void getPixel(BDEPTH *img, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	p->r = img[(CHANNELS * WIDTH * y) + (CHANNELS * x)];
	p->g = img[(CHANNELS * WIDTH * y) + (CHANNELS * x) + 1];
	p->b = img[(CHANNELS * WIDTH * y) + (CHANNELS * x) + 2];
	printf("RGB at %d, %d read: %d %d %d\n", x, y, p->r, p->g, p->b);
}

void setPixel(BDEPTH *img, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	img[(CHANNELS * WIDTH * y) + (CHANNELS * x)] = p->r;
	img[(CHANNELS * WIDTH * y) + (CHANNELS * x) + 1] = p->g;
	img[(CHANNELS * WIDTH * y) + (CHANNELS * x) + 2] = p->b;
	printf("RGB at %d, %d written: %d %d %d\n", x, y, p->r, p->g, p->b);
}

void paint(BDEPTH *img, BDEPTH *brush, BDEPTH x, BDEPTH y)
{
	int brushX, brushY, imageX, imageY;
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

			printf("brushX %d  brushY %d - ", brushX, brushY);
			printf("imageX %d  imageY %d\n", imageX, imageY);
			struct Pixel *p;
			getPixel(brush, brushX, brushY, p);
			setPixel(img, imageX, imageY, p);
		}
	}
}

int main() {
	size_t size = (size_t) sizeof(image);
	getImage("1024x1024x16b copy.raw", image, size);
	
	size = (size_t) sizeof(brush);
	getImage("brush.raw", brush, size);

	//printf("read %d bytes\n", sz_t);
	// for (i = 0; i < WIDTH; i++)
	// {
	// 	printf("element: %d\n", image[i]);
	// }
	struct Pixel *p;
	getPixel(image, 9, 0, p);

	getPixel(brush, BRUSHSIZE-1, 0, p);

	p->r = 0;
	p->g = 0;
	p->b = 0;
	setPixel(image, 0, 0, p);
	p->r = 0;
	p->g = 65535;
	p->b = 0;
	setPixel(image, 1, 1, p);

	//paint(image, brush, 15, 15);
	getPixel(brush, 1, 1, p);
	setPixel(image, 1, 1, p);

	size = (size_t) sizeof(image);
	setImage("out.raw", image, size);

	return 3;
}

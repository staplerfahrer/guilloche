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
BDEPTH brush[BRUSHSIZE * BRUSHSIZE];

void getImage(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen(name, "rb");  // r for read, b for binary
	fread(img, size, 1, ptr); // read all bytes to our image
	fclose(ptr);
}

void getPixel(BDEPTH *img, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	p->r = img[WIDTH * y + CHANNELS * x];
	p->g = img[WIDTH * y + CHANNELS * x + 1];
	p->b = img[WIDTH * y + CHANNELS * x + 2];
	printf("RGB at %d, %d read: %d %d %d\n", x, y, p->r, p->g, p->b);
}

void setPixel(BDEPTH *img, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	img[WIDTH * y + CHANNELS * x] = p->r;
	img[WIDTH * y + CHANNELS * x + 1] = p->g;
	img[WIDTH * y + CHANNELS * x + 2] = p->b;
	printf("RGB at %d, %d written: %d %d %d\n", x, y, p->r, p->g, p->b);
}

int main() {
	size_t size = (size_t) sizeof(image);
	getImage("1024x1024x16b copy.raw", image, size);
	printf("ok\n");
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

	return 3;
}

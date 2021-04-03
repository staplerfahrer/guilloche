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
	// int i;
	// for (i = 0; i < size; i++)
	// {
	// 	printf("%d ", img[i]);
	// }
	printf("%d ", img[32*3]);
	printf("%d ", img[32*3+1]);
	printf("%d ", img[32*3+2]);
}

void setImage(char *name, BDEPTH *img, size_t size)
{
	FILE *ptr;
	ptr = fopen(name, "wb");  // r for read, b for binary
	fwrite(img, size, 1, ptr); // write all bytes to the file
	fclose(ptr);
	printf("written\n");
}

void getPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	p->r = img[(CHANNELS * w * y) + (CHANNELS * x)];
	p->g = img[(CHANNELS * w * y) + (CHANNELS * x) + 1];
	p->b = img[(CHANNELS * w * y) + (CHANNELS * x) + 2];
	printf("RGB at %d, %d read: %d %d %d\n", x, y, p->r, p->g, p->b);
}

void setPixel(BDEPTH *img, BDEPTH w, BDEPTH x, BDEPTH y, struct Pixel *p)
{
	img[(CHANNELS * w * y) + (CHANNELS * x)] = p->r;
	img[(CHANNELS * w * y) + (CHANNELS * x) + 1] = p->g;
	img[(CHANNELS * w * y) + (CHANNELS * x) + 2] = p->b;
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
			getPixel(brush, BRUSHSIZE, brushX, brushY, p);
			setPixel(img, WIDTH, imageX, imageY, p);
		}
	}
}

int main() {
	getImage("1024x1024x16b copy.raw", image, sizeof(image));

	getImage("brush.raw", brush, sizeof(brush));

	paint(image, brush, 15, 15);
	struct Pixel *p;

	setImage("out.raw", image, sizeof(image));

	return 3;
}

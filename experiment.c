#pragma region Header and declarations
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <windows.h>

#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

#define USHORT unsigned short
#define ULONG unsigned long

USHORT imageSize = 1024;
USHORT image[16384 * 16384];      // 16k x 16k x 2 bytes per pixel = 512 MB

USHORT toolSize = 5040;
USHORT samplingTool[10240 * 10240];
USHORT toolSample = 30;
USHORT toolReach;

char   tifFormatFile[20];
char imageHeader[1048576];
char imageFooter[1048576];
USHORT resampleDivisor = 1;
USHORT imageHeaderSize = 0x449A;
USHORT imageFooterSize = 46;
ULONG imageFooterAddress = 0x20449A;
char status = '.';
#pragma endregion

#pragma region Loading and saving
void loadSamplingTool(char *name)
{
	FILE *ptr;
	ptr = fopen(name, "rb");
	fread(samplingTool, toolSize * toolSize * 2, 1, ptr); // W x H x 2 bytes per pixel
	fclose(ptr);
}

void save(char *name, USHORT *img)
{
	// 2 bytes per pixel
	ULONG outputBytes = imageSize * imageSize * 2 / resampleDivisor / resampleDivisor;

	FILE *ptr;
	ptr = fopen(tifFormatFile, "rb");
	fread(imageHeader, imageHeaderSize, 1, ptr);
	fseek(ptr, imageFooterAddress, SEEK_SET);
	fread(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);

	ptr = fopen(name, "wb");
	fwrite(imageHeader, imageHeaderSize, 1, ptr);
	fwrite(img, outputBytes, 1, ptr);
	fwrite(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);
}

void wipe(USHORT *img, ULONG pixelCount)
{
	for (ULONG i = 0; i < pixelCount; i++) img[i] = -1;
}

USHORT getPixel(USHORT *img, USHORT imgWidth, USHORT x, USHORT y)
{
	if (x >= imgWidth || y >= imgWidth) return 0xFFFF;
	return img[imgWidth * y + x];
}

void setPixel(USHORT x, USHORT y, ULONG brightness)
{
	// main image paint
	if (x >= imageSize || y >= imageSize) return;
	image[imageSize * y + x] = MIN(brightness, 0xFFFF);
}
#pragma endregion

USHORT sampleToolPixel(USHORT xCounter, USHORT yCounter, USHORT xSubpixel, USHORT ySubpixel)
{
	USHORT x = xCounter * toolSample - xSubpixel; 
	USHORT y = yCounter * toolSample - ySubpixel;
	if (x > toolSize || y > toolSize) return 0xFFFF; // white, if outside tool area
	return getPixel(samplingTool, toolSize, x, y);
}

void cut(float imageXAbsolute, float imageYAbsolute)
{
	/*
	imageXAbsolute 1.23
	imageYAbsolute 1.23
	-> imageXWhole 1
	-> imageYWhole 1
	imageXFraction 0.23
	imageYFraction 0.23
	toolReach = 5040 / 2 = 2520 / 10 = 252
	0 - 252 = -252
	0 + 252 = 252
	maxX = 251 because of <
	countX = 503 (*10=5030 so room to sample for 0.9 pixel more)
	*/


	USHORT imageXWhole    = imageXAbsolute;
	USHORT imageYWhole    = imageYAbsolute;
	USHORT xSubpixel      = round((imageXAbsolute - imageXWhole)*toolSample);
	USHORT ySubpixel      = round((imageYAbsolute - imageYWhole)*toolSample);
	USHORT toolReach      = toolSize / 2 / toolSample;
	int    x, y;
	int    minX           = MAX(imageXWhole - toolReach, 0);
	int    minY           = MAX(imageYWhole - toolReach, 0);
	int    maxX           = imageXWhole + toolReach; // exclusive because <
	int    maxY           = imageYWhole + toolReach; // exclusive because <
	USHORT xCounterStart  = minX - (imageXWhole - toolReach);
	USHORT yCounterStart  = minY - (imageYWhole - toolReach);
	USHORT xCounter, yCounter;
	yCounter = yCounterStart;
	for (y = minY; y < maxY; y++)
	{
		xCounter = xCounterStart;
		for (x = minX; x < maxX; x++)
		{
			setPixel(
				x, 
				y, 
				MIN(sampleToolPixel(xCounter, yCounter, xSubpixel, ySubpixel),
					getPixel(image, imageSize, x, y)));
			xCounter++;
		}
		yCounter++;
	}



	// AT X & Y CUT AN APPROPRIATELY SAMPLED PORTION OF THE TOOL
	// calculate area to draw
	// sample the right tool pixels
	// draw them
	// int imageXMin = xwhole - (half tool size / toolSampling)
	// same for y
	// same for max x
	// same for max y

	// count the pixels to draw, when sampling, do the math to space out the samples, based on fractional offset and tool sampling size

	// int sampletoolx = sampleToolPixel(sampleCounter, sampleFraction)
	// int sampletooly = sampleToolPixel(sampleCounter, sampleFraction)
	// toolPixel = getPixel(tool, toolx, tooly)
	// setPixel(imageXWhole, imageYWhole, toolPixel)

	// USHORT toolX, toolY, toolBrightness;
	
	// for (toolY = 0; toolY < toolSize; toolY += toolSample)
	// {
	// 	for (toolX = 0; toolX < toolSize; toolX += toolSample)
	// 	{
	// 		toolBrightness = getPixel(samplingTool, toolSize, toolX, toolY);
	// 		setPixel(wholeX, wholeY, toolBrightness);
	// 	}
	// }

}

int main(int argc, char *argv[])
{
	wipe(image, imageSize*imageSize);
	wipe(samplingTool, imageSize*imageSize);
	strcpy(tifFormatFile, "1kx1kx1x16b.tif");
	loadSamplingTool("cone_5040x5040_16b.raw");
	float x = 0;
	float y = 0;
	for (int copy = 0; copy < 800; copy += 1)
	{
		for (float circle = 0; circle < TWOPI; circle += PI/60)
		{
			x = cos(circle)*copy+512;
			y = sin(circle)*copy+512;
			cut(x, y);
		}
	}
	save("experiment.tif", image);
}
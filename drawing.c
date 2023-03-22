#include <stdio.h>
#include <math.h>

#include "drawing.h"
#include "types.h"
#include "threading.h"

void wipe(USHORT *img, ULONG elements)
{
	USHORT white = 0xFFFF;
	for (ULONG i = 0; i < elements; i++)
		img[i] = white;
}

void maximize()
{
	USHORT min = 0xFFFF;
	USHORT max = 0;
	ULONG pixelCount = imageSize * imageSize;
	for (ULONG i = 0; i < pixelCount; i++)
	{
		min = MIN(min, image[i]);
		max = MAX(max, image[i]);
	}
	double multiplier = 65535.0 / (max-min);
	printf("max: %i, min: %i, multiplier: %f\n", max, min, multiplier);
	double newVal;
	for (ULONG i = 0; i < pixelCount; i++)
	{
		newVal = image[i]-min;
		newVal *= multiplier;
		image[i] = (USHORT)newVal;
	}
}

USHORT getPixel(USHORT *img, USHORT imgWidth, USHORT x, USHORT y)
{
	if (x >= imgWidth || y >= imgWidth)
		return 0xFFFF;
	return img[imgWidth * y + x];
}

void setPixel(USHORT x, USHORT y, ULONG brightness)
{
	// main image paint
	if (x >= imageSize || y >= imageSize)
		return;
	image[imageSize * y + x] = MIN(brightness, 0xFFFF);
}
#pragma endregion

USHORT sampleToolPixel(USHORT xCounter, USHORT yCounter, USHORT xSubpixel, USHORT ySubpixel)
{
	// this +1 shifts the center of the tool up and left by 1 pixel, but AA works
	USHORT x = (xCounter+1) * toolSample - xSubpixel;
	USHORT y = (yCounter+1) * toolSample - ySubpixel;
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

	int imageXWhole = imageXAbsolute;
	int imageYWhole = imageYAbsolute;
	int xSubpixel = round((imageXAbsolute - imageXWhole) * toolSample);
	int ySubpixel = round((imageYAbsolute - imageYWhole) * toolSample);
	int toolReach = toolSize / 2 / toolSample;
	int x, y;
	int minX = imageXWhole - toolReach;
	int minY = imageYWhole - toolReach;
	int maxX = imageXWhole + toolReach; // exclusive because <
	int maxY = imageYWhole + toolReach; // exclusive because <
	if (maxX < 0 || minX > imageSize || maxY < 0 || minY > imageSize)
		return;

	int xCounterStart = minX - (imageXWhole - toolReach);
	int yCounterStart = minY - (imageYWhole - toolReach);
	int xCounter, yCounter;
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
}

// #pragma region special purpose drawing
// void drawConeTool()
// {
// 	// a very divisible number, center is towards the right and bottom
// 	// at 2520,2520
// 	// 2 * 3 * 4 * 5 * 6 * 7 = 5040
// 	imageSize = 5040;
// 	USHORT half = imageSize/2;
// 	ULONG elements = imageSize*imageSize;
// 	ULONG bytes = elements * 2;

// 	wipe(image, elements);

// 	USHORT x, y;
// 	ULONG distanceX, distanceY, intBrightness;
// 	float distance, brightness;
// 	for (y = 0; y < imageSize; y++)
// 	{
// 		distanceY = abs(y - half);
// 		for (x = 0; x < imageSize; x++)
// 		{
// 			distanceX = abs(x - half);
// 			distance = sqrt(distanceX * distanceX + distanceY * distanceY);
// 			brightness = 0xFFFF * (distance / (half));// 1 white column left, 1 white row on top, perfect edge right and bottom
// 			intBrightness = (ULONG) round(brightness);
// 			setPixel(x, y, intBrightness);
// 		}
// 	}

// 	// save raw bytes, little-endian straight from RAM
// 	FILE *ptr;
// 	char filename[50];
// 	sprintf(filename, "cone_%ix%i_16b.raw", imageSize, imageSize);
// 	ptr = fopen(filename, "wb");
// 	fwrite(image, bytes, 1, ptr);
// 	fclose(ptr);
// }
// #pragma endregion

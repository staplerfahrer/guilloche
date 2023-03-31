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

float distance(float x1, float y1, float x2, float y2)
{
	return sqrt(fabs(x2 - x1) * fabs(x2 - x1) + fabs(y2 - y1) * fabs(y2 - y1));
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

void cut(float toolRadius, float toolDepth, float imageXAbsolute, float imageYAbsolute)
{
	int x, y;
	
	// if (toolRadius < 1)
	// {
	// 	x = imageXAbsolute;
	// 	y = imageYAbsolute;
	// 	setPixel(
	// 		x,
	// 		y,
	// 		0x7FFF);
	// 		// MIN(
	// 		// 	rand() * (0xFFFF/RAND_MAX),
	// 		// 	getPixel(image, imageSize, x, y)));
	// 	return;
	// }

	toolRadius = MAX(toolRadius, 0.1);
	toolDepth  = MAX(MIN(toolDepth, 1), 0);

	int minX = imageXAbsolute - toolRadius;
	int minY = imageYAbsolute - toolRadius;
	int maxX = imageXAbsolute + toolRadius;
	int maxY = imageYAbsolute + toolRadius;
	if (maxX < 0 || minX > imageSize || maxY < 0 || minY > imageSize)
		return;

	for (y = minY; y <= maxY; y++)
	{
		for (x = minX; x <= maxX; x++)
		{
			setPixel(
				x,
				y,
				MIN(
					distance(imageXAbsolute, imageYAbsolute, x, y) * 65535.0 / toolRadius + (65535.0 * (1-toolDepth)), 
					getPixel(image, imageSize, x, y)));
		}
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

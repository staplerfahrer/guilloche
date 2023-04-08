#include <math.h>
#include <stdio.h>

#include "threading.h"
#include "patterns.h"
#include "drawing.h"
#include "types.h"

extern ParameterSet pSet;

/*
ax^2 + bx + c
*/
float quadratic(float x, float a, float b, float c)
{
	return a*x*x+b*x+c;
}

void rays(int threadId)
{
	ULONG cutCounter = 0;
	float x = 0;
	float y = 0;
	float copies = 1.42 * 4096;
	float cutsPerCopy = 720;
	float toolRadius = 0;
	for (float copy = 0; copy < copies; copy += 1)
	{
		for (float circle = 0; circle < TWOPI; circle += TWOPI / cutsPerCopy)
		{
			if (cutCounter % 10000 == 0)
				printf("%.3f %% reported by %i\n", 100 * copy / copies, threadId);
			if (notMyJob(++cutCounter, threadId))
				continue;
			x = cos(circle) * copy + 4096;
			y = sin(circle) * copy + 4096;
			toolRadius = distance(x, y, 4096, 4096) * 4 / cutsPerCopy;
			cut(toolRadius, 1, x, y);
		}
	}
	printf("Total cuts: %i\n", cutCounter);
}

void circles(int threadId)
{
	ULONG cutCounter = 0;
	float x = 0;
	float y = 0;
	float copies = 1.42 * 4096;
	float cutsPerCopy = 4096 * TWOPI * 2;
	float toolRadius = 0;
	for (float copy = 0; copy < copies; copy += 20)
	{
		toolRadius = 10;
		for (float circle = 0; circle < TWOPI; circle += TWOPI / cutsPerCopy)
		{
			if (cutCounter % 10000 == 0)
				printf("%.3f %% reported by %i\n", 100 * copy / copies, threadId);
			if (notMyJob(++cutCounter, threadId))
				continue;
			x = cos(circle) * copy + 4096;
			y = sin(circle) * copy + 4096;
			cut(toolRadius, 1, x, y);
		}
	}
	printf("Total cuts: %i\n", cutCounter);
}

void wavyCircles(int threadId)
{
	ULONG cutCounter     = 0;
	float x              = 0;
	float y              = 0;
	float imageCenter    = imageSize / 2;
	float maxRadius      = 1.42 * imageCenter;
	float cutsPerCopy    = imageCenter * TWOPI * 2;
	float toolRadius     = 0;
	float toolDepth      = 0;
	float wavePhase      = 0;
	float wavesPerCircle = 48;
	for (float radius = 0; radius < maxRadius; radius += 40)
	{
		toolDepth = quadratic(radius / maxRadius, 1, 0, 0.1);
		printf("Tool depth: %f", toolDepth);

		cutsPerCopy = radius * TWOPI * 2;
		for (float circle = 0; circle < TWOPI; circle += TWOPI / cutsPerCopy)
		{
			if (cutCounter % 10000 == 0)
				printf("%.3f %% reported by %i\n", 100 * radius / maxRadius, threadId);
			if (notMyJob(++cutCounter, threadId))
				continue;
			x = cos(circle) * radius + imageCenter;
			y = sin(circle) * radius + imageCenter;
			toolRadius = (cos(circle * wavesPerCircle + wavePhase) + 2) * 20;
			cut(toolRadius, toolDepth, x, y);
		}
		wavePhase -= 1;
	}
	printf("Total cuts: %i\n", cutCounter);
}

void slinky(int threadId)
{
	ULONG  cutCounter     = 0;
	double halfImage      = imageSize / 2;
	double maxRadius      = halfImage * 1.42;

	double wheel1SizeA    = pSet.wheel1SizeA * halfImage;
	double wheel1SizeB    = pSet.wheel1SizeB * halfImage;
	double toolWidth      = pSet.toolWidth * halfImage;
	ULONG  onePercent     = pSet.teethCountFixed;

	double x, y, toolDepth, radius;
	for (double wheelA = 0; wheelA < TWOPI; wheelA += TWOPI / pSet.teethCountFixed)
	{
		if (cutCounter * 100 % onePercent == 0)
			printf("Thread %i %i %% done\n", threadId, (int)(wheelA * 100 / TWOPI));
		if (notMyJob(++cutCounter, threadId))
			continue;
		x = halfImage + cos(wheelA) * wheel1SizeA + cos(wheelA*pSet.wheelCount) * wheel1SizeB;
		y = halfImage + sin(wheelA) * wheel1SizeA + sin(wheelA*pSet.wheelCount) * wheel1SizeB;
		radius = distance(halfImage, halfImage, x, y);
		toolDepth = quadratic(radius / maxRadius, pSet.depthA, 0, pSet.depthB);
		cut(toolWidth, toolDepth, x, y);
	}
}

void drawCone(int threadId)
{
	// float waves = 0;
	// float spiral = 0;
	// float depthA = 0;
	// float depthB = 0;
	// float wheel1SizeA = 1;
	// float wheel1SizeB = 0;
	// float wheelCenterOffset = 0;
	// float wheelCount = 16384;
	// float teethDensityRelative = 16;
	// int teethCountFixed = 0;

	// ULONG cutCounter = 0;
	// float wheel1Rotation;
	// float wheel1Size;
	// int wheel1Teeth;
	// float wheel1Tooth;
	// int wheelNumber;

	// USHORT halfX;
	// USHORT halfY;
	// USHORT imageX;
	// USHORT imageY;
	// halfX = (USHORT) (imageSize / 2);
	// halfY = (USHORT) (imageSize / 2);

	// for (wheelNumber = 1; wheelNumber <= wheelCount; wheelNumber++)
	// {
	// 	//printf("%d: wheel %d...", threadId, wheelNumber);
	// 	wheel1Size = wheel1SizeA*(wheelNumber/wheelCount)+wheel1SizeB;
	// 	wheel1Teeth = teethCountFixed > 0
	// 		? teethCountFixed
	// 		: PI * imageSize * wheel1Size * teethDensityRelative;
	// 	wheel1Tooth = TWOPI / wheel1Teeth;
	// 	float spiralTurn = 0-wheelNumber/wheelCount*spiral*TWOPI;
	// 	float wheelCenterX = cos(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
	// 	float wheelCenterY = sin(wheelNumber/wheelCount*TWOPI)*wheelCenterOffset;
	// 	for (wheel1Rotation = 0; wheel1Rotation < TWOPI; wheel1Rotation += wheel1Tooth)
	// 	{
	// 		cutCounter++;
	// 		if (notThisThread(threadId, cutCounter)) continue;
	// 		float cutX = wheelCenterX+wheel1Size*cos(wheel1Rotation+spiralTurn);
	// 		float cutY = wheelCenterY+wheel1Size*sin(wheel1Rotation+spiralTurn);
	// 		imageX = halfX + (USHORT) (cutX * (float) imageSize / 2);
	// 		imageY = imageSize - (halfY + (USHORT) (cutY * (float) imageSize / 2));
	// 		USHORT p = (USHORT) (0xFFFF*(wheelNumber-1)/wheelCount);
	// 		setPixel(image, imageSize, imageX, imageY, p);
	// 		if (!running) break;
	// 	}
	// 	//printf(" ");
	// 	//if (!running) break;
	// }
}
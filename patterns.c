#include <math.h>
#include <stdio.h>

#include "threading.h"
#include "patterns.h"
#include "drawing.h"

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
		toolDepth = quadratic(radius / maxRadius, 1, 0, 0.3); //ax2+bx+c
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
			toolRadius = (
				cos(circle * wavesPerCircle + wavePhase) + 2)
				* 20;
			cut(toolRadius, toolDepth, x, y);
		}
		wavePhase -= 1;
	}
	printf("Total cuts: %i\n", cutCounter);
}

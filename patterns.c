#include <math.h>
#include <stdio.h>

#include "threading.h"
#include "patterns.h"
#include "drawing.h"

void testDrawing(int threadId)
{
	ULONG cutCounter = 0;
	float x = 0;
	float y = 0;
	float copies = 1.42 * 2048;
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
			x = cos(circle) * copy + 2048;
			y = sin(circle) * copy + 2048;
			toolRadius = MAX(distance(x, y, 2048, 2048) * 4 / cutsPerCopy, 1);
			cut(toolRadius, x, y);
		}
	}
	printf("Total cuts: %i\n", cutCounter);
}

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
	float toolRadius = 126; // set it here because it depends on the intended drawing
	for (float copy = 0; copy < copies; copy += 0.5)
	{
		for (float circle = 0; circle < TWOPI; circle += PI / 360)
		{
			if (notMyJob(++cutCounter, threadId))
				continue;
			if (cutCounter % 10000 == 0)
				printf("%.3f %%\n", 100 * copy / copies);
			x = cos(circle) * copy + 2048;
			y = sin(circle) * copy + 2048;
			cut(10, x, y);
		}
	}
	printf("Total cuts: %i\n", cutCounter);
}

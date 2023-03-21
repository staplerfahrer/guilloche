#include <math.h>
#include <stdio.h>

#include "threading.h"
#include "patterns.h"
#include "drawing.h"

void testDrawing(int threadId)
{
	printf("testDrawing %i\n", threadId);
	// cut(2048.0, 2048.0);
	ULONG cutCounter = 0;
	float x = 0;
	float y = 0;
	float copies = 1.42 * 4096;
	for (float copy = 0; copy < copies; copy += 10)
	{
		for (float circle = 0; circle < TWOPI; circle += PI / 16384)
		{
			cutCounter++;
			if (notMyJob(cutCounter, threadId))
				continue;
			x = cos(circle) * copy + 2048;
			y = sin(circle) * copy;
			cut(x, y);
		}
		printf("%f %%\n", 100 * copy / copies);
	}
}

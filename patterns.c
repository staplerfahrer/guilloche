#include <math.h>
#include <stdio.h>

#include "threading.h"
#include "patterns.h"
#include "drawing.h"

void testDrawing(int threadId)
{
	printf("testDrawing %i\n", threadId);
	// cut(10, 10);
	// cut(20, 10.1);
	// cut(30, 10.2);
	// cut(40, 10.3);
	// cut(50, 10.4);
	// cut(60, 10.5);
	// cut(70, 10.6);
	// cut(80, 10.7);
	// cut(90, 10.8);
	// cut(100, 10.9);
	// cut(110, 11);

	ULONG cutCounter = 0;
	float x = 0;
	float y = 0;
	float copies = 1.42 * 4096;
	for (float copy = 0; copy < copies; copy += 10)
	{
		for (float circle = 0; circle < TWOPI; circle += PI / 32768)
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

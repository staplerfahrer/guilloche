#include <stdio.h>

#include "drawing.h"
#include "types.h"

void wipe(USHORT *img, ULONG elements)
{
	for (ULONG i = 0; i < elements; i++)
		img[i] = -1;
}

// void loadSamplingTool(char *name)
// {
// 	FILE *ptr;
// 	ptr = fopen(name, "rb");
// 	fread(samplingTool, toolSize * toolSize * 2, 1, ptr); // W x H x 2 bytes per pixel
// 	fclose(ptr);
// }

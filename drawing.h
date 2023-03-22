#ifndef DRAWING
#define DRAWING

#include "types.h"

USHORT imageSize;
USHORT toolSize;
USHORT samplingTool[10240 * 10240];
USHORT toolSample; // must create an even number of tool divisions, to sample the middle

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

void wipe(USHORT *img, ULONG elements);
void loadSamplingTool(char *name);
USHORT getPixel(USHORT *img, USHORT imgWidth, USHORT x, USHORT y);
void setPixel(USHORT x, USHORT y, ULONG brightness);
USHORT sampleToolPixel(USHORT xCounter, USHORT yCounter, USHORT xSubpixel, USHORT ySubpixel);
void cut(float imageXAbsolute, float imageYAbsolute);
void maximize();

#endif
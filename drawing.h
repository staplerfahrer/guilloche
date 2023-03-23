#ifndef DRAWING
#define DRAWING

#include "types.h"

USHORT imageSize;
USHORT toolSize;
USHORT samplingTool[10240 * 10240];

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

void wipe(USHORT *img, ULONG elements);
void loadSamplingTool(char *name);
USHORT getPixel(USHORT *img, USHORT imgWidth, USHORT x, USHORT y);
void setPixel(USHORT x, USHORT y, ULONG brightness);
USHORT sampleToolPixel(USHORT toolSample, USHORT xCounter, USHORT yCounter, USHORT xSubpixel, USHORT ySubpixel);
void cut(USHORT toolDivisor, float imageXAbsolute, float imageYAbsolute);
void maximize();

#endif
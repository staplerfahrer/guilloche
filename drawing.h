#ifndef DRAWING
#define DRAWING

#include "types.h"

USHORT imageSize;
USHORT toolSize; //  = 5040
USHORT samplingTool[10240 * 10240];
USHORT toolSample; // = 30;

#define PI    3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

void wipe(USHORT *img, ULONG elements);
void loadSamplingTool(char *name);
USHORT getPixel(USHORT *img, USHORT imgWidth, USHORT x, USHORT y);
void setPixel(USHORT x, USHORT y, ULONG brightness);
USHORT sampleToolPixel(USHORT xCounter, USHORT yCounter, USHORT xSubpixel, USHORT ySubpixel);
void cut(float imageXAbsolute, float imageYAbsolute);
void testDrawing(int threadId);

#endif
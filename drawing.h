#ifndef DRAWING
#define DRAWING

#include "types.h"

USHORT imageSize;

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

void wipe(USHORT *img, ULONG elements);
USHORT getPixel(USHORT *img, USHORT imgWidth, USHORT x, USHORT y);
void setPixel(USHORT x, USHORT y, ULONG brightness);
void cut(float toolRadius, float imageXAbsolute, float imageYAbsolute);
void maximize();
float distance(float x1, float y1, float x2, float y2);

#endif
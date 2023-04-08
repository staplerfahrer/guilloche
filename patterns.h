#ifndef PATTERNS
#define PATTERNS

#include "types.h"

#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559

char algorithm[256];

float quadratic(float x, float a, float b, float c);
void rays(int threadId);
void circles(int threadId);
void wavyCircles(int threadId);
void slinky(int threadId);
void drawCone(int threadId);

#endif
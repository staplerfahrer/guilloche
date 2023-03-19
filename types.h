#ifndef TYPES
#define TYPES

#include <windows.h>

// #define USHORT unsigned short
// #define ULONG unsigned long

typedef struct
{
	float waves;
	float spiral;
	float depthA;	   // ax
	float depthB;	   //    + b
	float wheel1SizeA; // ax
	float wheel1SizeB; //    + b
	float wheelCenterOffset;
	int wheelCount;
	float teethDensityRelative;
	int teethCountFixed;
} ParameterSet;

USHORT image[4096 * 4096]; // TODO smaller than 16x16k I used before could cause reading wrap-around or something // 4k x 4k x 2 bytes per pixel = 32 MB

#endif
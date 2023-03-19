#pragma region Header and declarations
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <windows.h>

#define PI 3.1415926535897932384626433832795
#define TWOPI 6.283185307179586476925286766559

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

#define USHORT unsigned short
#define ULONG unsigned long

USHORT imageSize = 1024;
USHORT image[16384 * 16384];      // 16k x 16k x 2 bytes per pixel = 512 MB

USHORT toolSize = 5040;
USHORT samplingTool[10240 * 10240];
USHORT toolSample = 30;
USHORT toolReach;

char tifFormatFile[20] = "1kx1kx1x16b.tif";
char imageHeader[1048576];
char imageFooter[1048576];
USHORT imageHeaderSize = 0x449A;
USHORT imageFooterSize = 46;
ULONG imageFooterAddress = 0x20449A;
#pragma endregion

#pragma region Loading and saving
void loadSamplingTool(char *name)
{
	FILE *ptr;
	ptr = fopen(name, "rb");
	fread(samplingTool, toolSize * toolSize * 2, 1, ptr); // W x H x 2 bytes per pixel
	fclose(ptr);
}

void save(char *name, USHORT *img)
{
	// 2 bytes per pixel
	ULONG outputBytes = imageSize * imageSize * 2;

	FILE *ptr;
	ptr = fopen(tifFormatFile, "rb");
	fread(imageHeader, imageHeaderSize, 1, ptr);
	fseek(ptr, imageFooterAddress, SEEK_SET);
	fread(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);

	ptr = fopen(name, "wb");
	fwrite(imageHeader, imageHeaderSize, 1, ptr);
	fwrite(img, outputBytes, 1, ptr);
	fwrite(imageFooter, imageFooterSize, 1, ptr);
	fclose(ptr);
}

void wipe(USHORT *img, ULONG pixelCount)
{
	for (ULONG i = 0; i < pixelCount; i++) img[i] = -1;
}



int main(int argc, char *argv[])
{

	maximize();
	save("experiment.tif", image);
}
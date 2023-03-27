#include <stdio.h>

#include "fileio.h"
#include "types.h"
#include "drawing.h"

void save(char *name, USHORT *img)
{
	// 2 bytes per pixel
	ULONG outputBytes = imageSize * imageSize * 2;

	printf("TIFF format %s\n", tifFormatFile);
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

void finish()
{
	USHORT *output = image;

	printf("saving...\n");
	save("C:\\Temp\\out.temp", output);

	remove("C:\\Temp\\out.tif");
	rename("C:\\Temp\\out.temp", "C:\\Temp\\out.tif");

	printf("Image saved to C:\\Temp\\out.tif\n");
	printf("Done.\n");
}
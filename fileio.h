#ifndef FILEIO
#define FILEIO

#include "types.h"

char imageHeader[1048576];
char imageFooter[1048576];
char tifFormatFile[256];
USHORT imageHeaderSize;
USHORT imageFooterSize;
ULONG imageFooterAddress;

void loadSamplingTool(char *name);
void save(char *name, USHORT *img);
void finish();

#endif
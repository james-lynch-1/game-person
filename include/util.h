#ifndef UTIL
#define UTIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL3/SDL.h"
#include "math.h"
#include "type.h"

#define ROMVAL getOpcodeAtPC(getPCAndInc());

void sortScanlineObjs(OamEntry* scanlineObjs, int numScanlineObjs);

// PixelFIFO functions

void initialiseFIFO(PixelFIFO* FIFO);

bool isEmptyFIFO(PixelFIFO* FIFO);

bool isFullFIFO(PixelFIFO* FIFO);

int getSizeFIFO(PixelFIFO* FIFO);

void enqueue(PixelFIFO* FIFO, Pixel p);

Pixel dequeue(PixelFIFO* FIFO);


char* decToBin(int num, char* buffer, int size);

char* decToHex(int num, char* buffer, int size);

void getTitle(char* buffer, FILE* romPtr);

void printRom(FILE* romPtr);

#endif

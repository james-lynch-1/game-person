#ifndef UTIL
#define UTIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "math.h"

// handy types
#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define s8  char
#define s16 short
#define s32 int

// helper macros
#define MMAP mMap.MMap_s // access memory map struct inside union more easily

// magic addresses
// ROM:
#define ROM_HEADER_ADDR 0x0100
#define ROM_TITLE_ADDR  0x0134

// Flags register
#define ZERO_FLAG       0b0001
#define SUBTR_FLAG      0b0010
#define HALFCARRY_FLAG  0b0100
#define CARRY_FLAG      0b1000

char* decToBin(int num, char* buffer, int size);

char* decToHex(int num, char* buffer, int size);

void getTitle(char* buffer, FILE* romPtr);

void printRom(FILE* romPtr);

#endif

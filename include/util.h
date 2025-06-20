#ifndef UTIL
#define UTIL

#include <stdio.h>
#include <stdlib.h>

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define s8  char
#define s16 short
#define s32 int

// Flags register
#define ZERO_FLAG       0b0001u
#define SUBTR_FLAG      0b0010u
#define HALFCARRY_FLAG  0b0100u
#define CARRY_FLAG      0b1000u

char* decToBin(int num, char* buffer, int size);

char* decToHex(int num, char* buffer, int size);

#endif

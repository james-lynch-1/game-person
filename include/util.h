#ifndef UTIL
#define UTIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "math.h"
#include "type.h"

char* decToBin(int num, char* buffer, int size);

char* decToHex(int num, char* buffer, int size);

void getTitle(char* buffer, FILE* romPtr);

void printRom(FILE* romPtr);

#endif

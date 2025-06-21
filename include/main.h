#ifndef MAIN
#define MAIN

#include "util.h"
#include "registers.h"
#include "ram.h"
#include "rom.h"

// globals
// values
u32 maxFPS;
int scale;
int WINDOW_WIDTH;
int WINDOW_HEIGHT;

// structs
extern MMap mMap;
extern Registers regs;

// renderer stuff
int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
static int gDone;

void initialiseValues();

#endif

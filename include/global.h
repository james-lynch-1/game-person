#ifndef GLOBAL
#define GLOBAL

#include "util.h"
#include "ram.h"
#include "registers.h"

// globals
// values
extern u32 maxFPS;
extern int scale;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

extern int cycles;

// structs
extern MMap mMap;
extern Registers registers;

// renderer stuff
extern int* gFrameBuffer;
extern SDL_Window* gSDLWindow;
extern SDL_Renderer* gSDLRenderer;
extern SDL_Texture* gSDLTexture;
static int gDone;

#endif

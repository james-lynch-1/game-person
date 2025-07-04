#ifndef GLOBAL
#define GLOBAL

#include "util.h"
#include "type.h"

// globals
// values
extern char title[30];
extern FILE* romPtr;
extern u32 maxFPS;
extern int scale;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

extern Cpu cpu;
extern Ppu ppu;
extern MMap mMap;

extern Fetcher fetcher;

// renderer stuff
extern int* gFrameBuffer;
extern SDL_Window* gSDLWindow;
extern SDL_Renderer* gSDLRenderer;
extern SDL_Texture* gSDLTexture;
extern int colourArr[4];
static int gDone;

#endif

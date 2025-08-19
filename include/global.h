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
extern int cycles;

extern Fetcher fetcher;
extern OamEntry scanlineObjs[10];
extern int numScanlineObjs;

// renderer stuff
extern int* gFrameBuffer;
extern SDL_Window* gSDLWindow;
extern SDL_Renderer* gSDLRenderer;
extern SDL_Texture* gSDLTexture;
extern int bgPal[4];
extern int objPalArr[2][4];
static int gDone;

#endif

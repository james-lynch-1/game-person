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

// timer
extern u16 cycles;
extern int andResult;
extern int timaOverflow;
extern bool timaUnsettable;

extern bool externalRamEnabled;
extern u8 romFile[32][16384];
extern u8 externalRam[16][8192];
extern int currRamBank;
extern int fileSize;
extern int romSize;
extern int romSizeLUT[12][3];

extern Fetcher fetcher;
extern OamEntry scanlineObjs[10];
extern int numScanlineObjs;
extern int bgPal[4];
extern int objPalArr[2][4];

// renderer stuff
extern int* gFrameBuffer;
extern SDL_Window* gSDLWindow;
extern SDL_Renderer* gSDLRenderer;
extern SDL_Texture* gSDLTexture;
static int gDone;

#endif

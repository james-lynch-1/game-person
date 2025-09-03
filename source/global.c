#include "global.h"

char title[30] = "";
FILE* romPtr;
u32 maxFPS;
int scale;
int WINDOW_WIDTH;
int WINDOW_HEIGHT;

Cpu cpu;
Ppu ppu;
MMap mMap;

// timer
u16 cycles = 0;
int andResult;
int timaOverflow = 0;
bool timaUnsettable = false;

bool externalRamEnabled = false;
u8 externalRam[16][8192];
int currRamBank = 0;
u8 romFile[32][16384];
int fileSize;
int romSize; // in KiB
// code at $0148, ROM size in KiB, num. ROM banks
int romSizeLUT[12][3] =
{ {0, 32, 0},
  {1, 64, 4},
  {2, 128, 8},
  {3, 256, 16},
  {4, 512, 32},
  {5, 1024, 64},
  {6, 2048, 128},
  {7, 4096, 256},
  {8, 8192, 512},
  {0x52, 1152, 72},
  {0x53, 1280, 80},
  {0x54, 1536, 96} };

Fetcher fetcher;
OamEntry scanlineObjs[10] = { {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {0, 0, 0, 0} };
int numScanlineObjs;
int bgPal[4] = { 0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000 };
int objPalArr[2][4] = {
    {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000},
    {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000}
};

int* gFrameBuffer;
SDL_Window* gSDLWindow = NULL;
SDL_Renderer* gSDLRenderer = NULL;
SDL_Texture* gSDLTexture = NULL;


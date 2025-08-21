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
int cycles = 0;
bool externalRamEnabled = false;
u8 romFile[32][16384];
int fileSize;

Fetcher fetcher;
OamEntry scanlineObjs[10] = { {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {0, 0, 0, 0} };
int numScanlineObjs;

int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
int bgPal[4] = {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000};
int objPalArr[2][4] = {
    {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000},
    {0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000}
};

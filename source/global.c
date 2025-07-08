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

Fetcher fetcher;

int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
int colourArr[4];

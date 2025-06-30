#include "global.h"

FILE* romPtr;
u32 maxFPS;
int scale;
int WINDOW_WIDTH;
int WINDOW_HEIGHT;

PPU ppu;

// cpu stuff

MMap mMap;
CPU cpu;

int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;

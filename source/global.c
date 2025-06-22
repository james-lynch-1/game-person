#include "global.h"

u32 maxFPS;
int scale;
int WINDOW_WIDTH;
int WINDOW_HEIGHT;

int cycles;

MMap mMap;
Registers registers;

int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;

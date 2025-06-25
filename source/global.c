#include "global.h"

FILE* romPtr;
u32 maxFPS;
int scale;
int WINDOW_WIDTH;
int WINDOW_HEIGHT;

// cpu stuff
int pc; // program counter - where we are in the ROM
int ticks;
enum CpuState cpuState;
bool prefixedInstr;

MMap mMap;
Registers regs;

int* gFrameBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;

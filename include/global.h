#ifndef GLOBAL
#define GLOBAL

#include "util.h"
#include "ram.h"
#include "registers.h"

// globals
// values
extern FILE* romPtr;
extern u32 maxFPS;
extern int scale;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

// cpu stuff
extern int pc; // program counter - where we are in the ROM
extern int ticks;
enum CpuState {
    fetchOpcode,
    executeInstruction
};
extern enum CpuState cpuState;
extern bool prefixedInstr;

// structs
extern MMap mMap;
extern Registers regs;

// renderer stuff
extern int* gFrameBuffer;
extern SDL_Window* gSDLWindow;
extern SDL_Renderer* gSDLRenderer;
extern SDL_Texture* gSDLTexture;
static int gDone;

#endif

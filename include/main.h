#ifndef MAIN
#define MAIN

#include "SDL3/SDL_main.h"
#include "util.h"
#include "type.h"
#include "cpu.h"
#include "ppu.h"
#include "global.h"
#include "input.h"
#include "timer.h"

bool update();

void loop();

void initialiseLCDProps();

void initialiseValues();

void setTitle();

bool initialiseVideo();

void quitSDL();

#endif

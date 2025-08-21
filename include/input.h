#ifndef INPUT
#define INPUT

#include "util.h"
#include "global.h"

#define KEY_RIGHT   0
#define KEY_LEFT    1
#define KEY_UP      2
#define KEY_DOWN    3
#define KEY_A       4
#define KEY_B       5
#define KEY_SEL     6
#define KEY_START   7

extern int keys;

void handleInput(SDL_KeyboardEvent* e);

void handleGamepadInput(SDL_GamepadButtonEvent* e);

void updateInputGB();

#endif

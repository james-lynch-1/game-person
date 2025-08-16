#include "input.h"

extern void requestInterrupt(int intr);

int keys = 0;

void handleInput(SDL_KeyboardEvent* e) {
    SDL_Event keyEvent;
    switch (e->key) {
        case SDLK_TAB:
            maxFPS = e->type == SDL_EVENT_KEY_DOWN ? 0 : 60;
            break;
        case SDLK_RIGHT:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x1 : keys & ~0x1;
            break;
        case SDLK_LEFT:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x2 : keys & ~0x2;
            break;
        case SDLK_UP:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x4 : keys & ~0x4;
            break;
        case SDLK_DOWN:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x8 : keys & ~0x8;
            break;
        case SDLK_Z:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x10 : keys & ~0x10;
            break;
        case SDLK_X:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x20 : keys & ~0x20;
            break;
        case SDLK_RSHIFT:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x40 : keys & ~0x40;
            break;
        case SDLK_RETURN:
            keys = e->type == SDL_EVENT_KEY_DOWN ? keys | 0x80 : keys & ~0x80;
            break;
        default:
            break;
    }
}

void updateInputGB() {
    MMAP.ioRegs.joypadInput &= 0xF0;
    int setBits = 0;
    if ((~MMAP.ioRegs.joypadInput & 0b00010000) != 0) // dpad
        setBits = keys;
    else if ((~MMAP.ioRegs.joypadInput & 0b00100000) != 0) // buttons
        setBits = keys >> 4;
    if (setBits)
        requestInterrupt(INTR_JOYPAD);
    MMAP.ioRegs.joypadInput = (MMAP.ioRegs.joypadInput & 0xF0) | (~setBits & 0xF);
}

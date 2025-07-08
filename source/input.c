#include "input.h"

bool keys[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
u8 lastJoypadInput = 0b00001111;

void handleInput(SDL_KeyboardEvent* e) {
    SDL_Event keyEvent;
    switch (e->key) {
        case SDLK_RIGHT:
            keys[0] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        case SDLK_LEFT:
            keys[1] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        case SDLK_UP:
            keys[2] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        case SDLK_DOWN:
            keys[3] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        case SDLK_Z:
            keys[4] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        case SDLK_X:
            keys[5] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        case SDLK_RSHIFT:
            keys[6] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        case SDLK_RETURN:
            keys[7] = e->type == SDL_EVENT_KEY_DOWN ? 0 : 1;
            break;
        default:
            break;
    }
}

void updateInputGB() {
    if (~MMAP.ioRegs.joypadInput & 0x00110000 != lastJoypadInput) {
        lastJoypadInput = MMAP.ioRegs.joypadInput;
        // MMAP.ioRegs.joypadInput &= 0xF0;
        MMAP.ioRegs.joypadInput |= 0xF;
        if ((~MMAP.ioRegs.joypadInput & 0b00010000) != 0) {
            for (int i = 0; i < 4; i++)
                MMAP.ioRegs.joypadInput |= keys[i] << i;
        }
        else if ((~MMAP.ioRegs.joypadInput & 0b00100000) != 0) {
            for (int i = 0; i < 4; i++)
                MMAP.ioRegs.joypadInput |= keys[i + 4] << i;
        }
    }
}

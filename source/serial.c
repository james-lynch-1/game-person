#include "serial.h"

extern void requestInterrupt(int requestedInterrupt);

void handleSCWrite(int val) {
    switch (val & 0b10000001) {
        case 0b10000000: // master

            break;
        case 0b10000001: // slave
            requestInterrupt(INTR_SERIAL);
            break;
        default:
            break;
    }
}

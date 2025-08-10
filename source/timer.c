#include "timer.h"

extern void requestInterrupt(int intr);

void updateTimer() {
    if (cycles % 256 == 0) MMAP.ioRegs.timerAndDivider.div++;
    bool tacEnabled = (MMAP.ioRegs.timerAndDivider.tac & 0b100) != 0;
    if (tacEnabled) {
        switch (MMAP.ioRegs.timerAndDivider.tac & 0b11) {
            case 0b00:
                if (cycles % 1024 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
            case 0b01:
                if (cycles % 16 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
            case 0b10:
                if (cycles % 64 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
            case 0b11:
                if (cycles % 256 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
        }
        if (MMAP.ioRegs.timerAndDivider.tima == 0) {
            MMAP.ioRegs.timerAndDivider.tima = MMAP.ioRegs.timerAndDivider.tma;
            requestInterrupt(INTR_TIMER);
        }
    }
    if (cycles == 1024) cycles = 0;
}

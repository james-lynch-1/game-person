#include "timer.h"

void updateTimer() {
    bool tacEnabled = MMAP.ioRegs.timerAndDivider.tac & 0b100 != 0;
    if (tacEnabled) {
        switch (MMAP.ioRegs.timerAndDivider.tac & 0b11) {
            case 0b00:
                if ((cycles / 4) % 256 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
            case 0b01:
                if ((cycles / 4) % 4 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
            case 0b10:
                if ((cycles / 4) % 16 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
            case 0b11:
                if ((cycles / 4) % 64 == 0)
                    MMAP.ioRegs.timerAndDivider.tima++;
                break;
        }
    }
}

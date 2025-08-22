#include "timer.h"
extern void doNothing();

extern void requestInterrupt(int intr);

void updateTimer() {
    if (MMAP.ioRegs.timerAndDivider.tima == 0 && timaOverflow) {
        timaOverflow = false;
        MMAP.ioRegs.timerAndDivider.tima = MMAP.ioRegs.timerAndDivider.tma;
        requestInterrupt(INTR_TIMER);
    }
    MMAP.ioRegs.timerAndDivider.div = cycles >> 8;
    int tacEnabled = (MMAP.ioRegs.timerAndDivider.tac & 0b100) >> 2;
    int clockSelect = (MMAP.ioRegs.timerAndDivider.tac & 0b11);
    int bitShiftLut[4] = { 9, 3, 5, 7 };
    int newAndResult = (cycles >> bitShiftLut[clockSelect]) & tacEnabled;
    if (newAndResult < andResult)
        MMAP.ioRegs.timerAndDivider.tima++;
    if (MMAP.ioRegs.timerAndDivider.tima == 0xFF) {
        timaOverflow = true;
    }
    andResult = newAndResult;
}

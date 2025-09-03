#include "cpu.h"

void(*iSROpQ[5])() = {
    writeByteToSPISR,
    writeByteToSPAndDecISR,
    decrementSPISR,
    doNothing,
    doNothing
};
int numISROpsQueued;
int iSRHandlerAddr = 0;

// special versions of cpu functions that use the ISR operation + param queues and indices

void decrementSPISR() {
    REGARR16[REGSP_IDX]--;
}

void writeByteToSPAndDecISR() {
    MMAPARR[REGARR16[REGSP_IDX]] = cpu.regs.file.PC >> 8;
    REGARR16[REGSP_IDX]--;
}

void writeByteToSPISR() { // index of 16-bit reg holding address, value
    MMAPARR[REGARR16[REGSP_IDX]] = (u8)(cpu.regs.file.PC & 0xFF);
}

void jumpISR() { // jump to value in register at provided 16-bit index
    cpu.regs.file.PC = iSRHandlerAddr;
}

void requestInterrupt(int intr) {
    MMAP.ioRegs.interrupts |= intr;
}

void handleInterrupt(int intr) {
    if (!cpu.ime) {
        cpu.halt = false;
        return;
    }
    switch (intr) {
        case INTR_VBLANK:
            iSRHandlerAddr = 0x40;
            break;
        case INTR_LCD:
            iSRHandlerAddr = 0x48;
            break;
        case INTR_TIMER:
            iSRHandlerAddr = 0x50;
            break;
        case INTR_SERIAL:
            iSRHandlerAddr = 0x58;
            break;
        case INTR_JOYPAD:
            iSRHandlerAddr = 0x60;
            break;
        default: // 0 was passed aka bit is unset
            return;
    }
    MMAP.ioRegs.interrupts &= ~intr;
    cpu.ime = false;
    numISROpsQueued = 5;
}

void checkUnhandledInterrupts() {
    int intr;
    for (int i = 0; i < 5; i++) {
        intr = MMAP.ioRegs.interrupts & (1 << i);
        if (MMAP.iEReg & intr) {
            handleInterrupt(intr);
            return;
        }
    }
}

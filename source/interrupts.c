#include "cpu.h"

void(*iSROpQ[6])(); int numISROpsQueued;
int iSRParamQ[6]; int iSRParamIndex;
void(*iSRMiscOp)();

// special versions of cpu functions that use the ISR operation + param queues and indices

void decrementReg16ISR() {
    REGARR16[iSRParamQ[iSRParamIndex++]]--;
}

void writeByteToMemAndDecISR() { // decrement the reg that held the dest addr
    MMAPARR[REGARR16[iSRParamQ[iSRParamIndex]]] = iSRParamQ[iSRParamIndex + 1];
    REGARR16[iSRParamQ[iSRParamIndex]]--;
    iSRParamIndex += 2;
}

void writeByteToMemISR() { // index of 16-bit reg holding address, value
    MMAPARR[REGARR16[iSRParamQ[iSRParamIndex]]] = (u8)iSRParamQ[iSRParamIndex + 1];
    iSRParamIndex += 2;
}

void jumpISR() { // jump to value in register at provided 16-bit index
    cpu.regs.file.PC = iSRParamQ[iSRParamIndex++];
}



void requestInterrupt(int intr) {
    MMAP.ioRegs.interrupts |= intr;
}

void handleInterrupt(int intr) {
    if (!(MMAP.iEReg & intr))
        return;
    if (!cpu.ime) {
        if (cpu.opcode == 0x76)
            cpu.opcode = MMAPARR[cpu.regs.file.PC]; // to escape HALT
        return;
    }
    int handlerAddr = 0x0;
    switch (intr) {
        case INTR_VBLANK:
            handlerAddr = 0x40;
            break;
        case INTR_LCD:
            handlerAddr = 0x48;
            break;
        case INTR_TIMER:
            handlerAddr = 0x50;
            break;
        case INTR_SERIAL:
            handlerAddr = 0x58;
            break;
        case INTR_JOYPAD:
            handlerAddr = 0x60;
            break;
        default: // 0 was passed aka bit is unset
            return;
    }
    MMAP.ioRegs.interrupts &= ~intr;
    cpu.ime = false;
    numISROpsQueued = 5;
    iSRParamIndex = 0;
    iSROpQ[4] = doNothing;
    iSROpQ[3] = doNothing;
    iSROpQ[2] = decrementReg16ISR; iSRParamQ[0] = REGSP_IDX;
    iSROpQ[1] = writeByteToMemAndDecISR;
    iSRParamQ[1] = REGSP_IDX; iSRParamQ[2] = cpu.regs.file.PC >> 8;
    iSROpQ[0] = writeByteToMemISR;
    iSRParamQ[3] = REGSP_IDX; iSRParamQ[4] = cpu.regs.file.PC & 0xFF;
    iSRMiscOp = jumpISR; iSRParamQ[5] = handlerAddr;
}

void checkUnhandledInterrupts() {
    int intr;
    for (int i = 0; i < 5; i++) {
        intr = MMAP.ioRegs.interrupts & (1 << i);
        handleInterrupt(intr);
        if (intr) return;
    }
}

#include "cpu.h"

// an operation can take no more than 6 cycles.
// we split each opcode into atomic operations - makes defining them easier

void(*opQ[6])(); int numOpsQueued; // counts down. Always pre-decrement when using.
int pQ[32]; int pIndex; // these act as parameters for the micro-ops. Counts up. Always post-inc.
void(*miscOp)(); // miscellaneous operation to be done at the end of the opqueue but doesn't incur cycles

// atomic ops:
/*
General:
NOP
Next instr
HALT
STOP (actually 2 cycles but w/e)
INC

Arithmetic
ADC
ADD
16-bit arithmetic op is just two 8-bit ops

Load
COPY VAL INTO REG
GET IMMEDIATE VAL (aka increment romPtr, i think). increment should return a val. DONE
GET VAL AT HL ADDR

things we need to do
read an opcode from memory
read the opcode's parameters from memory (if any)
update cpu internal state depending on that opcode and its parameters (if any).
write a result to memory (if needed).

basic ops: //
read a byte from memory (1 cycle/4 ticks)
write a byte to memory (1 cycle/4 ticks)
update the cpu's internal state (effectively instantaneous)

*/

// do not incur cycles

void doNothing() {
    return;
}

void add() {
    int a = REGARR8[REGA_IDX] + REGARR8[pQ[pIndex]];
    int halfCarry = (REGARR8[REGA_IDX] & 0xF) + (REGARR8[pQ[pIndex]] & 0xF) > 0xF;
    FLAGS = (a & 0xFF) == 0 ? ZERO_FLAG : 0 | halfCarry << 5 |
        ((a > 0xFF) ? CARRY_FLAG : 0);
    REGARR8[REGA_IDX] = a & 0xFF;
    pIndex++;
}

// dest reg, operand 1 (reg index), operand 2 (value)
void addAndReadToReg() { // just for 0xE8 and 0xF8
    int result = REGARR8[pQ[pIndex + 1]] + (s8)pQ[pIndex + 2];
    int halfCarry = (REGARR8[pQ[pIndex + 1]] & 0xF) + (REGARR8[pQ[pIndex + 2]] & 0xF) > 0xF;
    FLAGS = halfCarry << 5 | ((result > 0xFF) ? CARRY_FLAG : 0);
    REGARR8[pQ[pIndex]] = result & 0xFF;
    pIndex += 3;
}

void adc() {
    int a = REGARR8[REGA_IDX] + REGARR8[pQ[pIndex]] + ((FLAGS & CARRY_FLAG) >> 4);
    int halfCarry = (((REGARR8[REGA_IDX] & 0xF) + (REGARR8[pQ[pIndex]] & 0xF) +
        ((FLAGS & CARRY_FLAG) >> 4)) > 0xF) << 5;
    FLAGS = (a & 0xFF) == 0 ? ZERO_FLAG : 0 | halfCarry |
        ((a > 0xFF) ? CARRY_FLAG : 0);
    REGARR8[REGA_IDX] = a & 0xFF;
    pIndex++;
}

void and () {
    REGARR8[REGA_IDX] &= REGARR8[pQ[pIndex++]];
    FLAGS = ((REGARR8[REGA_IDX] == 0) << 7) | HALFCARRY_FLAG;
}

void or () {
    REGARR8[REGA_IDX] |= REGARR8[pQ[pIndex++]];
    FLAGS = (REGARR8[REGA_IDX] == 0) << 7;
}

void xor () {
    REGARR8[REGA_IDX] ^= REGARR8[pQ[pIndex++]];
    FLAGS = (REGARR8[REGA_IDX] == 0) << 7;
}

void sub() {
    int a = REGARR8[REGA_IDX] - REGARR8[pQ[pIndex]];
    int halfCarry = ((int)REGARR8[REGA_IDX] & 0xF) - ((int)REGARR8[pQ[pIndex]] & 0xF) < 0;
    FLAGS = a == 0 ? ZERO_FLAG : 0 | SUBTR_FLAG | halfCarry << 5 | ((a < 0) ? CARRY_FLAG : 0);
    REGARR8[REGA_IDX] = a & 0xFF;
    pIndex++;
}

void sbc() {
    int a = REGARR8[REGA_IDX] - REGARR8[pQ[pIndex]] - ((FLAGS & CARRY_FLAG) >> 4);
    int halfCarry = ((int)REGARR8[REGA_IDX] & 0xF) - ((int)REGARR8[pQ[pIndex]] & 0xF) -
        ((FLAGS & CARRY_FLAG) >> 4) < 0;
    FLAGS = a == 0 ? ZERO_FLAG : 0 | SUBTR_FLAG | halfCarry << 5 | ((a < 0) ? CARRY_FLAG : 0);
    REGARR8[REGA_IDX] = a & 0xFF;
    pIndex++;
}

void sla() {
    u8* r = &(REGARR8[pQ[pIndex++]]);
    int leftmostBit = *r & 0b10000000;
    *r = *r << 1;
    FLAGS = ((*r == 0) ? ZERO_FLAG : 0) | leftmostBit >> 3;
}

void sra() {
    s8* r = (s8*)&(REGARR8[pQ[pIndex++]]);
    int rightmostBit = *r & 0b00000001;
    *r = *r >> 1;
    FLAGS = ((*r == 0) ? ZERO_FLAG : 0) | rightmostBit << 4;
}

void srl() {
    u8* r = &(REGARR8[pQ[pIndex++]]);
    int rightmostBit = *r & 0b00000001;
    *r = *r >> 1;
    FLAGS = ((*r == 0) ? ZERO_FLAG : 0) | rightmostBit << 4;
}

void swap() {
    int ln = REGARR8[pQ[pIndex]] & 0xF0;
    int rn = REGARR8[pQ[pIndex]] & 0x0F;
    REGARR8[pQ[pIndex]] = ln >> 8 | rn << 8;
    FLAGS = ((REGARR8[pQ[pIndex++]] == 0) ? ZERO_FLAG : 0);
}

// rotate left THROUGH the carry flag
void rol() {
    u8* r = &(REGARR8[pQ[pIndex++]]);
    int leftmostBit = *r & 0b10000000;
    *r = (u8)((int)*r << 1) | ((FLAGS & CARRY_FLAG) >> 4);
    FLAGS = ((*r == 0) ? ZERO_FLAG : 0) | leftmostBit >> 3;
}

// rotate right THROUGH the carry flag
void ror() {
    u8* r = &(REGARR8[pQ[pIndex++]]);
    int rightmostBit = *r & 0b00000001;
    *r = (*r >> 1) | ((FLAGS & CARRY_FLAG) << 3);
    FLAGS = ((*r == 0) ? ZERO_FLAG : 0) | rightmostBit << 4;
}

// rotate left and set the carry flag accordingly
void rlc() {
    u8* r = &(REGARR8[pQ[pIndex++]]);
    int leftmostBit = *r & 0b10000000;
    *r = (*r << 1) | (leftmostBit >> 7);
    FLAGS = ((*r == 0) ? ZERO_FLAG : 0) | leftmostBit >> 3;
}

// rotate right and set the carry flag accordingly
void rrc() {
    u8* r = &(REGARR8[pQ[pIndex++]]);
    int rightmostBit = *r & 0b00000001;
    *r = (*r >> 1) | (rightmostBit << 7);
    FLAGS = ((*r == 0) ? ZERO_FLAG : 0) | rightmostBit << 4;
}

void res() {
    REGARR8[pQ[pIndex]] &= pQ[pIndex + 1];
    pIndex += 2;
}

void set() {
    REGARR8[pQ[pIndex]] |= pQ[pIndex + 1];
    pIndex += 2;
}

void daa() {
    int adj = 0;
    int carry = 0;
    if (FLAGS & SUBTR_FLAG) {
        if (FLAGS & HALFCARRY_FLAG) adj += 6;
        if (FLAGS & CARRY_FLAG) adj += 0x60;
        REGARR8[REGA_IDX] -= adj;
    }
    else {
        if ((FLAGS & HALFCARRY_FLAG) || ((REGARR8[REGA_IDX] & 0xF) > 9))
            adj += 6;
        if (FLAGS & CARRY_FLAG || (REGARR8[REGA_IDX] > 0x99)) {
            adj += 0x60;
            carry = CARRY_FLAG;
        }
        REGARR8[REGA_IDX] += adj;
    }
    FLAGS = (REGARR8[REGA_IDX] == 0) | FLAGS & SUBTR_FLAG | carry;
}

void cmp() {
    int result = REGARR8[REGA_IDX] - REGARR8[pQ[pIndex]];
    int halfCarry = (((int)REGARR8[REGA_IDX] & 0xF) - ((int)REGARR8[pQ[pIndex]] & 0xF) < 0) << 5;
    FLAGS = ((result == 0) << 7) | SUBTR_FLAG | halfCarry | ((result < 0) ? CARRY_FLAG : 0);
    pIndex++;
}

void readByteToReg16() { // read 16-bit val to 16-bit reg. A misc op. 16-bit reg idx, value.
    REGARR16[pQ[pIndex]] = pQ[pIndex + 1];
    pIndex += 2;
}

void readRegToReg16() { // dest 16-bit reg, source 16-bit reg
    REGARR16[pQ[pIndex]] = REGARR16[pQ[pIndex + 1]];
    pIndex += 2;
}

// incur cycles

void readToPC() {
    cpu.regs.file.PC = REGARR16[pQ[pIndex++]];
}

void readByteAndCmpBit() {
    int bitPos = pQ[pIndex++];
    FLAGS |= ~((pQ[pIndex++] & (1 << bitPos))) << (7 - bitPos) | HALFCARRY_FLAG; FLAGS &= ~SUBTR_FLAG;
}

void readByteAndInc() { // increment the address you got it from
    REGARR8[pQ[pIndex]] = MMAPARR[REGARR16[pQ[pIndex + 1]]];
    REGARR16[pQ[pIndex + 1]]++;
    pIndex += 2;
}

void readByteAndDec() { // decrement the address you got it from
    REGARR8[pQ[pIndex]] = MMAPARR[REGARR16[pQ[pIndex + 1]]];
    REGARR16[pQ[pIndex + 1]]--;
    pIndex += 2;
}

void readByteToReg() { // dest REGARR8 index, val: a value
    int dest = pQ[pIndex++];
    u8 val = pQ[pIndex++];
    REGARR8[dest] = val;
}

void readRegToReg() { // dest REGARR8 index, src REGARR8 index
    REGARR8[pQ[pIndex]] = REGARR8[pQ[pIndex + 1]];
    pIndex += 2;
}

void writeByteToHighMem() { // address, value
    MMAPARR[pQ[pIndex]] = (u8)pQ[pIndex + 1];
    pIndex += 2;
}

void writeByteToMem() { // index of 16-bit reg holding address, value
    MMAPARR[REGARR16[pQ[pIndex]]] = (u8)pQ[pIndex + 1];
    pIndex += 2;
}

void writeRegToMem() { // index of 16-bit reg holding dest address, src reg index
    MMAPARR[REGARR16[pQ[pIndex]]] = REGARR8[pQ[pIndex + 1]];
    pIndex += 2;
}

void writeByteToMemAndDec() { // decrement the reg that held the dest addr
    MMAPARR[REGARR16[pQ[pIndex]]] = pQ[pIndex + 1];
    REGARR16[pQ[pIndex]]--;
    pIndex += 2;
}

void jump() { // jump to value provided
    cpu.regs.file.PC = pQ[pIndex++];
}

void jumpToRegAddr() { // jump to value in register at provided 16-bit index
    cpu.regs.file.PC = REGARR16[pQ[pIndex++]];
}

void cmpJR() {
    int condition = pQ[pIndex++];
    s8 offset = pQ[pIndex++];
    if (condition) {
        opQ[numOpsQueued++] = jump;
        pQ[pIndex] = cpu.regs.file.PC + offset; // if program jumps to consecutive JR instrs, will overflow
    }
}

void cmpJump() {
    int condition = pQ[pIndex++];
    u16 addr = pQ[pIndex++];
    if (condition) {
        opQ[numOpsQueued++] = jump;
        pQ[pIndex] = addr;
    }
}

void cmpCall() {
    int condition = pQ[pIndex++];
    u16 addr = pQ[pIndex++];
    if (condition) { // operations will be done bottom to top
        miscOp = jumpToRegAddr; pQ[pIndex + 5] = REGWZ_IDX;
        opQ[numOpsQueued++] = writeByteToMem; pQ[pIndex + 3] = REGSP_IDX; pQ[pIndex + 4] = cpu.regs.file.PC & 0xFF;
        opQ[numOpsQueued++] = writeByteToMemAndDec; pQ[pIndex + 1] = REGSP_IDX; pQ[pIndex + 2] = cpu.regs.file.PC >> 8;
        opQ[numOpsQueued++] = decrementReg16; pQ[pIndex] = REGSP_IDX;
    }
}

void cmpRet() {
    int condition = pQ[pIndex++];
    if (condition) { // operations will be done bottom to top
        opQ[numOpsQueued++] = readToPC; pQ[pIndex + 4] = REGWZ_IDX;
        opQ[numOpsQueued++] = readByteAndInc; pQ[pIndex + 2] = REGW_IDX; pQ[pIndex + 3] = REGSP_IDX;
        opQ[numOpsQueued++] = readByteAndInc; pQ[pIndex] = REGZ_IDX; pQ[pIndex + 1] = REGSP_IDX;
    }
}

void cmpImmediate() {
    int result = cpu.regs.file.AF.A - pQ[pIndex];
    int zero = result == 0 ? ZERO_FLAG : 0;
    int halfCarry = ((int)(cpu.regs.file.AF.A & 0xF) - (int)(pQ[pIndex] & 0xF) < 0) << 5;
    int carry = result < 0 ? CARRY_FLAG : 0;
    FLAGS = zero | SUBTR_FLAG | halfCarry | carry;
    pIndex++;
}

void incrementReg16() {
    REGARR16[pQ[pIndex++]]++;
}

void decrementReg16() {
    REGARR16[pQ[pIndex++]]--;
}

void incrementReg8() {
    u8* regPtr = &(REGARR8[pQ[pIndex++]]);
    int initVal = *regPtr;
    int result = initVal + 1;
    int zero = (result == 0) << 7;
    int halfCarry = (((initVal & 0xF) + 1) > 0xF) << 5;
    FLAGS = zero | halfCarry | (FLAGS & CARRY_FLAG);
    (*regPtr)++;
}

void decrementReg8() {
    u8* regPtr = &(REGARR8[pQ[pIndex++]]);
    int initVal = *regPtr;
    int result = initVal - 1;
    int zero = (result == 0) << 7;
    int halfCarry = (((initVal & 0xF) - 1) < 0) << 5;
    FLAGS = zero | SUBTR_FLAG | halfCarry | (FLAGS & CARRY_FLAG);
    (*regPtr)--;
}

void writeByteToMemAndInc() { // dest: mmaparr index, val: value
    MMAPARR[REGARR16[pQ[pIndex]]] = pQ[pIndex + 1];
    REGARR16[pQ[pIndex]]++;
    pIndex += 2;
}

void addHL16() { // 0x09, 0x19, 0x29, 0x39
    int result = REGARR8[REGL_IDX] + REGARR8[pQ[pIndex]];
    int hc = ((((int)REGARR8[REGL_IDX] & 0xF) + ((int)REGARR8[pQ[pIndex]] & 0xF)) > 0xF) << 5;
    int c = (result > 0xFF) << 4;
    FLAGS &= ~HALFCARRY_FLAG;
    FLAGS &= ~CARRY_FLAG;
    FLAGS |= hc | c;
    REGARR8[REGL_IDX] = result & 0xFF;

    result = REGARR8[REGH_IDX] + REGARR8[pQ[pIndex + 1]] + ((FLAGS & CARRY_FLAG) >> 4);
    hc = (((REGARR8[REGH_IDX] & 0xF) + (REGARR8[pQ[pIndex + 1]] & 0xF)) > 0xF) << 5;
    c = (result > 0xFF) << 4;
    FLAGS &= ~HALFCARRY_FLAG;
    FLAGS &= ~CARRY_FLAG;
    FLAGS |= hc | c;

    FLAGS &= ~SUBTR_FLAG;

    miscOp = readByteToReg; pQ[pIndex] = REGH_IDX; pQ[pIndex + 1] = result & 0xFF;
}

void rst() {
    int rstAddr = pQ[0];
    opQ[2] = decrementReg16; pQ[0] = REGSP_IDX;
    opQ[1] = writeByteToMemAndDec; pQ[1] = REGSP_IDX; pQ[2] = cpu.regs.file.PC >> 8;
    opQ[0] = writeByteToMem; pQ[3] = REGSP_IDX; pQ[4] = cpu.regs.file.PC & 0xFF;
    miscOp = jump; pQ[5] = rstAddr;
}

void setIme() {
    cpu.ime = true;
}

// ---

void cpuTick() {
    if (++cpu.ticks < 4) return;
    cpu.ticks = 0;
    u8 opcode;
    switch (cpu.state) {
        case fetchOpcode:
            miscOp = doNothing;
            pIndex = 0;
            numOpsQueued = 0;
            opcode = MMAP.rom.bank0_1[cpu.regs.file.PC++];
            if (!cpu.prefixedInstr)
                switch (opcode) {
                    case 0x00: // NOP 1 4 - - - -
                        break;
                    case 0x01: // LD BC n16 3 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGC_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGB_IDX; pQ[3] = ROMVAL;
                        break;
                    case 0x02: // LD [BC] A 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGBC_IDX; pQ[1] = REGARR8[REGA_IDX];
                        break;
                    case 0x03: // INC BC 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = incrementReg16; pQ[0] = REGBC_IDX;
                        break;
                    case 0x04: // INC B 1 4 Z 0 H -
                        pQ[0] = REGB_IDX;
                        incrementReg8();
                        break;
                    case 0x05: // DEC B 1 4 Z 1 H -
                        pQ[0] = REGB_IDX;
                        decrementReg8();
                        break;
                    case 0x06: // LD B n8 2 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGB_IDX; pQ[1] = ROMVAL;
                        break;
                    case 0x07: // RLCA 1 4 0 0 0 C
                        pQ[0] = REGA_IDX;
                        rlc();
                        FLAGS = FLAGS & CARRY_FLAG;
                        break;
                    case 0x08: // LD [a16] SP 3 20 - - - -
                        numOpsQueued = 4;
                        opQ[3] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[2] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[1] = writeByteToMemAndInc; pQ[4] = REGWZ_IDX; pQ[5] = REGARR8[REGSPLO_IDX];
                        opQ[0] = writeByteToMem; pQ[6] = REGWZ_IDX; pQ[7] = REGARR8[REGSPHI_IDX];
                        break;
                    case 0x09: // ADD HL BC 1 8 - 0 H C
                        numOpsQueued = 1;
                        pQ[0] = REGC_IDX; pQ[1] = REGB_IDX;
                        opQ[0] = addHL16;
                        break;
                    case 0x0A: // LD A [BC] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGA_IDX; pQ[1] = MMAPARR[REGARR16[REGBC_IDX]];
                        break;
                    case 0x0B: // DEC BC 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = decrementReg16; pQ[0] = REGBC_IDX;
                        break;
                    case 0x0C: // INC C 1 4 Z 0 H -
                        pQ[0] = REGC_IDX;
                        incrementReg8();
                        break;
                    case 0x0D: // DEC C 1 4 Z 1 H -
                        pQ[0] = REGC_IDX;
                        decrementReg8();
                        break;
                    case 0x0E: // LD C n8 2 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGC_IDX; pQ[1] = ROMVAL;
                        break;
                    case 0x0F: // RRCA 1 4 0 0 0 C
                        rrc();
                        FLAGS &= FLAGS & CARRY_FLAG;
                        break;
                    case 0x10: // STOP n8 2 4 - - - -
                        break;
                    case 0x11: // LD DE n16 3 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGE_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGD_IDX; pQ[3] = ROMVAL;
                        break;
                    case 0x12: // LD [DE] A 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGDE_IDX; pQ[1] = REGARR8[REGA_IDX];
                        break;
                    case 0x13: // INC DE 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = incrementReg16; pQ[0] = REGDE_IDX;
                        break;
                    case 0x14: // INC D 1 4 Z 0 H -
                        pQ[0] = REGD_IDX;
                        incrementReg8();
                        break;
                    case 0x15: // DEC D 1 4 Z 1 H -
                        pQ[0] = REGD_IDX;
                        decrementReg8();
                        break;
                    case 0x16: // LD D n8 2 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGD_IDX; pQ[1] = ROMVAL;
                        break;
                    case 0x17: // RLA 1 4 0 0 0 C
                        pQ[0] = REGA_IDX;
                        rol();
                        FLAGS = FLAGS & CARRY_FLAG;
                        break;
                    case 0x18: // JR e8 2 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGW_IDX; pQ[1] = ROMVAL;
                        opQ[0] = jump; pQ[2] = cpu.regs.file.PC + (s8)pQ[1];
                        break;
                    case 0x19: // ADD HL DE 1 8 - 0 H C
                        numOpsQueued = 1;
                        opQ[0] = addHL16; pQ[0] = REGE_IDX; pQ[1] = REGD_IDX;
                        break;
                    case 0x1A: // LD A [DE] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGA_IDX; pQ[1] = MMAPARR[REGARR16[REGDE_IDX]];
                        break;
                    case 0x1B: // DEC DE 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = decrementReg16; pQ[0] = REGDE_IDX;
                        break;
                    case 0x1C: // INC E 1 4 Z 0 H -
                        pQ[0] = REGE_IDX;
                        incrementReg8();
                        break;
                    case 0x1D: // DEC E 1 4 Z 1 H -
                        pQ[0] = REGE_IDX;
                        decrementReg8();
                        break;
                    case 0x1E: // LD E n8 2 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGE_IDX; pQ[1] = ROMVAL;
                        break;
                    case 0x1F: // RRA 1 4 0 0 0 C
                        pQ[0] = REGA_IDX;
                        ror();
                        FLAGS &= FLAGS & CARRY_FLAG;
                        break;
                    case 0x20: // JR NZ e8 2 12/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpJR; pQ[0] = (FLAGS & ZERO_FLAG) == 0; pQ[1] = ROMVAL;
                        break;
                    case 0x21: // LD HL n16 3 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGL_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGH_IDX; pQ[3] = ROMVAL;
                        break;
                    case 0x22: // LD [HL+] A 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMemAndInc; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGA_IDX];
                        break;
                    case 0x23: // INC HL 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = incrementReg16; pQ[0] = REGHL_IDX;
                        break;
                    case 0x24: // INC H 1 4 Z 0 H -
                        pQ[0] = REGH_IDX;
                        incrementReg8();
                        break;
                    case 0x25: // DEC H 1 4 Z 1 H -
                        pQ[0] = REGH_IDX;
                        decrementReg8();
                        break;
                    case 0x26: // LD H n8 2 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGH_IDX; pQ[1] = ROMVAL;
                        break;
                    case 0x27: // DAA 1 4 Z - 0 C
                        daa();
                        break;
                    case 0x28: // JR Z e8 2 12/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpJR; pQ[0] = (FLAGS & ZERO_FLAG) != 0; pQ[1] = ROMVAL;
                        break;
                    case 0x29: // ADD HL HL 1 8 - 0 H C
                        numOpsQueued = 1;
                        opQ[0] = addHL16; pQ[0] = REGL_IDX; pQ[1] = REGH_IDX;
                        break;
                    case 0x2A: // LD A [HL+] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndInc; pQ[0] = REGA_IDX; pQ[1] = REGHL_IDX;
                        break;
                    case 0x2B: // DEC HL 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = decrementReg16; pQ[0] = REGHL_IDX;
                        break;
                    case 0x2C: // INC L 1 4 Z 0 H -
                        pQ[0] = REGL_IDX;
                        incrementReg8();
                        break;
                    case 0x2D: // DEC L 1 4 Z 1 H -
                        pQ[0] = REGL_IDX;
                        decrementReg8();
                        break;
                    case 0x2E: // LD L n8 2 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGL_IDX; pQ[1] = ROMVAL;
                        break;
                    case 0x2F: // CPL 1 4 - 1 1 -
                        REGARR8[REGA_IDX] = ~REGARR8[REGA_IDX];
                        FLAGS = FLAGS | SUBTR_FLAG | HALFCARRY_FLAG;
                        break;
                    case 0x30: // JR NC e8 2 12/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpJR; pQ[0] = (FLAGS & CARRY_FLAG) == 0; pQ[1] = ROMVAL;
                        break;
                    case 0x31: // LD SP n16 3 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGSPLO_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGSPHI_IDX; pQ[3] = ROMVAL;
                        break;
                    case 0x32: // LD [HL-] A 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMemAndDec; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGA_IDX];
                        break;
                    case 0x33: // INC SP 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = incrementReg16; pQ[0] = REGSP_IDX;
                        break;
                    case 0x34: // INC [HL] 1 12 Z 0 H -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = incrementReg8; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x35: // DEC [HL] 1 12 Z 1 H -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = decrementReg8; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x36: // LD [HL] n8 2 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = writeByteToMem; pQ[2] = REGHL_IDX; pQ[3] = pQ[1];
                        break;
                    case 0x37: // SCF 1 4 - 0 0 1
                        FLAGS = (FLAGS & ZERO_FLAG) | CARRY_FLAG;
                        break;
                    case 0x38: // JR C e8 2 12/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpJR; pQ[0] = (FLAGS & CARRY_FLAG) != 0; pQ[1] = ROMVAL;
                        break;
                    case 0x39: // ADD HL SP 1 8 - 0 H C
                        numOpsQueued = 1;
                        opQ[0] = addHL16; pQ[0] = REGSPLO_IDX; pQ[1] = REGSPHI_IDX;
                        break;
                    case 0x3A: // LD A [HL-] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndDec; pQ[0] = REGA_IDX; pQ[1] = REGHL_IDX;
                        break;
                    case 0x3B: // DEC SP 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = decrementReg16; pQ[0] = REGSP_IDX;
                        break;
                    case 0x3C: // INC A 1 4 Z 0 H -
                        pQ[0] = REGA_IDX;
                        incrementReg8();
                        break;
                    case 0x3D: // DEC A 1 4 Z 1 H -
                        pQ[0] = REGA_IDX;
                        decrementReg8();
                        break;
                    case 0x3E: // LD A n8 2 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGA_IDX; pQ[1] = ROMVAL;
                        break;
                    case 0x3F: // CCF 1 4 - 0 0 C
                        FLAGS = (FLAGS & ZERO_FLAG) | (FLAGS & CARRY_FLAG); // set sub + hc to 0
                        FLAGS &= ~CARRY_FLAG | ~(FLAGS & CARRY_FLAG);
                        break;
                    case 0x40: // LD B B 1 4 - - - -
                        pQ[0] = REGB_IDX; pQ[1] = cpu.regs.file.BC.B;
                        readByteToReg();
                        break;
                    case 0x41: // LD B C 1 4 - - - -
                        pQ[0] = REGB_IDX; pQ[1] = cpu.regs.file.BC.C;
                        readByteToReg();
                        break;
                    case 0x42: // LD B D 1 4 - - - -
                        pQ[0] = REGB_IDX; pQ[1] = cpu.regs.file.DE.D;
                        readByteToReg();
                        break;
                    case 0x43: // LD B E 1 4 - - - -
                        pQ[0] = REGB_IDX; pQ[1] = cpu.regs.file.DE.E;
                        readByteToReg();
                        break;
                    case 0x44: // LD B H 1 4 - - - -
                        pQ[0] = REGB_IDX; pQ[1] = cpu.regs.file.HL.H;
                        readByteToReg();
                        break;
                    case 0x45: // LD B L 1 4 - - - -
                        pQ[0] = REGB_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        break;
                    case 0x46: // LD B [HL] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGB_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        break;
                    case 0x47: // LD B A 1 4 - - - -
                        pQ[0] = REGB_IDX; pQ[1] = cpu.regs.file.AF.A;
                        readByteToReg();
                        break;
                    case 0x48: // LD C B 1 4 - - - -
                        pQ[0] = REGC_IDX; pQ[1] = cpu.regs.file.BC.B;
                        readByteToReg();
                        break;
                    case 0x49: // LD C C 1 4 - - - -
                        pQ[0] = REGC_IDX; pQ[1] = cpu.regs.file.BC.C;
                        readByteToReg();
                        break;
                    case 0x4A: // LD C D 1 4 - - - -
                        pQ[0] = REGC_IDX; pQ[1] = cpu.regs.file.DE.D;
                        readByteToReg();
                        break;
                    case 0x4B: // LD C E 1 4 - - - -
                        pQ[0] = REGC_IDX; pQ[1] = cpu.regs.file.DE.E;
                        readByteToReg();
                        break;
                    case 0x4C: // LD C H 1 4 - - - -
                        pQ[0] = REGC_IDX; pQ[1] = cpu.regs.file.HL.H;
                        readByteToReg();
                        break;
                    case 0x4D: // LD C L 1 4 - - - -
                        pQ[0] = REGC_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        break;
                    case 0x4E: // LD C [HL] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGC_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        break;
                    case 0x4F: // LD C A 1 4 - - - -
                        pQ[0] = REGC_IDX; pQ[1] = cpu.regs.file.AF.A;
                        readByteToReg();
                        break;
                    case 0x50: // LD D B 1 4 - - - -
                        pQ[0] = REGD_IDX; pQ[1] = cpu.regs.file.BC.B;
                        readByteToReg();
                        break;
                    case 0x51: // LD D C 1 4 - - - -
                        pQ[0] = REGD_IDX; pQ[1] = cpu.regs.file.BC.C;
                        readByteToReg();
                        break;
                    case 0x52: // LD D D 1 4 - - - -
                        pQ[0] = REGD_IDX; pQ[1] = cpu.regs.file.DE.D;
                        readByteToReg();
                        break;
                    case 0x53: // LD D E 1 4 - - - -
                        pQ[0] = REGD_IDX; pQ[1] = cpu.regs.file.DE.E;
                        readByteToReg();
                        break;
                    case 0x54: // LD D H 1 4 - - - -
                        pQ[0] = REGD_IDX; pQ[1] = cpu.regs.file.HL.H;
                        readByteToReg();
                        break;
                    case 0x55: // LD D L 1 4 - - - -
                        pQ[0] = REGD_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        break;
                    case 0x56: // LD D [HL] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGD_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        break;
                    case 0x57: // LD D A 1 4 - - - -
                        pQ[0] = REGD_IDX; pQ[1] = cpu.regs.file.AF.A;
                        readByteToReg();
                        break;
                    case 0x58: // LD E B 1 4 - - - -
                        pQ[0] = REGE_IDX; pQ[1] = cpu.regs.file.BC.B;
                        readByteToReg();
                        break;
                    case 0x59: // LD E C 1 4 - - - -
                        pQ[0] = REGE_IDX; pQ[1] = cpu.regs.file.BC.C;
                        readByteToReg();
                        break;
                    case 0x5A: // LD E D 1 4 - - - -
                        pQ[0] = REGE_IDX; pQ[1] = cpu.regs.file.DE.D;
                        readByteToReg();
                        break;
                    case 0x5B: // LD E E 1 4 - - - -
                        pQ[0] = REGE_IDX; pQ[1] = cpu.regs.file.DE.E;
                        readByteToReg();
                        break;
                    case 0x5C: // LD E H 1 4 - - - -
                        pQ[0] = REGE_IDX; pQ[1] = cpu.regs.file.HL.H;
                        readByteToReg();
                        break;
                    case 0x5D: // LD E L 1 4 - - - -
                        pQ[0] = REGE_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        break;
                    case 0x5E: // LD E [HL] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGE_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        break;
                    case 0x5F: // LD E A 1 4 - - - -
                        pQ[0] = REGE_IDX; pQ[1] = cpu.regs.file.AF.A;
                        readByteToReg();
                        break;
                    case 0x60: // LD H B 1 4 - - - -
                        pQ[0] = REGH_IDX; pQ[1] = cpu.regs.file.BC.B;
                        readByteToReg();
                        break;
                    case 0x61: // LD H C 1 4 - - - -
                        pQ[0] = REGH_IDX; pQ[1] = cpu.regs.file.BC.C;
                        readByteToReg();
                        break;
                    case 0x62: // LD H D 1 4 - - - -
                        pQ[0] = REGH_IDX; pQ[1] = cpu.regs.file.DE.D;
                        readByteToReg();
                        break;
                    case 0x63: // LD H E 1 4 - - - -
                        pQ[0] = REGH_IDX; pQ[1] = cpu.regs.file.DE.E;
                        readByteToReg();
                        break;
                    case 0x64: // LD H H 1 4 - - - -
                        pQ[0] = REGH_IDX; pQ[1] = cpu.regs.file.HL.H;
                        readByteToReg();
                        break;
                    case 0x65: // LD H L 1 4 - - - -
                        pQ[0] = REGH_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        break;
                    case 0x66: // LD H [HL] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGH_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        break;
                    case 0x67: // LD H A 1 4 - - - -
                        pQ[0] = REGH_IDX; pQ[1] = cpu.regs.file.AF.A;
                        readByteToReg();
                        break;
                    case 0x68: // LD L B 1 4 - - - -
                        pQ[0] = REGL_IDX; pQ[1] = cpu.regs.file.BC.B;
                        readByteToReg();
                        break;
                    case 0x69: // LD L C 1 4 - - - -
                        pQ[0] = REGL_IDX; pQ[1] = cpu.regs.file.BC.C;
                        readByteToReg();
                        break;
                    case 0x6A: // LD L D 1 4 - - - -
                        pQ[0] = REGL_IDX; pQ[1] = cpu.regs.file.DE.D;
                        readByteToReg();
                        break;
                    case 0x6B: // LD L E 1 4 - - - -
                        pQ[0] = REGL_IDX; pQ[1] = cpu.regs.file.DE.E;
                        readByteToReg();
                        break;
                    case 0x6C: // LD L H 1 4 - - - -
                        pQ[0] = REGL_IDX; pQ[1] = cpu.regs.file.HL.H;
                        readByteToReg();
                        break;
                    case 0x6D: // LD L L 1 4 - - - -
                        pQ[0] = REGL_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        break;
                    case 0x6E: // LD L [HL] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGL_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        break;
                    case 0x6F: // LD L A 1 4 - - - -
                        pQ[0] = REGL_IDX; pQ[1] = cpu.regs.file.AF.A;
                        readByteToReg();
                        break;
                    case 0x70: // LD [HL] B 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGB_IDX];
                        break;
                    case 0x71: // LD [HL] C 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGC_IDX];
                        break;
                    case 0x72: // LD [HL] D 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGD_IDX];
                        break;
                    case 0x73: // LD [HL] E 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGE_IDX];
                        break;
                    case 0x74: // LD [HL] H 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGH_IDX];
                        break;
                    case 0x75: // LD [HL] L 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem; pQ[0] = REGHL_IDX; pQ[1] = REGARR8[REGL_IDX];
                        break;
                    case 0x76: // HALT 1 4 - - - -
                        break;
                    case 0x77: // LD [HL] A 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToMem;
                        pQ[0] = REGHL_IDX; pQ[1] = cpu.regs.file.AF.A;
                        break;
                    case 0x78: // LD A B 1 4 - - - -
                        pQ[0] = REGA_IDX; pQ[1] = cpu.regs.file.BC.B;
                        readByteToReg();
                        break;
                    case 0x79: // LD A C 1 4 - - - -
                        pQ[0] = REGA_IDX; pQ[1] = cpu.regs.file.BC.C;
                        readByteToReg();
                        break;
                    case 0x7A: // LD A D 1 4 - - - -
                        pQ[0] = REGA_IDX; pQ[1] = cpu.regs.file.DE.D;
                        readByteToReg();
                        break;
                    case 0x7B: // LD A E 1 4 - - - -
                        pQ[0] = REGA_IDX; pQ[1] = cpu.regs.file.DE.E;
                        readByteToReg();
                        break;
                    case 0x7C: // LD A H 1 4 - - - -
                        pQ[0] = REGA_IDX; pQ[1] = cpu.regs.file.HL.H;
                        readByteToReg();
                        break;
                    case 0x7D: // LD A L 1 4 - - - -
                        pQ[0] = REGA_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        break;
                    case 0x7E: // LD A [HL] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGA_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        break;
                    case 0x7F: // LD A A 1 4 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGA_IDX; pQ[1] = REGARR8[REGA_IDX];
                        break;
                    case 0x80: // ADD A B 1 4 Z 0 H C
                        pQ[0] = REGB_IDX;
                        add();
                        break;
                    case 0x81: // ADD A C 1 4 Z 0 H C
                        pQ[0] = REGC_IDX;
                        add();
                        break;
                    case 0x82: // ADD A D 1 4 Z 0 H C
                        pQ[0] = REGD_IDX;
                        add();
                        break;
                    case 0x83: // ADD A E 1 4 Z 0 H C
                        pQ[0] = REGE_IDX;
                        add();
                        break;
                    case 0x84: // ADD A H 1 4 Z 0 H C
                        pQ[0] = REGH_IDX;
                        add();
                        break;
                    case 0x85: // ADD A L 1 4 Z 0 H C
                        pQ[0] = REGL_IDX;
                        add();
                        break;
                    case 0x86: // ADD A [HL] 1 8 Z 0 H C
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = add; pQ[2] = REGZ_IDX;
                        break;
                    case 0x87: // ADD A A 1 4 Z 0 H C
                        pQ[0] = REGA_IDX;
                        add();
                        break;
                    case 0x88: // ADC A B 1 4 Z 0 H C
                        pQ[0] = REGB_IDX;
                        adc();
                        break;
                    case 0x89: // ADC A C 1 4 Z 0 H C
                        pQ[0] = REGC_IDX;
                        adc();
                        break;
                    case 0x8A: // ADC A D 1 4 Z 0 H C
                        pQ[0] = REGD_IDX;
                        adc();
                        break;
                    case 0x8B: // ADC A E 1 4 Z 0 H C
                        pQ[0] = REGE_IDX;
                        adc();
                        break;
                    case 0x8C: // ADC A H 1 4 Z 0 H C
                        pQ[0] = REGH_IDX;
                        adc();
                        break;
                    case 0x8D: // ADC A L 1 4 Z 0 H C
                        pQ[0] = REGL_IDX;
                        adc();
                        break;
                    case 0x8E: // ADC A [HL] 1 8 Z 0 H C
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = adc; pQ[2] = REGZ_IDX;
                        break;
                    case 0x8F: // ADC A A 1 4 Z 0 H C
                        pQ[0] = REGA_IDX;
                        adc();
                        break;
                    case 0x90: // SUB A B 1 4 Z 1 H C
                        pQ[0] = REGB_IDX;
                        sub();
                        break;
                    case 0x91: // SUB A C 1 4 Z 1 H C
                        pQ[0] = REGC_IDX;
                        sub();
                        break;
                    case 0x92: // SUB A D 1 4 Z 1 H C
                        pQ[0] = REGD_IDX;
                        sub();
                        break;
                    case 0x93: // SUB A E 1 4 Z 1 H C
                        pQ[0] = REGE_IDX;
                        sub();
                        break;
                    case 0x94: // SUB A H 1 4 Z 1 H C
                        pQ[0] = REGH_IDX;
                        sub();
                        break;
                    case 0x95: // SUB A L 1 4 Z 1 H C
                        pQ[0] = REGL_IDX;
                        sub();
                        break;
                    case 0x96: // SUB A [HL] 1 8 Z 1 H C
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = sub; pQ[2] = REGZ_IDX;
                        break;
                    case 0x97: // SUB A A 1 4 1 1 0 0
                        pQ[0] = REGA_IDX;
                        sub();
                        break;
                    case 0x98: // SBC A B 1 4 Z 1 H C
                        pQ[0] = REGB_IDX;
                        sbc();
                        break;
                    case 0x99: // SBC A C 1 4 Z 1 H C
                        pQ[0] = REGC_IDX;
                        sbc();
                        break;
                    case 0x9A: // SBC A D 1 4 Z 1 H C
                        pQ[0] = REGD_IDX;
                        sbc();
                        break;
                    case 0x9B: // SBC A E 1 4 Z 1 H C
                        pQ[0] = REGE_IDX;
                        sbc();
                        break;
                    case 0x9C: // SBC A H 1 4 Z 1 H C
                        pQ[0] = REGH_IDX;
                        sbc();
                        break;
                    case 0x9D: // SBC A L 1 4 Z 1 H C
                        pQ[0] = REGL_IDX;
                        sbc();
                        break;
                    case 0x9E: // SBC A [HL] 1 8 Z 1 H C
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = sbc; pQ[2] = REGZ_IDX;
                        break;
                    case 0x9F: // SBC A A 1 4 Z 1 H C
                        pQ[0] = REGA_IDX;
                        sbc();
                        break;
                    case 0xA0: // AND A B 1 4 Z 0 1 0
                        pQ[0] = REGB_IDX;
                        and ();
                        break;
                    case 0xA1: // AND A C 1 4 Z 0 1 0
                        pQ[0] = REGC_IDX;
                        and ();
                        break;
                    case 0xA2: // AND A D 1 4 Z 0 1 0
                        pQ[0] = REGD_IDX;
                        and ();
                        break;
                    case 0xA3: // AND A E 1 4 Z 0 1 0
                        pQ[0] = REGE_IDX;
                        and ();
                        break;
                    case 0xA4: // AND A H 1 4 Z 0 1 0
                        pQ[0] = REGH_IDX;
                        and ();
                        break;
                    case 0xA5: // AND A L 1 4 Z 0 1 0
                        pQ[0] = REGL_IDX;
                        and ();
                        break;
                    case 0xA6: // AND A [HL] 1 8 Z 0 1 0
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = and; pQ[2] = REGZ_IDX;
                        break;
                    case 0xA7: // AND A A 1 4 Z 0 1 0
                        pQ[0] = REGA_IDX;
                        and ();
                        break;
                    case 0xA8: // XOR A B 1 4 Z 0 0 0
                        pQ[0] = REGB_IDX;
                        xor ();
                        break;
                    case 0xA9: // XOR A C 1 4 Z 0 0 0
                        pQ[0] = REGC_IDX;
                        xor ();
                        break;
                    case 0xAA: // XOR A D 1 4 Z 0 0 0
                        pQ[0] = REGD_IDX;
                        xor ();
                        break;
                    case 0xAB: // XOR A E 1 4 Z 0 0 0
                        pQ[0] = REGE_IDX;
                        xor ();
                        break;
                    case 0xAC: // XOR A H 1 4 Z 0 0 0
                        pQ[0] = REGH_IDX;
                        xor ();
                        break;
                    case 0xAD: // XOR A L 1 4 Z 0 0 0
                        pQ[0] = REGL_IDX;
                        xor ();
                        break;
                    case 0xAE: // XOR A [HL] 1 8 Z 0 0 0
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = xor; pQ[2] = REGZ_IDX;
                        break;
                    case 0xAF: // XOR A A 1 4 Z 0 0 0
                        pQ[0] = REGA_IDX;
                        xor ();
                        break;
                    case 0xB0: // OR A B 1 4 Z 0 0 0
                        pQ[0] = REGB_IDX;
                        or ();
                        break;
                    case 0xB1: // OR A C 1 4 Z 0 0 0
                        pQ[0] = REGC_IDX;
                        or ();
                        break;
                    case 0xB2: // OR A D 1 4 Z 0 0 0
                        pQ[0] = REGD_IDX;
                        or ();
                        break;
                    case 0xB3: // OR A E 1 4 Z 0 0 0
                        pQ[0] = REGE_IDX;
                        or ();
                        break;
                    case 0xB4: // OR A H 1 4 Z 0 0 0
                        pQ[0] = REGH_IDX;
                        or ();
                        break;
                    case 0xB5: // OR A L 1 4 Z 0 0 0
                        pQ[0] = REGL_IDX;
                        or ();
                        break;
                    case 0xB6: // OR A [HL] 1 8 Z 0 0 0
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = or ; pQ[2] = REGZ_IDX;
                        break;
                    case 0xB7: // OR A A 1 4 Z 0 0 0
                        pQ[0] = REGC_IDX;
                        or ();
                        break;
                    case 0xB8: // CP A B 1 4 Z 1 H C
                        pQ[0] = REGB_IDX;
                        cmp();
                        break;
                    case 0xB9: // CP A C 1 4 Z 1 H C
                        pQ[0] = REGC_IDX;
                        cmp();
                        break;
                    case 0xBA: // CP A D 1 4 Z 1 H C
                        pQ[0] = REGD_IDX;
                        cmp();
                        break;
                    case 0xBB: // CP A E 1 4 Z 1 H C
                        pQ[0] = REGE_IDX;
                        cmp();
                        break;
                    case 0xBC: // CP A H 1 4 Z 1 H C
                        pQ[0] = REGH_IDX;
                        cmp();
                        break;
                    case 0xBD: // CP A L 1 4 Z 1 H C
                        pQ[0] = REGL_IDX;
                        cmp();
                        break;
                    case 0xBE: // CP A [HL] 1 8 Z 1 H C
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        readByteToReg();
                        opQ[0] = cmp; pQ[2] = REGZ_IDX;
                        break;
                    case 0xBF: // CP A A 1 4 1 1 0 0
                        pQ[0] = REGA_IDX;
                        cmp();
                        break;
                    case 0xC0: // RET NZ 1 20/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpRet; pQ[0] = (FLAGS & ZERO_FLAG) == 0;
                        break;
                    case 0xC1: // POP BC 1 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteAndInc; pQ[0] = REGZ_IDX; pQ[1] = REGSP_IDX;
                        opQ[0] = readByteAndInc; pQ[2] = REGW_IDX; pQ[3] = REGSP_IDX;
                        miscOp = readRegToReg16; pQ[4] = REGBC_IDX; pQ[5] = REGWZ_IDX;
                        break;
                    case 0xC2: // JP NZ a16 3 16/12 - - - -
                        numOpsQueued = 2;
                        pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        readByteToReg();
                        opQ[1] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[0] = cmpJump; pQ[4] = (FLAGS & ZERO_FLAG) == 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xC3: // JP a16 3 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[1] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[0] = jumpToRegAddr; pQ[4] = REGWZ_IDX;
                        break;
                    case 0xC4: // CALL NZ a16 3 24/12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        miscOp = cmpCall; pQ[4] = (FLAGS & ZERO_FLAG) == 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xC5: // PUSH BC 1 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = decrementReg16; pQ[0] = REGSP_IDX;
                        opQ[1] = writeByteToMemAndDec; pQ[1] = REGSP_IDX; pQ[2] = REGARR8[REGB_IDX];
                        opQ[0] = writeByteToMem; pQ[3] = REGSP_IDX; pQ[4] = REGARR8[REGC_IDX];
                        break;
                    case 0xC6: // ADD A n8 2 8 Z 0 H C
                        numOpsQueued = 1;
                        pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        readByteToReg();
                        opQ[0] = add; pQ[2] = REGZ_IDX;
                        break;
                    case 0xC7: // RST $00 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x00;
                        rst();
                        break;
                    case 0xC8: // RET Z 1 20/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpRet; pQ[0] = (FLAGS & ZERO_FLAG) != 0;
                        break;
                    case 0xC9: // RET 1 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = readByteAndInc; pQ[0] = REGZ_IDX; pQ[1] = REGSP_IDX;
                        opQ[1] = readByteAndInc; pQ[2] = REGW_IDX; pQ[3] = REGSP_IDX;
                        opQ[0] = readToPC; pQ[4] = REGWZ_IDX;
                        break;
                    case 0xCA: // JP Z a16 3 16/12 - - - -
                        numOpsQueued = 2;
                        pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        readByteToReg();
                        opQ[1] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[0] = cmpJump; pQ[4] = (FLAGS & ZERO_FLAG) != 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xCB: // PREFIX 1 4 - - - -
                        cpu.prefixedInstr = true;
                        break;
                    case 0xCC: // CALL Z a16 3 24/12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        miscOp = cmpCall; pQ[4] = (FLAGS & ZERO_FLAG) != 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xCD: // CALL a16 3 24 - - - -
                        numOpsQueued = 5;
                        opQ[4] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[3] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[2] = decrementReg16; pQ[4] = REGSP_IDX;
                        opQ[1] = writeByteToMemAndDec;
                        pQ[5] = REGSP_IDX; pQ[6] = cpu.regs.file.PC >> 8;
                        opQ[0] = writeByteToMem;
                        pQ[7] = REGSP_IDX; pQ[8] = cpu.regs.file.PC & 0xFF;
                        miscOp = jumpToRegAddr; pQ[9] = REGWZ_IDX;
                        break;
                    case 0xCE: // ADC A n8 2 8 Z 0 H C
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        miscOp = adc; pQ[2] = REGZ_IDX;
                        break;
                    case 0xCF: // RST $08 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x08;
                        rst();
                        break;
                    case 0xD0: // RET NC 1 20/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpRet; pQ[0] = (FLAGS & CARRY_FLAG) == 0;
                        break;
                    case 0xD1: // POP DE 1 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteAndInc; pQ[0] = REGZ_IDX; pQ[1] = REGSP_IDX;
                        opQ[0] = readByteAndInc; pQ[2] = REGW_IDX; pQ[3] = REGSP_IDX;
                        miscOp = readRegToReg16; pQ[4] = REGDE_IDX; pQ[5] = REGWZ_IDX;
                        break;
                    case 0xD2: // JP NC a16 3 16/12 - - - -
                        numOpsQueued = 2;
                        pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        readByteToReg();
                        opQ[1] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[0] = cmpJump; pQ[4] = (FLAGS & CARRY_FLAG) == 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xD3: // ILLEGAL_D3 1 4 - - - -
                        break;
                    case 0xD4: // CALL NC a16 3 24/12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        miscOp = cmpCall; pQ[4] = (FLAGS & CARRY_FLAG) == 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xD5: // PUSH DE 1 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = decrementReg16; pQ[0] = REGSP_IDX;
                        opQ[1] = writeByteToMemAndDec; pQ[1] = REGSP_IDX; pQ[2] = REGARR8[REGD_IDX];
                        opQ[0] = writeByteToMem; pQ[3] = REGSP_IDX; pQ[4] = REGARR8[REGE_IDX];
                        break;
                    case 0xD6: // SUB A n8 2 8 Z 1 H C
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        miscOp = sub; pQ[2] = REGZ_IDX;
                        break;
                    case 0xD7: // RST $10 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x10;
                        rst();
                        break;
                    case 0xD8: // RET C 1 20/8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = cmpRet; pQ[0] = (FLAGS & CARRY_FLAG) != 0;
                        break;
                    case 0xD9: // RETI 1 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = readByteAndInc; pQ[0] = REGZ_IDX; pQ[1] = REGSP_IDX;
                        opQ[1] = readByteAndInc; pQ[2] = REGW_IDX; pQ[3] = REGSP_IDX;
                        opQ[0] = jumpToRegAddr; pQ[4] = REGWZ_IDX;
                        miscOp = setIme;
                        break;
                    case 0xDA: // JP C a16 3 16/12 - - - -
                        numOpsQueued = 2;
                        pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        readByteToReg();
                        opQ[1] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[0] = cmpJump; pQ[4] = (FLAGS & CARRY_FLAG) != 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xDB: // ILLEGAL_DB 1 4 - - - -
                        break;
                    case 0xDC: // CALL C a16 3 24/12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        miscOp = cmpCall; pQ[4] = (FLAGS & CARRY_FLAG) != 0; pQ[5] = (pQ[3] << 8) | pQ[1];
                        break;
                    case 0xDD: // ILLEGAL_DD 1 4 - - - -
                        break;
                    case 0xDE: // SBC A n8 2 8 Z 1 H C
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        miscOp = sbc; pQ[2] = REGZ_IDX;
                        break;
                    case 0xDF: // RST $18 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x18;
                        rst();
                        break;
                    case 0xE0: // LDH [a8] A 2 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = writeByteToHighMem;
                        pQ[2] = 0xFF00 | pQ[1]; pQ[3] = cpu.regs.file.AF.A;
                        break;
                    case 0xE1: // POP HL 1 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteAndInc; pQ[0] = REGZ_IDX; pQ[1] = REGSP_IDX;
                        opQ[0] = readByteAndInc; pQ[2] = REGW_IDX; pQ[3] = REGSP_IDX;
                        miscOp = readRegToReg16; pQ[4] = REGHL_IDX; pQ[5] = REGWZ_IDX;
                        break;
                    case 0xE2: // LDH [C] A 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = writeByteToHighMem;
                        pQ[0] = 0xFF00 | cpu.regs.file.BC.C; pQ[1] = cpu.regs.file.AF.A;
                        break;
                    case 0xE3: // ILLEGAL_E3 1 4 - - - -
                        break;
                    case 0xE4: // ILLEGAL_E4 1 4 - - - -
                        break;
                    case 0xE5: // PUSH HL 1 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = decrementReg16; pQ[0] = REGSP_IDX;
                        opQ[1] = writeByteToMemAndDec; pQ[1] = REGSP_IDX; pQ[2] = cpu.regs.file.HL.H;
                        opQ[0] = writeByteToMem; pQ[3] = REGSP_IDX; pQ[4] = cpu.regs.file.HL.L;
                        break;
                    case 0xE6: // AND A n8 2 8 Z 0 1 0
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        miscOp = and; pQ[2] = REGZ_IDX;
                        break;
                    case 0xE7: // RST $20 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x20;
                        rst();
                        break;
                    case 0xE8: // ADD SP e8 2 16 0 0 H C
                        numOpsQueued = 3;
                        opQ[2] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[1] = addAndReadToReg; pQ[2] = REGZ_IDX; pQ[3] = REGZ_IDX; pQ[4] = REGARR8[REGSPLO_IDX];
                        opQ[0] = readByteToReg; pQ[5] = REGW_IDX;
                        pQ[6] = REGARR8[REGSPHI_IDX] + (((int)REGARR8[REGSPLO_IDX] + (s8)pQ[1]) > 0xFF);
                        miscOp = readRegToReg16; pQ[7] = REGSP_IDX; pQ[8] = REGWZ_IDX;
                        break;
                    case 0xE9: // JP HL 1 4 - - - -
                        pQ[0] = REGARR16[REGHL_IDX];
                        jump();
                        break;
                    case 0xEA: // LD [a16] A 3 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[1] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[0] = writeByteToMem; pQ[4] = REGWZ_IDX; pQ[5] = cpu.regs.file.AF.A;
                        break;
                    case 0xEB: // ILLEGAL_EB 1 4 - - - -
                        break;
                    case 0xEC: // ILLEGAL_EC 1 4 - - - -
                        break;
                    case 0xED: // ILLEGAL_ED 1 4 - - - -
                        break;
                    case 0xEE: // XOR A n8 2 8 Z 0 0 0
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        miscOp = xor; pQ[2] = REGZ_IDX;
                        break;
                    case 0xEF: // RST $28 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x28;
                        rst();
                        break;
                    case 0xF0: // LDH A [a8] 2 12 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[2] = REGA_IDX; pQ[3] = MMAPARR[0xFF00 | pQ[1]];
                        break;
                    case 0xF1: // POP AF 1 12 Z N H C
                        numOpsQueued = 2;
                        opQ[1] = readByteAndInc; pQ[0] = REGZ_IDX; pQ[1] = REGSP_IDX;
                        opQ[0] = readByteAndInc; pQ[2] = REGW_IDX; pQ[3] = REGSP_IDX;
                        miscOp = readRegToReg16; pQ[4] = REGAF_IDX; pQ[5] = REGWZ_IDX;
                        break;
                    case 0xF2: // LDH A [C] 1 8 - - - -
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR8[REGC_IDX]];
                        miscOp = readRegToReg; pQ[2] = REGA_IDX; pQ[3] = REGZ_IDX;
                        break;
                    case 0xF3: // DI 1 4 - - - -
                        cpu.ime = false;
                        break;
                    case 0xF4: // ILLEGAL_F4 1 4 - - - -
                        break;
                    case 0xF5: // PUSH AF 1 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = decrementReg16; pQ[0] = REGSP_IDX;
                        opQ[1] = writeByteToMemAndDec; pQ[1] = REGSP_IDX; pQ[2] = cpu.regs.file.AF.A;
                        opQ[0] = writeByteToMem; pQ[3] = REGSP_IDX; pQ[4] = cpu.regs.file.AF.F;
                        break;
                    case 0xF6: // OR A n8 2 8 Z 0 0 0
                        numOpsQueued = 1;
                        opQ[0] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        miscOp = or ; pQ[2] = REGZ_IDX;
                        break;
                    case 0xF7: // RST $30 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x30;
                        rst();
                        break;
                    case 0xF8: // LD HL SP + e8 2 12 0 0 H C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[0] = addAndReadToReg; pQ[2] = REGL_IDX; pQ[3] = REGZ_IDX; pQ[4] = REGARR8[REGSPLO_IDX];
                        miscOp = readByteToReg; pQ[5] = REGH_IDX;
                        pQ[6] = REGARR8[REGSPHI_IDX] + ((REGARR8[REGSPLO_IDX] + (s8)pQ[1]) > 0xFF);
                        break;
                    case 0xF9: // LD SP HL 1 8 - - - -
                        numOpsQueued = 1;
                        pQ[0] = REGSPLO_IDX; pQ[1] = cpu.regs.file.HL.L;
                        readByteToReg();
                        opQ[0] = readByteToReg; pQ[2] = REGSPHI_IDX; pQ[3] = cpu.regs.file.HL.H;
                        break;
                    case 0xFA: // LD A [a16] 3 16 - - - -
                        numOpsQueued = 3;
                        opQ[2] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = ROMVAL;
                        opQ[1] = readByteToReg; pQ[2] = REGW_IDX; pQ[3] = ROMVAL;
                        opQ[0] = readByteToReg; pQ[4] = REGZ_IDX; pQ[5] = MMAPARR[pQ[1] | (pQ[3] << 8)];
                        miscOp = readRegToReg; pQ[6] = REGA_IDX; pQ[7] = REGZ_IDX;
                        break;
                    case 0xFB: // EI 1 4 - - - -
                        cpu.ime = true;
                        break;
                    case 0xFC: // ILLEGAL_FC 1 4 - - - -
                        break;
                    case 0xFD: // ILLEGAL_FD 1 4 - - - -
                        break;
                    case 0xFE: // CP A n8 2 8 Z 1 H C
                        numOpsQueued = 1;
                        opQ[0] = cmpImmediate; pQ[0] = ROMVAL;
                        break;
                    case 0xFF: // RST $38 1 16 - - - -
                        numOpsQueued = 3;
                        pQ[0] = 0x38;
                        rst();
                        break;
                }
            else {
                switch (opcode) {
                    case 0x00: // RLC B 2 8 Z 0 0 C
                        pQ[0] = REGB_IDX;
                        rlc();
                        break;
                    case 0x01: // RLC C 2 8 Z 0 0 C
                        pQ[0] = REGC_IDX;
                        rlc();
                        break;
                    case 0x02: // RLC D 2 8 Z 0 0 C
                        pQ[0] = REGD_IDX;
                        rlc();
                        break;
                    case 0x03: // RLC E 2 8 Z 0 0 C
                        pQ[0] = REGE_IDX;
                        rlc();
                        break;
                    case 0x04: // RLC H 2 8 Z 0 0 C
                        pQ[0] = REGH_IDX;
                        rlc();
                        break;
                    case 0x05: // RLC L 2 8 Z 0 0 C
                        pQ[0] = REGL_IDX;
                        rlc();
                        break;
                    case 0x06: // RLC [HL] 2 16 Z 0 0 C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = rlc; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x07: // RLC A 2 8 Z 0 0 C
                        pQ[0] = REGA_IDX;
                        rlc();
                        break;
                    case 0x08: // RRC B 2 8 Z 0 0 C
                        pQ[0] = REGB_IDX;
                        rrc();
                        break;
                    case 0x09: // RRC C 2 8 Z 0 0 C
                        pQ[0] = REGC_IDX;
                        rrc();
                        break;
                    case 0x0A: // RRC D 2 8 Z 0 0 C
                        pQ[0] = REGD_IDX;
                        rrc();
                        break;
                    case 0x0B: // RRC E 2 8 Z 0 0 C
                        pQ[0] = REGE_IDX;
                        rrc();
                        break;
                    case 0x0C: // RRC H 2 8 Z 0 0 C
                        pQ[0] = REGH_IDX;
                        rrc();
                        break;
                    case 0x0D: // RRC L 2 8 Z 0 0 C
                        pQ[0] = REGL_IDX;
                        rrc();
                        break;
                    case 0x0E: // RRC [HL] 2 16 Z 0 0 C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = rrc; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x0F: // RRC A 2 8 Z 0 0 C
                        pQ[0] = REGA_IDX;
                        rrc();
                        break;
                    case 0x10: // RL B 2 8 Z 0 0 C
                        pQ[0] = REGB_IDX;
                        rol();
                        break;
                    case 0x11: // RL C 2 8 Z 0 0 C
                        pQ[0] = REGC_IDX;
                        rol();
                        break;
                    case 0x12: // RL D 2 8 Z 0 0 C
                        pQ[0] = REGD_IDX;
                        rol();
                        break;
                    case 0x13: // RL E 2 8 Z 0 0 C
                        pQ[0] = REGE_IDX;
                        rol();
                        break;
                    case 0x14: // RL H 2 8 Z 0 0 C
                        pQ[0] = REGH_IDX;
                        rol();
                        break;
                    case 0x15: // RL L 2 8 Z 0 0 C
                        pQ[0] = REGL_IDX;
                        rol();
                        break;
                    case 0x16: // RL [HL] 2 16 Z 0 0 C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = rol; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x17: // RL A 2 8 Z 0 0 C
                        pQ[0] = REGA_IDX;
                        rol();
                        break;
                    case 0x18: // RR B 2 8 Z 0 0 C
                        pQ[0] = REGB_IDX;
                        ror();
                        break;
                    case 0x19: // RR C 2 8 Z 0 0 C
                        pQ[0] = REGC_IDX;
                        ror();
                        break;
                    case 0x1A: // RR D 2 8 Z 0 0 C
                        pQ[0] = REGD_IDX;
                        ror();
                        break;
                    case 0x1B: // RR E 2 8 Z 0 0 C
                        pQ[0] = REGE_IDX;
                        ror();
                        break;
                    case 0x1C: // RR H 2 8 Z 0 0 C
                        pQ[0] = REGH_IDX;
                        ror();
                        break;
                    case 0x1D: // RR L 2 8 Z 0 0 C
                        pQ[0] = REGL_IDX;
                        ror();
                        break;
                    case 0x1E: // RR [HL] 2 16 Z 0 0 C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = ror; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x1F: // RR A 2 8 Z 0 0 C
                        pQ[0] = REGA_IDX;
                        ror();
                        break;
                    case 0x20: // SLA B 2 8 Z 0 0 C
                        pQ[0] = REGB_IDX;
                        sla();
                        break;
                    case 0x21: // SLA C 2 8 Z 0 0 C
                        pQ[0] = REGC_IDX;
                        sla();
                        break;
                    case 0x22: // SLA D 2 8 Z 0 0 C
                        pQ[0] = REGD_IDX;
                        sla();
                        break;
                    case 0x23: // SLA E 2 8 Z 0 0 C
                        pQ[0] = REGE_IDX;
                        sla();
                        break;
                    case 0x24: // SLA H 2 8 Z 0 0 C
                        pQ[0] = REGH_IDX;
                        sla();
                        break;
                    case 0x25: // SLA L 2 8 Z 0 0 C
                        pQ[0] = REGL_IDX;
                        sla();
                        break;
                    case 0x26: // SLA [HL] 2 16 Z 0 0 C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = sla; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x27: // SLA A 2 8 Z 0 0 C
                        pQ[0] = REGA_IDX;
                        sla();
                        break;
                    case 0x28: // SRA B 2 8 Z 0 0 C
                        pQ[0] = REGB_IDX;
                        sra();
                        break;
                    case 0x29: // SRA C 2 8 Z 0 0 C
                        pQ[0] = REGC_IDX;
                        sra();
                        break;
                    case 0x2A: // SRA D 2 8 Z 0 0 C
                        pQ[0] = REGD_IDX;
                        sra();
                        break;
                    case 0x2B: // SRA E 2 8 Z 0 0 C
                        pQ[0] = REGE_IDX;
                        sra();
                        break;
                    case 0x2C: // SRA H 2 8 Z 0 0 C
                        pQ[0] = REGH_IDX;
                        sra();
                        break;
                    case 0x2D: // SRA L 2 8 Z 0 0 C
                        pQ[0] = REGL_IDX;
                        sra();
                        break;
                    case 0x2E: // SRA [HL] 2 16 Z 0 0 C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = sra; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x2F: // SRA A 2 8 Z 0 0 C
                        pQ[0] = REGA_IDX;
                        sra();
                        break;
                    case 0x30: // SWAP B 2 8 Z 0 0 0
                        pQ[0] = REGB_IDX;
                        swap();
                        break;
                    case 0x31: // SWAP C 2 8 Z 0 0 0
                        pQ[0] = REGC_IDX;
                        swap();
                        break;
                    case 0x32: // SWAP D 2 8 Z 0 0 0
                        pQ[0] = REGD_IDX;
                        swap();
                        break;
                    case 0x33: // SWAP E 2 8 Z 0 0 0
                        pQ[0] = REGE_IDX;
                        swap();
                        break;
                    case 0x34: // SWAP H 2 8 Z 0 0 0
                        pQ[0] = REGH_IDX;
                        swap();
                        break;
                    case 0x35: // SWAP L 2 8 Z 0 0 0
                        pQ[0] = REGL_IDX;
                        swap();
                        break;
                    case 0x36: // SWAP [HL] 2 16 Z 0 0 0
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = swap; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x37: // SWAP A 2 8 Z 0 0 0
                        pQ[0] = REGA_IDX;
                        swap();
                        break;
                    case 0x38: // SRL B 2 8 Z 0 0 C
                        pQ[0] = REGB_IDX;
                        srl();
                        break;
                    case 0x39: // SRL C 2 8 Z 0 0 C
                        pQ[0] = REGC_IDX;
                        srl();
                        break;
                    case 0x3A: // SRL D 2 8 Z 0 0 C
                        pQ[0] = REGD_IDX;
                        srl();
                        break;
                    case 0x3B: // SRL E 2 8 Z 0 0 C
                        pQ[0] = REGE_IDX;
                        srl();
                        break;
                    case 0x3C: // SRL H 2 8 Z 0 0 C
                        pQ[0] = REGH_IDX;
                        srl();
                        break;
                    case 0x3D: // SRL L 2 8 Z 0 0 C
                        pQ[0] = REGL_IDX;
                        srl();
                        break;
                    case 0x3E: // SRL [HL] 2 16 Z 0 0 C
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = srl; pQ[2] = REGZ_IDX;
                        miscOp = writeRegToMem; pQ[3] = REGHL_IDX; pQ[4] = REGZ_IDX;
                        break;
                    case 0x3F: // SRL A 2 8 Z 0 0 C
                        pQ[0] = REGA_IDX;
                        srl();
                        break;
                    case 0x40: // BIT 0 B 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.B & 1) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x41: // BIT 0 C 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.C & 1) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x42: // BIT 0 D 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.D & 1) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x43: // BIT 0 E 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.E & 1) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x44: // BIT 0 H 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.H & 1) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x45: // BIT 0 L 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.L & 1) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x46: // BIT 0 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit; pQ[0] = 0; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x47: // BIT 0 A 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.AF.A & 1) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x48: // BIT 1 B 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.B & 2) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x49: // BIT 1 C 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.C & 2) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x4A: // BIT 1 D 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.D & 2) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x4B: // BIT 1 E 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.E & 2) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x4C: // BIT 1 H 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.H & 2) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x4D: // BIT 1 L 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.L & 2) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x4E: // BIT 1 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit;
                        pQ[0] = 1; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x4F: // BIT 1 A 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.AF.A & 2) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x50: // BIT 2 B 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.B & 4) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x51: // BIT 2 C 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.C & 4) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x52: // BIT 2 D 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.D & 4) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x53: // BIT 2 E 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.E & 4) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x54: // BIT 2 H 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.H & 4) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x55: // BIT 2 L 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.L & 4) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x56: // BIT 2 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit;
                        pQ[0] = 2; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x57: // BIT 2 A 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.AF.A & 4) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x58: // BIT 3 B 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.B & 8) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x59: // BIT 3 C 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.C & 8) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x5A: // BIT 3 D 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.D & 8) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x5B: // BIT 3 E 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.E & 8) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x5C: // BIT 3 H 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.H & 8) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x5D: // BIT 3 L 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.L & 8) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x5E: // BIT 3 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit;
                        pQ[0] = 3; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x5F: // BIT 3 A 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.AF.A & 8) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x60: // BIT 4 B 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.B & 16) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x61: // BIT 4 C 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.C & 16) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x62: // BIT 4 D 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.D & 16) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x63: // BIT 4 E 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.E & 16) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x64: // BIT 4 H 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.H & 16) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x65: // BIT 4 L 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.L & 16) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x66: // BIT 4 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit;
                        pQ[0] = 4; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x67: // BIT 4 A 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.AF.A & 16) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x68: // BIT 5 B 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.B & 32) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x69: // BIT 5 C 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.C & 32) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x6A: // BIT 5 D 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.D & 32) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x6B: // BIT 5 E 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.E & 32) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x6C: // BIT 5 H 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.H & 32) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x6D: // BIT 5 L 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.L & 32) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x6E: // BIT 5 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit;
                        pQ[0] = 5; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x6F: // BIT 5 A 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.AF.A & 32) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x70: // BIT 6 B 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.B & 64) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x71: // BIT 6 C 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.BC.C & 64) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x72: // BIT 6 D 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.D & 64) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x73: // BIT 6 E 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.DE.E & 64) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x74: // BIT 6 H 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.H & 64) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x75: // BIT 6 L 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.HL.L & 64) << 7) | HALFCARRY_FLAG | FLAGS & SUBTR_FLAG;
                        break;
                    case 0x76: // BIT 6 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit;
                        pQ[0] = 6; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x77: // BIT 6 A 2 8 Z 0 1 -
                        FLAGS = (!(cpu.regs.file.AF.A & 64) << 7) | HALFCARRY_FLAG | (FLAGS & SUBTR_FLAG);
                        break;
                    case 0x78: // BIT 7 B 2 8 Z 0 1 -
                        FLAGS = !(cpu.regs.file.BC.B & 0b10000000u) << 7 | HALFCARRY_FLAG | (FLAGS & SUBTR_FLAG);
                        break;
                    case 0x79: // BIT 7 C 2 8 Z 0 1 -
                        FLAGS = !(cpu.regs.file.BC.C & 0b10000000u) << 7 | HALFCARRY_FLAG | (FLAGS & SUBTR_FLAG);
                        break;
                    case 0x7A: // BIT 7 D 2 8 Z 0 1 -
                        FLAGS = !(cpu.regs.file.DE.D & 0b10000000u) << 7 | HALFCARRY_FLAG | (FLAGS & SUBTR_FLAG);
                        break;
                    case 0x7B: // BIT 7 E 2 8 Z 0 1 -
                        FLAGS = !(cpu.regs.file.DE.E & 0b10000000u) << 7 | HALFCARRY_FLAG | (FLAGS & SUBTR_FLAG);
                        break;
                    case 0x7C: // BIT 7 H 2 8 Z 0 1 -
                        FLAGS = !(cpu.regs.file.HL.H & 0b10000000u) << 7 | HALFCARRY_FLAG | (FLAGS & SUBTR_FLAG);
                        break;
                    case 0x7D: // BIT 7 L 2 8 Z 0 1 -
                        FLAGS = !(cpu.regs.file.HL.L & 0b10000000u) << 7 | HALFCARRY_FLAG | (FLAGS & SUBTR_FLAG);
                        break;
                    case 0x7E: // BIT 7 [HL] 2 12 Z 0 1 -
                        numOpsQueued = 1;
                        opQ[0] = readByteAndCmpBit;
                        pQ[0] = 7; pQ[1] = MMAPARR[cpu.regs.file.HL.HL];
                        break;
                    case 0x7F: // BIT 7 A 2 8 Z 0 1 -
                        FLAGS |= (cpu.regs.file.AF.A & 0b10000000u) << 7 | HALFCARRY_FLAG; FLAGS &= ~SUBTR_FLAG;
                        break;
                    case 0x80: // RES 0 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b11111110;
                        break;
                    case 0x81: // RES 0 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b11111110;
                        break;
                    case 0x82: // RES 0 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b11111110;
                        break;
                    case 0x83: // RES 0 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b11111110;
                        break;
                    case 0x84: // RES 0 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b11111110;
                        break;
                    case 0x85: // RES 0 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b11111110;
                        break;
                    case 0x86: // RES 0 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b11111110;
                        break;
                    case 0x87: // RES 0 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b11111110;
                        break;
                    case 0x88: // RES 1 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b11111101;
                        break;
                    case 0x89: // RES 1 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b11111101;
                        break;
                    case 0x8A: // RES 1 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b11111101;
                        break;
                    case 0x8B: // RES 1 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b11111101;
                        break;
                    case 0x8C: // RES 1 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b11111101;
                        break;
                    case 0x8D: // RES 1 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b11111101;
                        break;
                    case 0x8E: // RES 1 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b11111101;
                        break;
                    case 0x8F: // RES 1 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b11111101;
                        break;
                    case 0x90: // RES 2 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b11111011;
                        break;
                    case 0x91: // RES 2 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b11111011;
                        break;
                    case 0x92: // RES 2 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b11111011;
                        break;
                    case 0x93: // RES 2 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b11111011;
                        break;
                    case 0x94: // RES 2 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b11111011;
                        break;
                    case 0x95: // RES 2 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b11111011;
                        break;
                    case 0x96: // RES 2 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b11111011;
                        break;
                    case 0x97: // RES 2 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b11111011;
                        break;
                    case 0x98: // RES 3 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b11110111;
                        break;
                    case 0x99: // RES 3 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b11110111;
                        break;
                    case 0x9A: // RES 3 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b11110111;
                        break;
                    case 0x9B: // RES 3 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b11110111;
                        break;
                    case 0x9C: // RES 3 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b11110111;
                        break;
                    case 0x9D: // RES 3 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b11110111;
                        break;
                    case 0x9E: // RES 3 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b11110111;
                        break;
                    case 0x9F: // RES 3 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b11110111;
                        break;
                    case 0xA0: // RES 4 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b11101111;
                        break;
                    case 0xA1: // RES 4 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b11101111;
                        break;
                    case 0xA2: // RES 4 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b11101111;
                        break;
                    case 0xA3: // RES 4 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b11101111;
                        break;
                    case 0xA4: // RES 4 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b11101111;
                        break;
                    case 0xA5: // RES 4 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b11101111;
                        break;
                    case 0xA6: // RES 4 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b11101111;
                        break;
                    case 0xA7: // RES 4 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b11011111;
                        break;
                    case 0xA8: // RES 5 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b11011111;
                        break;
                    case 0xA9: // RES 5 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b11011111;
                        break;
                    case 0xAA: // RES 5 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b11011111;
                        break;
                    case 0xAB: // RES 5 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b11011111;
                        break;
                    case 0xAC: // RES 5 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b11011111;
                        break;
                    case 0xAD: // RES 5 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b11011111;
                        break;
                    case 0xAE: // RES 5 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b11011111;
                        break;
                    case 0xAF: // RES 5 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b11011111;
                        break;
                    case 0xB0: // RES 6 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b10111111;
                        break;
                    case 0xB1: // RES 6 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b10111111;
                        break;
                    case 0xB2: // RES 6 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b10111111;
                        break;
                    case 0xB3: // RES 6 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b10111111;
                        break;
                    case 0xB4: // RES 6 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b10111111;
                        break;
                    case 0xB5: // RES 6 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b10111111;
                        break;
                    case 0xB6: // RES 6 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b10111111;
                        break;
                    case 0xB7: // RES 6 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b10111111;
                        break;
                    case 0xB8: // RES 7 B 2 8 - - - -
                        REGARR8[REGB_IDX] &= 0b01111111;
                        break;
                    case 0xB9: // RES 7 C 2 8 - - - -
                        REGARR8[REGC_IDX] &= 0b01111111;
                        break;
                    case 0xBA: // RES 7 D 2 8 - - - -
                        REGARR8[REGD_IDX] &= 0b01111111;
                        break;
                    case 0xBB: // RES 7 E 2 8 - - - -
                        REGARR8[REGE_IDX] &= 0b01111111;
                        break;
                    case 0xBC: // RES 7 H 2 8 - - - -
                        REGARR8[REGH_IDX] &= 0b01111111;
                        break;
                    case 0xBD: // RES 7 L 2 8 - - - -
                        REGARR8[REGL_IDX] &= 0b01111111;
                        break;
                    case 0xBE: // RES 7 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = res; pQ[2] = REGZ_IDX; pQ[3] = 0b01111111;
                        break;
                    case 0xBF: // RES 7 A 2 8 - - - -
                        REGARR8[REGA_IDX] &= 0b01111111;
                        break;
                    case 0xC0: // SET 0 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b00000001;
                        break;
                    case 0xC1: // SET 0 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b00000001;
                        break;
                    case 0xC2: // SET 0 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b00000001;
                        break;
                    case 0xC3: // SET 0 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b00000001;
                        break;
                    case 0xC4: // SET 0 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b00000001;
                        break;
                    case 0xC5: // SET 0 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b00000001;
                        break;
                    case 0xC6: // SET 0 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b00000001;
                        break;
                    case 0xC7: // SET 0 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b00000001;
                        break;
                    case 0xC8: // SET 1 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b00000010;
                        break;
                    case 0xC9: // SET 1 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b00000010;
                        break;
                    case 0xCA: // SET 1 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b00000010;
                        break;
                    case 0xCB: // SET 1 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b00000010;
                        break;
                    case 0xCC: // SET 1 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b00000010;
                        break;
                    case 0xCD: // SET 1 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b00000010;
                        break;
                    case 0xCE: // SET 1 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b00000010;
                        break;
                    case 0xCF: // SET 1 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b00000010;
                        break;
                    case 0xD0: // SET 2 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b00000100;
                        break;
                    case 0xD1: // SET 2 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b00000100;
                        break;
                    case 0xD2: // SET 2 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b00000100;
                        break;
                    case 0xD3: // SET 2 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b00000100;
                        break;
                    case 0xD4: // SET 2 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b00000100;
                        break;
                    case 0xD5: // SET 2 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b00000100;
                        break;
                    case 0xD6: // SET 2 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b00000100;
                        break;
                    case 0xD7: // SET 2 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b00000100;
                        break;
                    case 0xD8: // SET 3 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b00001000;
                        break;
                    case 0xD9: // SET 3 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b00001000;
                        break;
                    case 0xDA: // SET 3 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b00001000;
                        break;
                    case 0xDB: // SET 3 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b00001000;
                        break;
                    case 0xDC: // SET 3 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b00001000;
                        break;
                    case 0xDD: // SET 3 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b00001000;
                        break;
                    case 0xDE: // SET 3 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b00001000;
                        break;
                    case 0xDF: // SET 3 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b00001000;
                        break;
                    case 0xE0: // SET 4 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b00010000;
                        break;
                    case 0xE1: // SET 4 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b00010000;
                        break;
                    case 0xE2: // SET 4 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b00010000;
                        break;
                    case 0xE3: // SET 4 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b00010000;
                        break;
                    case 0xE4: // SET 4 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b00010000;
                        break;
                    case 0xE5: // SET 4 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b00010000;
                        break;
                    case 0xE6: // SET 4 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b00010000;
                        break;
                    case 0xE7: // SET 4 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b00010000;
                        break;
                    case 0xE8: // SET 5 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b00100000;
                        break;
                    case 0xE9: // SET 5 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b00100000;
                        break;
                    case 0xEA: // SET 5 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b00100000;
                        break;
                    case 0xEB: // SET 5 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b00100000;
                        break;
                    case 0xEC: // SET 5 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b00100000;
                        break;
                    case 0xED: // SET 5 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b00100000;
                        break;
                    case 0xEE: // SET 5 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b00100000;
                        break;
                    case 0xEF: // SET 5 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b00100000;
                        break;
                    case 0xF0: // SET 6 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b01000000;
                        break;
                    case 0xF1: // SET 6 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b01000000;
                        break;
                    case 0xF2: // SET 6 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b01000000;
                        break;
                    case 0xF3: // SET 6 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b01000000;
                        break;
                    case 0xF4: // SET 6 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b01000000;
                        break;
                    case 0xF5: // SET 6 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b01000000;
                        break;
                    case 0xF6: // SET 6 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b01000000;
                        break;
                    case 0xF7: // SET 6 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b01000000;
                        break;
                    case 0xF8: // SET 7 B 2 8 - - - -
                        REGARR8[REGB_IDX] |= 0b10000000;
                        break;
                    case 0xF9: // SET 7 C 2 8 - - - -
                        REGARR8[REGC_IDX] |= 0b10000000;
                        break;
                    case 0xFA: // SET 7 D 2 8 - - - -
                        REGARR8[REGD_IDX] |= 0b10000000;
                        break;
                    case 0xFB: // SET 7 E 2 8 - - - -
                        REGARR8[REGE_IDX] |= 0b10000000;
                        break;
                    case 0xFC: // SET 7 H 2 8 - - - -
                        REGARR8[REGH_IDX] |= 0b10000000;
                        break;
                    case 0xFD: // SET 7 L 2 8 - - - -
                        REGARR8[REGL_IDX] |= 0b10000000;
                        break;
                    case 0xFE: // SET 7 [HL] 2 16 - - - -
                        numOpsQueued = 2;
                        opQ[1] = readByteToReg; pQ[0] = REGZ_IDX; pQ[1] = MMAPARR[REGARR16[REGHL_IDX]];
                        opQ[0] = set; pQ[2] = REGZ_IDX; pQ[3] = 0b10000000;
                        break;
                    case 0xFF: // SET 7 A 2 8 - - - -
                        REGARR8[REGA_IDX] |= 0b10000000;
                        break;
                }
                cpu.prefixedInstr = false;
                break;
            }
            break;
        case executeInstruction:
            opQ[--numOpsQueued]();
            if (numOpsQueued == 0) {
                cpu.state = fetchOpcode;
                miscOp();
                miscOp = doNothing;
            }
            break;
    }

    // update colour palette if 0xFF47 has been written to
    for (int i = 0; i < 4; i++) {
        colourArr[i] = 0xff000000 | (0x00555555 * (3 - (MMAPARR[0xFF47] >> (i * 2)) & 0b00000011));
    }
    if (numOpsQueued > 0) cpu.state = executeInstruction;
}

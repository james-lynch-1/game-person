#include "opcodes.h"

// adds val to register A (Accumulator)
void add(u8 val) {
    registers.file.AF.A += val;
}

// 16-bit add. 1 extra cycle
void add16(u16 val) {
    registers.file.AF.F = 0;
    // overflow from bit 11: set half carry flag
    // overflow from bit 15: set carry flag
}

// load val into reg at given index into reg array (in handleOpcode). 1 extra cycle
void ldToReg(int destRegIndex, u8 val) {
    registers.arr8[destRegIndex] = val;
}

// load 16 bit val into 16 bit wide reg combo. 2 extra cycles
void ldToReg16(int destRegIndex16, u16 val) {

}

// load val into dest address. 1 extra cycle, 2 if it came from a non-register source (accounted for by caller)
void ldToAddr(u16 dest, u8 val) {
    mMap.mMapArr[dest] = val;
    cycles++;
}

void handleOpcode(FILE* romPtr) {
    u8 byte = fgetc(romPtr);
    u8 val = 0;
    int regIndex[8] = { // for ld operations; array index corresponds to position in opcode table
        /*B*/2, /*C*/3, /*D*/4, /*E*/5, /*H*/6, /*L*/7, /*HL-unused here*/1337, /*A*/0
    };
    switch (byte & 0b11000000) {
        case 0b00000000:
            switch (byte & 0b00111000) {
                case 0b00110000:
                    switch (byte & 0b00110111) {
                        case 0b00110001: printf("ld sp!\n"); break;
                    }
                default: break;
            }
            break;
        case 0b01000000: // LD
            if (byte == 0b01101101) {
                printf("HALT\n");
                break;
            }
            if ((byte & 0b00000110) == 0b00000110) { // dest is register, src is address stored in HL
                val = mMap.mMapArr[registers.file.HL.HL];
                cycles++;
            }
            else
                val = registers.arr8[regIndex[(byte & 0b00000111)]];
            if ((byte & 0b00110000) == 0b00110000) { // dest is address stored in HL, src is register
                ldToAddr(mMap.mMapArr[registers.file.HL.HL], val);
            }
            else ldToReg(regIndex[((byte & 0b00111000) >> 3)], val);
            break;
        case 0b10000000: // 8-bit arithmetic
            break;
        case 0b11000000:
            break;
        default: return;
    }
    cycles++;
}

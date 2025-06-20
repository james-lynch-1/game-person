#ifndef REGISTERS
#define REGISTERS

#include "util.h"

typedef union Registers_ {
    struct {
        union AF_ {
            u16 AF;
            struct { u8 A; u8 F; };
        } AF;
        union BC_ {
            u16 BC;
            struct { u8 B; u8 C; };
        } BC;
        union DE_ {
            u16 DE;
            struct { u8 D; u8 E; };
        } DE;
        union HL_ {
            u16 HL;
            struct { u8 H; u8 L; };
        } HL;
        u16 SP;
        u16 PC;
    } file;
    u16 arr16[6];
    u8 arr8[12];
} Registers;

extern Registers registers;

#endif

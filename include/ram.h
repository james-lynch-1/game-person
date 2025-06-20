#ifndef RAM
#define RAM

#include "util.h"
#include "rom.h"

typedef struct VRam_ {
    u8 vRam[0x2000];
} Vram;

typedef struct EWRam_ {
    u8 eWRam[0x2000];
} EWRam;

typedef struct IWRam_ {
    u8 iWRam[0x1000];
} IWRam;

typedef struct OAM_ {
    u8 oam[0xA0];
} OAM;

typedef struct IORegs_ {
    u8 joypadInput;
    u16 serialTransfer;
    u8 filler1;
    u8 timerAndDivider[4];
    u8 filler2[7];
    u8 interrupts;
    u8 audio[23];
    u8 filler3[10];
    u8 wavePattern[16];
    u8 lcdProps[12];
    u8 filler4[4]; // last byte is used by CGB for VRAM Bank Select
    u8 disableBootrom;
    u8 filler5[44];
} IORegs;

typedef struct HighRam_ {
    u8 hRam[127];
} HighRam;

typedef struct IEReg_ {
    u8 iEReg;
} IEReg;

typedef union MMap_ {
    struct MMap_s_ {
        Rom rom;
        u8 vRam[0x2000];
        u8 eWRam[0x2000];
        u8 iWRam1[0x1000];
        u8 iWRam2[0x1000];
        u8 echoRam[0x1E00];
        u8 oam[0xA0];
        u8 filler[0x60];
        IORegs ioRegs;
        u8 hRam[127];
        u8 iEReg;
    } MMap_s;
    u8 mMapArr[16384];
} MMap;

#endif

#ifndef ROM
#define ROM

#include "util.h"

typedef struct Rom_ {
    u8 bank0[0x4000];
    u8 bank1[0x4000];
} Rom;

typedef struct CartHeader_ {
    u8 entryPoint[0x4];
    u8 nintenLogo[0x30];
    union {
        u8 title[0x10];
        struct {
            u8 filler[0xB];
            u8 code[0x4];
            u8 cgbFlag;
        } manufacturerCodeAndCGBFlag;
    } titleAndCode;
    u8 newLicenseeCode[2];
    u8 sgbFlag;
    u8 cartType;
    u8 romSize;
    u8 ramSize;
    u8 destCode;
    u8 oldLicenseeCode;
    u8 maskRomVer;
    u8 headerChecksum;
    u16 globalChecksum;
} CartHeader;

#endif

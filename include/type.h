#ifndef TYPE
#define TYPE

// handy types
#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define s8  char
#define s16 short
#define s32 int

// helper macros
#define MMAP        mMap.MMap_s // access memory map struct inside union more easily
#define MMAPARR     mMap.mMapArr
#define REGARR8     cpu.regs.arr8
#define REGARR16    cpu.regs.arr16
#define ROM         mMap.MMap_s.rom

// magic addresses
// ROM:
#define ROM_HEADER_ADDR 0x0100
#define ROM_TITLE_ADDR  0x0134

enum CpuState {
    fetchOpcode,
    executeInstruction
};
enum PPUState {
    mode2, mode3, mode0, mode1 // OAM scan, drawing pixels, hBlank, vBlank
};

/* CPU */

// Flags register
#define FLAGS           cpu.regs.file.AF.F
#define ZERO_FLAG       0b10000000
#define SUBTR_FLAG      0b01000000
#define HALFCARRY_FLAG  0b00100000
#define CARRY_FLAG      0b00010000

typedef union Registers_ {
        struct {
            union AF_ {
                u16 AF;
                struct { u8 F; u8 A; }; // f: lower part, a: higher part
            } AF;
            union BC_ {
                u16 BC;
                struct { u8 C; u8 B; };
            } BC;
            union DE_ {
                u16 DE;
                struct { u8 E; u8 D; };
            } DE;
            union HL_ {
                u16 HL;
                struct { u8 L; u8 H; };
            } HL;
            u16 SP;
            u16 PC;
            u8 Z;
            u8 W;
        } file;
        u16 arr16[7];
        u8 arr8[14];
    } Registers;

typedef struct CPU_ {
    Registers regs;
    int ticks;
    enum CpuState state;
    bool prefixedInstr;
} CPU;

// register indices in regs.arr8
#define REGF_IDX    0
#define REGA_IDX    1
#define REGC_IDX    2
#define REGB_IDX    3
#define REGE_IDX    4
#define REGD_IDX    5
#define REGL_IDX    6
#define REGH_IDX    7
#define REGSPLO_IDX 8
#define REGSPHI_IDX 9
#define REGPCLO_IDX 10
#define REGPCHI_IDX 11
#define REGZ_IDX    12
#define REGW_IDX    13

// register indices in regs.arr16
#define REGAF_IDX   0
#define REGBC_IDX   1
#define REGDE_IDX   2
#define REGHL_IDX   3
#define REGSP_IDX   4
#define REGPC_IDX   5
#define REGWZ_IDX   6

/* PPU */

typedef struct PPU_ {
    enum PPUState state;
    u32 ticks;
    u8 LY;
    u8 x;
} PPU;



/* MEMORY MAP */

typedef union Rom_ {
    struct Rom_s_ {
        u8 bank0[0x4000];
        u8 bank1[0x4000];
    } Rom_s;
    u8 bank0_1[0x8000];
} Rom;

typedef union VRam_ {
    struct VRam_s_ {
        u16 block0[0x800];
        u16 block1[0x800];
        u16 block2[0x800];
        u8 map0[0x400];
        u8 map1[0x400];
    } VRam_s;
    u8 VRamArr[0x3800];
} VRam;

#define TILEID(addr)        addr / 16 % 256
#define MAPENTRY_X(addr)    addr % 32
#define MAPENTRY_Y(addr)    addr / 32 % 32

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
    u8 filler4[3];
    u8 cgbVRamBankSelect;
    u8 disableBootrom;
    u8 filler5[44];
} IORegs;

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
    u8 mMapArr[65536];
} MMap;

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

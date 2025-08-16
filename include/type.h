#ifndef TYPE
#define TYPE

#include <stdint.h>

// handy types
#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 uint64_t
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
#define OAM_DMA_ADDR    0xFF46
#define MBC_TYPE_ADDR   0x0147

enum CpuState {
    fetchOpcode, executeInstruction
};
enum PpuState {
    mode2, mode3, mode0, mode1 // OAM scan, drawing pixels, hBlank, vBlank
};
enum FetcherState {
    getTile, getTileDataLow, getTileDataHigh, push
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
        union WZ_ {
            u16 WZ;
            struct { u8 Z; u8 W; };
        } WZ;
    } file;
    u16 arr16[7];
    u8 arr8[14];
} Registers;

typedef struct Cpu_ {
    Registers regs;
    int ticks;
    enum CpuState state;
    int opcode;
    bool prefixedInstr;
    bool ime; // interrupt master enable flag
    int dmaCycle; // current DMA cycle completed
} Cpu;

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

typedef struct OamEntry_ {
    u8 yPos;
    u8 xPos;
    u8 tileIndex;
    u8 attrs;
} OamEntry;

typedef struct Ppu_ {
    enum PpuState state;
    u32 ticks;
    u8 x;
} Ppu;

typedef struct Pixel_ {
    u8 colour;
    u8 palette;
    u8 priority;
    u8 bgPriority;
} Pixel;

typedef struct PixelFIFO_ {
    Pixel pixels[8];
    int front;
    int rear;
} PixelFIFO;

typedef struct Fetcher_ {
    PixelFIFO bgFIFO;
    PixelFIFO objFIFO;
    int ticks;
    enum FetcherState state;
    int y;
    int mapAddr;
    u8 tileID; // addr in MMAPARR
    int tileIndex; // index within map, to be added to addr of tilemap to get tileID
    Pixel tileData[8];

    bool fetchingObj;
    int numObjsFetched;
    OamEntry* currObj;
} Fetcher;



/* MEMORY MAP */

typedef union Rom_ {
    struct Rom_s_ {
        u8 bank0[0x4000];
        u8 bank1[0x4000];
    } Rom_s;
    u8 bank0_1[0x8000];
} Rom;

typedef struct Tile_ {
    u16 row[8];
} Tile;

typedef union VRam_ {
    struct VRam_s_ {
        Tile block0[0x80]; // $8000-$87FF
        Tile block1[0x80]; // $8800-$8FFF
        Tile block2[0x80]; // $9000-$97FF
        u8 map0[0x400]; // $9800-$9BFF
        u8 map1[0x400]; // $9C00-$9FFF
    } VRam_s;
    u8 VRamArr[0x2000];
} VRam;

#define TILEID(addr)        addr / 16 % 256
#define MAPENTRY_X(addr)    addr % 32
#define MAPENTRY_Y(addr)    addr / 32 % 32

typedef struct LcdProps_ {
    u8 LCDC; // $FF40. LCD control
    u8 STAT; // $FF41. LCD status
    u8 SCY; // $FF42. Background viewport Y position
    u8 SCX; // $FF43. Background viewport X position
    u8 LY; // $FF44. LCD Y coordinate (read-only).
    u8 LYC; // $FF45. LY compare
    u8 filler; // $FF46. DMA register (defined elsewhere, don't use from this struct)
    u8 BGP; // $FF47. BG palette data (Non-CGB mode only)
    u8 OBP0; // $FF48. OBJ palette 0 data
    u8 OBP1; // $FF49. OBJ palette 1 data
    u8 WY; // $FF4A. Window Y position
    u8 WX; // $FF4B. Window X position plus 7
} LcdProps;

typedef struct TimerAndDivider_ {
    u8 div; // $FF04 Divider register
    u8 tima; // $FF05 Timer counter
    u8 tma; // $FF06 Timer modulo
    u8 tac; // $FF07 Timer control
} TimerAndDivider;

#define LCDPROPS mMap.MMap_s.ioRegs.lcdProps

// LCDC masks
#define LCD_PPU_ENABLE_MASK         0b10000000
#define WINDOW_TILEMAP_AREA_MASK    0b01000000
#define WINDOW_ENABLE_MASK          0b00100000
#define BG_WDW_TILE_DATA_AREA_MASK  0b00010000
#define BG_TILEMAP_AREA_MASK        0b00001000
#define OBJ_SIZE_MASK               0b00000100
#define OBJ_ENABLE_MASK             0b00000010
#define BG_WDW_ENABLE_PRIORITY_MASK 0b00000001

typedef struct IORegs_ {
    u8 joypadInput; // $FF00
    u8 serialTransfer[2]; // $FF01-$FF02
    u8 filler1; // $FF03
    TimerAndDivider timerAndDivider; // $FF04-$FF07
    u8 filler2[7]; // $FF08-$FF0E
    u8 interrupts; // $FF0F
    u8 audio[23]; // $FF10-$FF26
    u8 filler3[9]; // $FF27-$FF2F
    u8 wavePattern[16]; // $FF30-$FF3F
    LcdProps lcdProps; // $FF40-$FF4B
    u8 filler4[3]; // $FF4C-$FF4E
    u8 cgbVRamBankSelect; // $FF4F
    u8 disableBootrom; // $FF50
    u8 vRamDma[5]; // $FF51-$FF55
    u8 filler5[18]; // $FF56-$FF67
    u8 bgObjPalettes[4]; // $FF68-$FF6B
    u8 filler6[4]; // $FF6C-$FF6F
    u8 wRamBankSelect; // $FF70
    u8 filler7[15]; // $FF71-$FF7F
} IORegs;

// flag masks for both the interrupt flag reg and the interrupt enable reg (same bit positions)
#define INTR_VBLANK 0b00000001
#define INTR_LCD    0b00000010
#define INTR_TIMER  0b00000100
#define INTR_SERIAL 0b00001000
#define INTR_JOYPAD 0b00010000

typedef union MMap_ {
    struct MMap_s_ {
        Rom rom; // $0000-$3FFF (bank 0), $4000-$7FFF (bank 1)
        VRam vRam; // $8000-$9FFF
        u8 eWRam[0x2000]; // $A000-$BFFF
        u8 iWRam1[0x1000]; // $C000-$CFFF
        u8 iWRam2[0x1000]; // $D000-$DFFF
        u8 echoRam[0x1E00]; // $E000-$FDFF
        OamEntry oamEntry[40]; // $FE00-$FE9F
        u8 filler[0x60]; // $FEA0-$FEFF
        IORegs ioRegs; // $FF00-$FF7F
        u8 hRam[127]; // $FF80-$FFFE
        u8 iEReg; // $FFFF
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

#include "ppu.h"

extern void requestInterrupt(int intr);

void getTileData() {
    int hFlip = fetcher.fetchingObj && (fetcher.currObj->attrs & 0b00100000) >> 5;
    int vFlip = fetcher.fetchingObj && (fetcher.currObj->attrs & 0b01000000) >> 6;
    int blockOffset = LCDPROPS.LCDC & BG_WDW_TILE_DATA_AREA_MASK ? 0x8000 : 0x9000;
    int bit, highByteOffset = 0;
    int tileDataIndex;
    int addr = 0x8000 |
        (fetcher.fetchingObj ? 0 : !((LCDPROPS.LCDC & 0x10) || (fetcher.tileID & 0x80)) << 12) |
        ((int)fetcher.tileID << 4) |
        (((fetcher.fetchingObj ? (LCDPROPS.LY - fetcher.currObj->yPos) & 7 :
            fetcher.isWindow ? ppu.windowY & 7 : (LCDPROPS.LY + LCDPROPS.SCY) & 7)) << 1);
    if (fetcher.fetchingObj) {
        if (LCDPROPS.LCDC & OBJ_SIZE_MASK) {
            addr &= ~0b10000;
            addr |= (((LCDPROPS.LY - fetcher.currObj->yPos + 16 >= 8) ^ vFlip) << 4);
        }
        if (vFlip)
            addr ^= 0b1110;
    }
    if ((!fetcher.fetchingObj && (fetcher.state == getTileDataHigh)) || // regular, no obj
        (hFlip && (fetcher.state == getTileDataLow)) || // fetching obj and hflip
        (!hFlip && fetcher.fetchingObj && (fetcher.state == getTileDataHigh))) { // regular, obj
        // do the high byte if we are in gTDHigh, or fetching an hflip'd sprite tile in gTDLow
        highByteOffset = 1;
        addr += 1;
    }
    for (int i = 0; i <= 7; i++) {
        tileDataIndex = hFlip ? 7 - i : i;
        bit = (MMAPARR[addr] >> (7 - i)) & 1;
        fetcher.tileData[tileDataIndex].colour |= bit << highByteOffset;
        if (highByteOffset && fetcher.fetchingObj) {
            fetcher.tileData[tileDataIndex].palette = (fetcher.currObj->attrs & 0b10000) >> 4;
            fetcher.tileData[tileDataIndex].priority = (fetcher.currObj->attrs & 0b10000000) >> 7;
        }
    }
}

// initialise the fetcher at the start of a line or the window
void startFetcher() {
    fetcher.isWindow = ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) &&
        (ppu.x >= LCDPROPS.WX - 7) &&
        (LCDPROPS.LY >= LCDPROPS.WY));
    fetcher.x = 0;
    fetcher.state = getTile;
    fetcher.fetchingObj = false;
    fetcher.currObj = NULL;
    fetcher.ticks = 0;
    fetcher.numFetchedObjs = 0;
    initialiseFIFO(&fetcher.bgFIFO);
    initialiseFIFO(&fetcher.objFIFO);
}

void fetcherTick() {
    if (++fetcher.ticks < 2) return;
    fetcher.ticks = 0;
    Tile* t;
    Pixel p = { 0, 0, 0, 0 };
    int bit0, bit1;
    switch (fetcher.state) { // first four cases take two ticks (dots) each, last can vary
        case getTile:
            memset(fetcher.tileData, 0, 32);
            fetcher.fetchingObj = fetcher.currObj != NULL;
            if (!fetcher.fetchingObj)
                fetcher.mapAddr = 0x9800 |
                (!fetcher.isWindow ?
                    (((LCDPROPS.LCDC & BG_TILEMAP_AREA_MASK) << 7) |
                        ((((LCDPROPS.LY + LCDPROPS.SCY) / 8) & 0x1F) << 5) |
                        ((fetcher.x + LCDPROPS.SCX / 8) & 0x1F)) :
                    (((LCDPROPS.LCDC & WINDOW_TILEMAP_AREA_MASK) << 4) |
                        ((ppu.windowY / 8) << 5) | fetcher.x));
            if (fetcher.fetchingObj)
                fetcher.tileID = fetcher.currObj->tileIndex;
            else
                fetcher.tileID = MMAPARR[fetcher.mapAddr];
            fetcher.state = getTileDataLow;
            break;
        case getTileDataLow:
            getTileData();
            fetcher.state = getTileDataHigh;
            break;
        case getTileDataHigh:
            getTileData();
            if (!fetcher.fetchingObj && ppu.ticks <= 86) // discard first tile
                fetcher.state = getTile;
            else
                fetcher.state = push;
            break;
        case push:
            if (fetcher.fetchingObj) { // merge overlaying obj pixels
                while (!isFullFIFO(&fetcher.objFIFO))
                    enqueue(&fetcher.objFIFO, p);
                for (int i = 0; i < 8; i++) {
                    p = fetcher.tileData[i];
                    if (p.priority > fetcher.objFIFO.pixels[i].priority ||
                        (fetcher.objFIFO.pixels[(fetcher.objFIFO.front + i) % 8].colour == 0))
                        fetcher.objFIFO.pixels[(fetcher.objFIFO.front + i) % 8] = p;
                }
            }
            else {
                if (!isEmptyFIFO(&fetcher.bgFIFO)) break;
                for (int i = 0; i < 8; i++)
                    enqueue(&fetcher.bgFIFO, fetcher.tileData[i]);
                fetcher.x++;
            }
            fetcher.state = getTile;
            if (fetcher.fetchingObj) {
                fetcher.currObj = NULL;
                fetcher.fetchingObj = false;
            }
            break;
    }
}

void switchPpuState(enum PpuState state) {
    switch (state) {
        case mode2:
            ppu.state = mode2;
            cpu.statIntrLine &= ~(STAT_MODE0 | STAT_MODE1);
            cpu.statIntrLine |= LCDPROPS.STAT & STAT_MODE2;
            LCDPROPS.STAT = (LCDPROPS.STAT & 0b11111100) | 0b10;
            numScanlineObjs = 0;
            break;
        case mode3:
            ppu.state = mode3;
            ppu.justEnabled = false;
            cpu.statIntrLine &= ~STAT_MODE2;
            LCDPROPS.STAT |= 0b11;
            ppu.x = 0;
            startFetcher();
            break;
        case mode0:
            ppu.state = mode0;
            cpu.statIntrLine |= LCDPROPS.STAT & STAT_MODE0;
            LCDPROPS.STAT &= 0b11111100;
            if ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) && LCDPROPS.WX && LCDPROPS.WX < 144 && LCDPROPS.LY >= LCDPROPS.WY)
                ppu.windowY++;
            break;
        case mode1:
            ppu.state = mode1;
            cpu.statIntrLine |= LCDPROPS.STAT & STAT_MODE1;
            ppu.windowY = 0;
            LCDPROPS.STAT = (LCDPROPS.STAT & 0b11111100) | 0b01;
            requestInterrupt(INTR_VBLANK);
            break;
    }
}

void ppuTick() {
    if (!(LCDPROPS.LCDC & LCD_PPU_ENABLE_MASK))
        return;
    Pixel bgPixel, objPixel;
    int size, oamEntryNum, offset8x16, adjustedLYObjPos, prevStatIntr = cpu.statIntrLine;

    ppu.ticks++;
    if ((ppu.ticks % 4) == 1) {
        if (LCDPROPS.LY == LCDPROPS.LYC) {
            LCDPROPS.STAT |= STAT_LYC_LY;
            cpu.statIntrLine |= LCDPROPS.STAT & STAT_LYC;
        }
        else {
            LCDPROPS.STAT &= ~STAT_LYC_LY;
            cpu.statIntrLine &= ~STAT_LYC;
        }
    }
    switch (ppu.state) {
        case mode2: // OAM scan
            cpu.statIntrLine &= ~STAT_MODE2;
            cpu.statIntrLine |= LCDPROPS.STAT & STAT_MODE2;
            if ((ppu.ticks % 2 == 1) && (numScanlineObjs < 10)) {
                oamEntryNum = ppu.ticks / 2;
                offset8x16 = ((LCDPROPS.LCDC & OBJ_SIZE_MASK) >> 2) * 8;
                adjustedLYObjPos = LCDPROPS.LY - MMAP.oamEntry[oamEntryNum].yPos + 16;
                if (adjustedLYObjPos >= 0 && adjustedLYObjPos < 8 + offset8x16)
                    scanlineObjs[numScanlineObjs++] = MMAP.oamEntry[oamEntryNum];
            }
            if (ppu.ticks == 80) {
                sortScanlineObjs(scanlineObjs, numScanlineObjs);
                switchPpuState(mode3);
            }
            break;
        case mode3: // drawing pixels
            if (!fetcher.isWindow && (LCDPROPS.LCDC & WINDOW_ENABLE_MASK) &&
                (ppu.x >= LCDPROPS.WX - 7) && (LCDPROPS.LY >= LCDPROPS.WY)) {
                fetcher.isWindow = true;
                fetcher.x = 0;
                fetcher.bgFIFO.front = fetcher.bgFIFO.rear = -1;
                fetcher.state = getTile;
            }
            if ((LCDPROPS.LCDC & OBJ_ENABLE_MASK) && !fetcher.currObj) {
                for (int i = fetcher.numFetchedObjs; i < numScanlineObjs; i++) {
                    if ((scanlineObjs[i].xPos > ppu.x) &&
                        (scanlineObjs[i].xPos - 8 <= ppu.x)) {
                        fetcher.currObj = &scanlineObjs[i];
                        fetcher.numFetchedObjs++;
                        break;
                    }
                }
            }
            if (fetcher.currObj && !fetcher.fetchingObj) {
                // advance the fetcher till it has 8 pixels of bg tile,
                // so we can overlay the sprite we're about to fetch
                if (fetcher.state != push || isEmptyFIFO(&fetcher.bgFIFO)) {
                    fetcherTick();
                    return;
                }
                if (!fetcher.fetchingObj) {
                    fetcher.state = getTile;
                    fetcherTick();
                    return;
                }
                // TODO: do the ppu.x = 0 special case here
            }
            fetcherTick();
            if (isEmptyFIFO(&fetcher.bgFIFO) || fetcher.fetchingObj) break;
            bgPixel = dequeue(&fetcher.bgFIFO);
            objPixel = dequeue(&fetcher.objFIFO);
            if ((ppu.x == 0) && (!fetcher.isWindow) && ((getSizeFIFO(&fetcher.bgFIFO) + 1) > (8 - (LCDPROPS.SCX % 8))))
                break;
            gFrameBuffer[LCDPROPS.LY * 160 + ppu.x] =
                !(LCDPROPS.LCDC & BG_WDW_ENABLE_PRIORITY_MASK) ?
                objPalArr[objPixel.palette][objPixel.colour] :
                !(LCDPROPS.LCDC & OBJ_ENABLE_MASK) ||
                (objPixel.priority && (bgPixel.colour != 0)) ||
                (objPixel.colour == 0) ?
                bgPal[bgPixel.colour] : objPalArr[objPixel.palette][objPixel.colour];
            ppu.x++;
            if (ppu.x == 160)
                switchPpuState(mode0);
            break;
        case mode0: // hBlank
            cpu.statIntrLine &= ~STAT_MODE0;
            cpu.statIntrLine |= LCDPROPS.STAT & STAT_MODE0;
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                LCDPROPS.LY = LCDPROPS.LY + 1 - ppu.justEnabled;
                if (LCDPROPS.LY == 144)
                    switchPpuState(mode1);
                else
                    switchPpuState(mode2 + ppu.justEnabled);
            }
            break;
        case mode1: // vBlank
            cpu.statIntrLine &= ~STAT_MODE1;
            cpu.statIntrLine |= LCDPROPS.STAT & STAT_MODE1;
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                LCDPROPS.LY = (LCDPROPS.LY + 1) % 154;
                if (LCDPROPS.LY == 0)
                    switchPpuState(mode2);
            }
            break;
    }
    if (cpu.statIntrLine && !prevStatIntr)
        requestInterrupt(INTR_LCD);
}

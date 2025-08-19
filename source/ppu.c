#include "ppu.h"

extern void requestInterrupt(int intr);

void getTileData() {
    int hFlip = fetcher.fetchingObj && fetcher.currObj && (fetcher.currObj->attrs & 0b00100000) >> 5;
    int vFlip = fetcher.fetchingObj && fetcher.currObj && (fetcher.currObj->attrs & 0b01000000) >> 6;
    int blockOffset = LCDPROPS.LCDC & BG_WDW_TILE_DATA_AREA_MASK ? 0x8000 : 0x9000;
    int addr, bit0, bit1;
    int tileDataIndex;
    addr = fetcher.fetchingObj ? 0x8000 + fetcher.tileID * 16 :
        blockOffset + (blockOffset == 0x8000 ? (u8)fetcher.tileID * 16 :
            (s8)fetcher.tileID * 16);
    addr ^= vFlip ? 0b1110 : 0;
    Tile* t = (Tile*)&MMAPARR[addr];
    if ((!fetcher.fetchingObj && (fetcher.state == getTileDataHigh)) ||
        (hFlip && (fetcher.state == getTileDataLow)) ||
        (!hFlip && fetcher.fetchingObj && (fetcher.state == getTileDataHigh))) {
        // do the high byte instead
        for (int i = 4; i <= 7; i++) {
            tileDataIndex = hFlip ? 7 - i : i;
            bit0 = (t->row[fetcher.fetchingObj ? (LCDPROPS.LY - fetcher.currObj->yPos) & 7 : fetcher.y % 8] >> (7 - i)) & 1; // lsb
            bit1 = (t->row[fetcher.fetchingObj ? (LCDPROPS.LY - fetcher.currObj->yPos) & 7 : fetcher.y % 8] >> (15 - i)) & 1; // msb
            fetcher.tileData[tileDataIndex].colour = bit0 | (bit1 << 1);
            if (fetcher.fetchingObj) {
                fetcher.tileData[tileDataIndex].palette = fetcher.currObj->attrs & 0b10000 >> 4;
                fetcher.tileData[tileDataIndex].priority = fetcher.currObj->attrs & 0b1000 >> 4;
            }
        }
    }
    else
        for (int i = 0; i <= 3; i++) {
            tileDataIndex = hFlip ? 7 - i : i;
            bit0 = (t->row[fetcher.fetchingObj ? (LCDPROPS.LY - fetcher.currObj->yPos) & 7 : fetcher.y % 8] >> (7 - i)) & 1; // lsb
            bit1 = (t->row[fetcher.fetchingObj ? (LCDPROPS.LY - fetcher.currObj->yPos) & 7 : fetcher.y % 8] >> (15 - i)) & 1; // msb
            fetcher.tileData[tileDataIndex].colour = bit0 | (bit1 << 1);
            if (fetcher.fetchingObj) {
                fetcher.tileData[tileDataIndex].palette = fetcher.currObj->attrs & 0b10000 >> 4;
                fetcher.tileData[tileDataIndex].priority = fetcher.currObj->attrs & 0b1000 >> 4;
            }
        }
    // for (int i = 0; i <= 3; i++) {
    //     bit0 = (t->row[LCDPROPS.LY % 8] >> (7 - i)) & 1; // lsb
    //     bit1 = (t->row[LCDPROPS.LY % 8] >> (15 - i)) & 1; // msb
    //     // hflip
    //     // if (fetcher.fetchingObj && hFlip)
    //     //     fetcher.tileData[3 - i].colour = bit0 | (bit1 << 1);
    //     // else
    //     fetcher.tileData[i].colour = bit0 | (bit1 << 1);
    // }
    // if (fetcher.state == getTileDataLow)
    //     fetcher.state = hFlip ? push : getTileDataHigh;
    // else
    //     fetcher.state = hFlip ? getTileDataLow : push;
}

// initialise the fetcher at the start of a line or the window
void startFetcher() {
    bool isWindow = ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) && (ppu.x > LCDPROPS.WX - 7) && (LCDPROPS.LY > LCDPROPS.WY));
    fetcher.y = isWindow ? LCDPROPS.LY - LCDPROPS.WY : (LCDPROPS.LY + LCDPROPS.SCY) % 256;
    fetcher.tileIndex = fetcher.y / 8 * 32; // index into the map
    fetcher.state = getTile;
    fetcher.fetchingObj = false;
    fetcher.numObjsFetched = 0;
    fetcher.currObj = NULL;
    fetcher.ticks = 0;
    initialiseFIFO(&fetcher.bgFIFO);
    initialiseFIFO(&fetcher.objFIFO);
}

void fetcherTick() {
    if (++fetcher.ticks < 2) return;
    fetcher.ticks = 0;
    bool isWindow = ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) &&
        (ppu.x > LCDPROPS.WX - 7) &&
        (LCDPROPS.LY > LCDPROPS.WY));
    // int blockOffset = LCDPROPS.LCDC & BG_WDW_TILE_DATA_AREA_MASK ? 0x8000 : 0x9000;
    Tile* t;
    Pixel p;
    int bit0, bit1;
    // int addr;
    switch (fetcher.state) { // first four cases take two ticks (dots) each, last can vary
        case getTile:
            if (fetcher.currObj)
                fetcher.fetchingObj = true;
            fetcher.mapAddr = ((LCDPROPS.LCDC & BG_TILEMAP_AREA_MASK) ||
                ppu.x < LCDPROPS.WX - 7 ||
                (LCDPROPS.LY < LCDPROPS.WY)) ||
                isWindow ? 0x9C00 : 0x9800;
            fetcher.tileID = fetcher.fetchingObj ? fetcher.currObj->tileIndex :
                MMAPARR[fetcher.mapAddr + fetcher.tileIndex];
            fetcher.state = getTileDataLow;
            break;
        case getTileDataLow:
            getTileData();
            fetcher.state = getTileDataHigh;
            break;
        case getTileDataHigh:
            getTileData();
            fetcher.state = push;
            break;
        case push:
            for (int i = 0; i < 8; i++)
                enqueue(fetcher.fetchingObj ? &fetcher.objFIFO : &fetcher.bgFIFO, fetcher.tileData[i]);
            fetcher.state = getTile;
            if (!fetcher.fetchingObj)
                fetcher.tileIndex++;
            else {
                fetcher.currObj = NULL;
                fetcher.fetchingObj = false;
            }
            break;
    }
}

void ppuTick() {
    Pixel bgPixel, objPixel;
    int size, oamEntryNum, offset8x16, adjustedLYObjPos;
    ppu.ticks++;
    switch (ppu.state) {
        case mode2: // OAM scan
            if ((ppu.ticks % 2 == 1) && (numScanlineObjs < 10)) {
                oamEntryNum = ppu.ticks / 2;
                offset8x16 = ((LCDPROPS.LCDC & OBJ_SIZE_MASK) >> 2) * 8;
                adjustedLYObjPos = LCDPROPS.LY - MMAP.oamEntry[oamEntryNum].yPos + 16;
                if (adjustedLYObjPos >= 0 && adjustedLYObjPos < 8 + offset8x16) {
                    scanlineObjs[numScanlineObjs++] = MMAP.oamEntry[oamEntryNum];
                }
            }
            if (ppu.ticks == 80) {
                sortScanlineObjs(scanlineObjs, numScanlineObjs);
                ppu.state = mode3;
                LCDPROPS.STAT &= 0b11111100;
                LCDPROPS.STAT |= 0b11;
                ppu.x = 0;
                startFetcher();
            }
            break;
        case mode3: // drawing pixels
            if (!fetcher.currObj) {
                for (int i = fetcher.numObjsFetched; i < numScanlineObjs; i++) {
                    if (scanlineObjs[i].xPos - 8 == ppu.x) {
                        fetcher.currObj = &scanlineObjs[i];
                        fetcher.numObjsFetched++;
                        break;
                    }
                }
            }
            if (fetcher.currObj) {
                // advance the fetcher till it has 8 pixels of bg tile,
                // so we can overlay the sprite we're about to fetch
                if (fetcher.state != push || isEmptyFIFO(&fetcher.bgFIFO)) {
                    fetcherTick();
                    return;
                }
                if (!fetcher.fetchingObj) fetcher.state = getTile;
                // TODO: do the ppu.x = 0 special case here
            }
            fetcherTick();
            if (isEmptyFIFO(&fetcher.bgFIFO) || fetcher.fetchingObj) break;
            bgPixel = dequeue(&fetcher.bgFIFO);
            objPixel = dequeue(&fetcher.objFIFO);
            gFrameBuffer[LCDPROPS.LY * 160 + ppu.x] = objPixel.colour != 0 ?
                objPalArr[objPixel.palette][objPixel.colour] :
                bgPal[bgPixel.colour];
            ppu.x++;
            if (ppu.x == 160) {
                ppu.state = mode0;
                LCDPROPS.STAT &= 0b11111100;
                if (LCDPROPS.STAT & STAT_MODE0)
                    requestInterrupt(INTR_LCD);
            }
            break;
        case mode0: // hBlank
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                numScanlineObjs = 0;
                if (++LCDPROPS.LY == LCDPROPS.LYC) {
                    LCDPROPS.STAT |= STAT_LYC_LY;
                    requestInterrupt(INTR_LCD);
                }
                if (LCDPROPS.LY == 144) {
                    ppu.state = mode1;
                    LCDPROPS.STAT &= 0b11111100;
                    LCDPROPS.STAT |= 0b01;
                    if (LCDPROPS.STAT & STAT_MODE1)
                        requestInterrupt(INTR_LCD);
                    requestInterrupt(INTR_VBLANK);
                }
                else {
                    ppu.state = mode2;
                    LCDPROPS.STAT &= 0b11111100;
                    LCDPROPS.STAT |= 0b10;
                    if (LCDPROPS.STAT & STAT_MODE2)
                        requestInterrupt(INTR_LCD);
                }
            }
            break;
        case mode1: // vBlank
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                if (++LCDPROPS.LY == LCDPROPS.LYC) {
                    LCDPROPS.STAT |= STAT_LYC_LY;
                    requestInterrupt(INTR_LCD);
                }
                if (LCDPROPS.LY == 154) {
                    LCDPROPS.LY = 0;
                    ppu.state = mode2;
                    LCDPROPS.STAT &= 0b11111100;
                    LCDPROPS.STAT |= 0b10;
                    if (LCDPROPS.STAT & STAT_MODE2)
                        requestInterrupt(INTR_LCD);
                    numScanlineObjs = 0;
                }
            }
            break;
    }
}

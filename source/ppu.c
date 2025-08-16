#include "ppu.h"

extern void requestInterrupt(int intr);

// initialise the fetcher at the start of a line or the window
void startFetcher(Fetcher* fetcher) {
    bool isWindow = ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) && (ppu.x > LCDPROPS.WX - 7) && (LCDPROPS.LY > LCDPROPS.WY));
    fetcher->y = isWindow ? LCDPROPS.LY - LCDPROPS.WY : (LCDPROPS.LY + LCDPROPS.SCY) % 256;
    fetcher->tileIndex = fetcher->y / 8 * 32; // index into the map
    fetcher->state = getTile;
    fetcher->fetchingObj = false;
    fetcher->numObjsFetched = 0;
    fetcher->currObj = NULL;
    fetcher->ticks = 0;
    initialiseFIFO(&fetcher->bgFIFO);
    initialiseFIFO(&fetcher->objFIFO);
}

void fetcherTick(Fetcher* fetcher) {
    if (++fetcher->ticks < 2) return;
    fetcher->ticks = 0;
    bool isWindow = ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) &&
        (ppu.x > LCDPROPS.WX - 7) &&
        (LCDPROPS.LY > LCDPROPS.WY));
    int blockOffset = LCDPROPS.LCDC & BG_WDW_TILE_DATA_AREA_MASK ? 0x8000 : 0x9000;
    Tile* t;
    Pixel p;
    int bit0, bit1;
    switch (fetcher->state) { // first four cases take two ticks (dots) each, last can vary
        case getTile:
            if (fetcher->currObj)
                fetcher->fetchingObj = true;
            fetcher->mapAddr = ((LCDPROPS.LCDC & BG_TILEMAP_AREA_MASK) &&
                (ppu.x < LCDPROPS.WX - 7) &&
                (LCDPROPS.LY < LCDPROPS.WY)) ||
                isWindow ? 0x9C00 : 0x9800;
            fetcher->tileID = fetcher->fetchingObj ? fetcher->currObj->tileIndex :
                MMAPARR[fetcher->mapAddr + fetcher->tileIndex];
            fetcher->state = getTileDataLow;
            break;
        case getTileDataLow:
            t = fetcher->fetchingObj ? (Tile*)&MMAPARR[0x8000 + fetcher->tileID * 16] :
                (Tile*)&MMAPARR[blockOffset +
                (blockOffset == 0x8000 ? (u8)fetcher->tileID * 16 : (s8)fetcher->tileID * 16)];
            for (int i = 0; i < 4; i++) {
                bit0 = (t->row[fetcher->fetchingObj ? LCDPROPS.LY % 8 : fetcher->y % 8] >> (7 - i)) & 1; // lsb
                bit1 = (t->row[fetcher->fetchingObj ? LCDPROPS.LY % 8 : fetcher->y % 8] >> (15 - i)) & 1; // msb
                fetcher->tileData[i].colour = bit0 | (bit1 << 1);
            }
            fetcher->state = getTileDataHigh;
            break;
        case getTileDataHigh:
            t = fetcher->fetchingObj ? (Tile*)&MMAPARR[0x8000 + fetcher->tileID * 16] :
                (Tile*)&MMAPARR[blockOffset +
                (blockOffset == 0x8000 ? (u8)fetcher->tileID * 16 : (s8)fetcher->tileID * 16)];
            for (int i = 4; i < 8; i++) {
                bit0 = (t->row[fetcher->fetchingObj ? LCDPROPS.LY % 8 : fetcher->y % 8] >> (7 - i)) & 1; // lsb
                bit1 = (t->row[fetcher->fetchingObj ? LCDPROPS.LY % 8 : fetcher->y % 8] >> (15 - i)) & 1; // msb
                fetcher->tileData[i].colour = bit0 | (bit1 << 1);
            }
            fetcher->state = push;
            break;
        case push:
            for (int i = 0; i < 8; i++)
                enqueue(fetcher->fetchingObj ? &fetcher->objFIFO : &fetcher->bgFIFO, fetcher->tileData[i]);
            fetcher->state = getTile;
            if (!fetcher->fetchingObj)
                fetcher->tileIndex++;
            else {
                fetcher->currObj = NULL;
                fetcher->fetchingObj = false;
            }
            break;
    }
}

void ppuTick() {
    int bgColour, objColour, size, oamEntryNum, offset8x16, adjustedLYObjPos;
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
                ppu.x = 0;
                startFetcher(&fetcher);
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
                    fetcherTick(&fetcher);
                    return;
                }
                // TODO: do the ppu.x = 0 special case here

                if (fetcher.state < getTileDataLow) {
                    fetcherTick(&fetcher);
                    return;
                }
            }
            fetcherTick(&fetcher);
            if (isEmptyFIFO(&fetcher.bgFIFO) || fetcher.fetchingObj) break;
            bgColour = dequeue(&fetcher.bgFIFO).colour;
            objColour = !isEmptyFIFO(&fetcher.objFIFO) ? dequeue(&fetcher.objFIFO).colour : 0;
            gFrameBuffer[LCDPROPS.LY * 160 + ppu.x] = colourArr[
                (objColour != 0) ? objColour : bgColour
            ];
            ppu.x++;
            if (ppu.x == 160)
                ppu.state = mode0;
            break;
        case mode0: // hBlank
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                numScanlineObjs = 0;
                if (++LCDPROPS.LY == LCDPROPS.LYC) {
                    LCDPROPS.STAT |= 0b10;
                    requestInterrupt(INTR_LCD);
                }
                if (LCDPROPS.LY == 144) {
                    ppu.state = mode1;
                    requestInterrupt(INTR_VBLANK);
                }
                else ppu.state = mode2;
            }
            break;
        case mode1: // vBlank
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                if (++LCDPROPS.LY == LCDPROPS.LYC) {
                    LCDPROPS.STAT |= 0b10;
                    requestInterrupt(INTR_LCD);
                }
                if (LCDPROPS.LY == 154) {
                    LCDPROPS.LY = 0;
                    ppu.state = mode2;
                    numScanlineObjs = 0;
                }
            }
            break;
    }
}

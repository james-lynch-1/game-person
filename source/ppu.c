#include "ppu.h"

// initialise the fetcher at the start of a line or the window
void startFetcher(Fetcher* fetcher) {
    bool isWindow = ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) && (ppu.x > LCDPROPS.WX - 7) && (LCDPROPS.LY > LCDPROPS.WY));
    fetcher->mapAddr = ((LCDPROPS.LCDC & BG_TILEMAP_AREA_MASK) && (ppu.x < LCDPROPS.WX - 7) && (LCDPROPS.LY < LCDPROPS.WY)) ||
        isWindow ? 0x9C00 : 0x9800;
    fetcher->y = isWindow ? LCDPROPS.LY - LCDPROPS.WY : (LCDPROPS.LY + LCDPROPS.SCY) % 256;
    fetcher->tileIndex = fetcher->y / 8 * 32; // index into the map
    fetcher->state = getTile;
    fetcher->ticks = 0;
    initialiseFIFO(&fetcher->bgFIFO);
    initialiseFIFO(&fetcher->objFIFO);
}

void fetcherTick(Fetcher* fetcher) {
    if (++fetcher->ticks < 2) return;
    bool isWindow = ((LCDPROPS.LCDC & WINDOW_ENABLE_MASK) && (ppu.x > LCDPROPS.WX - 7) && (LCDPROPS.LY > LCDPROPS.WY));
    int blockOffset = LCDPROPS.LCDC & BG_WDW_TILE_DATA_AREA_MASK ? 0x8000 : 0x8800;
    Tile* t;
    // u16 tileRow;
    Pixel p;
    int bit0, bit1;
    fetcher->ticks = 0;
    switch (fetcher->state) { // first four cases take two ticks (dots) each, last can vary
        case getTile:
            // fetcher->tileID = isWindow ? fetcher->mapAddr + fetcher->tileIndex :
            //     fetcher->mapAddr + fetcher->tileIndex - LCDPROPS.SCX % 8;
            fetcher->tileID = MMAPARR[fetcher->mapAddr + fetcher->tileIndex];
            fetcher->state = getTileDataLow;
            break;
        case getTileDataLow:
            // tileRow = MMAPARR[blockOffset + fetcher->tileID * 16 + ]
            t = (Tile*)&MMAPARR[blockOffset + fetcher->tileID * 16];
            for (int i = 0; i < 4; i++) {
                bit0 = (t->row[fetcher->y % 8] >> (8 + i)) & 1; // lsb
                bit1 = (t->row[fetcher->y % 8] >> i) & 1; // msb
                fetcher->tileData[i].colour = bit0 | (bit1 << 1);
            }
            fetcher->state = getTileDataHigh;
            break;
        case getTileDataHigh:
            t = (Tile*)&MMAPARR[blockOffset + fetcher->tileID * 16];
            for (int i = 4; i < 8; i++) {
                bit0 = (t->row[fetcher->y % 8] >> (8 + i)) & 1; // lsb
                bit1 = (t->row[fetcher->y % 8] >> i) & 1; // msb
                fetcher->tileData[i].colour = bit0 | (bit1 << 1);
            }
            fetcher->state = push;
            break;
        case sleep:
            break;
        case push:
            if (getSizeFIFO(&fetcher->bgFIFO) <= 8) {
                for (int i = 7; i >= 0; i--)
                    enqueue(&fetcher->bgFIFO, fetcher->tileData[i]);
                fetcher->state = getTile;
            }
            fetcher->tileIndex++;
            break;
    }
}

void ppuTick() {
    int colour, size;
    ppu.ticks++;
    switch (ppu.state) {
        case mode2: // OAM scan
            if (ppu.ticks == 80) {
                ppu.state = mode3;
                ppu.x = 0;
                startFetcher(&fetcher);
            }
            break;
        case mode3: // drawing pixels
            fetcherTick(&fetcher);
            size = getSizeFIFO(&(fetcher.bgFIFO));
            if (size <= 8) break;
            colour = dequeue(&(fetcher.bgFIFO)).colour;
            gFrameBuffer[LCDPROPS.LY * 160 + ppu.x] = colourArr[colour];
            frameBufferIter++;
            ppu.x++;
            if (ppu.x == 160)
                ppu.state = mode0;
            break;
        case mode0: // hBlank
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                LCDPROPS.LY++;
                if (LCDPROPS.LY == 144)
                    ppu.state = mode1;
                else ppu.state = mode2;
            }
            break;
        case mode1: // vBlank
            if (ppu.ticks == 456) {
                ppu.ticks = 0;
                LCDPROPS.LY++;
                if (LCDPROPS.LY == 154) {
                    LCDPROPS.LY = 0;
                    ppu.state = mode2;
                }
            }
            break;
    }
}

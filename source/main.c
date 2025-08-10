#include "main.h"
u64 startTime = 0;
u64 frameTime = 0;
bool update() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                handleInput(&e.key);
                break;
            case SDL_EVENT_QUIT:
                return false;
        }
    }

    char* pix;
    int pitch;

    SDL_LockTexture(gSDLTexture, NULL, (void**)&pix, &pitch);
    for (int i = 0, sp = 0, dp = 0; i < 144; i++, dp += 160, sp += pitch)
        memcpy(pix + sp, gFrameBuffer + dp, 160 * 4);

    SDL_UnlockTexture(gSDLTexture);
    SDL_RenderTexture(gSDLRenderer, gSDLTexture, NULL, NULL);
    SDL_RenderPresent(gSDLRenderer);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("no rom file supplied\n");
        exit(1);
    }
    if ((romPtr = fopen(argv[1], "rb")) == NULL) {
        printf("couldn't open file\n");
        exit(1);
    }
    initialiseValues();
    fread(MMAP.rom.bank0_1, 4, 0x2000, romPtr);
    setTitle();

    if (!initialiseVideo()) return -1;

    gDone = 0;
    while (!gDone) {
        loop();
    }

    fclose(romPtr);
    printf("\n");
    return 0;
}

void initialiseLCDProps() {
    LCDPROPS.LCDC = 0b10010011;
    LCDPROPS.LY = 0;
}

void loop() {
    startTime = SDL_GetTicksNS();
    if (!update()) {
        gDone = 1;
    }
    for (int i = 0; i < 70224; i++) {
        cpuTick();
        ppuTick();
        updateTimer();
        cycles++;
    }
    frameTime = SDL_GetTicksNS() - startTime;
    if (frameTime < 16666666) SDL_DelayNS(16666666 - frameTime);
}

void initialiseHardwareRegs() {
    MMAPARR[0xFF00] = 0xCF;
    MMAPARR[0xFF01] = 0x00;
    MMAPARR[0xFF02] = 0x7E;
    MMAPARR[0xFF04] = 0xAB;
    MMAPARR[0xFF05] = 0x00;
    MMAPARR[0xFF06] = 0x00;
    MMAPARR[0xFF07] = 0xF8;
    MMAPARR[0xFF0F] = 0xE1;
    MMAPARR[0xFF10] = 0x80;
    MMAPARR[0xFF11] = 0xBF;
    MMAPARR[0xFF12] = 0xF3;
    MMAPARR[0xFF13] = 0xFF;
    MMAPARR[0xFF14] = 0xBF;
    MMAPARR[0xFF16] = 0x3F;
    MMAPARR[0xFF17] = 0x00;
    MMAPARR[0xFF18] = 0xFF;
    MMAPARR[0xFF19] = 0xBF;
    MMAPARR[0xFF1A] = 0x7F;
    MMAPARR[0xFF1B] = 0xFF;
    MMAPARR[0xFF1C] = 0x9F;
    MMAPARR[0xFF1D] = 0xFF;
    MMAPARR[0xFF1E] = 0xBF;
    MMAPARR[0xFF20] = 0xFF;
    MMAPARR[0xFF21] = 0x00;
    MMAPARR[0xFF22] = 0x00;
    MMAPARR[0xFF23] = 0xBF;
    MMAPARR[0xFF24] = 0x77;
    MMAPARR[0xFF25] = 0xF3;
    MMAPARR[0xFF26] = 0xF1;
    MMAPARR[0xFF40] = 0x91;
    MMAPARR[0xFF41] = 0x85;
    MMAPARR[0xFF42] = 0x00;
    MMAPARR[0xFF43] = 0x00;
    MMAPARR[0xFF44] = 0x00;
    MMAPARR[0xFF45] = 0x00;
    MMAPARR[0xFF46] = 0xFF;
    MMAPARR[0xFF47] = 0xFC;
    MMAPARR[0xFF48] = 0x00;
    MMAPARR[0xFF49] = 0x00;
    MMAPARR[0xFF4A] = 0x00;
    MMAPARR[0xFF4B] = 0x00;
    MMAPARR[0xFF4C] = 0x00;
    MMAPARR[0xFF4D] = 0x7E;
    MMAPARR[0xFF4F] = 0xFE;
    MMAPARR[0xFF50] = 0x00;
    MMAPARR[0xFF51] = 0xFF;
    MMAPARR[0xFF52] = 0xFF;
    MMAPARR[0xFF53] = 0xFF;
    MMAPARR[0xFF54] = 0xFF;
    MMAPARR[0xFF55] = 0xFF;
    MMAPARR[0xFF56] = 0x3E;
    MMAPARR[0xFF68] = 0x00;
    MMAPARR[0xFF69] = 0x00;
    MMAPARR[0xFF6A] = 0x00;
    MMAPARR[0xFF6B] = 0x00;
    MMAPARR[0xFF70] = 0xF8;
    MMAPARR[0xFFFF] = 0x00;
}

void initialiseCpuRegs() {
    cpu.regs.arr16[REGAF_IDX] = 0x01B0;
    cpu.regs.arr16[REGBC_IDX] = 0x0013;
    cpu.regs.arr16[REGDE_IDX] = 0x00D8;
    cpu.regs.arr16[REGHL_IDX] = 0x014D;
    cpu.regs.arr16[REGPC_IDX] = 0x0100;
    cpu.regs.arr16[REGSP_IDX] = 0xFFFE;
}

void initialiseValues() {
    maxFPS = 60;
    scale = 4;
    WINDOW_WIDTH = 160 * scale;
    WINDOW_HEIGHT = 144 * scale;
    colourArr[0] = 0xffffffff;
    colourArr[1] = 0xffaaaaaa;
    colourArr[2] = 0xff555555;
    colourArr[3] = 0xff000000;

    memset(&mMap.MMap_s.rom.bank0_1, 0xFF, 32768);

    cpu.ime = false;
    initialiseCpuRegs();
    initialiseHardwareRegs();
    cpu.ticks = 0;
    cpu.state = fetchOpcode;
    cpu.prefixedInstr = false;
    cpu.dmaCycle = 0;

    ppu.state = mode2;
    ppu.ticks = 0;
    ppu.x = 0;

    initialiseFIFO(&fetcher.bgFIFO);
    initialiseFIFO(&fetcher.objFIFO);
}

void setTitle() {
    getTitle(title, romPtr);
    strcat(title, " - GamePerson");
}

bool initialiseVideo() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return false;
    SDL_SetDefaultTextureScaleMode(gSDLRenderer, SDL_SCALEMODE_NEAREST);
    gFrameBuffer = malloc(160 * 144 * sizeof(int));
    gSDLWindow = SDL_CreateWindow(title, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    gSDLRenderer = SDL_CreateRenderer(gSDLWindow, NULL);
    // SDL_SetRenderVSync(gSDLRenderer, 1);
    SDL_SetDefaultTextureScaleMode(gSDLRenderer, SDL_SCALEMODE_NEAREST);
    gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if (!gFrameBuffer || !gSDLWindow || !gSDLRenderer || !gSDLTexture)
        return false;
    return true;
}

void quitSDL() {
    SDL_DestroyTexture(gSDLTexture);
    SDL_DestroyRenderer(gSDLRenderer);
    SDL_DestroyWindow(gSDLWindow);
    SDL_Quit();
}

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
        updateInputGB();
        updateTimer();
        cycles++;
    }
    frameTime = SDL_GetTicksNS() - startTime;
    if (frameTime < 16666666) SDL_DelayNS(16666666 - frameTime);
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

    cpu.regs.file.PC = 0;
    cpu.ticks = 0;
    cpu.state = fetchOpcode;
    cpu.prefixedInstr = false;
    for (int i = 0; i < 7; i++)
        cpu.regs.arr16[i] = 0;

    ppu.state = mode2;
    ppu.ticks = 0;
    ppu.x = 0;

    initialiseLCDProps();

    mMap.MMap_s.ioRegs.joypadInput = 0xCF;

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

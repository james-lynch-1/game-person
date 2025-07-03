#include "main.h"
u64 startTime = 0;
u64 frameTime = 0;
bool update() {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
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

void loop() {
    startTime = SDL_GetTicksNS();
    for (int i = 0; i < 70224; i++) {
        cpuTick();
        ppuTick();
    }
    if (!update()) {
        gDone = 1;
    }
    frameTime = SDL_GetTicksNS() - startTime;
    if (frameTime < 16666666) SDL_DelayNS(16666666 - frameTime);
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
    // while (1) {
    //     // char pcHex[5];
    //     // decToHex(MMAP.rom.bank0_1[pc], pcHex, 5);
    //     // if (i % 4 == 0) printf("%s ", pcHex);
    //     cpuTick();
    //     // if (i % 4 == 0 && cpuState == fetchOpcode) printf("%x ", MMAP.rom.bank0_1[pc]);
    //     // if (i % 4 == 0 && cpuState == fetchOpcode) printf("%d ", pc);
    // }
    // printf("\n");
    // char regLetters[] = {'F', 'A', 'C', 'B', 'E', 'D', 'L', 'H'};
    // for (int i = 0; i < 8; i++) {
    //     printf("%c: %d\n", regLetters[i], regs.arr8[i]);
    // }

    // return 0;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return -1;

    char title[30] = "";
    getTitle(title, romPtr);
    strncat(title, " - GamePerson", strlen(title) - 1);

    SDL_SetDefaultTextureScaleMode(gSDLRenderer, SDL_SCALEMODE_NEAREST);
    gFrameBuffer = malloc(160 * 144 * sizeof(int));
    gSDLWindow = SDL_CreateWindow(title, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    gSDLRenderer = SDL_CreateRenderer(gSDLWindow, NULL);
    // SDL_SetRenderVSync(gSDLRenderer, 1);
    SDL_SetDefaultTextureScaleMode(gSDLRenderer, SDL_SCALEMODE_NEAREST);
    gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    if (!gFrameBuffer || !gSDLWindow || !gSDLRenderer || !gSDLTexture)
        return -1;

    gDone = 0;
    while (!gDone) {
        loop();
    }
    SDL_DestroyTexture(gSDLTexture);
    SDL_DestroyRenderer(gSDLRenderer);
    SDL_DestroyWindow(gSDLWindow);
    SDL_Quit();

    fclose(romPtr);
    printf("\n");
    return 0;
}

void initialiseLCDProps() {
    LCDPROPS.LCDC = 0b10010011;
    LCDPROPS.LY = 0;
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

    initialiseFIFO(&fetcher.bgFIFO);
    initialiseFIFO(&fetcher.objFIFO);
}

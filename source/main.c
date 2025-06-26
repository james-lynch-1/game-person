#include "main.h"

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
    for (int i = 0, sp = 0, dp = 0; i < WINDOW_HEIGHT; i++, dp += WINDOW_WIDTH, sp += pitch)
        memcpy(pix + sp, gFrameBuffer + dp, WINDOW_WIDTH * 4);

    SDL_UnlockTexture(gSDLTexture);
    SDL_RenderTexture(gSDLRenderer, gSDLTexture, NULL, NULL);
    SDL_RenderPresent(gSDLRenderer);
    SDL_Delay(1);
    return true;
}

void putpixel(int x, int y, int colour) {
    gFrameBuffer[y * WINDOW_WIDTH + x] = colour;
}

void render(Uint64 aTicks) {
    for (int i = 0, c = 0; i < WINDOW_HEIGHT; i++) {
        for (int j = 0; j < WINDOW_WIDTH; j++, c++) {
            gFrameBuffer[c] = (int)(i * i + j * j + aTicks) | 0xff000000;
        }
    }
}

void loop() {
    if (!update()) {
        gDone = 1;
    }
    else {
        render(SDL_GetTicks());
    }
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
    while (1) {
        // char pcHex[5];
        // decToHex(MMAP.rom.bank0_1[pc], pcHex, 5);
        // if (i % 4 == 0) printf("%s ", pcHex);
        cpuTick();
        // if (i % 4 == 0 && cpuState == fetchOpcode) printf("%x ", MMAP.rom.bank0_1[pc]);
        // if (i % 4 == 0 && cpuState == fetchOpcode) printf("%d ", pc);
    }
    printf("\n");
    char regLetters[] = {'F', 'A', 'C', 'B', 'E', 'D', 'L', 'H'};
    for (int i = 0; i < 8; i++) {
        printf("%c: %d\n", regLetters[i], regs.arr8[i]);
    }

    return 0;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return -1;

    char title[30] = "";
    getTitle(title, romPtr);
    strncat(title, " - GamePerson", strlen(title) - 1);

    gFrameBuffer = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(int));
    gSDLWindow = SDL_CreateWindow(title, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    gSDLRenderer = SDL_CreateRenderer(gSDLWindow, NULL);
    gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

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

void initialiseValues() {
    maxFPS = 60;
    scale = 4;
    WINDOW_WIDTH = 160 * scale;
    WINDOW_HEIGHT = 144 * scale;

    pc = 0;
    ticks = 0;
    cpuState = fetchOpcode;
    prefixedInstr = false;
    for (int i = 0; i < 7; i++) {
        regs.arr16[i] = 0;
    }
}

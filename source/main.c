#include "main.h"

int main(int argc, char* argv[]) {
    if (argc == 0) {
        printf("no rom file supplied;\n");
        exit(1);
    }
    FILE* romPtr;
    u8 byte;
    char buffer[3];
    if ((romPtr = fopen(argv[1], "rb")) == NULL) {
        printf("couldn't open file\n");
        exit(1);
    }
    fseek(romPtr, 0L, SEEK_END);
    int romSize = ftell(romPtr);
    fseek(romPtr, 0L, SEEK_SET);
    for (int i = 0; i < romSize / 16; i++) {
        for (int j = 0; j < 16; j++) {
            fread(&byte, 1, 1, romPtr);
            printf("0x%s ", decToHex(byte, buffer, 3));
        }
        printf("\n");
    }
    fclose(romPtr);
    return 0;
}

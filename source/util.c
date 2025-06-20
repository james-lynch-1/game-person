#include "util.h"

// supplied buffer should be of size: length of bin number + 1 (for null terminator)
char* decToBin(int num, char* buffer, int size) {
    for (int i = 0; i < size - 1; i++) {
        buffer[i] = ((num >> (size - i - 2)) & 1) + 48;
    }
    buffer[size - 1] = '\0';
    return buffer;
}

// supplied buffer should be of size: length of hex number + 1 (for null terminator)
char* decToHex(int num, char* buffer, int size) {
    char hexChars[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
    for (int i = 0; i < size - 1; i++)
        buffer[size - i - 2] = hexChars[(num >> (i * 4)) & 15];
    buffer[size - 1] = '\0';
    return buffer;
}

// supplied buffer should be 16 characters
void getTitle(char* buffer, FILE* romPtr) {
    fseek(romPtr, ROM_TITLE_ADDR, SEEK_SET);
    fgets(buffer, 16, romPtr);
}

void printRom(FILE* romPtr) {
    u8 byte;
    char buffer[3];
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
}

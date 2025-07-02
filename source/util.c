#include "util.h"

// PixelFIFO functions

void initialiseFIFO(PixelFIFO* FIFO) {
    Pixel p = { 0, 0, 0, 0 };
    for (int i = 0; i < 16; i++)
        FIFO->pixels[i] = p;
    FIFO->front = FIFO->rear = -1;
}

int getSizeFIFO(PixelFIFO* FIFO) {
    int size = FIFO->rear - FIFO->front;
    if (size < 0) size += 16;
    return ++size;
}

bool isEmptyFIFO(PixelFIFO* FIFO) {
    return (FIFO->front == -1);
}

bool isFullFIFO(PixelFIFO* FIFO) {
    return ((FIFO->rear + 1) % 16 == FIFO->front);
}

void enqueue(PixelFIFO* FIFO, Pixel p) {
    if (isFullFIFO(FIFO)) return;
    // if (isEmptyFIFO(FIFO)) {
    //     FIFO->front = 0;
    //     FIFO->rear = 0;
    // }
    if (FIFO->front == -1) FIFO->front = 0;
    FIFO->rear = (FIFO->rear + 1) % 16;
    FIFO->pixels[FIFO->rear] = p;
}

Pixel dequeue(PixelFIFO* FIFO) {
    if (isEmptyFIFO(FIFO)) {
        Pixel p = { 0, 0, 0, 0 }; return p;
    }
    Pixel data = FIFO->pixels[FIFO->front];
    if (FIFO->front == FIFO->rear)
        FIFO->front = FIFO->rear = -1;
    else
        FIFO->front = (FIFO->front + 1) % 16;
    return data;
}

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

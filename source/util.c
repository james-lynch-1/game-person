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

#include "main.h"

int main(int argc, char* argv[]) {
    if (argc == 0) {
        printf("no rom file supplied;\n");
        exit(1);
    }
    FILE* romPtr;
    if ((romPtr = fopen(argv[1], "rb")) == NULL) {
        printf("couldn't open file\n");
        exit(1);
    }
    
    printRom(romPtr);

    char title[16];
    getTitle(title, romPtr);
    printf("%s\n", title);
    
    fclose(romPtr);
    return 0;
}

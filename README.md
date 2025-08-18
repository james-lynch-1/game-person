# GamePerson - Game Boy emulator written in C
A Game Boy emulator  
Work in progress. Reaches the Tetris title screen:  
![tetris](https://github.com/user-attachments/assets/7062924b-db58-49ba-9d12-22de96daf4c3)  
## Implemented:
All CPU instructions implemented at a micro-operation level; preserves timing (Blargg CPU tests passed)  
Basic PPU (Pixel Processing Unit) implementation  
Basic input, timer, interrupts implementation  
## To do:
Finish PPU  
Basic Memory Bank Controller (MBC) support  
Implement audio  
Game Boy Color support  
## Libraries used:
[SDL (Simple DirectMedia Layer)](https://github.com/libsdl-org/SDL) for input, display, sound, OS-level stuff  
## Resources used:
[Ultimate Game Boy Talk](https://youtu.be/HyzD8pNlpwI)  
[Pan Docs](https://gbdev.io/pandocs)  
[GB Opcode table](https://gbdev.io/gb-opcodes/optables)  
[GB Opcode reference](https://rgbds.gbdev.io/docs/v0.9.3/gbz80.7)  
[Game Boy: Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf)  
[Gameboy Doctor](https://github.com/robert/gameboy-doctor)  
gbdev Discord  
## Building (on Linux using CMake):
1. Clone the repository:  
```
git clone https://github.com/james-lynch-1/game-person.git
```
2. Clone the SDL repository:  
```
git clone https://github.com/libsdl-org/SDL.git vendored/SDL
```
3. Build the application:
```
cmake -S . -B build  
cmake --build build
```
(For debug flags, use this command:)
```
cmake -DCMAKE_BUILD_TYPE=Debug build
```
4. Run:
```
./build/gp <path-to-ROM>
```
If you compiled with debug flags, run:
```
./build/Debug/gp <path-to-ROM>
```

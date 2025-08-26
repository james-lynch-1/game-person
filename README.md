# GamePerson - Game Boy emulator written in C
A Game Boy emulator  
Work in progress. Behold Tetris:  
![t](https://github.com/user-attachments/assets/1c979e19-da24-4b79-b3cd-824bd1abce01)

## Implemented:
All CPU instructions implemented at a micro-operation level; preserves timing (Blargg CPU tests passed)  
Basic PPU (Pixel Processing Unit) implementation  
Basic input, timer, interrupts implementation  
Basic Memory Bank Controller (MBC1) support  
## To do:
Finish PPU  
Pass all mooneye tests  
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
[Blargg tests](https://gbdev.gg8.se/files/roms/blargg-gb-tests/)  
[Gameboy Doctor](https://github.com/robert/gameboy-doctor) for testing output of Blargg cpu instruction tests  
[Mooneye Test Suite](https://github.com/Gekkio/mooneye-test-suite)  
[Gameboy Emulator Development Guide](https://github.com/Hacktix/GBEDG)  
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

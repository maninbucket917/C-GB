#include <stdio.h>

#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "opcodes.h"

int main(int argc, char *argv[]){
    
    // Check for correct usage
    if (argc != 2) {
        printf("Usage: %s rom.gb\n", argv[0]);
        return 1;
    }

    // Initialize CPU
    CPU cpu;
    int step_cycles;
    cpu_init(&cpu);

    // Initialize memory
    Memory mem;
    mem_init(&mem);

    // Initialize PPU
    PPU ppu;
    ppu_init(&ppu);

    // Load ROM into memory
    if(mem_rom_load(&mem, argv[1]) == -1){
        printf("Failed to read ROM: %s\n", argv[1]);
        exit(1);
    }
    
    SDL_Event event;

    int running = 1;
    while (running) {

        // Poll events once per frame
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        uint32_t frame_start = SDL_GetTicks();
        int frame_cycles = CYCLES_PER_FRAME;

        while (frame_cycles > 0 && running) {

            // Execute one instruction
            step_cycles = cpu_step(&cpu, &mem);
            if (step_cycles == -1) {running = 0; break;}  

            // Advance PPU
            ppu_step(&ppu, &mem, step_cycles);

            // Advance timers
            mem_timer_update(&mem, step_cycles);

            // Decrement frame count
            frame_cycles -= step_cycles;
        }

        // Cap to 60FPS
        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_MS) SDL_Delay(FRAME_MS - frame_time);
    }

    SDL_DestroyTexture(ppu.texture);
    SDL_DestroyRenderer(ppu.renderer);
    SDL_DestroyWindow(ppu.window);
    SDL_Quit();

    return 0;
}
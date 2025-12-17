#include <stdio.h>

#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "opcodes.h"


int main(int argc, char *argv[]) {

    // Argument check
    if (argc != 2) {
        printf("Usage: %s path/to/rom.gb\n", argv[0]);
        return 1;
    }

    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize core components
    CPU cpu;
    PPU ppu;
    Memory mem;

    cpu_init(&cpu);
    ppu_init(&ppu);
    mem_init(&mem);

    cpu.ppu = &ppu;
    cpu.memory = &mem;

    uint8_t joypad_state = 0xFF;
    mem.joypad_state = &joypad_state;

    // Load ROM
    if (mem_rom_load(&mem, argv[1]) == -1) {
        printf("Failed to read ROM: %s\n", argv[1]);
        return 1;
    }

    // Timing constants
    uint64_t perf_freq = SDL_GetPerformanceFrequency();
    uint64_t start_counter = SDL_GetPerformanceCounter();
    double next_frame_time = 0.0;

    // FPS tracking
    uint64_t fps_timer = start_counter;
    int fps_frames = 0;
    double fps = 0.0;
    char title[128];

    SDL_Event event;
    int running = 1;

    // Main loop
    while (running) {

        // SDL event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {running = 0; break;}

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                int pressed = (event.type == SDL_KEYDOWN);

                switch (event.key.keysym.sym) {

                    // D-pad
                    case BUTTON_RIGHT: joypad_state = pressed ? (joypad_state & ~0x01) : (joypad_state | 0x01); break;
                    case BUTTON_LEFT: joypad_state = pressed ? (joypad_state & ~0x02) : (joypad_state | 0x02); break;
                    case BUTTON_UP: joypad_state = pressed ? (joypad_state & ~0x04) : (joypad_state | 0x04); break;
                    case BUTTON_DOWN: joypad_state = pressed ? (joypad_state & ~0x08) : (joypad_state | 0x08); break;

                    // Buttons
                    case BUTTON_A: joypad_state = pressed ? (joypad_state & ~0x10) : (joypad_state | 0x10); break;
                    case BUTTON_B: joypad_state = pressed ? (joypad_state & ~0x20) : (joypad_state | 0x20); break;
                    case BUTTON_SELECT: joypad_state = pressed ? (joypad_state & ~0x40) : (joypad_state | 0x40); break;
                    case BUTTON_START: joypad_state = pressed ? (joypad_state & ~0x80) : (joypad_state | 0x80); break;

                    // Extra functions
                    case BUTTON_RESET:

                        if(event.key.repeat || !pressed) break;

                        // Reset state
                        cpu_init(&cpu);
                        mem_init(&mem);
                        ppu_reset(&ppu);
                        cpu.ppu = &ppu;
                        cpu.memory = &mem;
                        mem.joypad_state = &joypad_state;

                        // Reload ROM
                        mem_rom_load(&mem, argv[1]);

                        break;
                    case BUTTON_PALETTE_SWAP: if(event.key.repeat || !pressed) break; ppu_palette_swap(&ppu); break;
                }
            }
        }

        // Emulate 1 frame
        cpu.frame_cycles = CYCLES_PER_FRAME;
        while (cpu.frame_cycles > 0) {
            int cycles = cpu_step(&cpu, &mem);
            if (cycles < 0) {running = 0; break;}
        }

        // Update FPS counter
        fps_frames++;
        uint64_t now_counter = SDL_GetPerformanceCounter();
        double fps_elapsed = (double)(now_counter - fps_timer) / perf_freq;

        if (fps_elapsed >= 1.0) {
            fps = fps_frames / fps_elapsed;
            snprintf(title, sizeof(title), "C-GB | %.2f FPS", fps);
            SDL_SetWindowTitle(ppu.window, title);

            fps_frames = 0;
            fps_timer = now_counter;
        }

        // Limit performance to ~59.7 FPS
        double now = (double)(now_counter - start_counter) / perf_freq;
        if (now < next_frame_time) {
            double delay = next_frame_time - now;
            SDL_Delay((uint32_t)(delay * 1000.0));
        }

        next_frame_time += FRAME_TIME;
    }

    // Cleanup
    SDL_DestroyTexture(ppu.texture);
    SDL_DestroyRenderer(ppu.renderer);
    SDL_DestroyWindow(ppu.window);
    SDL_Quit();

    return 0;
}
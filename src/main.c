#include <stdio.h>

#include "gb.h"
#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "opcodes.h"


int main(int argc, char *argv[]) {

    // Initialize core components
    GB gb;
    CPU cpu;
    PPU ppu;
    Memory mem;

    Status status;

    // Argument check
    if (argc != 2) {
        printf("Usage: %s path/to/rom.gb\n", argv[0]);
        return ERR_BAD_ARGS;
    }

    // SDL init
    status = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
    if (status != OK) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return status;
    }

    status = GB_init(&gb, &cpu, &ppu, &mem);
    if (status != OK) {
        printf("System initialization error.\n");
        return status;
    }

    uint8_t joypad_state = 0xFF;
    gb.mem -> joypad_state = &joypad_state;

    // Load ROM
    status = mem_rom_load(gb.mem, argv[1]);
    if (status != OK) {
        printf("Failed to read ROM: %s\n", argv[1]);
        return status;
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
    int turbo = 0;

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
                        cpu_init(gb.cpu, &gb);
                        mem_init(gb.mem, &gb);
                        ppu_reset(gb.ppu);
                        gb.mem -> joypad_state = &joypad_state;

                        // Reload ROM
                        mem_rom_load(gb.mem, argv[1]);

                        break;
                    case BUTTON_PALETTE_SWAP: if(event.key.repeat || !pressed) break; ppu_palette_swap(gb.ppu); break;
                    case BUTTON_TURBO: if(event.key.repeat || !pressed) break; turbo ^= 1; break;
                }
            }
        }

        // Emulate 1 frame
        gb.cpu -> frame_cycles = CYCLES_PER_FRAME;
        while (gb.cpu -> frame_cycles > 0) {
            cpu_step(gb.cpu, gb.mem);
        }

        // Update FPS counter
        fps_frames++;
        uint64_t now_counter = SDL_GetPerformanceCounter();
        double fps_elapsed = (double)(now_counter - fps_timer) / perf_freq;

        if (fps_elapsed >= 1.0) {
            fps = fps_frames / fps_elapsed;
            snprintf(title, sizeof(title), "C-GB | %.2f FPS", fps);
            SDL_SetWindowTitle(gb.ppu -> window, title);

            fps_frames = 0;
            fps_timer = now_counter;
        }

        // Limit performance to ~59.7 FPS when not in turbo mode
        double now = (double)(now_counter - start_counter) / perf_freq;
        if (!turbo) {
            if (now < next_frame_time) {
                double delay = next_frame_time - now;
                SDL_Delay((uint32_t)(delay * 1000.0));
            }
            next_frame_time += FRAME_TIME;
        } else {
            next_frame_time = now;
        }
    }

    // Cleanup
    SDL_DestroyTexture(gb.ppu -> texture);
    SDL_DestroyRenderer(gb.ppu -> renderer);
    SDL_DestroyWindow(gb.ppu -> window);
    SDL_Quit();

    return 0;
}
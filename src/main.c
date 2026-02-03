#include <stdio.h>

#include "cpu.h"
#include "gb.h"
#include "memory.h"
#include "opcodes.h"
#include "ppu.h"

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
    if (status != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return status;
    }

    status = GB_init(&gb, &cpu, &ppu, &mem);
    if (status != OK) {
        printf("System initialization error.\n");
        return status;
    }

    // Load ROM
    status = mem_rom_load(gb.mem, argv[1]);
    if (status != OK) {
        printf("Failed to read ROM: %s\n", argv[1]);
        return status;
    }

    // Timing constants
    uint64_t perf_freq = SDL_GetPerformanceFrequency();
    double perf_freq_inv = 1.0 / (double)perf_freq; // Precalculate inverse of perf_freq to avoid
                                                    // doing extra division per frame
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
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            }

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                int pressed = (event.type == SDL_KEYDOWN);

                switch (event.key.keysym.sym) {

                // D-pad
                case BUTTON_RIGHT:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x01) : (gb.joypad_state | 0x01);
                    break;
                case BUTTON_LEFT:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x02) : (gb.joypad_state | 0x02);
                    break;
                case BUTTON_UP:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x04) : (gb.joypad_state | 0x04);
                    break;
                case BUTTON_DOWN:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x08) : (gb.joypad_state | 0x08);
                    break;

                // Buttons
                case BUTTON_A:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x10) : (gb.joypad_state | 0x10);
                    break;
                case BUTTON_B:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x20) : (gb.joypad_state | 0x20);
                    break;
                case BUTTON_SELECT:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x40) : (gb.joypad_state | 0x40);
                    break;
                case BUTTON_START:
                    gb.joypad_state =
                        pressed ? (gb.joypad_state & ~0x80) : (gb.joypad_state | 0x80);
                    break;

                // Extra functions
                case BUTTON_RESET:

                    if (event.key.repeat || !pressed)
                        break;

                    // Reset state
                    cpu_init(gb.cpu, &gb);
                    mem_init(gb.mem, &gb);
                    ppu_reset(gb.ppu);

                    // Reload ROM
                    mem_rom_load(gb.mem, argv[1]);

                    break;
                case BUTTON_PALETTE_SWAP:
                    if (event.key.repeat || !pressed)
                        break;
                    ppu_palette_swap(gb.ppu);
                    break;
                case BUTTON_TURBO:
                    if (event.key.repeat || !pressed)
                        break;
                    gb.turbo ^= 1;
                    break;
                case BUTTON_PAUSE:
                    if (event.key.repeat || !pressed)
                        break;
                    gb.paused ^= 1;
                    break;
                }
            }
        }

        // Emulate 1 frame
        gb.cpu->frame_cycles = gb.paused ? 0 : CYCLES_PER_FRAME;
        while (gb.cpu->frame_cycles > 0) {
            cpu_step(gb.cpu, gb.mem);
        }

        // Update FPS counter
        fps_frames++;
        uint64_t now_counter = SDL_GetPerformanceCounter();
        double fps_elapsed = (double)(now_counter - fps_timer) * perf_freq_inv;

        if (fps_elapsed >= 1.0) {
            fps = fps_frames / fps_elapsed;
            snprintf(title, sizeof(title), "C-GB | %.2f FPS", fps);
            SDL_SetWindowTitle(gb.ppu->window, title);

            fps_frames = 0;
            fps_timer = now_counter;
        }

        // Limit performance to ~59.7 FPS when not in turbo mode
        double now = (double)(now_counter - start_counter) * perf_freq_inv;
        if (!gb.turbo) {
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
    SDL_DestroyTexture(gb.ppu->texture);
    SDL_DestroyRenderer(gb.ppu->renderer);
    SDL_DestroyWindow(gb.ppu->window);
    SDL_Quit();

    return 0;
}
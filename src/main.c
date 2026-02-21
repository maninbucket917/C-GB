#include <stdio.h>

#include "cpu.h"
#include "gb.h"
#include "memory.h"
#include "ppu.h"
#include "keybinds.h"

int main(int argc, char *argv[]) {

    // Initialize core components
    GB gb;
    CPU cpu;
    PPU ppu;
    Memory mem;

    Status status;

    // Argument check
    if (argc > 2) {
        printf("Usage: %s [path/to/rom.gb]\n", argv[0]);
        printf("Or drag and drop a ROM file onto the window.\n");
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
        SDL_Quit();
        return status;
    }

    // Load ROM if provided as argument
    if (argc == 2) {
        status = GB_load_rom(&gb, argv[1]);
        if (status != OK) {
            printf("Failed to read ROM: %s\n", argv[1]);
            printf("Drag and drop a .gb file onto the window to load a ROM.\n");
        } else {
            printf("ROM loaded: %s\n", argv[1]);
        }
    } else {
        printf("No ROM loaded. Drag and drop a .gb file onto the window.\n");
    }

    // Timing constants
    uint64_t perf_freq = SDL_GetPerformanceFrequency();
    double perf_freq_inv = 1.0 / (double) perf_freq; // Precalculate inverse of perf_freq to avoid doing extra division per frame
    uint64_t start_counter = SDL_GetPerformanceCounter();
    double next_frame_time = 0.0;

    // FPS tracking
    uint64_t fps_timer = start_counter;
    int fps_frames = 0;
    double fps = 0.0;
    char title[128];

    // Keybinds for menu
    Keybinds keybinds = {
        .up = SDLK_UP,
        .down = SDLK_DOWN,
        .left = SDLK_LEFT,
        .right = SDLK_RIGHT,
        .a = SDLK_z,
        .b = SDLK_x,
        .start = SDLK_RETURN,
        .select = SDLK_RSHIFT,
        .keybinds_menu = SDLK_F1,
        .reset = SDLK_F2,
        .palette_swap = SDLK_F3,
        .turbo = SDLK_F4,
        .pause = SDLK_F5,
        .fullscreen = SDLK_F11
    };

    // Attempt to read keybinds from file
    load_keybinds(&keybinds);

    SDL_Event event;
    int running = 1;
    int fullscreen = 0;

    // Main loop
    while (running) {

        // SDL event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            }

            if (event.type == SDL_DROPFILE) {
                char *dropped_file = event.drop.file;
                printf("Loading ROM: %s\n", dropped_file);

                // Load new ROM
                status = GB_load_rom(&gb, dropped_file);
                if (status != OK) {
                    printf("Failed to load ROM: %s\n", dropped_file);
                } else {
                    printf("ROM loaded successfully\n");
                }

                SDL_free(dropped_file);
            }

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                int pressed = (event.type == SDL_KEYDOWN);

                // Dynamic keybinds
                if (event.key.keysym.sym == keybinds.right)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x01) : (gb.joypad_state | 0x01);
                if (event.key.keysym.sym == keybinds.left)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x02) : (gb.joypad_state | 0x02);
                if (event.key.keysym.sym == keybinds.up)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x04) : (gb.joypad_state | 0x04);
                if (event.key.keysym.sym == keybinds.down)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x08) : (gb.joypad_state | 0x08);
                if (event.key.keysym.sym == keybinds.a)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x10) : (gb.joypad_state | 0x10);
                if (event.key.keysym.sym == keybinds.b)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x20) : (gb.joypad_state | 0x20);
                if (event.key.keysym.sym == keybinds.select)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x40) : (gb.joypad_state | 0x40);
                if (event.key.keysym.sym == keybinds.start)
                    gb.joypad_state = pressed ? (gb.joypad_state & ~0x80) : (gb.joypad_state | 0x80);
                if (event.key.keysym.sym == keybinds.keybinds_menu) {
                    if (event.key.repeat || !pressed) {
                        break;
                    }
                    show_keybind_menu(&keybinds);

                    // Resync timers after returning from menu to avoid large time jumps
                    start_counter = SDL_GetPerformanceCounter();
                    next_frame_time = 0.0;
                    fps_timer = start_counter;
                }
                if (event.key.keysym.sym == keybinds.reset) {
                    if (event.key.repeat || !pressed) {
                        break;
                    }
                    if (!gb.rom_loaded) {
                        printf("No ROM loaded to reset\n");
                        break;
                    }
                    cpu_init(gb.cpu, &gb);
                    mem_init(gb.mem, &gb);
                    ppu_reset(gb.ppu);
                    printf("Emulator reset\n");
                }
                if (event.key.keysym.sym == keybinds.palette_swap) {
                    if (event.key.repeat || !pressed) {
                        break;
                    }
                    ppu_palette_swap(gb.ppu);
                }
                if (event.key.keysym.sym == keybinds.turbo) {
                    if (event.key.repeat || !pressed) {
                        break;
                    }
                    gb.turbo ^= 1;
                }
                if (event.key.keysym.sym == keybinds.pause) {
                    if (event.key.repeat || !pressed) {
                        break;
                    }
                    gb.paused ^= 1;
                }
                if (event.key.keysym.sym == keybinds.fullscreen) {
                    if (event.key.repeat || !pressed) {
                        break;
                    }
                    fullscreen ^= 1;
                    if (SDL_SetWindowFullscreen(gb.ppu->window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
                        printf("Warning: Failed to toggle fullscreen: %s\n", SDL_GetError());
                        fullscreen ^= 1;
                    }
                    SDL_RenderSetIntegerScale(gb.ppu->renderer, SDL_TRUE);
                    SDL_RenderSetLogicalSize(gb.ppu->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
                }
            }
        }

        // Only run emulation if ROM is loaded
        if (gb.rom_loaded) {
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
    }

    // Save keybinds to file on exit
    save_keybinds(&keybinds);

    // Cleanup
    SDL_DestroyTexture(gb.ppu->texture);
    SDL_DestroyRenderer(gb.ppu->renderer);
    SDL_DestroyWindow(gb.ppu->window);
    SDL_Quit();

    return 0;
}
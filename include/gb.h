#ifndef GB_H
#define GB_H

#include "config.h"
#include <stdint.h>

// Forward declarations of components
typedef struct CPU CPU;
typedef struct PPU PPU;
typedef struct Memory Memory;

typedef struct GB {
    // Components
    CPU *cpu;
    PPU *ppu;
    Memory *mem;

    // State
    uint8_t joypad_state;
    int turbo;
    int paused;
    int rom_loaded;
} GB;

// Initialization

Status GB_init(GB *gb, CPU *cpu, PPU *ppu, Memory *mem);

// ROM loading

Status GB_load_rom(GB *gb, const char *filepath);

#endif
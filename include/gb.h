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

    // Current state
    uint8_t joypad_state;
    int turbo;
    int paused;
} GB;

Status GB_init(GB *gb, CPU *cpu, PPU *ppu, Memory *mem);

#endif
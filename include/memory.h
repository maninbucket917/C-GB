#include <stdint.h>

#include "config.h"

typedef struct CPU CPU;

#ifndef MEMORY_H
#define MEMORY_H

typedef struct Memory {
    uint8_t rom0[ROM_BANK_0_SIZE];      // 0000–3FFF
    uint8_t romN[ROM_BANK_N_SIZE];      // 4000–7FFF
    uint8_t vram[VRAM_SIZE];            // 8000–9FFF
    uint8_t eram[ERAM_SIZE];            // A000–BFFF
    uint8_t wram0[WRAM_BANK_0_SIZE];    // C000–CFFF
    uint8_t wram1[WRAM_BANK_1_SIZE];    // D000–DFFF
    uint8_t oam[OAM_SIZE];              // FE00–FE9F
    uint8_t io[IO_REGISTERS_SIZE];      // FF00–FF7F
    uint8_t hram[HRAM_SIZE];            // FF80–FFFE
    uint8_t ie;                         // FFFF

    uint16_t div_internal;
    uint32_t timer_counter;
    uint8_t  tima_reload_delay;
    uint8_t tima_overflowed;
    uint8_t pending;
} Memory;

// ROM loading

int mem_rom_load(Memory * mem, const char * filename);


// Initialization

void mem_init(Memory * mem);


// Memory read/write

uint8_t mem_read8(Memory * mem, uint16_t addr);
void mem_write8(Memory * mem, uint16_t addr, uint8_t value);
uint16_t mem_read16(Memory * mem, uint16_t addr);
void mem_write16(Memory * mem, uint16_t addr, uint16_t value);


// Stack push/pop

void push8(CPU * cpu, Memory * mem, uint8_t value);
void push16(CPU * cpu, Memory * mem, uint16_t value);
uint8_t pop8(CPU * cpu, Memory * mem);
uint16_t pop16(CPU * cpu, Memory * mem);


// Timer update

void mem_timer_update(Memory *mem, int cycles);

#endif
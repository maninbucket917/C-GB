#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#include "config.h"


typedef struct CPU CPU;
typedef struct GB GB;

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
    uint8_t tima_reload_delay;

    uint8_t * joypad_state;

    // Pointer to parent struct
    GB * gb;
} Memory;

// ROM loading

Status mem_rom_load(Memory * mem, const char * filename);


// Initialization

Status mem_init(Memory * mem, GB * gb);

// -----------------
// Memory read/write
// -----------------

// Read an 8-bit value from memory at [addr].
static inline uint8_t mem_read8(Memory * mem, uint16_t addr) {

    // 0000–3FFF: ROM bank 0
    if (addr < 0x4000) {
        return mem -> rom0[addr];
    }

    // 4000–7FFF: ROM bank N
    else if (addr < 0x8000) {
        return mem -> romN[addr - 0x4000];
    }

    // 8000–9FFF: VRAM
    else if (addr < 0xA000) {
        return mem -> vram[addr - 0x8000];
    }

    // A000–BFFF: External RAM
    else if (addr < 0xC000) {
        return mem -> eram[addr - 0xA000];
    }

    // C000–CFFF: WRAM bank 0
    else if (addr < 0xD000) {
        return mem -> wram0[addr - 0xC000];
    }

    // D000–DFFF: WRAM bank 1
    else if (addr < 0xE000) {
        return mem -> wram1[addr - 0xD000];
    }

    // E000–EFFF: Echo of WRAM0
    else if (addr < 0xF000) {
        return mem -> wram0[addr - 0xE000];
    }

    // F000–FDFF: Echo of WRAM1
    else if (addr < 0xFE00) {
        return mem -> wram1[addr - 0xF000];
    }

    // FE00–FE9F: OAM
    else if (addr < 0xFEA0) {
        return mem -> oam[addr - 0xFE00];
    }

    // FEA0–FEFF: unusable region
    else if (addr < 0xFF00) {
        return 0xFF;
    }

    // FF00–FF7F: I/O registers
    else if (addr < 0xFF80) {

        if (addr == 0xFF00) {
            uint8_t select = mem -> io[0x00] & 0x30;
            uint8_t result = 0xCF;

            if (!(select & 0x10)) {
                // D-pad selected
                result &= (*mem -> joypad_state & 0x0F) | 0xF0;
            }
            if (!(select & 0x20)) {
                // Buttons selected
                result &= ((*mem -> joypad_state >> 4) & 0x0F) | 0xF0;
            }

            return result;
        }

        // Force bits 5-7 of IF to high
        if (addr == 0xFF0F) return 0xE0 | mem -> io[0x0F];
        return mem -> io[addr - 0xFF00];
    }

    // FF80–FFFE: HRAM
    else if (addr < 0xFFFF) {
        return mem -> hram[addr - 0xFF80];
    }

    // FFFF: Interrupt Enable
    else {
        return mem -> ie;
    }

}

// Write an 8-bit value [value] to memory at [addr].
static inline void mem_write8(Memory * mem, uint16_t addr, uint8_t value) {

    // Ignore writes to ROM for now (TODO: Implement ROM bank switching)
    if (addr < 0x8000) {
        return;
    }

    // 8000–9FFF: VRAM
    else if (addr < 0xA000) {
        mem -> vram[addr - 0x8000] = value;
    }

    else if (addr < 0xC000) { // ERAM
        mem -> eram[addr - 0xA000] = value;
    }

    else if (addr < 0xD000) { // WRAM0
        mem -> wram0[addr - 0xC000] = value;
    }

    else if (addr < 0xE000) { // WRAM1
        mem -> wram1[addr - 0xD000] = value;
    }

    else if (addr < 0xF000) { // Echo WRAM0
        mem -> wram0[addr - 0xE000] = value;
    }

    else if (addr < 0xFE00) { // Echo WRAM1
        mem -> wram1[addr - 0xF000] = value;
    }

    else if (addr < 0xFEA0) { // OAM
        mem -> oam[addr - 0xFE00] = value;
    }

    else if (addr < 0xFF00) { // unusable
        return;
    }

    else if (addr < 0xFF80) { // VRAM

        // Only bits 4 and 5 are writable
        if (addr == 0xFF00) {
            mem -> io[0x00] = (mem -> io[0x00] & 0xCF) | (value & 0x30);
            return;
        }

        // Writing to FF04 resets DIV
        if (addr == 0xFF04) {
            mem -> div_internal = 0;
            mem -> io[0x04] = 0;
        }

        // Prevent writes to IF from clearing bits 5-7
        if(addr == 0xFF0F) {
            mem -> io[0x0F] = 0xE0 | value;
        }

        // Block writes to LY
        if(addr == 0xFF44) return;

        // DMA transfer
        if (addr == 0xFF46) {
            uint16_t source = value * 0x100;
            for (int i = 0; i < 0xA0; i++) {
                mem -> oam[i] = mem_read8(mem, source + i);
            }
            return;
        }
        
        mem -> io[addr - 0xFF00] = value;
    }

    else if (addr < 0xFFFF) { // HRAM
        mem -> hram[addr - 0xFF80] = value;
    }

    else { // IE
        mem -> ie = value;
    }

}

uint16_t mem_read16(Memory * mem, uint16_t addr);
void mem_write16(Memory * mem, uint16_t addr, uint16_t value);

// Stack push/pop

void push8(CPU * cpu, Memory * mem, uint8_t value);
void push16(CPU * cpu, Memory * mem, uint16_t value);
uint8_t pop8(CPU * cpu, Memory * mem);
uint16_t pop16(CPU * cpu, Memory * mem);


// Timer update

void mem_timer_update(Memory * mem, int cycles);

#endif
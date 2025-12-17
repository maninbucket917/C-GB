#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "memory.h"
#include "cpu.h"

// Load the first 32KB of a ROM file [filename] into memory.
int mem_rom_load(Memory * mem, const char * filename) {

    FILE * rom_file = fopen(filename, "rb");
    if (!rom_file) {
        return -1;
    }

    // Read ROM bank 0 (0000–3FFF)
    size_t read_bytes = fread(mem -> rom0, 1, ROM_BANK_0_SIZE, rom_file);
    if (read_bytes != ROM_BANK_0_SIZE) {
        fclose(rom_file);
        return -1;
    }

    // Read ROM bank N (4000–7FFF)
    read_bytes = fread(mem -> romN, 1, ROM_BANK_N_SIZE, rom_file);
    if (read_bytes != ROM_BANK_N_SIZE) {
        for (size_t i = read_bytes; i < ROM_BANK_N_SIZE; i++) {
            mem -> romN[i] = 0xFF;
        }
    }

    fclose(rom_file);
    return 0;
}

// Initialize memory and set registers to post-BIOS values.
void mem_init(Memory * mem) {

    // Clear memory to 0
    memset(mem, 0, sizeof(Memory));

    mem -> io[0x00] = 0xCF;

    // Timer registers
    mem -> io[0x04] = 0x00; // DIV
    mem -> io[0x05] = 0x00; // TIMA
    mem -> io[0x06] = 0x00; // TMA
    mem -> io[0x07] = 0x00; // TAC

    // Interrupt flag
    mem -> io[0x0F] = 0xE1;

    // LCD / PPU registers
    mem -> io[0x40] = 0x91; // LCDC
    mem -> io[0x41] = 0x85; // STAT
    mem -> io[0x42] = 0x00; // SCY
    mem -> io[0x43] = 0x00; // SCX
    mem -> io[0x44] = 0x00; // LY
    mem -> io[0x45] = 0x00; // LYC

    mem -> io[0x47] = 0xFC; // BGP
    mem -> io[0x48] = 0xFF; // OBP0
    mem -> io[0x49] = 0xFF; // OBP1

    mem -> io[0x4A] = 0x00; // WY
    mem -> io[0x4B] = 0x00; // WX

    // DMA
    mem -> io[0x46] = 0x00; // DMA

    // IE register (FFFF)
    mem -> ie = 0x00;

    // Timer counters
    mem -> div_internal = 0;

}

// Read a 16-bit value from memory starting at [addr].
uint16_t mem_read16(Memory * mem, uint16_t addr) {
    uint8_t low = mem_read8(mem, addr);
    uint8_t high = mem_read8(mem, addr + 1);
    return (high << 8) | low;
}

// Write a 16-bit value [value] in memory starting at [addr].
void mem_write16(Memory * mem, uint16_t addr, uint16_t value) {
    uint8_t low = value & 0xFF;
    uint8_t high = (value >> 8) & 0xFF;
    mem_write8(mem, addr, low);
    mem_write8(mem, addr + 1, high);
}

// Push an 8-bit value [value] onto the stack.
void push8(CPU * cpu, Memory * mem, uint8_t value) {
    mem_write8(mem, --cpu -> sp, value);
}

// Push a 16-bit value [value] onto the stack.
void push16(CPU * cpu, Memory * mem, uint16_t value) {
    push8(cpu, mem, (value >> 8) & 0xFF);
    push8(cpu, mem, value & 0xFF);
}

// Pop an 8-bit value from the stack.
uint8_t pop8(CPU * cpu, Memory * mem) {
    return mem_read8(mem, cpu -> sp++);
}

// Pop a 16-bit value from the stack.
uint16_t pop16(CPU * cpu, Memory * mem) {
    uint8_t low = pop8(cpu, mem);
    uint8_t high = pop8(cpu, mem);
    return ((uint16_t)high << 8) | low;
}

// Update timer registers
void mem_timer_update(Memory *mem, int cycles) {

    for (int i = 0; i < cycles; i++) {

        // Check delayed reload
        if (mem -> tima_reload_delay) {
            mem -> tima_reload_delay--;

            // Reset TIMA to TMA and request timer interrupt
            if (mem -> tima_reload_delay == 0) {
                mem -> io[0x05] = mem->io[0x06];
                mem -> io[0x0F] |= 0x04;
            }
        }

        // DIV register
        uint16_t old_div = mem -> div_internal;
        mem -> div_internal++;
        mem -> io[0x04] = mem -> div_internal >> 8;

        // Check if timer is enabled
        uint8_t tac = mem -> io[0x07];
        if (tac & 0x04) {


            // Check which bit to test falling edge of
            int bit;
            switch (tac & 0x03) {
                case 0: bit = 9; break;
                case 1: bit = 3; break;
                case 2: bit = 5; break;
                case 3: bit = 7; break;
            }

            int old_bit = (old_div >> bit) & 1;
            int new_bit = (mem -> div_internal >> bit) & 1;

            // Increment TIMA on falling edge of bit, and prepare for timer interrupt on overflow
            if (old_bit && !new_bit) {
                if (mem -> io[0x05] == 0xFF) {
                    mem -> io[0x05] = 0x00;
                    mem -> tima_reload_delay = 1;
                } else {
                    mem -> io[0x05]++;
                }
            }
        }
    }
}

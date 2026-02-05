#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"

/*
mem_rom_load

Load the first 32KB of a ROM file into ROM bank 0 and ROM bank N.
Pads incomplete bank N reads with 0xFF.
*/
Status mem_rom_load(Memory *mem, const char *filename) {

    FILE *rom_file = fopen(filename, "rb");
    if (!rom_file) {
        return ERR_FILE_NOT_FOUND;
    }

    // Read ROM bank 0 (0000–3FFF)
    size_t read_bytes = fread(mem->rom0, 1, ROM_BANK_0_SIZE, rom_file);
    if (read_bytes != ROM_BANK_0_SIZE) {
        fclose(rom_file);
        return ERR_BAD_FILE;
    }

    // Read ROM bank N (4000–7FFF)
    read_bytes = fread(mem->romN, 1, ROM_BANK_N_SIZE, rom_file);
    if (read_bytes != ROM_BANK_N_SIZE) {
        for (size_t i = read_bytes; i < ROM_BANK_N_SIZE; i++) {
            mem->romN[i] = 0xFF;
        }
    }

    fclose(rom_file);
    return OK;
}

/*
mem_init

Clear memory, initialize IO registers to post-BIOS defaults, and attach the parent GB pointer.
*/
Status mem_init(Memory *mem, GB *gb) {
    if (gb == NULL) {
        return ERR_NO_PARENT;
    }

    // Clear memory to 0, preserving ROM data
    memset(mem->vram, 0, VRAM_SIZE);
    memset(mem->eram, 0, ERAM_SIZE);
    memset(mem->wram0, 0, WRAM_BANK_0_SIZE);
    memset(mem->wram1, 0, WRAM_BANK_1_SIZE);
    memset(mem->oam, 0, OAM_SIZE);
    memset(mem->io, 0, IO_REGISTERS_SIZE);
    memset(mem->hram, 0, HRAM_SIZE);

    mem->io[0x00] = 0xCF;

    // Timer registers
    mem->io[0x04] = 0x00; // DIV
    mem->io[0x05] = 0x00; // TIMA
    mem->io[0x06] = 0x00; // TMA
    mem->io[0x07] = 0x00; // TAC

    // Interrupt flag
    mem->io[0x0F] = 0xE1;

    // LCD / PPU registers
    mem->io[0x40] = 0x91; // LCDC
    mem->io[0x41] = 0x85; // STAT
    mem->io[0x42] = 0x00; // SCY
    mem->io[0x43] = 0x00; // SCX
    mem->io[0x44] = 0x00; // LY
    mem->io[0x45] = 0x00; // LYC

    mem->io[0x47] = 0xFC; // BGP
    mem->io[0x48] = 0xFF; // OBP0
    mem->io[0x49] = 0xFF; // OBP1

    mem->io[0x4A] = 0x00; // WY
    mem->io[0x4B] = 0x00; // WX

    // DMA
    mem->io[0x46] = 0x00; // DMA

    // IE register (FFFF)
    mem->ie = 0x00;

    // Timer counters
    mem->div_internal = 0;

    // Set parent pointer
    mem->gb = gb;

    return OK;
}

/*
push8

Push an 8-bit value onto the stack (pre-decrement SP).
*/
void push8(CPU *cpu, Memory *mem, uint8_t value) {
    mem_write8(mem, --cpu->sp, value);
}

/*
push16

Push a 16-bit value onto the stack (high byte then low byte).
*/
void push16(CPU *cpu, Memory *mem, uint16_t value) {
    push8(cpu, mem, (value >> 8) & 0xFF);
    push8(cpu, mem, value & 0xFF);
}

/*
pop8

Pop an 8-bit value from the stack (post-increment SP).
*/
uint8_t pop8(CPU *cpu, Memory *mem) {
    return mem_read8(mem, cpu->sp++);
}

/*
pop16

Pop a 16-bit value from the stack (low byte then high byte).
*/
uint16_t pop16(CPU *cpu, Memory *mem) {
    uint8_t low = pop8(cpu, mem);
    uint8_t high = pop8(cpu, mem);
    return ((uint16_t)high << 8) | low;
}

/*
mem_timer_update

Advance DIV/TIMA by [cycles], handle TIMA overflow and delayed reload.
*/
void mem_timer_update(Memory *mem, int cycles) {

    for (int i = 0; i < cycles; i++) {

        // Check delayed reload
        if (mem->tima_reload_delay) {
            mem->tima_reload_delay--;

            // Reset TIMA to TMA and request timer interrupt
            if (mem->tima_reload_delay == 0) {
                mem->io[0x05] = mem->io[0x06];
                mem->io[0x0F] |= 0x04;
            }
        }

        // DIV register
        uint16_t old_div = mem->div_internal;
        mem->div_internal++;
        mem->io[0x04] = mem->div_internal >> 8;

        // Check if timer is enabled
        uint8_t tac = mem->io[0x07];
        if (tac & 0x04) {

            // Check which bit to test falling edge of
            int bit;
            switch (tac & 0x03) {
            case 0:
                bit = 9;
                break;
            case 1:
                bit = 3;
                break;
            case 2:
                bit = 5;
                break;
            case 3:
                bit = 7;
                break;
            }

            int old_bit = (old_div >> bit) & 1;
            int new_bit = (mem->div_internal >> bit) & 1;

            // Increment TIMA on falling edge of bit, and prepare for timer interrupt on overflow
            if (old_bit && !new_bit) {
                if (mem->io[0x05] == 0xFF) {
                    mem->io[0x05] = 0x00;
                    mem->tima_reload_delay = 1;
                } else {
                    mem->io[0x05]++;
                }
            }
        }
    }
}

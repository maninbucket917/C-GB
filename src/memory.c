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

    mem -> io[0x00] = 0x0F;

    // Timer registers
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

// Read an 8-bit value from memory at [addr].
uint8_t mem_read8(Memory * mem, uint16_t addr) {

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
        if (addr == 0xFF00) return 0xF;
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
void mem_write8(Memory * mem, uint16_t addr, uint8_t value) {

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

        // Writing to FF04 resets DIV
        if (addr == 0xFF04) {
            mem->div_internal = 0;
            mem->io[0x04] = 0;
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
    return mem_read8(mem, cpu->sp++);
}

// Pop a 16-bit value from the stack.
uint16_t pop16(CPU * cpu, Memory * mem) {
    uint8_t low = pop8(cpu, mem);
    uint8_t high = pop8(cpu, mem);
    return ((uint16_t)high << 8) | low;
}

// Update timer registers
void mem_timer_update(Memory *mem, int cycles)
{

    while (cycles--) {

        mem->div_internal++;

        // Update DIV register
        mem->io[0x04] = mem->div_internal >> 8;
    }
}
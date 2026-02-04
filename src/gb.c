#include "gb.h"
#include "cpu.h"
#include "memory.h"
#include "ppu.h"

#include <stdio.h>
#include <string.h>

// Initialize all GB components
Status GB_init(GB *gb, CPU *cpu, PPU *ppu, Memory *mem) {

    Status status;

    // Assign pointers to GB components
    gb->cpu = cpu;
    gb->ppu = ppu;
    gb->mem = mem;

    // Check for errors upon initialization
    status = cpu_init(cpu, gb);
    if (status != OK) {
        printf("Error: CPU initialization failed\n");
        return status;
    }

    status = mem_init(mem, gb);
    if (status != OK) {
        printf("Error: Memory initialization failed\n");
        return status;
    }

    status = ppu_init(ppu, gb);
    if (status != OK) {
        printf("Error: PPU initialization failed\n");
        return status;
    }

    // Initialize joypad state
    gb->joypad_state = 0xFF;
    gb->rom_loaded = 0;

    return OK;
}

/*
GB_load_rom

Reset emulator state and load a new ROM from the given filepath.
*/
Status GB_load_rom(GB *gb, const char *filepath) {
    Status status;

    // Validate file extension
    size_t len = strlen(filepath);
    if (len < 3 || strcmp(filepath + len - 3, ".gb") != 0) {
        printf("Error: File must have .gb extension\n");
        gb->rom_loaded = 0;
        return ERR_BAD_FILE;
    }

    // Check file exists
    FILE *test_file = fopen(filepath, "rb");
    if (!test_file) {
        printf("Error: Cannot open file: %s\n", filepath);
        gb->rom_loaded = 0;
        return ERR_FILE_NOT_FOUND;
    }
    fclose(test_file);

    // Reset emulator state
    status = cpu_init(gb->cpu, gb);
    if (status != OK) {
        printf("Error: Failed to reset CPU\n");
        return status;
    }

    status = mem_init(gb->mem, gb);
    if (status != OK) {
        printf("Error: Failed to reset memory\n");
        return status;
    }

    ppu_reset(gb->ppu);

    // Load ROM
    status = mem_rom_load(gb->mem, filepath);
    if (status != OK) {
        gb->rom_loaded = 0;
        return status;
    }

    gb->rom_loaded = 1;
    return OK;
}
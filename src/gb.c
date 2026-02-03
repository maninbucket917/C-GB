#include "gb.h"
#include "cpu.h"
#include "memory.h"
#include "opcodes.h"
#include "ppu.h"

// Initialize all GB components
Status GB_init(GB *gb, CPU *cpu, PPU *ppu, Memory *mem) {

    Status status;

    // Assign pointers to GB components
    gb->cpu = cpu;
    gb->ppu = ppu;
    gb->mem = mem;

    // Check for errors upon initialization
    status = cpu_init(cpu, gb);
    if (status != OK)
        return status;

    status = mem_init(mem, gb);
    if (status != OK)
        return status;

    status = ppu_init(ppu, gb);
    if (status != OK)
        return status;

    // Initialize joypad state
    gb->joypad_state = 0xFF;

    return OK;
}
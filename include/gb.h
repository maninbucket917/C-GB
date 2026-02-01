#ifndef GB_H
#define GB_H

#include "config.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"

// GB struct to store system components
typedef struct CPU CPU;
typedef struct PPU PPU;
typedef struct Memory Memory;

typedef struct GB{
    CPU * cpu;
    PPU * ppu;
    Memory * mem;
}GB;

Status GB_init(GB * gb, CPU * cpu, PPU * ppu, Memory * mem);

#endif
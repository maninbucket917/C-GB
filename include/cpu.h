#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "gb.h"
#include "memory.h"
#include "ppu.h"

typedef struct Memory Memory;
typedef struct PPU PPU;

typedef struct CPU {

    // 8-bit registers
    uint8_t a;
    uint8_t f;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;

    // 16-bit registers
    uint16_t pc;
    uint16_t sp;

    // Interrupt handling
    uint8_t ime;
    uint8_t ime_delay;

    // HALT instruction handling
    uint8_t halted;
    uint8_t halt_bug;

    // STOP instruction handling
    uint8_t stopped;

    // Counter of cycles remaining for current frame
    int frame_cycles;

    // Pointer to parent struct
    GB *gb;
} CPU;

// CPU initialization

Status cpu_init(CPU *cpu, GB *gb);

// ----------------------------------
// Register pair read/write functions
// ----------------------------------

// Return the value of register pair af.
static inline uint16_t get_af(CPU *cpu) {
    return (cpu->a << 8) | (cpu->f & 0xF0);
}

// Return the value of register pair bc.
static inline uint16_t get_bc(CPU *cpu) {
    return (cpu->b << 8) | cpu->c;
}

// Return the value of register pair de.
static inline uint16_t get_de(CPU *cpu) {
    return (cpu->d << 8) | cpu->e;
}

// Return the value of register pair hl.
static inline uint16_t get_hl(CPU *cpu) {
    return (cpu->h << 8) | cpu->l;
}

// Set the value of register pair af to [val].
static inline void set_af(CPU *cpu, uint16_t val) {
    cpu->a = (val >> 8) & 0xFF;
    cpu->f = val & 0xF0;
}

// Set the value of register pair bc to [val].
static inline void set_bc(CPU *cpu, uint16_t val) {
    cpu->b = (val >> 8) & 0xFF;
    cpu->c = val & 0xFF;
}

// Set the value of register pair de to [val].
static inline void set_de(CPU *cpu, uint16_t val) {
    cpu->d = (val >> 8) & 0xFF;
    cpu->e = val & 0xFF;
}

// Set the value of register pair hl to [val].
static inline void set_hl(CPU *cpu, uint16_t val) {
    cpu->h = (val >> 8) & 0xFF;
    cpu->l = val & 0xFF;
}

// Return the value of register pair hl and decrement it by 1.
static inline uint16_t get_hl_minus(CPU *cpu) {
    uint16_t value = get_hl(cpu);
    set_hl(cpu, value - 1);
    return value;
}

// Return the value of register pair hl and increment it by 1.
static inline uint16_t get_hl_plus(CPU *cpu) {
    uint16_t value = get_hl(cpu);
    set_hl(cpu, value + 1);
    return value;
}

// Additional register pair functions

uint16_t get_hl_minus(CPU *cpu);
uint16_t get_hl_plus(CPU *cpu);

// ---------------
// Flag read/write
// ---------------

// Set the flag [flag] to [val].
static inline void set_flag(CPU *cpu, uint8_t flag, uint8_t val) {
    cpu->f = (cpu->f & ~flag) | (-(val != 0) & flag);
}

// Return the flag [flag].
static inline uint8_t get_flag(CPU *cpu, uint8_t flag) {
    return (cpu->f & flag) != 0;
}

// ----------------
// Helper functions
// ----------------

// Return the next opcode to execute and increments the PC by 1.
static inline uint8_t get_opcode(CPU *cpu, Memory *mem) {
    return mem_read8(mem, cpu->pc++);
}

// Return the immediate 8-bit operand and increment the PC by 1.
static inline uint8_t get_imm8(CPU *cpu, Memory *mem) {
    return mem_read8(mem, cpu->pc++);
}

// Return the immediate 16-bit operand and increment the PC by 2.
static inline uint16_t get_imm16(CPU *cpu, Memory *mem) {
    uint8_t low = mem_read8(mem, cpu->pc++);
    uint8_t high = mem_read8(mem, cpu->pc++);
    return (high << 8) | low;
}

// Interrupt handler
void cpu_handle_interrupts(CPU *cpu, Memory *mem);

// Execution step
void cpu_step(CPU *cpu, Memory *mem);

// Tick for timers, PPU, and frame cycles
void tick(CPU *cpu, int cycles);

// Debug helpers
void print_cpu_state(CPU *cpu, Memory *mem);

#endif
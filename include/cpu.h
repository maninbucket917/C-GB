#include <stdint.h>

#include "memory.h"

typedef struct Memory Memory;

#ifndef CPU_H
#define CPU_H

typedef struct CPU{

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

    // Total cycle counter
    uint64_t cycles;

}CPU;

// CPU initialization

void cpu_init(CPU * cpu);

// Register pair read/write

uint16_t get_af(CPU * cpu);
uint16_t get_bc(CPU * cpu);
uint16_t get_de(CPU * cpu);
uint16_t get_hl(CPU * cpu);

void set_af(CPU * cpu, uint16_t val);
void set_bc(CPU * cpu, uint16_t val);
void set_de(CPU * cpu, uint16_t val);
void set_hl(CPU * cpu, uint16_t val);

// Additional register pair functions

uint16_t get_hl_minus(CPU * cpu);
uint16_t get_hl_plus(CPU * cpu);

// Flag read/write

void set_zero_flag(CPU *cpu, uint8_t val);
void set_subtract_flag(CPU *cpu, uint8_t val);
void set_half_carry_flag(CPU *cpu, uint8_t val);
void set_carry_flag(CPU *cpu, uint8_t val);

uint8_t get_zero_flag(CPU * cpu);
uint8_t get_subtract_flag(CPU * cpu);
uint8_t get_half_carry_flag(CPU * cpu);
uint8_t get_carry_flag(CPU * cpu);

// Helper functions

uint8_t get_opcode(CPU * cpu, Memory * mem);
uint8_t get_imm8(CPU * cpu, Memory * mem);
uint16_t get_imm16(CPU * cpu, Memory * mem);

// Interrupt handler

int cpu_handle_interrupts(CPU *cpu, Memory * mem);

// Execution step

int cpu_step(CPU * cpu, Memory * mem);

// Debug helpers

void print_cpu_state(CPU * cpu, Memory * mem, FILE * file);

#endif
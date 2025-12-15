#include <stdio.h>

#include "cpu.h"
#include "memory.h"
#include "opcodes.h"
#include "ppu.h"


/*
cpu_init

Set the CPU to post-BIOS state.
*/
void cpu_init(CPU * cpu) {

    cpu -> a = 0x01;
    cpu -> f = 0xB0;
    cpu -> b = 0x00;
    cpu -> c = 0x13;
    cpu -> d = 0x00;
    cpu -> e = 0xD8;
    cpu -> h = 0x01;
    cpu -> l = 0x4D;

    cpu -> pc = 0x0100;
    cpu -> sp = 0xFFFE;

    cpu -> ime = 1;
    cpu -> ime_delay = 0;

    cpu -> halted = 0;
    cpu -> halt_bug = 0;

    cpu -> stopped = 0;

    cpu -> cycles = 0;
}

// Functions for register pairs

// Return the value of register pair af.
uint16_t get_af(CPU * cpu) {
    return (cpu -> a << 8) | (cpu -> f & 0xF0);
}

// Return the value of register pair bc.
uint16_t get_bc(CPU * cpu) {
    return (cpu -> b << 8) | cpu -> c;
}

// Return the value of register pair de.
uint16_t get_de(CPU * cpu) {
    return (cpu -> d << 8) | cpu -> e;
}

// Return the value of register pair hl.
uint16_t get_hl(CPU * cpu) {
    return (cpu -> h << 8) | cpu -> l;
} 

// Set the value of register pair af to [val].
void set_af(CPU * cpu, uint16_t val) {
    cpu -> a = (val >> 8) & 0xFF;
    cpu -> f = val & 0xF0;
}

// Set the value of register pair bc to [val].
void set_bc(CPU * cpu, uint16_t val) {
    cpu -> b = (val >> 8) & 0xFF;
    cpu -> c = val & 0xFF;
}

// Set the value of register pair de to [val].
void set_de(CPU * cpu, uint16_t val) {
    cpu -> d = (val >> 8) & 0xFF;
    cpu -> e = val & 0xFF;
}

// Set the value of register pair hl to [val].
void set_hl(CPU * cpu, uint16_t val) {
    cpu -> h = (val >> 8) & 0xFF;
    cpu -> l = val & 0xFF;
}

// Return the value of register pair hl and decrement it by 1.
uint16_t get_hl_minus(CPU * cpu){
    uint16_t value = get_hl(cpu);
    set_hl(cpu, value - 1);
    return value;
}

// Return the value of register pair hl and increment it by 1.
uint16_t get_hl_plus(CPU * cpu){
    uint16_t value = get_hl(cpu);
    set_hl(cpu, value + 1);
    return value;
}

// Set the zero flag to [val].
void set_zero_flag(CPU * cpu, uint8_t val) {
    cpu->f = (cpu->f & ~0x80) | ((val ? 1 : 0) << 7);
    cpu->f &= 0xF0;
}

// Set the subtract flag to [val].
void set_subtract_flag(CPU * cpu, uint8_t val) {
    cpu->f = (cpu->f & ~0x40) | ((val ? 1 : 0) << 6);
    cpu->f &= 0xF0;
}

// Set the half carry flag to [val].
void set_half_carry_flag(CPU * cpu, uint8_t val) {
    cpu->f = (cpu->f & ~0x20) | ((val ? 1 : 0) << 5);
    cpu->f &= 0xF0;
}

// Set the carry flag to [val].
void set_carry_flag(CPU * cpu, uint8_t val) {
    cpu->f = (cpu->f & ~0x10) | ((val ? 1 : 0) << 4);
    cpu->f &= 0xF0;
}

// Return the value of the zero flag.
uint8_t get_zero_flag(CPU * cpu) {
    return (cpu -> f & 0x80) != 0;
}

// Return the value of the subtract flag.
uint8_t get_subtract_flag(CPU * cpu) {
    return (cpu -> f & 0x40) != 0;
}

// Return the value of the half carry flag.
uint8_t get_half_carry_flag(CPU * cpu) {
    return (cpu -> f & 0x20) != 0;
}

// Return the value of the carry flag.
uint8_t get_carry_flag(CPU * cpu) {
    return (cpu -> f & 0x10) != 0;
}

// Return the next opcode to execute and increments the PC by 1.
uint8_t get_opcode(CPU * cpu, Memory * mem) {
    return mem_read8(mem, cpu -> pc++);
}

// Return the immediate 8-bit operand and increment the PC by 1.
uint8_t get_imm8(CPU * cpu, Memory * mem) {
    return mem_read8(mem, cpu -> pc++);
}

// Return the immediate 16-bit operand and increment the PC by 2.
uint16_t get_imm16(CPU * cpu, Memory * mem) {
    uint8_t low = mem_read8(mem, cpu -> pc++);
    uint8_t high = mem_read8(mem, cpu -> pc++);
    return (high << 8) | low;
}

/*
cpu_handle_interrupts

Check for and begin servicing interrupts if interrupt flags are set.
*/
int cpu_handle_interrupts(CPU * cpu, Memory * mem) {

    uint8_t IE = mem_read8(mem, 0xFFFF); // Interrupt Enable
    uint8_t IF = mem_read8(mem, 0xFF0F); // Interrupt Flag
    uint8_t pending = IE & IF;

    // No cycles taken if no interrupts are pending
    if(!pending) {
        return 0;
    }

    // Check for HALT bug
    if(cpu -> halted && !cpu -> ime) {
        cpu -> halted = 0;
        cpu -> halt_bug = 1;
        return 0;
    }

    if(!cpu -> ime) {
        return 0;
    }

    // Find highest priority interrupt
    for (int i = 0; i < 5; i++) {

        if (pending & (1 << i)) {

            // Disable interrupts and wake CPU
            cpu -> ime = 0;
            cpu -> halted = 0;

            // Push high byte of PC
            push8(cpu, mem, cpu -> pc >> 8);

            // Check for early cancellation
            IE = mem_read8(mem, 0xFFFF);
            IF = mem_read8(mem, 0xFF0F);

            uint8_t cancelled = !(IE & IF & (1 << i));

            // Push low byte of PC
            push8(cpu, mem, cpu -> pc & 0xFF);

            // Interrupt is cancelled if IE or IF change during the push
            if (cancelled) {

                // Check if any other interrupts are available to process
                uint8_t remaining = (IE & IF) & ~(1 << i);
                if (!remaining) {
                    cpu -> pc = 0x0000;
                    return 20;
                }
                continue;
            }

            // Jump to interrupt vector
            switch (i) {
                case 0: cpu -> pc = 0x40; break; // V-Blank
                case 1: cpu -> pc = 0x48; break; // LCD STAT
                case 2: cpu -> pc = 0x50; break; // Timer
                case 3: cpu -> pc = 0x58; break; // Serial
                case 4: cpu -> pc = 0x60; break; // Joypad
            }

            // Clear the corresponding IF bit
            mem_write8(mem, 0xFF0F, IF & ~(1 << i));

            // Add cycles for interrupt handling
            return 20;
        }
    }

    return 0;
}


/*
cpu_step

Executes one instruction, and handles halt and interrupt logic.
*/
int cpu_step(CPU * cpu, Memory * mem) {

    // CPU halt logic
    if (cpu -> halted) {
        uint8_t IE = mem_read8(mem, 0xFFFF);
        uint8_t IF = mem_read8(mem, 0xFF0F);
        uint8_t pending = IE & IF;

        if (!pending) {

            // No interrupt, stay halted 
            mem_timer_update(mem,4);
            return 4;
        }

        // If interrupts are pending, wake CPU
        cpu -> halted = 0;

    }

    // Fetch opcode
    int instruction_cycles = 0;
    uint16_t fetch_pc = cpu->pc;
    uint8_t op = mem_read8(mem, fetch_pc);

    
    if (!cpu -> halt_bug) {
        cpu -> pc++;
    } else {
        cpu -> halt_bug = 0;
    }

    opcode_fn handler;

    // Check for CB prefixed unstructions
    if (op == 0xCB) {
        uint8_t cb = mem_read8(mem, cpu -> pc);
        cpu->pc++;

        handler = cb_opcode_table[cb];
        if (!handler) {
            printf("Unrecognized opcode: CB %02X at PC=%04X\n", cb, fetch_pc);
            return -1;
        }

        instruction_cycles += handler(cpu, mem);
    }else{
        handler = opcode_table[op];

        if (!handler) {
            printf("Unrecognized opcode: %02X at PC=%04X\n", op, fetch_pc);
            return -1;
        }

        instruction_cycles += handler(cpu, mem);
    }

    // Check for EI delay
    if (cpu -> ime_delay) {
        cpu -> ime_delay = 0;
        cpu -> ime = 1;
    }

    cpu -> cycles += instruction_cycles;
    instruction_cycles += cpu_handle_interrupts(cpu, mem);

    return instruction_cycles;
}

/*
Used for debugging
*/
void print_cpu_state(CPU *cpu, Memory * mem, FILE * file) {
    fprintf(file, "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
    cpu->a, cpu->f, cpu->b, cpu->c, cpu->d, cpu->e, cpu->h, cpu->l, cpu->sp, cpu->pc, mem_read8(mem, cpu->pc), mem_read8(mem, cpu->pc+1),mem_read8(mem, cpu->pc+2),mem_read8(mem, cpu->pc+3));
}
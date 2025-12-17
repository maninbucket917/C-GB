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

    cpu -> frame_cycles = 0;
}

/*
cpu_handle_interrupts

Check for and begin servicing interrupts if interrupt flags are set.
*/
void cpu_handle_interrupts(CPU * cpu, Memory * mem) {

    uint8_t IE = mem_read8(mem, 0xFFFF); // Interrupt Enable
    uint8_t IF = mem_read8(mem, 0xFF0F); // Interrupt Flag
    uint8_t pending = IE & IF;

    // No cycles taken if no interrupts are pending
    if(!pending) {
        return;
    }

    if(!cpu -> ime) {
        return;
    }

    // Find highest priority interrupt
    for (int i = 0; i < 5; i++) {

        if (pending & (1 << i)) {

            // Disable interrupts and wake CPU
            cpu -> ime = 0;
            cpu -> halted = 0;

            // Push high byte of PC
            push8(cpu, mem, cpu -> pc >> 8);
            tick(cpu, 4);

            // Check for early cancellation
            IE = mem_read8(mem, 0xFFFF);
            IF = mem_read8(mem, 0xFF0F);

            uint8_t cancelled = !(IE & IF & (1 << i));

            // Push low byte of PC
            push8(cpu, mem, cpu -> pc & 0xFF);
            tick(cpu, 4);

            // Interrupt is cancelled if IE or IF change during the push
            if (cancelled) {

                // Check if any other interrupts are available to process
                uint8_t remaining = (IE & IF) & ~(1 << i);
                if (!remaining) {
                    cpu -> pc = 0x0000;
                    tick(cpu, 12);
                    return;
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
            tick(cpu, 12);
            return;
        }
    }

    return;
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
            tick(cpu, 4);
            return 4;
        }

        // If interrupts are pending, wake CPU
        cpu -> halted = 0;

    }

    // Fetch opcode
    int instruction_cycles = 0;
    uint16_t fetch_pc = cpu -> pc;
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
        cpu -> pc++;

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

    tick(cpu, instruction_cycles);

    cpu_handle_interrupts(cpu, mem);

    return instruction_cycles;
}

/*
Used for debugging
*/
void print_cpu_state(CPU * cpu, Memory * mem, FILE * file) {
    fprintf(file, "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
    cpu -> a, cpu -> f, cpu -> b, cpu -> c, cpu -> d, cpu -> e, cpu -> h, cpu -> l, cpu -> sp, cpu -> pc, mem_read8(mem, cpu -> pc), mem_read8(mem, cpu -> pc+1),mem_read8(mem, cpu -> pc+2),mem_read8(mem, cpu -> pc+3));
}
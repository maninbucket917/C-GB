#include <stdio.h>

#include "cpu.h"
#include "opcodes.h"

/*
cpu_init

Set the CPU to post-BIOS state. Also assigns GB parent pointer of CPU.
*/
Status cpu_init(CPU *cpu, GB *gb) {
    if (gb == NULL) {
        return ERR_NO_PARENT;
    }

    cpu->gb = gb;

    cpu->a = 0x01;
    cpu->f = 0xB0;
    cpu->b = 0x00;
    cpu->c = 0x13;
    cpu->d = 0x00;
    cpu->e = 0xD8;
    cpu->h = 0x01;
    cpu->l = 0x4D;

    cpu->pc = 0x0100;
    cpu->sp = 0xFFFE;

    cpu->ime = 1;
    cpu->ime_delay = 0;

    cpu->halted = 0;
    cpu->halt_bug = 0;

    cpu->stopped = 0;

    cpu->frame_cycles = 0;

    return OK;
}

/*
cpu_handle_interrupts

Check for and begin servicing interrupts if interrupt flags are set.
*/
void cpu_handle_interrupts(CPU *cpu, Memory *mem) {
    // Interrupts only taken if IME is set
    if (!cpu->ime) {
        return;
    }

    uint8_t IE = mem_read8(mem, 0xFFFF); // Interrupt Enable
    uint8_t IF = mem_read8(mem, 0xFF0F); // Interrupt Flag
    uint8_t pending = IE & IF;

    if (!pending) {
        return;
    }

    // Interrupt acknowledge (atomic)
    cpu->ime = 0;
    cpu->halted = 0;

    // Highest priority first
    for (int i = 0; i < 5; i++) {
        if (pending & (1 << i)) {

            // Clear IF bit IMMEDIATELY
            mem_write8(mem, 0xFF0F, IF & ~(1 << i));

            // Push PC (16-bit, high first)
            push8(cpu, mem, cpu->pc >> 8);
            push8(cpu, mem, cpu->pc & 0xFF);

            // Jump to interrupt vector
            static const uint16_t vectors[5] = {
                0x40, // V-Blank
                0x48, // LCD STAT
                0x50, // Timer
                0x58, // Serial
                0x60  // Joypad
            };

            cpu->pc = vectors[i];

            // Interrupt handling cost
            tick(cpu, 20);
            return;
        }
    }
}

// Set ime to 1 if ime_delay reaches 1, decrement counter otherwise
static inline void check_ei_delay(CPU *cpu) {
    if (cpu->ime_delay > 0) {
        cpu->ime_delay--;
        if (cpu->ime_delay == 0) {
            cpu->ime = 1;
        }
    }
}

// Request a serial interrupt if start bit is set.
static inline void serial_check(Memory *mem) {
    static int serial_count = 0;

    if (mem->io[0x02] & 0x80) {

        // Make a dummy transfer
        mem->io[0x01] = (mem->io[0x01] << 1) | 1;
        serial_count++;

        if (serial_count >= 8) {
            mem->io[0x02] &= ~0x80;
            serial_count = 0;

            if (mem->io[0x02] & 0x01) {
                mem->io[0x0F] |= 0x08;
            }
        }
    } else {
        serial_count = 0;
    }
}

/*
cpu_step

Executes one instruction, and handles halt and interrupt logic.
*/
void cpu_step(CPU *cpu, Memory *mem) {

    // CPU halt logic
    if (cpu->halted) {
        uint8_t IE = mem_read8(mem, 0xFFFF);
        uint8_t IF = mem_read8(mem, 0xFF0F);
        uint8_t pending = IE & IF;

        if (!pending) {
            tick(cpu, 4);
            check_ei_delay(cpu);
            return;
        }

        cpu->halted = 0;
    }

    // Check for interrupts
    cpu_handle_interrupts(cpu, mem);

    // Fetch opcode
    uint8_t op = mem_read8(mem, cpu->pc);

    // Check for halt bug behaviour
    if (!cpu->halt_bug) {
        cpu->pc++;
    } else {
        cpu->halt_bug = 0;
    }

    // Run instruction handler
    opcode_fn handler = opcode_table[(op == 0xCB) ? 0x100 + get_opcode(cpu, mem) : op];
    uint8_t instruction_cycles = handler(cpu, mem);

    tick(cpu, instruction_cycles);

    // Check EI delay after instruction completes
    check_ei_delay(cpu);

    // Check for serial transfer
    serial_check(mem);

    // DEBUG: print CPU state
    // WARNING: Uncommenting this line destroys performance
    // print_cpu_state(cpu, mem);

    return;
}

void tick(CPU *cpu, int cycles) {
    mem_timer_update(cpu->gb->mem, cycles);
    ppu_step(cpu->gb->ppu, cpu->gb->mem, cycles);
    cpu->frame_cycles -= cycles;
}

/*
Used for debugging
*/
void print_cpu_state(CPU *cpu, Memory *mem) {
    printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
           cpu->a, cpu->f, cpu->b, cpu->c, cpu->d, cpu->e, cpu->h, cpu->l, cpu->sp, cpu->pc,
           mem_read8(mem, cpu->pc), mem_read8(mem, cpu->pc + 1), mem_read8(mem, cpu->pc + 2), mem_read8(mem, cpu->pc + 3));
}
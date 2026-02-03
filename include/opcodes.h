#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

struct CPU;
struct Memory;

// Define function pointer type for opcodes
typedef uint8_t (*opcode_fn)(struct CPU *cpu, struct Memory *mem);

// Function pointer table
extern opcode_fn opcode_table[NUM_OPCODES];

#endif
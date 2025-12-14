#include <stdint.h>

#ifndef OPCODES_H
#define OPCODES_H

struct CPU;
struct Memory;

// Define function pointer type for opcodes
typedef uint8_t (*opcode_fn)(struct CPU * cpu, struct Memory * m);

// Function pointer tables
extern opcode_fn opcode_table[256];
extern opcode_fn cb_opcode_table[256];

#endif
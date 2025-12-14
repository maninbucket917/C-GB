/*
This file declares constants used by the system.
*/

#ifndef CONFIG_H
#define CONFIG_H

// Screen size

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

// Screen scale setting

#define SCREEN_SCALING 3

// Memory sizes

#define ROM_BANK_0_SIZE 0x4000   // 16 KB (0000 - 3FFF)
#define ROM_BANK_N_SIZE 0x4000   // 16 KB (4000 - 7FFF)
#define VRAM_SIZE 0x2000   // 8 KB (8000 - 9FFF)
#define ERAM_SIZE 0x2000   // 8 KB (A000 - BFFF)
#define WRAM_BANK_0_SIZE 0x1000   // 4 KB (C000 - CFFF)
#define WRAM_BANK_1_SIZE 0x1000   // 4 KB (D000 - DFFF)
#define OAM_SIZE 0x00A0   // 160 bytes (FE00 - FE9F)
#define UNUSED_SIZE 0x0060   // unusable region (FEA0–FEFF)
#define IO_REGISTERS_SIZE 0x0080   // 128 bytes (FF00–FF7F)
#define HRAM_SIZE 0x007F   // 127 bytes (FF80–FFFE)

// Number of opcodes

#define NUM_OPCODES 0x100 // 256 opcodes per table

// Frame timing constants

#define CYCLES_PER_FRAME 456 * 154
#define FRAME_MS 1000 / 60

#endif
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

// Palette settings (Lightest -> Darkest)
#define PALETTE_0 0xFFFFFFFF, 0xC0C0C0FF, 0x606060FF, 0x000000FF
#define PALETTE_1 0x9BBC0FFF, 0x8BAC0FFF, 0x306230FF, 0x0F380FFF

#define NUM_PALETTES 2
#define DEFAULT_PALETTE 0

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

#define CYCLES_PER_FRAME 70224
#define GB_FPS 4194304.0 / 70224.0
#define FRAME_TIME 1.0 / (GB_FPS)

// Input keys

#define BUTTON_A SDLK_x
#define BUTTON_B SDLK_z
#define BUTTON_SELECT SDLK_RSHIFT
#define BUTTON_START SDLK_RETURN

#define BUTTON_RIGHT SDLK_RIGHT
#define BUTTON_LEFT SDLK_LEFT
#define BUTTON_UP SDLK_UP
#define BUTTON_DOWN SDLK_DOWN

#define BUTTON_RESET SDLK_F1
#define BUTTON_PALETTE_SWAP SDLK_F2

#endif
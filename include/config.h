/*
This file declares constants used by the system.
*/

#ifndef CONFIG_H
#define CONFIG_H

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
#define BUTTON_TURBO SDLK_F3
#define BUTTON_PAUSE SDLK_F4
#define BUTTON_FULLSCREEN SDLK_F11

// Screen size

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

// Screen scale setting

#define SCREEN_SCALING 3

// Palette settings (Lightest -> Darkest)

#define PALETTE_0 0xFFFFFFFF, 0xC0C0C0FF, 0x606060FF, 0x000000FF // Grayscale
#define PALETTE_1 0x9BBC0FFF, 0x8BAC0FFF, 0x306230FF, 0x0F380FFF // DMG green
#define PALETTE_2 0xE0F8D0FF, 0x88C070FF, 0x346856FF, 0x081820FF // Pocket green
#define PALETTE_3 0xFFF7E8FF, 0xF4CBAAFF, 0xC7875EFF, 0x6B3F2BFF // Sepia
#define PALETTE_4 0xE8F0FFFF, 0xA9C9FFFF, 0x5E88C7FF, 0x2B3F6BFF // Light blue
#define PALETTE_5 0xEF0000FF, 0xA40000FF, 0x550000FF, 0x000000FF // VB

#define NUM_PALETTES 6
#define DEFAULT_PALETTE 0

// Memory sizes

#define ROM_BANK_0_SIZE 0x4000   // 16 KB (0000 - 3FFF)
#define ROM_BANK_N_SIZE 0x4000   // 16 KB (4000 - 7FFF)
#define VRAM_SIZE 0x2000         // 8 KB (8000 - 9FFF)
#define ERAM_SIZE 0x2000         // 8 KB (A000 - BFFF)
#define WRAM_BANK_0_SIZE 0x1000  // 4 KB (C000 - CFFF)
#define WRAM_BANK_1_SIZE 0x1000  // 4 KB (D000 - DFFF)
#define OAM_SIZE 0x00A0          // 160 bytes (FE00 - FE9F)
#define IO_REGISTERS_SIZE 0x0080 // 128 bytes (FF00–FF7F)
#define HRAM_SIZE 0x007F         // 127 bytes (FF80–FFFE)

// Number of opcodes

#define NUM_OPCODES 0x200 // 256 opcodes per table, for a total of 512

// Frame timing constants

#define CYCLES_PER_FRAME 70224
#define FRAME_TIME 0.016742706298828125 // 1.0 / 59.7275005696

// Flag constants

#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

// Error codes
typedef enum {
    OK = 0,
    ERR_BAD_ARGS,
    ERR_SDL_NOT_INITIALIZED,
    ERR_BAD_FILE,
    ERR_FILE_NOT_FOUND,
    ERR_NO_PARENT
} Status;

#endif
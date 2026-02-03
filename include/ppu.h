#ifndef PPU_H
#define PPU_H

#include <SDL2/SDL.h>
#include <stdint.h>

#include "config.h"
#include "gb.h"

typedef struct Memory Memory;

typedef struct PPU {

    // Keep track of PPU location

    uint16_t dot; // 0–455
    uint8_t ly;   // 0–153
    uint8_t mode; // 0 = HBlank, 1 = VBlank, 2 = OAM, 3 = VRAM

    // Window timing
    uint8_t window_line;
    uint8_t window_drawn;

    // SDL components
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    // Framebuffer
    uint32_t framebuffer[160 * 144];

    // Active palette ID
    uint8_t palette_id;

    // Pointer to parent struct
    GB *gb;

} PPU;

typedef struct Sprite {

    // Index of sprite in OAM
    uint8_t index;

    // X and Y position on screen
    int y;
    int x;

    // Address of tile in OAM
    uint16_t tile;

    // Attribute bits
    uint8_t attr;
} Sprite;

// --------------
// Initialization
// --------------

Status ppu_init(PPU *ppu, GB *gb);
void ppu_reset(PPU *ppu);

// -------
// Drawing
// -------

void ppu_draw_tiles(PPU *ppu, Memory *mem);
void ppu_draw_sprites(PPU *ppu, Memory *mem);

void ppu_step(PPU *ppu, Memory *mem, int cycles);

// -------------
// Miscellaneous
// -------------

void ppu_palette_swap(PPU *ppu);

#endif
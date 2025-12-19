#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <SDL2/SDL.h>

typedef struct Memory Memory;

typedef struct PPU{

    // Keep track of PPU location

    uint16_t dot; // 0–455
    uint8_t ly; // 0–153
    uint8_t mode; // 0 = HBlank, 1 = VBlank, 2 = OAM, 3 = VRAM

    // SDL components
    SDL_Window * window;
    SDL_Renderer * renderer;
    SDL_Texture * texture;
    
    // Framebuffer
    uint32_t framebuffer[160 * 144];

    // Active palette ID
    uint8_t palette_id;

} PPU;

// --------------
// Initialization
// --------------

void ppu_init(PPU * ppu);
void ppu_reset(PPU * ppu);

// -------
// Drawing
// -------

void ppu_draw_tiles(PPU * ppu, Memory * mem);
void ppu_draw_sprites(PPU * ppu, Memory * mem);

void ppu_step(PPU * ppu, Memory * mem, int cycles);

// -------------
// Miscellaneous
// -------------

void ppu_palette_swap(PPU * ppu);

#endif
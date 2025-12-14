#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include "memory.h"

typedef struct PPU{

    int dot;        // 0–455
    int ly;         // 0–153
    int mode;       // 0 = HBlank, 1 = VBlank, 2 = OAM, 3 = VRAM

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    
    uint32_t framebuffer[160 * 144];
} PPU;

void ppu_init(PPU *ppu);
void ppu_step(PPU *ppu, Memory *mem, int cycles);

uint32_t gb_palette(int colour);
void ppu_draw_scanline(PPU * ppu, Memory * mem);
uint8_t ppu_read_tile_pixel(Memory * mem, int tiledata_unsigned, uint16_t tilemap, uint16_t x, uint16_t y);

#endif
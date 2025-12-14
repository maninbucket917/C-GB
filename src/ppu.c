#include <stdio.h>
#include <stdbool.h>

#include "ppu.h"
#include "memory.h"
#include "config.h"

/*
ppu_init

Initializes the PPU and creates all necessary SDL components.
*/
void ppu_init(PPU * ppu) {
    SDL_Init(SDL_INIT_VIDEO);

    ppu -> dot = 0;
    ppu -> ly = 0;
    ppu -> mode = 2;

    ppu -> window = SDL_CreateWindow("C-GB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * SCREEN_SCALING, 144 * SCREEN_SCALING, SDL_WINDOW_SHOWN);
    ppu -> renderer = SDL_CreateRenderer(ppu->window, -1, SDL_RENDERER_ACCELERATED);
    ppu -> texture = SDL_CreateTexture(ppu->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
}

/*
ppu_read_tile_pixel

Returns the colour of the pixel specified by [tiledata_unsigned], [tilemap], [x], and [y].
*/
uint8_t ppu_read_tile_pixel(Memory * mem, int tiledata_unsigned, uint16_t tilemap, uint16_t x, uint16_t y) {

    // Get tile row and column
    uint16_t tile_row = y >> 3;
    uint16_t tile_col = x >> 3;

    // Get the index in memory of the pixel
    uint16_t index_addr = tilemap + tile_row * 32 + tile_col;
    uint8_t raw_index = mem_read8(mem, index_addr);

    uint16_t tile_addr;
    if (tiledata_unsigned) {
        tile_addr = 0x8000 + raw_index * 16;
    } else {
        tile_addr = 0x9000 + ((int8_t)raw_index) * 16;
    }

    // Get colour map of pixel by adding both bitplanes
    uint8_t line = y & 7;
    uint8_t b1 = mem_read8(mem, tile_addr + line * 2);
    uint8_t b2 = mem_read8(mem, tile_addr + line * 2 + 1);

    int bit = 7 - (x & 7);
    return ((b2 >> bit) & 1) << 1 | ((b1 >> bit) & 1);
}

/*
ppu_draw_scanline

Draw the scanline at ly to the PPU framebuffer.
*/
void ppu_draw_scanline(PPU * ppu, Memory * mem)
{

    // Read registers
    uint8_t lcdc = mem_read8(mem, 0xFF40);
    uint8_t ly   = ppu->ly;
    uint8_t scx  = mem_read8(mem, 0xFF43);
    uint8_t scy  = mem_read8(mem, 0xFF42);
    uint8_t wx   = mem_read8(mem, 0xFF4B);
    uint8_t wy   = mem_read8(mem, 0xFF4A);

    // Check for background and window enable bits
    bool bg_enable = lcdc & 0x01;
    bool win_enable = lcdc & 0x20;

    // Check which tilemap to use and whether to use signed or unsigned indexing
    uint16_t bg_map = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    uint16_t win_map = (lcdc & 0x40) ? 0x9C00 : 0x9800;
    bool unsigned_tiles = lcdc & 0x10;

    // Get the colour of each background pixel and update the framebuffer
    for (int x = 0; x < SCREEN_WIDTH; x++) {

        uint8_t bg_colour = 0;

        if (bg_enable) {
            uint16_t bx = (x + scx) & 0xFF;
            uint16_t by = (ly + scy) & 0xFF;
            bg_colour = ppu_read_tile_pixel(mem, unsigned_tiles, bg_map, bx, by);
        }

        if (win_enable && bg_enable && ly >= wy && x >= wx - 7) {
            uint16_t wx_x = x - (wx - 7);
            uint16_t wy_y = ly - wy;
            bg_colour = ppu_read_tile_pixel(mem, unsigned_tiles, win_map, wx_x, wy_y);
        }

        // Check palette
        uint8_t bgp = mem_read8(mem, 0xFF47);
        uint8_t mapped_colour = (bgp >> (bg_colour * 2)) & 0x03;
        ppu->framebuffer[ly * SCREEN_WIDTH + x] = gb_palette(mapped_colour);
    }
}

/*
ppu_step

Step through [cycles] cycles of the PPU.
*/
void ppu_step(PPU * ppu, Memory * mem, int cycles) {

    // Emulate one cycle at a time
    while (cycles--) {

        // If LCDC bit 7 is active, reset dot and ly, and set ppu mode to 0 (Hblank)
        uint8_t lcdc = mem_read8(mem, 0xFF40);
        if (!(lcdc & 0x80)) {
            ppu -> dot = 0;
            ppu -> ly = 0;
            mem_write8(mem, 0xFF44, 0);
            ppu -> mode = 0;
            continue;
        }

        // Each cycle increments dot counter by 1
        ppu->dot++;

        switch (ppu->mode) {

            case 0: // HBlank

                if (ppu->dot >= 456) {

                    // Increment LY at end of scanline
                    ppu -> dot = 0;
                    ppu -> ly++;
                    mem -> io [0x44] = ppu -> ly;

                    if (ppu -> ly == 144) {
                        // Enter VBlank
                        ppu->mode = 1;

                        // Request VBlank interrupt
                        uint8_t IF = mem_read8(mem, 0xFF0F);
                        mem_write8(mem, 0xFF0F, IF | 0x01);

                        // Present frame
                        SDL_UpdateTexture(ppu -> texture, NULL, ppu -> framebuffer, SCREEN_WIDTH * sizeof(uint32_t));
                        SDL_RenderClear(ppu -> renderer);
                        SDL_RenderCopy(ppu -> renderer, ppu -> texture, NULL, NULL);
                        SDL_RenderPresent(ppu -> renderer);

                    } else {
                        // Next scanline is OAM scan
                        ppu -> mode = 2;
                    }
                }

                break;

            case 1: // VBlank

                // Increment LY at end of scanline
                if (ppu -> dot >= 456) {
                    ppu -> dot = 0;
                    ppu -> ly++;
                    mem -> io [0x44] = ppu -> ly;

                    // Reset LY and start OAM scan
                    if (ppu -> ly > 153) {
                        ppu -> ly = 0;
                        ppu -> mode = 2;
                        mem_write8(mem, 0xFF44, 0);
                    }
                }
                break;

            case 2: // OAM scan

                // Change to drawing mode after 80 dots
                if (ppu -> dot >= 80) {
                    ppu -> mode = 3;
                }
                break;

            case 3: // Drawing
                if (ppu -> dot >= 252) {
                    
                    // Draw scanline at end of mode 3 and reset to Hblank
                    ppu_draw_scanline(ppu, mem);
                    ppu -> mode = 0;
                }
                break;

        }
    }
}

/*
gb_palette

Returns the colour to be written into the framebuffer associated with [colour].
*/
uint32_t gb_palette(int colour) {
    switch (colour){
        case 0x00: return 0xFFFFFFFF;
        case 0x01: return 0xC0C0C0FF;
        case 0x02: return 0x606060FF;
        default:   return 0x000000FF;
    }
}
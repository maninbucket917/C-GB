#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#include "memory.h"
#include "ppu.h"

const uint32_t palettes[NUM_PALETTES][6] = {{PALETTE_0}, {PALETTE_1}, {PALETTE_2}, {PALETTE_3}, {PALETTE_4}, {PALETTE_5}};

/*
gb_palette

Map a 2-bit colour index to an RGB pixel using the active palette.
*/
static inline uint32_t gb_palette(uint8_t palette_id, uint8_t colour) {
    return palettes[palette_id][colour & 3];
}

/*
ppu_update_stat

Update STAT register and check for STAT interrupt conditions.
The STAT interrupt line is active if ANY enabled condition is true.
An interrupt is only requested on a rising edge.
*/
static inline void ppu_update_stat(PPU *ppu, Memory *mem) {
    uint8_t stat = mem->io[0x41];
    uint8_t lyc = mem->io[0x45];

    // Update LYC coincidence flag
    bool lyc_match = (ppu->ly == lyc);
    if (lyc_match) {
        stat |= 0x04;
    } else {
        stat &= ~0x04;
    }

    // Update mode bits
    stat = (stat & 0xFC) | (ppu->mode & 0x03);
    mem->io[0x41] = stat | 0x80;

    // Calculate if STAT interrupt line is high
    // Each condition is checked against its enable bit
    bool stat_line = false;

    if ((stat & 0x40) && lyc_match) {
        stat_line = true; // LYC=LY interrupt
    }
    if ((stat & 0x20) && ppu->mode == 2) {
        stat_line = true; // Mode 2 (OAM) interrupt
    }
    if ((stat & 0x10) && ppu->mode == 1) {
        stat_line = true; // Mode 1 (VBlank) interrupt
    }
    if ((stat & 0x08) && ppu->mode == 0) {
        stat_line = true; // Mode 0 (HBlank) interrupt
    }

    // Request STAT interrupt on rising edge only
    if (stat_line && !ppu->stat_irq_line) {
        mem->io[0x0F] |= 0x02;
    }

    ppu->stat_irq_line = stat_line;
}

/*
ppu_read_tile_pixel

Return the 2-bit colour for a background/window pixel at [x, y].
*/
static inline uint8_t ppu_read_tile_pixel(Memory *mem, int tiledata_unsigned, uint16_t tilemap, uint16_t x, uint16_t y) {

    // Get tile row and column
    uint16_t tile_row = y >> 3;
    uint16_t tile_col = x >> 3;

    // Get the index in memory of the pixel
    uint16_t index_addr = tilemap + (tile_row << 5) + tile_col;
    uint8_t raw_index = mem_read8(mem, index_addr);

    uint16_t tile_addr = tiledata_unsigned ? (0x8000 + (raw_index << 4)) : (0x9000 + (((int8_t)raw_index) << 4));

    // Get colour map of pixel by adding both bitplanes

    uint8_t b1 = mem_read8(mem, tile_addr + ((y & 7) << 1));
    uint8_t b2 = mem_read8(mem, tile_addr + ((y & 7) << 1) + 1);
    uint8_t bit = 7 - (x & 7);
    return ((b2 >> bit) & 1) << 1 | ((b1 >> bit) & 1);
}

/*
ppu_read_sprite_pixel

Return the colour of the sprite pixel at [x, y] for a given tile line.
*/
static inline uint8_t ppu_read_sprite_pixel(Memory *mem, uint16_t tile_addr, uint8_t x, uint8_t y, bool xflip) {
    x = xflip ? (7 - x) : x;

    uint8_t b1 = mem_read8(mem, tile_addr + ((y & 7) << 1));
    uint8_t b2 = mem_read8(mem, tile_addr + ((y & 7) << 1) + 1);
    uint8_t bit = 7 - (x & 7);
    return ((b2 >> bit) & 1) << 1 | ((b1 >> bit) & 1);
}

/*
ppu_init

Initialize the PPU and create the SDL window/renderer/texture.
*/
Status ppu_init(PPU *ppu, GB *gb) {
    if (gb == NULL) {
        return ERR_NO_PARENT;
    }
    ppu->gb = gb;

    ppu_reset(ppu);

    ppu->window = SDL_CreateWindow("C-GB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * SCREEN_SCALING, 144 * SCREEN_SCALING, SDL_WINDOW_SHOWN);
    if (!ppu->window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return ERR_SDL_NOT_INITIALIZED;
    }

    ppu->renderer = SDL_CreateRenderer(ppu->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ppu->renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(ppu->window);
        return ERR_SDL_NOT_INITIALIZED;
    }

    ppu->texture = SDL_CreateTexture(ppu->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if (!ppu->texture) {
        printf("Failed to create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ppu->renderer);
        SDL_DestroyWindow(ppu->window);
        return ERR_SDL_NOT_INITIALIZED;
    }

    return OK;
}

/*
ppu_reset

Reset PPU state to power-on values without recreating SDL resources.
*/
void ppu_reset(PPU *ppu) {
    ppu->dot = 0;
    ppu->ly = 0;
    ppu->mode = 2;

    ppu->stat_irq_line = false;

    ppu->window_line = 0;
    ppu->window_drawn = 0;

    ppu->palette_id = DEFAULT_PALETTE;
}

/*
ppu_draw_tiles

Render the background/window tiles for the current scanline into the framebuffer.
*/
static void ppu_draw_tiles(PPU *ppu, Memory *mem) {

    // Read registers
    uint8_t lcdc = mem_read8(mem, 0xFF40);
    uint8_t ly = ppu->ly;
    uint8_t scx = mem_read8(mem, 0xFF43);
    uint8_t scy = mem_read8(mem, 0xFF42);
    uint8_t wx = mem_read8(mem, 0xFF4B);
    uint8_t wy = mem_read8(mem, 0xFF4A);

    // Check for background and window enable bits
    bool bg_enable = lcdc & 0x01;
    bool win_enable = lcdc & 0x20;

    // Check which tilemap to use and whether to use signed or unsigned indexing
    uint16_t bg_map = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    uint16_t win_map = (lcdc & 0x40) ? 0x9C00 : 0x9800;
    bool unsigned_tiles = lcdc & 0x10;

    // Get the colour of each background pixel and update the framebuffer
    for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {

        uint8_t bg_colour = 0;

        if (bg_enable) {
            uint16_t bx = (x + scx) & 0xFF;
            uint16_t by = (ly + scy) & 0xFF;
            bg_colour = ppu_read_tile_pixel(mem, unsigned_tiles, bg_map, bx, by);
        }

        if (win_enable && ly >= wy && x >= (int)wx - 7) {
            uint16_t wx_x = x - ((int)wx - 7);
            uint16_t wy_y = ppu->window_line;
            bg_colour = ppu_read_tile_pixel(mem, unsigned_tiles, win_map, wx_x, wy_y);
            ppu->window_drawn = 1;
        }

        // Check palette
        uint8_t bgp = mem_read8(mem, 0xFF47);
        uint8_t mapped_colour = (bgp >> (bg_colour << 1)) & 0x03;
        ppu->framebuffer[ly * SCREEN_WIDTH + x] = gb_palette(ppu->palette_id, mapped_colour);
    }
}

/*
ppu_draw_sprites

Render visible sprites for the current scanline into the framebuffer.
*/
static void ppu_draw_sprites(PPU *ppu, Memory *mem) {

    uint8_t lcdc = mem_read8(mem, 0xFF40);

    // Check if sprites are enabled
    if (!(lcdc & 0x02)) {
        return;
    }

    uint8_t ly = ppu->ly;

    // 8x16 sprite
    bool tall_sprites = lcdc & 0x04;
    uint8_t height = tall_sprites ? 16 : 8;

    // Array for sprites
    Sprite line_sprites[10];

    // Populate line_sprites array
    uint8_t drawn = 0;
    for (uint8_t i = 0; i < 40 && drawn < 10; i++) {

        // Fetch address of sprite in OAM
        uint16_t addr = 0xFE00 + (i * 4);

        // Fetch remaining sprite attributes
        int sprite_y = mem_read8(mem, addr) - 16;
        int sprite_x = mem_read8(mem, addr + 1) - 8;
        uint8_t tile = mem_read8(mem, addr + 2);
        uint8_t attr = mem_read8(mem, addr + 3);

        // Check that sprite line is onscreen
        if (ly < sprite_y || ly >= sprite_y + height) {
            continue;
        }

        // Create Sprite and add it to array
        line_sprites[drawn] = (Sprite){i, sprite_y, sprite_x, tile, attr};
        drawn++;
    }

    // Sort sprites by X-value, then by OAM order
    for (int i = 0; i < drawn - 1; i++) {
        for (int j = 0; j < drawn - 1 - i; j++) {
            Sprite *a = &line_sprites[j];
            Sprite *b = &line_sprites[j + 1];

            bool swap = (a->x > b->x) || (a->x == b->x && a->index > b->index);

            if (swap) {
                Sprite tmp = *a;
                *a = *b;
                *b = tmp;
            }
        }
    }

    // Draw all sprites on current line from right to left
    for (int i = drawn - 1; i >= 0; i--) {

        // Get sprite attributes
        Sprite sprite = line_sprites[i];
        uint8_t priority = sprite.attr & 0x80;
        uint8_t yflip = sprite.attr & 0x40;
        uint8_t xflip = sprite.attr & 0x20;
        uint8_t palette = sprite.attr & 0x10;

        // Get sprite palette
        uint8_t obj_palette = mem_read8(mem, palette ? 0xFF49 : 0xFF48);

        // Get line and apply vertical flip
        int line = yflip ? (height - 1 - (ly - sprite.y)) : (ly - sprite.y);

        // Get tile index
        uint16_t tile_index = sprite.tile;

        // Handle line of tall sprites
        if (tall_sprites) {
            tile_index &= 0xFE;
            if (line >= 8) {
                tile_index += 1;
                line -= 8;
            }
        }

        // Get address of tile
        uint16_t tile_addr = 0x8000 + (tile_index << 4);

        // Draw sprite line
        for (int x = 0; x < 8; x++) {
            int pixel_x = sprite.x + x;

            // Don't draw off the edge of the screen
            if (pixel_x < 0 || pixel_x >= SCREEN_WIDTH) {
                continue;
            }

            // Get mapped colour of sprite pixel
            uint8_t colour = ppu_read_sprite_pixel(mem, tile_addr, x, line, xflip);

            // Don't draw transparent pixels
            if (colour == 0) {
                continue;
            }

            // If priority is set, only draw over bg colour 0
            if (priority) {
                uint32_t bg = ppu->framebuffer[ly * SCREEN_WIDTH + pixel_x];
                if (bg != gb_palette(ppu->palette_id, 0)) {
                    continue;
                }
            }

            // Remap colour to obj_palette and update framebuffer
            uint8_t mapped = (obj_palette >> (colour * 2)) & 3;
            ppu->framebuffer[ly * SCREEN_WIDTH + pixel_x] = gb_palette(ppu->palette_id, mapped);
        }
    }
}

/*
ppu_mode_change

Update PPU mode.
*/
static inline void ppu_mode_change(PPU *ppu, uint8_t mode) {
    ppu->mode = mode;
}

/*
ppu_step

Advance PPU timing by [cycles], handling mode transitions and scanline draw.
*/
void ppu_step(PPU *ppu, Memory *mem, int cycles) {

    // Emulate one cycle at a time
    while (cycles--) {

        // If LCDC bit 7 is clear, PPU is disabled
        uint8_t lcdc = mem_read8(mem, 0xFF40);
        if (!(lcdc & 0x80)) {
            ppu->dot = 0;
            ppu->ly = 0;
            ppu->mode = 0;
            ppu->stat_irq_line = false;
            mem->io[0x44] = 0;
            // Update STAT mode bits but don't trigger interrupts when LCD is off
            mem->io[0x41] = (mem->io[0x41] & 0xFC) | 0x80;
            continue;
        }

        // Each cycle increments dot counter by 1
        ppu->dot++;

        switch (ppu->mode) {

        case 0: // HBlank

            if (ppu->dot >= 456) {

                // Increment LY at end of scanline
                ppu->dot = 0;
                ppu->ly++;
                mem->io[0x44] = ppu->ly;

                if (ppu->ly == 144) {
                    // Enter VBlank
                    ppu_mode_change(ppu, 1);
                    ppu_update_stat(ppu, mem);

                    // Request VBlank interrupt
                    mem->io[0x0F] |= 0x01;

                    // Present frame
                    SDL_UpdateTexture(ppu->texture, NULL, ppu->framebuffer, SCREEN_WIDTH * sizeof(uint32_t));
                    SDL_RenderClear(ppu->renderer);
                    SDL_RenderCopy(ppu->renderer, ppu->texture, NULL, NULL);
                    SDL_RenderPresent(ppu->renderer);

                } else {
                    // Next scanline is OAM scan
                    ppu_mode_change(ppu, 2);
                    ppu_update_stat(ppu, mem);
                }
            }

            break;

        case 1: // VBlank

            // Increment LY at end of scanline
            if (ppu->dot >= 456) {
                ppu->dot = 0;
                ppu->ly++;
                mem->io[0x44] = ppu->ly;

                // Reset LY and start OAM scan
                if (ppu->ly > 153) {
                    ppu->ly = 0;
                    ppu->window_line = 0;
                    mem->io[0x44] = 0;
                    ppu_mode_change(ppu, 2);
                }
                ppu_update_stat(ppu, mem);
            }
            break;

        case 2: // OAM scan

            // Change to drawing mode after 80 dots
            if (ppu->dot >= 80) {
                ppu_mode_change(ppu, 3);
                ppu_update_stat(ppu, mem);
            }
            break;

        case 3: // Drawing

            // Draw scanline at end of mode 3 and reset to HBlank
            if (ppu->dot >= 252) {

                ppu->window_drawn = 0;
                ppu_draw_tiles(ppu, mem);
                if (ppu->window_drawn) {
                    ppu->window_line++;
                }
                ppu_draw_sprites(ppu, mem);
                ppu_mode_change(ppu, 0);
                ppu_update_stat(ppu, mem);
            }
            break;
        }
    }
}

/*
ppu_check_stat

Wrapper to evaluate STAT interrupt conditions.
Called when LYC or STAT registers are written.
*/
void ppu_check_stat(PPU *ppu, Memory *mem) {
    // Only check if LCD is enabled
    if (mem->io[0x40] & 0x80) {
        ppu_update_stat(ppu, mem);
    }
}

/*
ppu_palette_swap

Cycle to the next selectable palette.
*/
void ppu_palette_swap(PPU *ppu) {
    ppu->palette_id++;
    ppu->palette_id %= NUM_PALETTES;
}

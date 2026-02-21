#include "keybinds.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

void show_keybind_menu(Keybinds *keybinds)
{
    // Keybind labels and pointers

    const char *actions[] = {
        "Up", "Down", "Left", "Right",
        "A", "B", "Start", "Select",
        "Open Menu", "Quick Reset",
        "Palette Cycle", "Toggle Turbo",
        "Pause/Unpause", "Toggle Fullscreen"
    };

    SDL_Keycode *binds[] = {
        &keybinds->up, &keybinds->down, &keybinds->left, &keybinds->right,
        &keybinds->a, &keybinds->b, &keybinds->start, &keybinds->select,
        &keybinds->keybinds_menu, &keybinds->reset,
        &keybinds->palette_swap, &keybinds->turbo,
        &keybinds->pause, &keybinds->fullscreen
    };

    const int num_actions = 14;

    // Constants for menu layout

    const int outer_margin = 10;     // space around menu
    const int item_height  = 40;     // row height
    const int item_gap     = 6;      // vertical space between rows
    const int item_padding = 6;      // padding inside each row
    const int column_gap   = 8;      // space between label and key
    const int text_padding = 8;      // left padding for text

    const int menu_width   = 400;
    const int num_visible  = 8;

    const int arrow_margin = 6;   // distance from menu border
    const int arrow_height = 12;
    const int arrow_half_width = 6;

    const int menu_height = (num_visible * item_height) + ((num_visible - 1) * item_gap) + (outer_margin * 2);

    // Setup SDL window and renderer
    SDL_Window *menu_window = SDL_CreateWindow(
        "C-GB: Keybind Configuration",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        menu_width + outer_margin * 2,
        menu_height + outer_margin * 2,
        SDL_WINDOW_SHOWN
    );

    if (!menu_window) return;

    SDL_Renderer *renderer = SDL_CreateRenderer(menu_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_DestroyWindow(menu_window);
        return;
    }

    if (TTF_Init() != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(menu_window);
        return;
    }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
    if (!font) font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 18);
    if (!font) {
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(menu_window);
        return;
    }

    // Current state of the menu

    int selected = 0;
    int scroll_offset = 0;
    int rebinding = 0;
    int running = 1;

    SDL_Event event;

    // Main loop

    while (running) {
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Menu background
        SDL_Rect menu_rect = {outer_margin, outer_margin, menu_width, menu_height};
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &menu_rect);

        // Draw visible items
        for (int i = 0; i < num_visible; i++) {
            int idx = i + scroll_offset;
            if (idx >= num_actions) break;

            // Background for the item
            SDL_Rect item_rect = {
                outer_margin * 2,
                outer_margin * 2 + i * (item_height + item_gap),
                menu_width - outer_margin * 2,
                item_height
            };

            // Highlight red if actively rebinding, blue if selected but not rebinding, gray otherwise
            if (rebinding && idx == selected) {
                SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
            } else if (idx == selected) {
                SDL_SetRenderDrawColor(renderer, 50, 100, 200, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
            }

            SDL_RenderFillRect(renderer, &item_rect);

            // Innter content area
            SDL_Rect content = {
                item_rect.x + item_padding,
                item_rect.y + item_padding,
                item_rect.w - item_padding * 2,
                item_rect.h - item_padding * 2
            };

            int half = (content.w - column_gap) / 2;

            SDL_Rect label_rect = {content.x, content.y, half, content.h};

            SDL_Rect key_rect = {content.x + half + column_gap, content.y, half, content.h};

            // Label box
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderFillRect(renderer, &label_rect);
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderDrawRect(renderer, &label_rect);

            // Key box
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            SDL_RenderFillRect(renderer, &key_rect);
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderDrawRect(renderer, &key_rect);

            SDL_Color text_color = {20, 20, 20, 255};

            // Action text
            SDL_Surface *text_surf = TTF_RenderText_Blended(font, actions[idx], text_color);
            if (text_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, text_surf);

                SDL_Rect dst = {
                    label_rect.x + text_padding,
                    label_rect.y + (label_rect.h - text_surf->h) / 2,
                    text_surf->w,
                    text_surf->h
                };

                SDL_RenderCopy(renderer, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(text_surf);
            }

            // Key text
            char keyname[64];
            snprintf(keyname, sizeof(keyname), "%s", SDL_GetKeyName(*binds[idx]));

            SDL_Surface *key_surf = TTF_RenderText_Blended(font, keyname, text_color);
            if (key_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, key_surf);

                SDL_Rect dst = {
                    key_rect.x + text_padding,
                    key_rect.y + (key_rect.h - key_surf->h) / 2,
                    key_surf->w,
                    key_surf->h
                };

                SDL_RenderCopy(renderer, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(key_surf);
            }
        }

        // Draw scroll indicators if needed
        if (scroll_offset > 0) {
            SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);

            int center_x = menu_rect.x + menu_rect.w / 2;
            int top_y = menu_rect.y + arrow_margin;

            for (int y = 0; y < arrow_height; y++) {
                int half = (arrow_half_width * y) / arrow_height;
                SDL_RenderDrawLine(renderer, center_x - half, top_y + y, center_x + half, top_y + y);
            }
        }

        if (scroll_offset < num_actions - num_visible) {
            SDL_SetRenderDrawColor(renderer, 200, 40, 40, 255);

            int center_x = menu_rect.x + menu_rect.w / 2;
            int top_y = menu_rect.y + menu_rect.h - arrow_margin - arrow_height;

            for (int y = 0; y < arrow_height; y++) {
                int half = (arrow_half_width * (arrow_height - y)) / arrow_height;

                SDL_RenderDrawLine(renderer, center_x - half, top_y + y, center_x + half, top_y + y);
            }
        }

        SDL_RenderPresent(renderer);

        // Event handling

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) {
                running = 0;
            }

            if (event.type == SDL_KEYDOWN) {
                if (rebinding) {
                    *binds[selected] = event.key.keysym.sym;
                    rebinding = 0;
                } else {
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                            if (selected > 0) selected--;
                            if (selected < scroll_offset) scroll_offset = selected;
                            break;

                        case SDLK_DOWN:
                            if (selected < num_actions - 1) selected++;
                            if (selected >= scroll_offset + num_visible)
                                scroll_offset = selected - num_visible + 1;
                            break;

                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            rebinding = 1;
                            break;

                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                    }
                }
            }
        }

        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(menu_window);
}

int save_keybinds(const Keybinds *k) {
    FILE *f = fopen("keybinds.cfg", "wb");
    if (!f) return 0;

    fwrite(k, sizeof(Keybinds), 1, f);
    fclose(f);
    return 1;
}

int load_keybinds(Keybinds *k) {
    FILE *f = fopen("keybinds.cfg", "rb");
    if (!f) return 0;

    size_t n = fread(k, sizeof(Keybinds), 1, f);
    fclose(f);

    return n == 1;
}
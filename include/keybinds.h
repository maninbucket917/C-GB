#ifndef KEYBINDS_H
#define KEYBINDS_H

#include <SDL2/SDL.h>

// Struct of keybinds for all actions, passed to the keybind menu
typedef struct {
    SDL_Keycode up;
    SDL_Keycode down;
    SDL_Keycode left;
    SDL_Keycode right;
    SDL_Keycode a;
    SDL_Keycode b;
    SDL_Keycode start;
    SDL_Keycode select;
    SDL_Keycode keybinds_menu;
    SDL_Keycode reset;
    SDL_Keycode palette_swap;
    SDL_Keycode turbo;
    SDL_Keycode pause;
    SDL_Keycode fullscreen;
} Keybinds;

// Function to show the keybind configuration menu, allowing the user to view and change keybinds
void show_keybind_menu(Keybinds *keybinds);

// Functions to save and load keybinds to/from a file for persistence
int save_keybinds(const Keybinds *k);
int load_keybinds(Keybinds *k);

#endif
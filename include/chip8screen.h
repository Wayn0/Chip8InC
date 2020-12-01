#ifndef CHIP8_SCREEN_H
#define CHIP8_SCREEN_H

#include "config.h"
#include <stdbool.h>

struct chip8_screen
{
    bool pixels[CHIP8_WIDTH][CHIP8_HEIGHT];
};


void chip8_screen_clear(struct chip8_screen* screen);
void chip8_screen_set_on(struct chip8_screen* screen, int x, int y);
void chip8_screen_set_off(struct chip8_screen* screen, int x, int y);
bool chip8_screen_draw_sprite(struct chip8_screen* screen, int x, int y, const unsigned char* sprite, int num);
bool chip8_screen_is_set(struct chip8_screen* screen, int x, int y);

#endif
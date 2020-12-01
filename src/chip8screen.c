#include "chip8screen.h"
#include <memory.h>
#include <assert.h>

static void chip8_check_screen_bounds(int x, int y)
{
    assert(x >=0 && x < CHIP8_WIDTH && y >= 0 && y < CHIP8_HEIGHT);
}

void chip8_screen_clear(struct chip8_screen* screen)
{
    memset(screen->pixels, 0, sizeof(screen->pixels));
}

void chip8_screen_set_on(struct chip8_screen* screen, int x, int y)
{
    chip8_check_screen_bounds(x, y);
    screen->pixels[x][y] = true;
}

void chip8_screen_set_off(struct chip8_screen* screen, int x, int y)
{
    chip8_check_screen_bounds(x, y);
    screen->pixels[x][y] = false;
}

bool chip8_screen_draw_sprite(struct chip8_screen* screen, int x, int y, const unsigned char* sprite, int num)
{
    bool pixel_collision = false;

    for(int ly = 0; ly < num; ly++)
    {
        char c = sprite[ly];
        for(int lx = 0; lx < 8; lx++)
        {
            if((c & (0b10000000 >> lx)) == 0)
            {
                continue;
            }

            if(screen->pixels[(lx+x) % CHIP8_WIDTH][(ly+y) % CHIP8_HEIGHT])
            {
                pixel_collision = true;
            }

            screen->pixels[(lx+x) % CHIP8_WIDTH][(ly+y) % CHIP8_HEIGHT] ^= true;
        }
    }

    return pixel_collision;
}


bool chip8_screen_is_set(struct chip8_screen* screen, int x, int y)
{
    chip8_check_screen_bounds(x, y);
    return screen->pixels[x][y];
}
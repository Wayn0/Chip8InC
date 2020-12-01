#include "chip8.h"
#include <memory.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "SDL2/SDL.h"

const char chip8_default_character_set[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //zero
    0x20, 0x60, 0x20, 0x20, 0x70, //one
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //two
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //three
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //four
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //five
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //six
    0xF0, 0x10, 0x20, 0x40, 0x40, //seven
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //eight
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //nine
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80, //F
};

void chip8_init(struct chip8 *chip8)
{
    memset(chip8, 0, sizeof(struct chip8));
    memcpy(&chip8->memory.memory, chip8_default_character_set, sizeof(chip8_default_character_set));
}

void chip8_load(struct chip8 *chip8, const char *buf, size_t size)
{
    assert(size + CHIP8_LOAD_ADDRESS < CHIP8_MEMORY_SIZE);
    memcpy(&chip8->memory.memory[CHIP8_LOAD_ADDRESS], buf, size);
}

static char chip8_wait_for_key(struct chip8 *chip8)
{
    SDL_Event event;
    while (SDL_WaitEvent(&event))
    {
        if(event.type != SDL_KEYDOWN)
            continue;
        
        char c = event.key.keysym.sym;
        char vkey = chip8_keyboard_map(&chip8->keyboard, c);
        if (vkey != -1)
        {
            return vkey;
        }
    }
    return -1;
}

static void chip8_exec_extended_prefix_F(struct chip8 *chip8, unsigned short opcode)
{
    unsigned short x = (opcode >> 8) & 0x000F;
    unsigned short y = (opcode >> 4) & 0x000F;
    switch (opcode & 0x00FF)
    {
        // Fx07 - LD Vx, DT - Set Vx = delay timer value.
        case 0x07:
            chip8->registers.V[x] = chip8->registers.delay_timer;
        break;

        // Fx0A - LD Vx, K - Wait for a key press, store the value of the key in Vx.
        case 0x0A:
            chip8->registers.V[x] = chip8_wait_for_key(chip8);
        break;

        // Fx15 - LD DT, Vx - Set delay timer = Vx.
        case 0x15:
            chip8->registers.delay_timer = chip8->registers.V[x];
        break;

        // Fx18 - LD ST, Vx - Set sound timer = Vx.
        case 0x18:
            chip8->registers.sound_timer = chip8->registers.V[x];
        break;

        // Fx1E - ADD I, Vx - Set I = I + Vx.
        case 0x1E:
            chip8->registers.I = chip8->registers.I + chip8->registers.V[x];
        break;

        // Fx29 - LD F, Vx - Set I = location of sprite for digit Vx.
        case 0x29:
            chip8->registers.I = CHIP8_CHARACTER_SET_LOAD_ADDRESS + (chip8->registers.V[x] * CHIP8_CHARACTER_SET_DEFAULT_HEIGHT);
        break;

        // Fx33 - LD B, Vx - Store BCD representation of Vx in memory locations I, I+1, and I+2.
        // The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
        case 0x33:
        {
            unsigned char hundreds = chip8->registers.V[x] / 100;
            unsigned char tens = chip8->registers.V[x] / 10 % 10;
            unsigned char units = chip8->registers.V[x] % 10;
            chip8_memory_set(&chip8->memory, chip8->registers.I, hundreds);
            chip8_memory_set(&chip8->memory, chip8->registers.I+1, tens);
            chip8_memory_set(&chip8->memory, chip8->registers.I+2, units);
        }
        break;

        // Fx55 - LD [I], Vx - Store registers V0 through Vx in memory starting at location I.
        case 0x55:
            for(int i = 0; i <= x; i++)
            {
                chip8_memory_set(&chip8->memory, chip8->registers.I+i, chip8->registers.V[i]);
            }
        break;

        // Fx65 - LD Vx, [I] - Read registers V0 through Vx from memory starting at location I.
        case 0x65:
            for(int i = 0; i <= x; i++)
            {
                chip8->registers.V[i] = chip8_memory_get(&chip8->memory, chip8->registers.I+i);
            }
        break;
    }
}

static void chip8_exec_extended_prefix_8(struct chip8 *chip8, unsigned short opcode)
{
    unsigned short nnn = opcode & 0x0FFF;
    unsigned short x = (opcode >> 8) & 0x000F;
    unsigned short y = (opcode >> 4) & 0x000F;
    unsigned short kk = opcode & 0x00FF;
    unsigned short last_bit = opcode & 0x000F;
    unsigned short temp = 0;
    switch (last_bit) 
    {
        // 8xy0 - LD Vx, Vy - Set Vx = Vy.
        case 0x00:
            chip8->registers.V[x] = chip8->registers.V[y];
            break;
            
        // 8xy1 - OR Vx, Vy - Set Vx = Vx OR Vy.
        case 0x01:
            chip8->registers.V[x] = chip8->registers.V[x] | chip8->registers.V[y];
            break;

        // 8xy2 - AND Vx, Vy - Set Vx = Vx AND Vy.
        case 0x02:
            chip8->registers.V[x] = chip8->registers.V[x] & chip8->registers.V[y];
            break;

        // 8xy3 - XOR Vx, Vy - Set Vx = Vx XOR Vy.
        case 0x03:
            chip8->registers.V[x] = chip8->registers.V[x] ^ chip8->registers.V[y];
            break;

        // 8xy4 - ADD Vx, Vy - Set Vx = Vx + Vy, set VF = carry.
        case 0x04:
            temp = chip8->registers.V[x] + chip8->registers.V[y];
            chip8->registers.V[0x0F] = 0x00;
            if(temp > 0xFF)
            {
                chip8->registers.V[0x0F] = 0x01;
            }
            chip8->registers.V[x] = temp;
            break;

        // 8xy5 - SUB Vx, Vy - Set Vx = Vx - Vy, set VF = NOT borrow.
        case 0x05:
            chip8->registers.V[0x0F] = 0x00;
            if(chip8->registers.V[x] > chip8->registers.V[y])
            {
                chip8->registers.V[0x0F] = 0x01;
            }
            chip8->registers.V[x] = chip8->registers.V[x] - chip8->registers.V[y];
            break;

        // 8xy6 - SHR Vx {, Vy} - Set Vx = Vx SHR 1.
        // If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
        case 0x06:
            chip8->registers.V[0x0F] = chip8->registers.V[x] & 0x01;
            chip8->registers.V[x] = chip8->registers.V[x] / 2;
            break;

        // 8xy7 - SUBN Vx, Vy - Set Vx = Vy - Vx, set VF = NOT borrow.
        // If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
        case 0x07:
            chip8->registers.V[0x0F] = chip8->registers.V[y] > chip8->registers.V[x];
            chip8->registers.V[x] = chip8->registers.V[y] - chip8->registers.V[x];
            break;

        // 8xyE - SHL Vx {, Vy} - Set Vx = Vx SHL 1.
        // If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
        case 0x0E:
            chip8->registers.V[0x0F] = chip8->registers.V[x] & 0b10000000;
            chip8->registers.V[x] = chip8->registers.V[x] * 2;
            break;
    }
}

static void chip8_exec_extended(struct chip8 *chip8, unsigned short opcode)
{
    unsigned short nnn = opcode & 0x0FFF;
    unsigned short x = (opcode >> 8) & 0x000F;
    unsigned short y = (opcode >> 4) & 0x000F;
    unsigned short kk = opcode & 0x00FF;
    unsigned short n = opcode & 0x000F;
    switch (opcode & 0xF000)
    {
        // 1nnn - JP addr - Jump to location nnn.
        case 0x1000:
            chip8->registers.PC = nnn;
            break;

        // 2nnn - CALL addr - Call subroutine at nnn.
        case 0x2000:
            chip8_stack_push(chip8, chip8->registers.PC);
            chip8->registers.PC = nnn;
            break;

        // 3xkk - SE Vx, byte - Skip next instruction if Vx = kk.
        case 0x3000:
            if(chip8->registers.V[x] == kk)
            {
                chip8->registers.PC += 2;
            }
            break;

        // 4xkk - SNE Vx, byte - Skip next instruction if Vx != kk.
        case 0x4000:
            if(chip8->registers.V[x] != kk)
            {
                chip8->registers.PC += 2;
            }
            break;

        // 5xy0 - SE Vx, Vy - Skip next instruction if Vx = Vy.
        case 0x5000:
            if(chip8->registers.V[x] == chip8->registers.V[y])
            {
                chip8->registers.PC += 2;
            }
            break;

        // 6xkk - LD Vx, byte - Set Vx = kk.
        case 0x6000:
            chip8->registers.V[x] = kk;
            break;

        // 7xkk - ADD Vx, byte - Set Vx = Vx + kk.
        case 0x7000:
            chip8->registers.V[x] += kk;
            break;

        // 8 prefix
        case 0x8000:
            chip8_exec_extended_prefix_8(chip8, opcode);
        break;

        // 9xy0 - SNE Vx, Vy - Skip next instruction if Vx != Vy.
        case 0x9000:
            if(chip8->registers.V[x] != chip8->registers.V[y])
            {
                chip8->registers.PC += 2;
            }
            break;

        // Annn - LD I, addr - Set I = nnn.
        case 0xA000:
            chip8->registers.I = nnn;
            break;

        // Bnnn - JP V0, addr - Jump to location nnn + V0.
        case 0xB000:
            chip8->registers.PC = chip8->registers.V[0x00] + nnn;
            break;

        // Cxkk - RND Vx, byte - Set Vx = random byte AND kk.
        case 0xC000:
            srand(clock());
            chip8->registers.V[x] = (rand() % 255) & kk;
            break;

        // Dxyn - DRW Vx, Vy, nibble
        // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
        case 0xD000:
        {
            const unsigned char* sprite = &chip8->memory.memory[chip8->registers.I];
            chip8->registers.V[0x0F] = chip8_screen_draw_sprite(&chip8->screen, chip8->registers.V[x], chip8->registers.V[y], sprite, n);
        }
        break;

        // E Prefix
        // Keyboard
        case 0xE000:
            switch (opcode & 0x00FF)
            {
                // Ex9E - SKP Vx - Skip next instruction if key with the value of Vx is pressed.
                case 0x9E:
                    if(chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x]))
                    {
                        chip8->registers.PC += 2;
                    }
                    break;
                
                // ExA1 - SKNP Vx - Skip next instruction if key with the value of Vx is not pressed.
                case 0xA1:
                    if(!chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x]))
                    {
                        chip8->registers.PC += 2;
                    }
                    break;            
            }
        break;

        case 0xF000:
            chip8_exec_extended_prefix_F(chip8, opcode);
        break;


        
        default:
            break;
    }

}

void chip8_exec(struct chip8 *chip8, unsigned short opcode)
{
    switch (opcode)
    {
        // CLS: clear screen
        case 0x00E0:
            chip8_screen_clear(&chip8->screen);
            break;

        // RET: return from sub 
        case 0x00EE:
            chip8->registers.PC = chip8_stack_pop(chip8);
            break;

        // NON FIXED OPCODES
        default:
            chip8_exec_extended(chip8, opcode);
            break;
    }
}
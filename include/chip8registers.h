#ifndef CHIP8_REGISTERS_H
#define CHIP8_REGISTERS_H

#include "config.h"

struct chip8_registers
{
    unsigned char V[CHIP8_DATA_REGISTERS];  // Data registers 8bit
    unsigned short I;                       // I register 16bit
    unsigned short PC;                      // Program Counter 16bit
    unsigned char SP;                       // Stack Poitner 8bit
    unsigned char delay_timer;               // Delay timer 8bit
    unsigned char sound_timer;              // Sound timer 8bit
  
};

#endif
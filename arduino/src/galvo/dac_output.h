#pragma once
#include <Arduino.h>

// DAC configuration
#define DAC_FLAGS_A 0b00010000
#define DAC_FLAGS_B 0b10010000

// Debug flags
extern bool g_flip_x;
extern bool g_flip_y;
extern bool g_swap_xy;
extern bool g_dac_serial;

// Function declarations
void dac_output_init(void);
void outputDAC(uint16_t x, uint16_t y);

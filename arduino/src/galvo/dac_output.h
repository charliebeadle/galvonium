#pragma once
#include <Arduino.h>

// DAC configuration
#define DAC_FLAGS_A 0b00010000
#define DAC_FLAGS_B 0b10010000

// Function declarations
void dac_output_init(void);
void outputDAC(uint8_t x, uint8_t y);

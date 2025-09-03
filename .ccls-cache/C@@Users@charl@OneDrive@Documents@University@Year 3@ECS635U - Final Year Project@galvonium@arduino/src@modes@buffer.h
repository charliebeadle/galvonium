#pragma once
#include "../config/config.h"
#include "../config/globals.h"
#include <Arduino.h>

// Fixed maximum size for array declarations (must be known at compile time)
#define MAX_STEPS_FIXED 256

// Dynamic size based on config for runtime validation
#define MAX_STEPS (g_config.max_step_length + 1)

// === FUNCTION DECLARATIONS ===
void buffer_init();
void buffer_clear(volatile Step *buf);
int buffer_write(volatile Step *buf, int idx, uint8_t x, uint8_t y,
                 uint8_t flags);
void buffer_swap();

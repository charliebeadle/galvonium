#pragma once
#include "../config/config.h"
#include <Arduino.h>

// Fixed maximum size for array declarations (must be known at compile time)
#define MAX_STEPS_FIXED 256

// Dynamic size based on config for runtime validation
#define MAX_STEPS (g_config.max_step_length + 1)

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t flags;
} Step;

// === EXTERN DECLARATIONS FOR GLOBALS ===
extern volatile Step g_buffer_A[MAX_STEPS_FIXED];
extern volatile Step g_buffer_B[MAX_STEPS_FIXED];
extern volatile Step *g_buffer_active;
extern volatile Step *g_buffer_inactive;
extern volatile int g_buffer_active_steps;
extern volatile int g_buffer_inactive_steps;

// === FUNCTION DECLARATIONS ===
void buffer_init();
void buffer_clear(volatile Step *buf);
int buffer_write(volatile Step *buf, int idx, uint8_t x, uint8_t y,
                 uint8_t flags);
void buffer_swap();

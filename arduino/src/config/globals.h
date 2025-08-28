#pragma once

#include <Arduino.h>

// Forward declarations to avoid including full headers
struct GalvoConfig;

// Step struct definition (needed for array declarations)
typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t flags;
} Step;

// InterpolationState struct definition (needed for global declaration)
typedef struct {
  uint16_t current_x, current_y;
  uint16_t target_x, target_y;
  int16_t step_x, step_y;
  uint8_t steps_remaining;
  uint8_t distance;
  bool is_active;
} InterpolationState;

// === DEBUG/CONFIG GLOBALS ===
extern bool g_verbose;
extern bool g_flip_x;
extern bool g_flip_y;
extern bool g_swap_xy;
extern bool g_dac_serial;

// === TIMER/CORE GLOBALS ===
extern volatile bool g_frame_shown_once;
extern volatile bool g_swap_requested;
extern volatile int g_current_step;
extern volatile uint8_t g_last_x;
extern volatile uint8_t g_last_y;

// === BUFFER GLOBALS ===
extern volatile Step
    g_buffer_A[256]; // Use fixed size instead of MAX_STEPS_FIXED
extern volatile Step g_buffer_B[256];
extern volatile Step *g_buffer_active;
extern volatile Step *g_buffer_inactive;
extern volatile int g_buffer_active_steps;
extern volatile int g_buffer_inactive_steps;

// === INTERPOLATION GLOBALS ===
extern InterpolationState g_interpolation;

// === CONFIG GLOBALS ===
extern GalvoConfig g_config;

// === SERIAL/COMMUNICATION GLOBALS ===
extern char g_parse_buf[12]; // Use fixed sizes instead of constants
extern char g_serial_buf[24];
extern int g_serial_buf_pos;

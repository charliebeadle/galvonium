#pragma once
#include <Arduino.h>

typedef struct {
  uint16_t current_x, current_y;
  uint16_t target_x, target_y;
  int16_t step_x, step_y;
  uint8_t steps_remaining;
  uint8_t distance;
  bool is_active;
} InterpolationState;

extern InterpolationState g_interpolation;

bool interpolation_init(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
bool interpolation_next_point();
bool interpolation_is_active(void);

uint8_t chebyshev_distance(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
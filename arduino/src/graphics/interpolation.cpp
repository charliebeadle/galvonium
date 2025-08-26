#include "interpolation.h"
#include "../config/config.h"

// InterpolationState g_interpolation is now defined in globals.cpp

bool interpolation_init(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  g_interpolation.distance = chebyshev_distance(x0, y0, x1, y1);
  if (g_interpolation.distance > g_config.max_step_length) {
    g_interpolation.steps_remaining =
        (g_interpolation.distance + g_config.max_step_length - 1) /
        g_config.max_step_length;

    g_interpolation.step_x = (int16_t)x1 - (int16_t)x0;
    g_interpolation.step_y = (int16_t)y1 - (int16_t)y0;

    g_interpolation.step_x = (int16_t)(((int32_t)g_interpolation.step_x << 8) /
                                       g_interpolation.steps_remaining);
    g_interpolation.step_y = (int16_t)(((int32_t)g_interpolation.step_y << 8) /
                                       g_interpolation.steps_remaining);

    g_interpolation.current_x = (uint16_t)x0 << 8;
    g_interpolation.current_y = (uint16_t)y0 << 8;

    g_interpolation.target_x = (uint16_t)x1 << 8;
    g_interpolation.target_y = (uint16_t)y1 << 8;

    g_interpolation.is_active = true;
    return true;
  }
  g_interpolation.is_active = false;
  return false;
}

bool interpolation_next_point() {
  if (!g_interpolation.is_active || g_interpolation.steps_remaining == 0) {
    g_interpolation.is_active = false;
    return false;
  }

  g_interpolation.steps_remaining--;

  if (g_interpolation.steps_remaining == 0) {
    // Final step - snap to target
    g_interpolation.current_x = g_interpolation.target_x;
    g_interpolation.current_y = g_interpolation.target_y;
    g_interpolation.is_active = false;
  } else {
    // Intermediate step
    g_interpolation.current_x += g_interpolation.step_x;
    g_interpolation.current_y += g_interpolation.step_y;
  }

  return true;
}

bool interpolation_is_active(void) { return g_interpolation.is_active; }

uint8_t chebyshev_distance(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  uint8_t dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
  uint8_t dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);

  return (dx > dy) ? dx : dy;
}

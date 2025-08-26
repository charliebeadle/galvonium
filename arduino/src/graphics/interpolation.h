#pragma once
#include "../config/globals.h"
#include <Arduino.h>

bool interpolation_init(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
bool interpolation_next_point();
bool interpolation_is_active(void);

uint8_t chebyshev_distance(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
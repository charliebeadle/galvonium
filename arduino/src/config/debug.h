#pragma once

#include <Arduino.h>

extern bool g_verbose;

void debug_set_dac_serial(bool enable);
void debug_set_flip_x(bool enable);
void debug_set_flip_y(bool enable);
void debug_set_swap_xy(bool enable);
void debug_set_verbose(bool enable);
void debug_update_all(void);
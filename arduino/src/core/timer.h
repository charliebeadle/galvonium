#pragma once
#include "../config/config.h"
#include <Arduino.h>

// === EXTERN DECLARATIONS FOR GLOBALS ===
extern volatile bool g_frame_shown_once;
extern volatile bool g_swap_requested;
extern volatile int g_current_step;
extern volatile uint16_t g_last_x;
extern volatile uint16_t g_last_y;

// === FUNCTION DECLARATIONS ===
void initTimer();
void requestBufferSwap();
void set_pps(uint16_t pps);
void set_pps_from_config();
bool is_frame_shown_once();

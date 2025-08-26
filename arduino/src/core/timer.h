#pragma once
#include "../config/config.h"
#include <Arduino.h>

void initTimer();
void requestBufferSwap();
void set_pps(uint16_t pps);
void set_pps_from_config();
bool is_frame_shown_once();

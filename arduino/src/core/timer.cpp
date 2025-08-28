
#include "timer.h"
#include "../config/config.h"
#include "../galvo/dac_output.h"
#include "../graphics/interpolation.h"
#include "../modes/buffer.h"
#include <Arduino.h>

// Timer configuration constants
#define CLOCK_FREQ 16000000 // Arduino clock frequency in Hz
#define LASER_PIN 3         // Pin for laser control
#define LASER_BIT 0x01      // Bit mask for laser state in flags

// State variables are now defined in globals.cpp

void initTimer() {

  // Configure Timer1
  TCCR1A = 0; // Clear Timer/Counter Control Registers
  TCCR1B = 0;

  // Set CTC mode (Clear Timer on Compare Match)
  TCCR1B |= (1 << WGM12);

  // No prescaling for higher precision
  TCCR1B |= (1 << CS10);

  // Set compare value for the desired PPS
  set_pps_from_config();

  // Enable Timer1 compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  // Initialize DAC output
  dac_output_init();

  // Initialize laser pin
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  // Initialize state variables
  g_frame_shown_once = true;
  g_swap_requested = false;
  g_current_step = 0;
}

void set_pps(uint16_t pps) { OCR1A = (CLOCK_FREQ / pps) - 1; }

void set_pps_from_config() {
  uint16_t pps = config_get(PARAM_PPS);
  set_pps(pps);
}

// Function to request buffer swap (called from serial command handler)
void requestBufferSwap() { g_swap_requested = true; }

bool is_frame_shown_once() { return g_frame_shown_once; }

ISR(TIMER1_COMPA_vect) {
  // Check if we've completed the current frame
  if (g_current_step >= g_buffer_active_steps) {
    if (g_dac_serial && g_buffer_active_steps > 0 && !g_frame_shown_once) {
      Serial.println("END");
    }
    g_frame_shown_once = true;
    g_current_step = 0;
  }

  // Handle buffer swap request if ready
  if (g_swap_requested && g_frame_shown_once && g_current_step == 0) {

    buffer_swap();

    if (g_dac_serial && g_buffer_active_steps > 0) {
      Serial.println("START");
    }
    g_swap_requested = false;
    g_frame_shown_once = false;
    return;
  }

  if (interpolation_is_active()) {
    interpolation_next_point();
    outputDAC(g_interpolation.current_x, g_interpolation.current_y);
  } else {
    uint8_t x = g_buffer_active[g_current_step].x;
    uint8_t y = g_buffer_active[g_current_step].y;
    // uint8_t flags = g_buffer_active[g_current_step].flags;
    if (interpolation_init(g_last_x, g_last_y, x, y)) {
      interpolation_next_point();
      outputDAC(g_interpolation.current_x, g_interpolation.current_y);
    } else {
      outputDAC((uint16_t)x << 8, (uint16_t)y << 8);
    }
    g_last_x = x;
    g_last_y = y;
    g_current_step++;
  }

  // // Extract current step data

  // // Extract laser state from flags
  // bool laserOn = (flags & LASER_BIT) != 0;

  // // Output to DAC and control laser
  // outputDAC(x, y);
  // digitalWrite(LASER_PIN, laserOn ? HIGH : LOW);
}
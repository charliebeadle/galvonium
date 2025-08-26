
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

// State variables
volatile bool frameShownOnce = true;
volatile bool swapRequested = false;
volatile int currentStep = 0;

volatile uint16_t last_x = 0;
volatile uint16_t last_y = 0;

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
  frameShownOnce = true;
  swapRequested = false;
  currentStep = 0;
}

void set_pps(uint16_t pps) { OCR1A = (CLOCK_FREQ / pps) - 1; }

void set_pps_from_config() {
  uint16_t pps = config_get(PARAM_PPS);
  set_pps(pps);
}

// Function to request buffer swap (called from serial command handler)
void requestBufferSwap() { swapRequested = true; }

bool is_frame_shown_once() { return frameShownOnce; }

ISR(TIMER1_COMPA_vect) {
  // Check if we've completed the current frame
  if (currentStep >= buffer_active_steps) {
    frameShownOnce = true;
    currentStep = 0;
  }

  // Handle buffer swap request if ready
  if (swapRequested && frameShownOnce) {
    buffer_swap();
    swapRequested = false;
    frameShownOnce = false;
    return;
  }

  if (interpolation_is_active()) {
    interpolation_next_point();
    outputDAC(g_interpolation.current_x, g_interpolation.current_y);
  } else {
    uint8_t x = buffer_active[currentStep].x;
    uint8_t y = buffer_active[currentStep].y;
    // uint8_t flags = buffer_active[currentStep].flags;
    if (interpolation_init(last_x, last_y, x, y)) {
      interpolation_next_point();
      outputDAC(g_interpolation.current_x, g_interpolation.current_y);
    } else {
      outputDAC((uint16_t)x << 8, (uint16_t)y << 8);
    }
    last_x = x;
    last_y = y;
    currentStep++;
  }

  // // Extract current step data

  // // Extract laser state from flags
  // bool laserOn = (flags & LASER_BIT) != 0;

  // // Output to DAC and control laser
  // outputDAC(x, y);
  // digitalWrite(LASER_PIN, laserOn ? HIGH : LOW);
}
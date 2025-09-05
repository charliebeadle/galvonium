#pragma once

#include "../config.h"
#include "../debug.h"
#include "../types.h"

enum interp_state_t {
  INTERP_STATE_READY,
  INTERP_STATE_FIRST,
  INTERP_STATE_INTERPOLATE,
  INTERP_STATE_LAST,
  INTERP_STATE_FINISHED
};

struct interpolation_t {

  point_q12_4_t step;
  uint8_t current_step;
  uint8_t total_steps;

  uint8_t acc_factor;

  uint8_t dec_factor;

  interp_state_t state;

  inline void print() const {
    DEBUG_INFO_VAL2("Interpolation: Step ", step.x, step.y);
    DEBUG_INFO_VAL("Interpolation: Current step ", current_step);
    DEBUG_INFO_VAL("Interpolation: Total steps ", total_steps);
    DEBUG_INFO_VAL("Interpolation: Acc factor ", acc_factor);
    DEBUG_INFO_VAL("Interpolation: Dec factor ", dec_factor);
  }
};

extern interpolation_t interp;
extern transition_t *transition;

bool interp_init(transition_t *transition,
                 uint8_t step_size = g_config.renderer.max_step_size,
                 uint8_t acc_factor = g_config.renderer.acc_factor,
                 uint8_t dec_factor = g_config.renderer.dec_factor);

bool interp_next_step();

bool interp_active();

bool interp_clear();
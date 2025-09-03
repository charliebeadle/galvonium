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
};

extern interpolation_t interp;
extern transition_t *transition;

bool interp_init(transition_t *transition,
                 uint8_t step_size = DEFAULT_STEP_SIZE,
                 uint8_t acc_factor = DEFAULT_ACC_FACTOR,
                 uint8_t dec_factor = DEFAULT_DEC_FACTOR);

bool interp_next_step();

bool interp_active();

bool interp_clear();
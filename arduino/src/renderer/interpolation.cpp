#include "interpolation.h"
#include "../debug.h"
#include "../types.h"

// Global variable definitions
interpolation_t interp;
transition_t *transition;

bool interp_clear() {
  transition = nullptr;
  interp.acc_factor = 0;
  interp.dec_factor = 0;
  interp.current_step = 0;
  interp.total_steps = 0;
  interp.step = point_q12_4_t{0, 0};
  interp.state = INTERP_STATE_FINISHED;
  return true;
}
bool interp_init(transition_t *transition, uint8_t step_size,
                 uint8_t acc_factor, uint8_t dec_factor) {
  DEBUG_INFO("Interpolation init");

  // Validate input parameters - critical for interpolation safety
  if (transition == nullptr) {
    DEBUG_ERROR("interp_init: null transition pointer");
    return false;
  }

  if (step_size < MIN_STEP_SIZE || step_size > MAX_STEP_SIZE) {
    DEBUG_INFO("CLIP: step_size out of range");
    step_size = (step_size < MIN_STEP_SIZE) ? MIN_STEP_SIZE : MAX_STEP_SIZE;
  }

  if (acc_factor < MIN_ACC_FACTOR || acc_factor > MAX_ACC_FACTOR) {
    DEBUG_INFO("CLIP: acc_factor out of range");
    acc_factor =
        (acc_factor < MIN_ACC_FACTOR) ? MIN_ACC_FACTOR : MAX_ACC_FACTOR;
  }

  if (dec_factor < MIN_DEC_FACTOR || dec_factor > MAX_DEC_FACTOR) {
    DEBUG_INFO("CLIP: dec_factor out of range");
    dec_factor =
        (dec_factor < MIN_DEC_FACTOR) ? MIN_DEC_FACTOR : MAX_DEC_FACTOR;
  }

  ::transition = transition;
  interp.acc_factor = acc_factor;
  interp.dec_factor = dec_factor;

  interp.state = INTERP_STATE_FIRST;
  interp.current_step = 0;

  int16_t delta_x = transition->end_point.x - transition->start_point.x;
  int16_t delta_y = transition->end_point.y - transition->start_point.y;
  uint16_t max_distance = MAX(ABS(delta_x), ABS(delta_y));

  if ((int16_t)max_distance < COORD8_TO_Q12_4(step_size)) {
    // neither distance is larger than the step size
    interp.total_steps = 1;
    interp.step = transition->end_point - transition->start_point;
    interp.acc_factor = 0;
    interp.dec_factor = 0;
    interp.state = INTERP_STATE_LAST;
  } else if (ABS(delta_x) == ABS(delta_y)) {
    interp.step.x = COORD8_TO_Q12_4(step_size);
    interp.step.y = COORD8_TO_Q12_4(step_size);
    if (interp.step.x == 0) {
      DEBUG_ERROR("Division by zero in diagonal interpolation");
      return false;
    }
    interp.total_steps = delta_x / interp.step.x;
  } else if (ABS(delta_x) > ABS(delta_y)) {
    // X is the longer axis
    interp.step.x = COORD8_TO_Q12_4(step_size);
    if (interp.step.x == 0) {
      DEBUG_ERROR("Division by zero in X-dominant interpolation");
      return false;
    }
    interp.total_steps = delta_x / interp.step.x;
    if (interp.total_steps > 0) {
      interp.step.y = delta_y / interp.total_steps;
    } else {
      interp.step.y = 0;
    }
  } else {
    // Y is the longer axis
    interp.step.y = COORD8_TO_Q12_4(step_size);
    if (interp.step.y == 0) {
      DEBUG_ERROR("Division by zero in Y-dominant interpolation");
      return false;
    }
    interp.total_steps = delta_y / interp.step.y;
    if (interp.total_steps > 0) {
      interp.step.x = delta_x / interp.total_steps;
    } else {
      interp.step.x = 0;
    }
  }

  DEBUG_VERBOSE("Interpolation setup complete");

  return true;
}

bool interp_next_step() {
  if (transition == nullptr) {
    DEBUG_ERROR("Interpolation called with null transition");
    return false;
  }

  switch (interp.state) {
  case INTERP_STATE_READY:
    interp.state = INTERP_STATE_FIRST;
  case INTERP_STATE_FIRST:
    if (interp.acc_factor > 0) {
      transition->current_point =
          transition->start_point + (interp.step >> interp.acc_factor);
      interp.acc_factor--;
      return true;
    } else {
      transition->current_point = transition->start_point + interp.step;
      interp.state = INTERP_STATE_INTERPOLATE;
      interp.current_step++;
      return true;
    }
  case INTERP_STATE_INTERPOLATE:
    if (interp.current_step < interp.total_steps - 1) {
      transition->current_point += interp.step;
      interp.current_step++;
      return true;
    } else {
      interp.state = INTERP_STATE_LAST;
    }
  case INTERP_STATE_LAST:
    if (interp.dec_factor > 0) {
      interp.step >>= 1;
      transition->current_point += interp.step;
      interp.dec_factor--;
      return true;
    } else {
      transition->current_point = transition->end_point;
      interp.state = INTERP_STATE_FINISHED;
      return true;
    }
  case INTERP_STATE_FINISHED:
    return false;
  default:
    DEBUG_ERROR_VAL("Unknown interpolation state: ", interp.state);
    return false;
  }
}

bool interp_active() { return interp.state != INTERP_STATE_FINISHED; }

/*
For divison by a power of 2, we can use:
shift_amount = ctz(stepsize);
count = distance >> shift_amount;
remainder = distance & (stepsize - 1);
*/
void fast_divide_by_power_of_2_uint8(uint8_t *result, uint8_t *remainder,
                                     uint8_t dividend, uint8_t divisor) {
  if (result == nullptr || remainder == nullptr) {
    DEBUG_ERROR("fast_divide_uint8: null pointer");
    return;
  }

  if (divisor == 0 || (divisor & (divisor - 1)) != 0) {
    DEBUG_ERROR_VAL("Invalid divisor (not power of 2): ", divisor);
    return;
  }

  uint8_t shift_amount = __builtin_ctz(divisor);
  *result = dividend >> shift_amount;
  *remainder = dividend & (divisor - 1);
}

void fast_divide_by_power_of_2_uint16(uint16_t *result, uint16_t *remainder,
                                      uint16_t dividend, uint16_t divisor) {
  if (result == nullptr || remainder == nullptr) {
    DEBUG_ERROR("fast_divide_uint16: null pointer");
    return;
  }

  if (divisor == 0 || (divisor & (divisor - 1)) != 0) {
    DEBUG_ERROR_VAL("Invalid divisor (not power of 2): ", divisor);
    return;
  }

  uint8_t shift_amount = __builtin_ctz(divisor);
  *result = dividend >> shift_amount;
  *remainder = dividend & (divisor - 1);
}
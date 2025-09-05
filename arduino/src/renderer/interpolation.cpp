#include "interpolation.h"

// TODO: add validation for all parameters

// Global variable definitions
interpolation_t interp;
transition_t *transition;

bool interp_clear() {
  DEBUG_VERBOSE(F("Interpolation: Clearing"));
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

  DEBUG_VERBOSE(F("Interpolation: Initializing"));

  // Convert the step size to Q12.4
  int16_t _step_size = COORD8_TO_Q12_4(step_size);

  // Get the transition and acc/dec factors
  ::transition = transition;
  interp.acc_factor = acc_factor;
  interp.dec_factor = dec_factor;

  transition->print();

  // Set the initial state
  interp.state = INTERP_STATE_FIRST;

  // Set the current step to 0
  interp.current_step = 0;

  // Get the deltas between the start and end points
  point_q12_4_t deltas = transition->end_point - transition->start_point;

  // Get the largest distance of either axis
  uint16_t max_distance = MAX(ABS(deltas.x), ABS(deltas.y));

  DEBUG_VERBOSE_VAL2("Interpolation: Deltas ", delta.x, delta.y);
  DEBUG_VERBOSE_VAL("Interpolation: Max distance ", max_distance);
  DEBUG_VERBOSE_VAL("Interpolation: Step size ", step_size);

  if ((int16_t)max_distance < _step_size) {

    // neither distance is larger than the step size

    DEBUG_VERBOSE(
        F("Interpolation: Neither distance is larger than the step size"));

    interp.total_steps = 1;
    interp.step = deltas;
    interp.acc_factor = 0;
    interp.dec_factor = 0;
    interp.state = INTERP_STATE_LAST;

  } else {
    // Calculate total steps with ceiling division
    interp.total_steps = (max_distance + _step_size - 1) / _step_size;

    if (interp.total_steps > 0) {
      interp.step.x = deltas.x / interp.total_steps;
      interp.step.y = deltas.y / interp.total_steps;
    } else {
      // This should never happen - caught by the above if statement
      DEBUG_ERROR(F("Interpolation: Total steps is 0"));
      interp.step = deltas;
    }
  }

  interp.print();

  return true;
}

bool interp_next_step() {

  DEBUG_VERBOSE(F("Interpolation: Next step"));
  interp.print();

  switch (interp.state) {
  case INTERP_STATE_READY:

    DEBUG_VERBOSE(F("Interpolation: Ready"));
    interp.state = INTERP_STATE_FIRST;

    // No break - fall through to first

  case INTERP_STATE_FIRST:

    DEBUG_VERBOSE(F("Interpolation: First"));

    // If we're doing first-step acceleration
    if (interp.acc_factor > 0) {

      // First steps are bitshifted right by the acceleration factor
      // i.e. if acc factor is 3, and step is 16, then
      // 16 >> 3 = 2
      // 16 >> 2 = 4
      // 16 >> 1 = 8
      // So our first points are 2, 4, 8, and then we go to the normal step

      transition->current_point =
          transition->start_point + (interp.step >> interp.acc_factor);

      interp.acc_factor--;

      return true;

    } else {

      // If no acceleration (or acc done) add the step to start point

      transition->current_point = transition->start_point + interp.step;

      interp.state = INTERP_STATE_INTERPOLATE;
      interp.current_step++;

      return true;
    }
  case INTERP_STATE_INTERPOLATE:

    DEBUG_VERBOSE(F("Interpolation: Interpolate"));

    if (interp.current_step < interp.total_steps - 1) {

      // Regular interpolation - add the step to the current point

      transition->current_point += interp.step;
      interp.current_step++;

      return true;

    } else {

      interp.state = INTERP_STATE_LAST;
      // No break - fall through to last
    }
  case INTERP_STATE_LAST:

    DEBUG_VERBOSE(F("Interpolation: Last"));

    if (interp.dec_factor > 0) {

      // Last steps are bitshifted right by 1 for each in dec_factor
      // i.e. if dec factor is 3, and step is 16, then
      // 16 >> 1 = 8
      // 8 >> 1 = 4
      // 4 >> 1 = 2
      // So our last steps are 8, 4, 2, and then we go to the end point
      // NOTE: This is destructive - we edit the step value itself
      // Maybe change this later (just have a copy of the step value)

      interp.step >>= 1;
      transition->current_point += interp.step;

      interp.dec_factor--;

      return true;

    } else {

      // If no deceleration (or dec done), just go to the end
      // (no accumulated errors)

      transition->current_point = transition->end_point;
      interp.state = INTERP_STATE_FINISHED;
      return true;
    }

  case INTERP_STATE_FINISHED:
    DEBUG_ERROR(F("interp_next_step: Unexpected finished state"));
    return false;

  default:

    return false;
  }
}

bool interp_active() {
  DEBUG_VERBOSE_VAL("Interpolation active: ",
                    interp.state != INTERP_STATE_FINISHED);
  return interp.state != INTERP_STATE_FINISHED;
}

/*
For divison by a power of 2, we can use:
shift_amount = ctz(stepsize);
count = distance >> shift_amount;
remainder = distance & (stepsize - 1);
*/
void fast_divide_by_power_of_2_uint8(uint8_t *result, uint8_t *remainder,
                                     uint8_t dividend, uint8_t divisor) {
  uint8_t shift_amount = __builtin_ctz(divisor);
  *result = dividend >> shift_amount;
  *remainder = dividend & (divisor - 1);
}

void fast_divide_by_power_of_2_uint16(uint16_t *result, uint16_t *remainder,
                                      uint16_t dividend, uint16_t divisor) {
  uint8_t shift_amount = __builtin_ctz(divisor);
  *result = dividend >> shift_amount;
  *remainder = dividend & (divisor - 1);
}
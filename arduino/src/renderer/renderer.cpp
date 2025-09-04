#include "renderer.h"
#include "../config.h"
#include "../types.h"
#include "interpolation.h"
#include <Arduino.h>

void Renderer::init() {
  DEBUG_VERBOSE(F("Renderer::init"));
  step_buf.clear();
  interp_clear();
  point_buf_a.clear();
  point_buf_b.clear();
  active_point_buf = &point_buf_a;
  inactive_point_buf = &point_buf_b;
  point_buf_index = 0;
  swap_requested = false;
  step_buf_wait = 0;
  renderer_state = R_STATE_BUFFER_EMPTY;
  process_state = INTERP_FINISHED;
  buffer_status = EMPTY;
  dwell = 0;
  last_point = {0, 0};
  last_laser_state = false;
  next_point = {0, 0};
  next_laser_state = false;
  current_transition = transition_t();
  DEBUG_INFO(F("Renderer initialized"));

  // dummy data for the buffer
  point_coord8_t dummy_points[] = {
      {0, 0, 0},
      {255, 0, 0},
      {255, 255, 0},
      {0, 255, 0},
  };
  for (int i = 0; i < 4; i++) {
    inactive_point_buf->set_point(i, dummy_points[i]);
  }
  inactive_point_buf->set_point_count(4);
  DEBUG_VERBOSE(F("Renderer::init: Dummy data set"));
}

bool Renderer::swap_buffers() {
  DEBUG_VERBOSE(F("Renderer::swap_buffers"));

  noInterrupts();

  // Swap the active and inactive point buffers
  coord8_point_buf_t *tmp = active_point_buf;
  active_point_buf = inactive_point_buf;
  inactive_point_buf = tmp;

  interrupts();

  DEBUG_VERBOSE(F("Renderer::swap_buffers: Buffers swapped"));

  return true;
}

void Renderer::process() {
  DEBUG_VERBOSE(F("Renderer::process"));
  switch (this->renderer_state) {
  case R_STATE_FIRST_POINT:
    DEBUG_INFO(F("Renderer::process: R_STATE_FIRST_POINT"));
    handle_first_point();
    break;
  case R_STATE_NEXT_POINT:
    DEBUG_INFO(F("Renderer::process: R_STATE_NEXT_POINT"));
    handle_next_point();
    break;
  case R_STATE_NEW_TRANSITION:
    DEBUG_INFO(F("Renderer::process: R_STATE_NEW_TRANSITION"));
    handle_new_transition();
    break;
  case R_STATE_INTERPOLATE:
    DEBUG_INFO(F("Renderer::process: R_STATE_INTERPOLATE"));
    process_next_step();
    break;
  case R_STATE_BUFFER_FINISHED:
    DEBUG_INFO(F("Renderer::process: R_STATE_BUFFER_FINISHED"));
    handle_buffer_finished();
    break;
  case R_STATE_BUFFER_EMPTY:
    DEBUG_INFO(F("Renderer::process: R_STATE_BUFFER_EMPTY"));
    handle_buffer_finished(); // For the time being
    break;
  case R_STATE_INTERP_ERROR:
    DEBUG_ERROR(F("Renderer::process: R_STATE_INTERP_ERROR"));
    break;
  }
}

void Renderer::handle_first_point() {
  if (get_next_point(&this->next_point, &this->next_laser_state)) {
    this->renderer_state = R_STATE_NEXT_POINT;
  } else {
    this->renderer_state = R_STATE_BUFFER_EMPTY;
  }
  DEBUG_VERBOSE_VAL2("Renderer::handle_first_point: Next point ",
                     this->next_point.x, this->next_point.y);
}

void Renderer::handle_next_point() {

  // Temporarily store the next point and laser state
  point_q12_4_t tmp_point = this->next_point;
  bool tmp_laser_state = this->next_laser_state;

  // Get the next point from the active point buffer
  if (get_next_point(&this->next_point, &this->next_laser_state)) {

    // Update the last point and laser state
    this->last_point = tmp_point;
    this->last_laser_state = tmp_laser_state;

    // Calculate the laser dwell
    calc_laser_dwell();

    this->renderer_state = R_STATE_NEW_TRANSITION;
  } else {

    // If get next point returns false, we're at the end of the buffer
    this->renderer_state = R_STATE_BUFFER_FINISHED;
  }
  DEBUG_VERBOSE_VAL2("Renderer::handle_next_point: Next point ",
                     this->next_point.x, this->next_point.y);
}

void Renderer::handle_new_transition() {

  // Create a new transition
  this->current_transition = transition_t(this->last_point, this->next_point);

  DEBUG_VERBOSE(F("Renderer::handle_new_transition: Current transition"));
  this->current_transition.print();

  // Calculate the laser dwell
  calc_laser_dwell();

  DEBUG_VERBOSE_VAL("Renderer::handle_new_transition: Laser dwell ",
                    this->dwell);

  // Initialize the interpolation
  if (interp_init(&current_transition)) {
    this->renderer_state = R_STATE_INTERPOLATE;
  } else {
    this->renderer_state = R_STATE_INTERP_ERROR;
  }
}

void Renderer::handle_buffer_finished() {

  DEBUG_VERBOSE(F("Renderer::handle_buffer_finished"));

  // Swap the buffers
  // TODO: make this a parameter (swap_requested)
  if (1) {
    DEBUG_VERBOSE(F("Renderer::handle_buffer_finished: Swapping buffers"));
    swap_buffers();
  }

  // Reset the point buffer index
  this->point_buf_index = 0;
  this->renderer_state = R_STATE_NEXT_POINT;
}

void Renderer::calc_laser_dwell() {

  // Calculate the laser dwell - depending on if the laser is going from on to
  // off or vice versa

  if (last_laser_state == true && next_laser_state == false) {
    this->dwell = LASER_ON_DWELL_TIME;
  } else if (last_laser_state == false && next_laser_state == true) {
    this->dwell = LASER_OFF_DWELL_TIME;
  } else {
    this->dwell = 0;
  }
}

bool Renderer::get_next_point(point_q12_4_t *point, bool *laser_state) {

  DEBUG_VERBOSE(F("Renderer::get_next_point"));

  // Check if the active point buffer is empty
  if (active_point_buf->get_point_count() == 0) {
    DEBUG_VERBOSE(F("Renderer::get_next_point: Active point buffer is empty"));
    buffer_status = EMPTY;
    return false;
  }

  // Check if we're at the end of the active point buffer
  // (this shouldn't happen - we check at the end of this function)
  if (point_buf_index == active_point_buf->get_point_count()) {
    DEBUG_ERROR(F("Renderer::get_next_point: Unexpected end of buffer"));
    buffer_status = FINISHED;
    return false;
  }

  // Get the next point from the active point buffer
  point_coord8_t new_point;
  active_point_buf->get_point(point_buf_index, &new_point);

  // Extract the Q12.4 and laser state
  *point =
      point_q12_4_t(COORD8_TO_Q12_4(new_point.x), COORD8_TO_Q12_4(new_point.y));
  *laser_state = new_point.flags & BLANKING_BIT;

  DEBUG_VERBOSE_VAL2("Renderer::get_next_point: New point ", new_point.x,
                     new_point.y);
  DEBUG_VERBOSE_VAL("Renderer::get_next_point: Laser state ", *laser_state);

  point_buf_index++;

  // Check if we're at the end of the active point buffer
  if (point_buf_index == active_point_buf->get_point_count()) {
    DEBUG_VERBOSE(F("Renderer::get_next_point: End of buffer"));
    buffer_status = FINISHED;
    return false;
  } else {
    buffer_status = ACTIVE;
  }
  return true;
}

bool Renderer::process_next_step() {
  if (step_buf.is_full()) {
    // If the step buffer is full, set the process state to RING_BUF_FULL
    // and increment the step buffer wait
    process_state = RING_BUF_FULL;
    step_buf_wait++;

    DEBUG_VERBOSE_VAL("Renderer::process_next_step: Step buffer wait: ",
                      step_buf_wait);

    return false;

  } else { // There is space in the step buffer

    DEBUG_INFO_VAL("Renderer::process_next_step: Step buffer space: ",
                   step_buf.space());

    // Reset the step buffer wait
    step_buf_wait = 0;

    if (dwell) {

      DEBUG_VERBOSE_VAL("Renderer::process_next_step: Dwell: ", dwell);

      process_state = DWELL_ACTIVE;

      // Dwell once for each in dwell
      step_buf.push(current_transition.current_point, next_laser_state);
      dwell--;

      return true;

    } else {

      if (interp_active()) {

        DEBUG_VERBOSE(
            F("Renderer::process_next_step: Interpolation is active"));

        // If interpolation is active, get the next step
        if (interp_next_step()) {

          // Push the latest step to the step buffer
          step_buf.push(current_transition.current_point, next_laser_state);

          process_state = INTERP_ACTIVE;

          DEBUG_VERBOSE(F("Renderer::process_next_step: Pushed to buffer"));
          current_transition.print();

          return true;

        } else {

          // interp_next_step shouldn't fail if interp is active
          DEBUG_ERROR(F("Renderer::process_next_step: Interpolation error"));
          process_state = INTERP_ERROR;
          return false;
        }
      } else {

        // Interpolation is finished - tell renderer to move to the next point

        DEBUG_VERBOSE(
            F("Renderer::process_next_step: Interpolation is finished"));

        process_state = INTERP_FINISHED;
        renderer_state = R_STATE_NEXT_POINT;

        return false;
      }
    }
  }
}

// Global renderer instance
Renderer renderer;

// Getter function for renderer access
Renderer &getRenderer() {
  DEBUG_VERBOSE(F("renderer::getRenderer"));
  return renderer;
}

// Data source function that bridges renderer to hardware
bool renderer_data_source(void *point, void *laser_state) {
  return renderer.get_next_step((point_q12_4_t *)point, (bool *)laser_state);
}
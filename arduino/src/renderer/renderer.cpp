#include "renderer.h"
#include "../config.h"
#include "../debug.h"
#include "../types.h"
#include "interpolation.h"
#include <Arduino.h>

void Renderer::init() {
  DEBUG_INFO("Renderer init");

  step_buf.clear();
  interp_clear();
  point_buf_a.clear();
  point_buf_b.clear();

  // Critical: ensure buffer pointers are valid
  active_point_buf = &point_buf_a;
  inactive_point_buf = &point_buf_b;

  if (active_point_buf == nullptr || inactive_point_buf == nullptr) {
    DEBUG_ERROR("Failed to initialize buffer pointers");
    return;
  }

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

  DEBUG_INFO("Renderer init complete");

  // Add some dummy data to the inactive buffer for testing
  inactive_point_buf->clear();
  inactive_point_buf->set_coords(0, 128, 128);  // Center point
  inactive_point_buf->set_laser_state(0, false);
  inactive_point_buf->set_coords(1, 200, 150);  // Move to another point
  inactive_point_buf->set_laser_state(1, true);
  inactive_point_buf->set_coords(2, 100, 200);  // Third point
  inactive_point_buf->set_laser_state(2, true);
  inactive_point_buf->point_count = 3;

  // Request a buffer swap to use the dummy data
  swap_requested = true;
}

bool Renderer::swap_buffers() {
  noInterrupts();
  coord8_point_buf_t *tmp = active_point_buf;
  active_point_buf = inactive_point_buf;
  inactive_point_buf = tmp;
  interrupts();
  return true;
}

void Renderer::process() {
  switch (this->renderer_state) {
  case R_STATE_FIRST_POINT:
    handle_first_point();
    break;
  case R_STATE_NEXT_POINT:
    handle_next_point();
    break;
  case R_STATE_NEW_TRANSITION:
    handle_new_transition();
    break;
  case R_STATE_INTERPOLATE:
    process_next_step();
    break;
  case R_STATE_BUFFER_FINISHED:
    handle_buffer_finished();
    break;
  case R_STATE_BUFFER_EMPTY:
    handle_buffer_finished(); // For the time being
    break;
  case R_STATE_INTERP_ERROR:
    DEBUG_ERROR("Renderer in error state - halted");
    break;
  default:
    DEBUG_ERROR_VAL("Unknown renderer state: ", this->renderer_state);
    this->renderer_state = R_STATE_BUFFER_EMPTY;
    break;
  }
}

void Renderer::handle_first_point() {
  if (get_next_point(&this->next_point, &this->next_laser_state)) {
    this->renderer_state = R_STATE_NEXT_POINT;
  } else {
    this->renderer_state = R_STATE_BUFFER_EMPTY;
  }
}

void Renderer::handle_next_point() {
  this->last_point = this->next_point;
  this->last_laser_state = this->next_laser_state;
  if (get_next_point(&this->next_point, &this->next_laser_state)) {
    calc_laser_dwell();
    this->renderer_state = R_STATE_NEW_TRANSITION;
  } else {
    this->renderer_state = R_STATE_BUFFER_FINISHED;
  }
}

void Renderer::handle_new_transition() {
  this->current_transition = transition_t(this->last_point, this->next_point);
  calc_laser_dwell();

  if (interp_init(&current_transition)) {
    this->renderer_state = R_STATE_INTERPOLATE;
  } else {
    DEBUG_ERROR("Interpolation init failed");
    this->renderer_state = R_STATE_INTERP_ERROR;
  }
}

void Renderer::handle_buffer_finished() {
  if (swap_requested) {
    swap_buffers();
  }
  this->point_buf_index = 0;
  this->renderer_state = R_STATE_FIRST_POINT;
}

void Renderer::calc_laser_dwell() {
  if (last_laser_state == true && next_laser_state == false) {
    this->dwell = LASER_ON_DWELL_TIME;
  } else if (last_laser_state == false && next_laser_state == true) {
    this->dwell = LASER_OFF_DWELL_TIME;
  } else {
    this->dwell = 0;
  }

  // Validate dwell time is within safe limits
  if (this->dwell < MIN_DWELL_TIME || this->dwell > MAX_DWELL_TIME) {
    DEBUG_INFO("CLIP: dwell time out of range");
    this->dwell =
        (this->dwell < MIN_DWELL_TIME) ? MIN_DWELL_TIME : MAX_DWELL_TIME;
  }

  DEBUG_VERBOSE("Laser dwell calculated");
}

bool Renderer::get_next_point(point_q12_4_t *point, bool *laser_state) {
  // Critical validation - null pointers could crash system
  if (point == nullptr || laser_state == nullptr ||
      active_point_buf == nullptr) {
    DEBUG_ERROR("get_next_point: null pointer");
    return false;
  }

  if (active_point_buf->get_point_count() == 0) {
    buffer_status = EMPTY;
    return false;
  }

  if (point_buf_index >= active_point_buf->get_point_count()) {
    buffer_status = FINISHED;
    return false;
  }

  // Critical: validate buffer index bounds
  if (point_buf_index >= MAX_POINTS) {
    DEBUG_ERROR_VAL("Buffer index out of bounds: ", point_buf_index);
    return false;
  }

  point_coord8_t new_point;
  active_point_buf->get_point(point_buf_index, &new_point);
  *point = point_q12_4_t(new_point.x, new_point.y);
  *laser_state = new_point.flags & BLANKING_BIT;
  point_buf_index++;

  if (point_buf_index == active_point_buf->get_point_count()) {
    buffer_status = FINISHED;
  } else {
    buffer_status = ACTIVE;
  }
  return true;
}

bool Renderer::process_next_step() {
  if (step_buf.is_full()) {
    process_state = RING_BUF_FULL;
    step_buf_wait++;
    if (step_buf_wait > MAX_STEP_BUFFER_WAIT) {
      DEBUG_ERROR_VAL("Step buffer wait timeout: ", step_buf_wait);
      step_buf_wait = 0;
    }
    return false;
  } else {
    step_buf_wait = 0;
    if (dwell) {
      process_state = DWELL_ACTIVE;
      if (!step_buf.push(current_transition.current_point, next_laser_state)) {
        DEBUG_ERROR("Failed to push dwell step to buffer");
        return false;
      }
      dwell--;
      return true;
    } else {
      if (interp_active()) {
        if (interp_next_step()) {
          if (!step_buf.push(current_transition.current_point,
                             next_laser_state)) {
            DEBUG_ERROR("Failed to push interpolation step to buffer");
            return false;
          }
          process_state = INTERP_ACTIVE;
          return true;
        } else {
          DEBUG_ERROR("Interpolation step failed");
          process_state = INTERP_ERROR;
          return false;
        }
      } else {
        process_state = INTERP_FINISHED;
        return false;
      }
    }
  }
}

// Global renderer instance
Renderer renderer;

// Getter function for renderer access
Renderer &getRenderer() { return renderer; }

// Data source function that bridges renderer to hardware
// NOTE: Called from ISR - use ISR error flags only, no Serial debug
bool renderer_data_source(void *point, void *laser_state) {
  if (point == nullptr || laser_state == nullptr) {
    DEBUG_ISR_ERROR(ISR_ERROR_NULL_POINTER);
    return false;
  }

  bool result =
      renderer.get_next_step((point_q12_4_t *)point, (bool *)laser_state);
  if (!result) {
    DEBUG_ISR_ERROR(ISR_ERROR_BUFFER_EMPTY);
  }

  return result;
}
#include "renderer.h"
#include "../config.h"
#include "../types.h"
#include "interpolation.h"
#include <Arduino.h>

void Renderer::init() {
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
}

bool Renderer::get_next_point(point_q12_4_t *point, bool *laser_state) {
  if (active_point_buf->get_point_count() == 0) {
    buffer_status = EMPTY;
    return false;
  }
  if (point_buf_index == active_point_buf->get_point_count()) {
    buffer_status = FINISHED;
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
    return false;
  } else {
    step_buf_wait = 0;
    if (dwell) {
      process_state = DWELL_ACTIVE;
      step_buf.push(current_transition.current_point, next_laser_state);
      dwell--;
      return true;
    } else {
      if (interp_active()) {
        if (interp_next_step()) {
          step_buf.push(current_transition.current_point, next_laser_state);
          process_state = INTERP_ACTIVE;
          return true;
        } else {
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
bool renderer_data_source(void *point, void *laser_state) {
  return renderer.get_next_step((point_q12_4_t *)point, (bool *)laser_state);
}
#include "renderer.h"
#include "../config.h"
#include "../types.h"
#include "interpolation.h"
#include <Arduino.h>

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

  transition = transition_t();
  stats = render_stats_t();
  dwell = 0;

  render_state = IDLE_EMPTY;

  DEBUG_INFO(F("Renderer initialized"));

  // dummy data for the buffer
  point_coord8_t dummy_points[] = {
      {0, 0, 0}, {200, 0, 255}, {200, 200, 0}, {0, 200, 0}};
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

  swap_requested = true;
  transition.print();

  switch (render_state) {
  case IDLE_EMPTY:

    if (active_point_buf->is_empty() && inactive_point_buf->is_empty()) {
      stats.point_buf_wait++;
      return;
    }

    if (active_point_buf->is_empty()) {
      render_state = IDLE_BUFFER_SWAP;
    } else {
      render_state = IDLE_READY;
    }
    break;

  case IDLE_READY:

    point_buf_index = 0;

    // This loads the first point - the "end" of the transition is point 0 and
    // the "start" is undefined. Hence we do not init interp.
    if (get_next_transition(&transition)) {
      render_state = RENDER_GET_POINT;
    } else {
      render_state = ERROR_BUFFER_FAULT;
      return;
    }
    break;

  case IDLE_BUFFER_SWAP:
    if (!swap_requested) {
      stats.point_buf_wait++;
      return;
    }

    if (!swap_buffers()) {
      render_state = ERROR_BUFFER_FAULT;
      stats.point_buf_wait = 0;
      return;
    }

    stats.point_buf_wait = 0;

    if (active_point_buf->is_empty()) {
      render_state = ERROR_BUFFER_FAULT;
      return;
    }

    render_state = IDLE_READY;

    break;

  case RENDER_GET_POINT:

    if (!get_next_transition(&transition)) {
      render_state = RENDER_BUFFER_END;
      return;
    }

    interp_init(&transition);

    if (get_dwell()) {
      render_state = RENDER_DWELL;
    } else {
      render_state = RENDER_INTERPOLATE;
    }
    break;

  case RENDER_DWELL:

    if (step_buf.is_full()) {
      stats.step_buf_wait++;
      return;
    } else {
      stats.step_buf_wait = 0;
    }

    step_buf.push(transition.current_point, transition.get_current_laser());
    dwell--;

    if (dwell == 0) {
      render_state = RENDER_INTERPOLATE;
    }

    break;

  case RENDER_INTERPOLATE:

    if (step_buf.is_full()) {
      stats.step_buf_wait++;
      return;
    } else {
      stats.step_buf_wait = 0;
    }

    if (!interp_next_step()) {
      render_state = ERROR_INTERP_FAULT;
      return;
    }

    step_buf.push(transition.current_point, transition.get_current_laser());

    if (!interp_active()) {
      render_state = RENDER_GET_POINT;
    }

    break;

  case RENDER_BUFFER_END:

    point_buf_index = 0;
    if (swap_requested) {
      render_state = RENDER_BUFFER_SWAP;
    } else {
      stats.point_buf_repeat++;
      render_state = RENDER_GET_POINT;
    }
    break;

  case RENDER_BUFFER_SWAP:

    if (inactive_point_buf->is_empty()) {
      stats.point_buf_repeat++;
      render_state = RENDER_GET_POINT;
      return;
    }

    if (!swap_buffers()) {
      render_state = ERROR_BUFFER_FAULT;
      return;
    }
    stats.point_buf_wait = 0;
    render_state = RENDER_GET_POINT;

    break;

  case ERROR_INTERP_FAULT:
    DEBUG_ERROR(F("Renderer::process: Interpolation fault"));
    // TODO: Handle error gracefully
    render_state = IDLE_READY;
    break;
  case ERROR_BUFFER_FAULT:
    DEBUG_ERROR(F("Renderer::process: Buffer fault"));
    // TODO: Handle error gracefully
    render_state = IDLE_EMPTY;
    break;
  }
}

bool Renderer::get_next_transition(transition_t *transition) {

  if (active_point_buf->is_empty()) {
    return false;
  }

  if (point_buf_index == active_point_buf->get_point_count()) {
    return false;
  }

  point_coord8_t new_point;
  active_point_buf->get_point(point_buf_index, &new_point);

  transition->set_next(
      point_q12_4_t(COORD8_TO_Q12_4(new_point.x), COORD8_TO_Q12_4(new_point.y)),
      new_point.flags & BLANKING_BIT);

  point_buf_index++;

  return true;
}

bool Renderer::get_dwell() {

  // Calculate the laser dwell - depending on if the laser is going from on to
  // off or vice versa

  if (transition.get_start_laser() == true &&
      transition.get_end_laser() == false) {
    this->dwell = LASER_OFF_DWELL_TIME;
  } else if (transition.get_start_laser() == false &&
             transition.get_end_laser() == true) {
    this->dwell = LASER_ON_DWELL_TIME;
  } else {
    this->dwell = 0;
    return false;
  }
  return true;
}

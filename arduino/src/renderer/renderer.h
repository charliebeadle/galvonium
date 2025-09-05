#pragma once
#include "../config.h"
#include "../debug.h"
#include "../types.h"
#include "buffers.h"
#include "interpolation.h"
#include <Arduino.h>

enum render_state_t {
  IDLE_EMPTY,
  IDLE_READY,

  IDLE_BUFFER_SWAP,
  RENDER_GET_POINT,
  RENDER_DWELL,
  RENDER_INTERPOLATE,
  RENDER_BUFFER_END,
  RENDER_BUFFER_SWAP,

  ERROR_INTERP_FAULT,
  ERROR_BUFFER_FAULT,

};

class Renderer {

public:
  void init();
  void request_swap();
  void process();
  inline bool get_next_step(point_q12_4_t *point, bool *laser_state) {
    return step_buf.pop(point, laser_state);
  }

private:
  step_ring_buf_16_t step_buf;
  interpolation_t interp;
  coord8_point_buf_t point_buf_a;
  coord8_point_buf_t point_buf_b;
  coord8_point_buf_t *active_point_buf;
  coord8_point_buf_t *inactive_point_buf;
  uint8_t point_buf_index;
  bool swap_requested;
  render_state_t render_state;

  render_stats_t stats;
  uint8_t dwell;

  transition_t transition;

  bool swap_buffers();
  void process_next_point();

  bool get_next_transition(transition_t *transition);
  bool get_dwell();
};

// Global renderer instance
extern Renderer renderer;

// Getter function for renderer access
Renderer &getRenderer();

// Data source function that bridges renderer to hardware
bool renderer_data_source(void *point, void *laser_state);
#pragma once
#include "../config.h"
#include "../debug.h"
#include "../types.h"
#include "buffers.h"
#include "interpolation.h"
#include <Arduino.h>

#define LASER_ON_DWELL_TIME 10
#define LASER_OFF_DWELL_TIME 10

enum renderer_state_t {
  R_STATE_FIRST_POINT,
  R_STATE_NEXT_POINT,
  R_STATE_NEW_TRANSITION,
  R_STATE_INTERPOLATE,
  R_STATE_BUFFER_FINISHED,
  R_STATE_BUFFER_EMPTY,

  R_STATE_INTERP_ERROR
};

enum process_state_t {
  RING_BUF_FULL,
  DWELL_ACTIVE,
  INTERP_ACTIVE,
  INTERP_ERROR,
  INTERP_FINISHED
};

enum buffer_status_t { ACTIVE, EMPTY, FINISHED };

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
  uint8_t step_buf_wait;
  renderer_state_t renderer_state;
  process_state_t process_state;
  buffer_status_t buffer_status;
  uint8_t dwell;
  point_q12_4_t last_point;
  bool last_laser_state;
  point_q12_4_t next_point;
  bool next_laser_state;

  transition_t current_transition;

  bool swap_buffers();
  void process_next_point();

  bool get_next_point(point_q12_4_t *point, bool *laser_state);
  bool frame_finished();
  void handle_buffer_finished();
  void handle_first_point();
  void handle_next_point();
  void handle_new_transition();
  bool process_next_step();
  void calc_laser_dwell();
  bool is_empty();
};

// Global renderer instance
extern Renderer renderer;

// Getter function for renderer access
Renderer &getRenderer();

// Data source function that bridges renderer to hardware
bool renderer_data_source(void *point, void *laser_state);
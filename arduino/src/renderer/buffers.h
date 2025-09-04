#pragma once

#include "../config.h"
#include "../debug.h"
#include "../types.h"
#include <Arduino.h>

struct step_ring_buf_16_t {
  // STEP_RING_BUFFER_SIZE steps - hardcoded because flag_buf packed into a
  // uint16_t Obviously could be bigger but flag_buf implementation would have
  // to change Also 2^n allows bitwise operations for modulo
  point_q12_4_t point_buf[STEP_RING_BUFFER_SIZE] = {0};
  uint16_t flag_buf = 0;
  volatile uint8_t head = 0;
  volatile uint8_t tail = 0;

  inline void clear() {
    memset(point_buf, 0, sizeof(point_buf));
    flag_buf = 0;
    head = 0;
    tail = 0;
  }

  // Buffer is empty when head == tail
  inline bool is_empty() const {
    noInterrupts();
    bool result = head == tail;
    interrupts();
    return result;
  }

  // Buffer is full when head + 1 == tail
  // There is always one empty slot, which keeps this thread safe
  inline bool is_full() const {
    noInterrupts();
    bool result = ((head + 1) & STEP_RING_BUFFER_MASK) == tail;
    interrupts();
    return result;
  }

  // The number of elements in the buffer
  inline uint8_t size() const {
    noInterrupts();
    uint8_t result = (head - tail) & STEP_RING_BUFFER_MASK;
    interrupts();
    return result;
  }
  inline uint8_t space() const {
    noInterrupts();
    uint8_t result =
        STEP_RING_BUFFER_SIZE - ((head - tail) & STEP_RING_BUFFER_MASK);
    interrupts();
    return result;
  }

  // Pop the next step from the buffer
  // Returns false if the buffer is empty
  inline bool pop(point_q12_4_t *point, bool *flag) {
    if (is_empty()) {
      return false;
    }

    noInterrupts(); // Critical section start
    *point = point_buf[tail];
    *flag = (flag_buf & (1 << tail)) != 0;
    tail = (tail + 1) & STEP_RING_BUFFER_MASK; // modulo STEP_RING_BUFFER_SIZE
    interrupts();                              // Critical section end

    DEBUG_VERBOSE_VAL2("step_ring_buf_16_t::pop: Point ", point->x, point->y);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::pop: Flag ", *flag);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::pop: Head ", head);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::pop: Tail ", tail);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::pop: Size ", size());
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::pop: Space ", space());
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::pop: Is full ", is_full());
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::pop: Is empty ", is_empty());

    return true;
  }

  // Push a new step into the buffer
  // Returns false if the buffer is full
  inline bool push(point_q12_4_t point, bool flag) {
    if (is_full()) {
      return false;
    }

    noInterrupts(); // Critical section start
    point_buf[head] = point;

    // Clear and set
    flag_buf &= ~(1 << head);
    flag_buf |= (flag << head);

    head = (head + 1) & STEP_RING_BUFFER_MASK; // modulo STEP_RING_BUFFER_SIZE
    interrupts();                              // Critical section end

    DEBUG_VERBOSE_VAL2("step_ring_buf_16_t::push: Point ", point.x, point.y);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::push: Flag ", flag);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::push: Head ", head);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::push: Tail ", tail);
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::push: Size ", size());
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::push: Space ", space());
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::push: Is full ", is_full());
    DEBUG_VERBOSE_VAL("step_ring_buf_16_t::push: Is empty ", is_empty());

    return true;
  }

  // Just in case
  inline bool peek(point_q12_4_t *point, bool *flag) const {
    noInterrupts(); // Critical section start

    if (is_empty()) {
      interrupts(); // Critical section end
      return false;
    }

    *point = point_buf[tail];
    *flag = (flag_buf & (1 << tail)) != 0;

    interrupts(); // Critical section end
    return true;
  }
};

struct coord8_point_buf_t {
  point_coord8_t points[MAX_POINTS];
  uint8_t point_count;

  inline void clear() {
    DEBUG_VERBOSE("coord8_point_buf_t::clear");
    memset(points, 0, MAX_POINTS);

    point_count = 0;
  }

  void set_laser_state(uint8_t index, bool state) {
    if (index >= MAX_POINTS) {
      DEBUG_ERROR("coord8_point_buf_t::set_laser_state: Index out of range");
      return;
    }
    this->points[index].flags = state ? BLANKING_BIT : 0;
  }

  bool get_laser_state(uint8_t index) {
    if (index >= MAX_POINTS) {
      DEBUG_ERROR("coord8_point_buf_t::get_laser_state: Index out of range");
      return false;
    }
    return this->points[index].flags & BLANKING_BIT;
  }

  void set_coords(uint8_t index, uint8_t x, uint8_t y) {
    if (index >= MAX_POINTS) {
      DEBUG_ERROR("coord8_point_buf_t::set_coords: Index out of range");
      return;
    }
    this->points[index].x = x;
    this->points[index].y = y;
  }

  void get_coords(uint8_t index, uint8_t *x, uint8_t *y) {
    if (index >= MAX_POINTS) {
      DEBUG_ERROR("coord8_point_buf_t::get_coords: Index out of range");
      return;
    }
    *x = this->points[index].x;
    *y = this->points[index].y;
  }

  void set_point(uint8_t index, point_coord8_t point) {
    if (index >= MAX_POINTS) {
      DEBUG_ERROR("coord8_point_buf_t::set_point: Index out of range");
      return;
    }
    this->points[index] = point;
  }

  void get_point(uint8_t index, point_coord8_t *point) {
    if (index >= MAX_POINTS) {
      DEBUG_ERROR("coord8_point_buf_t::get_point: Index out of range");
      return;
    }
    *point = this->points[index];
  }

  void set_point_count(uint8_t count) {
    if (count > MAX_POINTS) {
      DEBUG_ERROR("coord8_point_buf_t::set_point_count: Count out of range");
      return;
    }
    this->point_count = count;
  }

  uint8_t get_point_count() const { return this->point_count; }
};

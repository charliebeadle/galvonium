#pragma once
#include "debug.h"
#include <stdint.h>

#define BIT_MASK(bit) (1 << bit)
#define BIT_SET(value, bit) (value | BIT_MASK(bit))
#define BIT_CLEAR(value, bit) (value & ~BIT_MASK(bit))
#define BIT_TOGGLE(value, bit) (value ^ BIT_MASK(bit))
#define BIT_TEST(value, bit) (value & BIT_MASK(bit))
#define BIT_WRITE(value, bit, state)                                           \
  (state ? BIT_SET(value, bit) : BIT_CLEAR(value, bit))

// Stored in the double buffer
struct point_coord8_t {
  uint8_t x;
  uint8_t y;
  uint8_t flags;

  point_coord8_t() : x(0), y(0), flags(0){};
  point_coord8_t(uint8_t x, uint8_t y, uint8_t flags)
      : x(x), y(y), flags(flags){};
};

// According to ILDA IDTF
struct point_ilda_t {
  int16_t x;
  int16_t y;
  uint8_t flags;

  point_ilda_t() : x(0), y(0), flags(0){};
  point_ilda_t(int16_t x, int16_t y, uint8_t flags)
      : x(x), y(y), flags(flags){};
};

struct point_q12_4_t {
  int16_t x;
  int16_t y;

  point_q12_4_t(int16_t x_val = 0, int16_t y_val = 0) {
    x = x_val;
    y = y_val;
  }

  point_q12_4_t to_integer() const { return point_q12_4_t{x >> 4, y >> 4}; }

  point_q12_4_t from_integer(int16_t int_x, int16_t int_y) const {
    return point_q12_4_t{int_x << 4, int_y << 4};
  }

  // Operators

  point_q12_4_t operator+(const point_q12_4_t &other) const {
    return point_q12_4_t{x + other.x, y + other.y};
  }
  point_q12_4_t operator-(const point_q12_4_t &other) const {
    return point_q12_4_t{x - other.x, y - other.y};
  }
  point_q12_4_t operator>>(const uint8_t &other) const {
    return point_q12_4_t{x >> other, y >> other};
  }
  point_q12_4_t operator<<(const uint8_t &other) const {
    return point_q12_4_t{x << other, y << other};
  }
  point_q12_4_t &operator=(const point_q12_4_t &other) {
    x = other.x;
    y = other.y;
    return *this;
  }
  bool operator==(const point_q12_4_t &other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const point_q12_4_t &other) const {
    return x != other.x || y != other.y;
  }
  bool operator<(const point_q12_4_t &other) const {
    return x < other.x && y < other.y;
  }
  bool operator>(const point_q12_4_t &other) const {
    return x > other.x && y > other.y;
  }
  bool operator<=(const point_q12_4_t &other) const {
    return x <= other.x && y <= other.y;
  }
  bool operator>=(const point_q12_4_t &other) const {
    return x >= other.x && y >= other.y;
  }
  point_q12_4_t &operator++() {
    x++;
    y++;
    return *this;
  }
  point_q12_4_t &operator--() {
    x--;
    y--;
    return *this;
  }
  point_q12_4_t &operator+=(const point_q12_4_t &other) {
    x += other.x;
    y += other.y;
    return *this;
  }
  point_q12_4_t &operator-=(const point_q12_4_t &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
  point_q12_4_t &operator>>=(const uint8_t &other) {
    x >>= other;
    y >>= other;
    return *this;
  }
  point_q12_4_t &operator<<=(const uint8_t &other) {
    x <<= other;
    y <<= other;
    return *this;
  }
};

#define LASER_START_BIT 0
#define LASER_CURRENT_BIT 1
#define LASER_END_BIT 2

/*
transition_t represents a transition between two points (the start, and the end)
laser_state is a bitfield representing the laser state

current_point is where we are at the moment
it is initialised to the start point, and updated as we interpolate

current_laser is initialised to the end laser state, since if it is on we want
it to be on throughout the transition and vice versa

*/

struct transition_t {
  point_q12_4_t start_point;
  point_q12_4_t current_point;
  point_q12_4_t end_point;

  uint8_t laser_states; // Bit 0: Laser start state, Bit 1: Laser current state,
                        // Bit 2: Laser end state

  transition_t()
      : start_point(0, 0), current_point(0, 0), end_point(0, 0),
        laser_states(0) {}
  transition_t(point_q12_4_t start, point_q12_4_t end, bool laser_start,
               bool laser_end)
      : start_point(start), current_point(start), end_point(end),
        laser_states(BIT_WRITE(0, LASER_START_BIT, laser_start) |
                     BIT_WRITE(0, LASER_CURRENT_BIT, laser_end) |
                     BIT_WRITE(0, LASER_END_BIT, laser_end)) {}

  inline bool get_start_laser() const {
    return BIT_TEST(laser_states, LASER_START_BIT);
  }
  inline bool get_current_laser() const {
    return BIT_TEST(laser_states, LASER_CURRENT_BIT);
  }
  inline bool get_end_laser() const {
    return BIT_TEST(laser_states, LASER_END_BIT);
  }
  inline void set_start_laser(bool start_laser) {
    laser_states = BIT_WRITE(laser_states, LASER_START_BIT, start_laser);
  }
  inline void set_current_laser(bool current_laser) {
    laser_states = BIT_WRITE(laser_states, LASER_CURRENT_BIT, current_laser);
  }
  inline void set_end_laser(bool end_laser) {
    laser_states = BIT_WRITE(laser_states, LASER_END_BIT, end_laser);
  }

  inline void set_next_point(point_q12_4_t next_point) {
    start_point = end_point;
    current_point = start_point;
    end_point = next_point;
  }

  inline void set_next_laser(bool next_laser) {
    set_start_laser(get_end_laser());
    set_current_laser(next_laser);
    set_end_laser(next_laser);
  }

  inline void set_next(point_q12_4_t next_point, bool next_laser) {
    set_next_point(next_point);
    set_next_laser(next_laser);
  }

  inline void print() const {
    DEBUG_INFO_VAL2("Transition: Start point ", start_point.x, start_point.y);
    DEBUG_INFO_VAL2("Transition: End point ", end_point.x, end_point.y);
    DEBUG_INFO_VAL2("Transition: Current point ", current_point.x,
                    current_point.y);
  }
};

struct render_stats_t {
  uint8_t point_buf_wait;
  uint8_t point_buf_repeat;
  uint8_t step_buf_wait;
};

// Data about a buffer - not used to store critical buffer state or data
struct BufferInfo {
  uint8_t point_count; // Number of points in buffer
  uint8_t capacity;    // Max buffer capacity
  uint8_t status;
  uint8_t buffer_id;        // 0 or 1
  uint8_t frame_counter;    // Number of times buffer has been displayed
  uint32_t last_updated_ms; // Timestamp of last update

  BufferInfo()
      : point_count(0), capacity(0), status(0), buffer_id(0), frame_counter(0),
        last_updated_ms(0) {}
};

// Point flags
/*
As per ILDA IDTF:

Bit 7 (MSB) - Last Point Bit - always 0 except last point of image
Bit 6 - Blanking Bit - if 1, laser is off. If 0, laser is on.

Bits 0 - 5 are unused in the IDTF and reserved here for future use.
*/
enum PointFlagBits {
  LAST_POINT_BIT = 0x80, // Bit 7 - always 0 except last point of image
  BLANKING_BIT = 0x40    // Bit 6 - if 1, laser is off
};

#define IS_LASER_ON(flags) (!(flags & BLANKING_BIT))
#define IS_LAST_POINT(flags) (flags & LAST_POINT_BIT)

enum SystemMode { MODE_DUAL_BUFFER = 0, MODE_COUNT };

enum CommandResult {
  CMD_OK = 0,
  CMD_ERROR_INVALID_COMMAND = 1,
  CMD_ERROR_INVALID_PARAMS = 2,
  CMD_ERROR_BUSY = 3,
  CMD_ERROR_BUFFER_FULL = 4,
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))
#define CHEBYSHEV_DISTANCE(x1, y1, x2, y2) (MAX(ABS(x1 - x2), ABS(y1 - y2)))
#define COORD8_TO_Q12_4(coord8) ((int16_t)(coord8) << 4)
#define Q12_4_TO_COORD8(q12_4) ((uint8_t)(q12_4 >> 4))
#define Q12_4_TO_FLOAT(q12_4) ((float)(q12_4) / (16.0f))

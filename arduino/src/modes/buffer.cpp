#include "buffer.h"

// Buffers and pointers are now defined in globals.cpp

void buffer_init() {
  g_buffer_active = g_buffer_A;
  g_buffer_inactive = g_buffer_B;
  g_buffer_active_steps = 0;
  g_buffer_inactive_steps = 0;
  buffer_clear(g_buffer_active);
  buffer_clear(g_buffer_inactive);
}

void buffer_clear(volatile Step *buf) {
  // Use fixed size for clearing since arrays are fixed size
  for (int i = 0; i < MAX_STEPS_FIXED; ++i) {
    buf[i].x = 0;
    buf[i].y = 0;
    buf[i].flags = 0;
  }
}

int buffer_write(volatile Step *buf, int idx, uint8_t x, uint8_t y,
                 uint8_t flags) {
  // Use dynamic size for validation
  if (idx < 0 || idx >= MAX_STEPS)
    return -1;
  buf[idx].x = x;
  buf[idx].y = y;
  buf[idx].flags = flags;
  return 0;
}

// Atomically swap active/inactive buffers
void buffer_swap() {
  noInterrupts();
  volatile Step *tmp = g_buffer_active;
  g_buffer_active = g_buffer_inactive;
  g_buffer_inactive = tmp;

  int tmp_steps = g_buffer_active_steps;
  g_buffer_active_steps = g_buffer_inactive_steps;
  g_buffer_inactive_steps = tmp_steps;
  interrupts();
}

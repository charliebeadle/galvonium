#include "buffer.h"

// Buffers and pointers
volatile Step buffer_A[MAX_STEPS];
volatile Step buffer_B[MAX_STEPS];
volatile Step *buffer_active = buffer_A;
volatile Step *buffer_inactive = buffer_B;
volatile int buffer_active_steps = 0;
volatile int buffer_inactive_steps = 0;

void buffer_init()
{
    buffer_active = buffer_A;
    buffer_inactive = buffer_B;
    buffer_active_steps = 0;
    buffer_inactive_steps = 0;
    buffer_clear(buffer_active);
    buffer_clear(buffer_inactive);
}

void buffer_clear(volatile Step *buf)
{
    for (int i = 0; i < MAX_STEPS; ++i)
    {
        buf[i].x = 0;
        buf[i].y = 0;
        buf[i].flags = 0;
    }
}

int buffer_write(volatile Step *buf, int idx, uint8_t x, uint8_t y, uint8_t flags)
{
    if (idx < 0 || idx >= MAX_STEPS)
        return -1;
    buf[idx].x = x;
    buf[idx].y = y;
    buf[idx].flags = flags;
    return 0;
}

// Atomically swap active/inactive buffers
void buffer_swap()
{
    noInterrupts();
    volatile Step *tmp = buffer_active;
    buffer_active = buffer_inactive;
    buffer_inactive = tmp;

    int tmp_steps = buffer_active_steps;
    buffer_active_steps = buffer_inactive_steps;
    buffer_inactive_steps = tmp_steps;
    interrupts();
}

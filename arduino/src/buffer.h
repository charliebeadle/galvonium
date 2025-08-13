#pragma once
#include <Arduino.h>

#define MAX_STEPS 256

typedef struct
{
    uint8_t x;
    uint8_t y;
    uint8_t flags;
} Step;

extern volatile Step buffer_A[MAX_STEPS];
extern volatile Step buffer_B[MAX_STEPS];
extern volatile Step *buffer_active;
extern volatile Step *buffer_inactive;
extern volatile int buffer_active_steps;
extern volatile int buffer_inactive_steps;

void buffer_init();
void buffer_clear(volatile Step *buf);
int buffer_write(volatile Step *buf, int idx, uint8_t x, uint8_t y, uint8_t flags);
void buffer_swap();

#include "../communication/serial_cmd.h"
#include "../core/timer.h"
#include "../galvo/dac_output.h"
#include "../graphics/interpolation.h"
#include "../modes/buffer.h"
#include "config.h"
#include "debug.h"

// === DEBUG/CONFIG GLOBALS ===
bool g_verbose = false;
bool g_flip_x = false;
bool g_flip_y = false;
bool g_swap_xy = false;
bool g_dac_serial = false;

// === TIMER/CORE GLOBALS ===
volatile bool g_frame_shown_once = true;
volatile bool g_swap_requested = false;
volatile int g_current_step = 0;
volatile uint16_t g_last_x = 0;
volatile uint16_t g_last_y = 0;

// === BUFFER GLOBALS ===
volatile Step g_buffer_A[MAX_STEPS_FIXED];
volatile Step g_buffer_B[MAX_STEPS_FIXED];
volatile Step *g_buffer_active = g_buffer_A;
volatile Step *g_buffer_inactive = g_buffer_B;
volatile int g_buffer_active_steps = 0;
volatile int g_buffer_inactive_steps = 0;

// === INTERPOLATION GLOBALS ===
InterpolationState g_interpolation = {0};

// === CONFIG GLOBALS ===
GalvoConfig g_config;

// === SERIAL/COMMUNICATION GLOBALS ===
char g_parse_buf[PARSE_BUFFER_SIZE];
char g_serial_buf[SERIAL_BUFFER_SIZE];
int g_serial_buf_pos = 0;

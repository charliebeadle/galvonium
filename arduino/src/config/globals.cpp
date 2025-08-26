#include "config.h"
#include "graphics/interpolation.h"
#include "modes/buffer.h"

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
volatile Step g_buffer_A[256];
volatile Step g_buffer_B[256];
volatile Step *g_buffer_active = g_buffer_A;
volatile Step *g_buffer_inactive = g_buffer_B;
volatile int g_buffer_active_steps = 0;
volatile int g_buffer_inactive_steps = 0;

// === INTERPOLATION GLOBALS ===
InterpolationState g_interpolation = {0};

// === CONFIG GLOBALS ===
GalvoConfig g_config;

// === SERIAL/COMMUNICATION GLOBALS ===
char g_parse_buf[12];
char g_serial_buf[24];
int g_serial_buf_pos = 0;

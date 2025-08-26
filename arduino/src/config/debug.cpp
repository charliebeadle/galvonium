#include "debug.h"
#include "../core/timer.h"
#include "../galvo/dac_output.h"
#include "config.h"

#define DEBUG_PPS 10

void debug_set_dac_serial(bool enable) {
  extern bool g_dac_serial;
  g_dac_serial = enable;

  if (enable) {
    set_pps(DEBUG_PPS);
  } else {
    set_pps_from_config();
  }
  if (g_verbose) {
    Serial.print(F("DEBUG: DAC Serial "));
    Serial.println(enable ? F("ON") : F("OFF"));
  }
}

void debug_set_flip_x(bool enable) {
  extern bool g_flip_x;
  g_flip_x = enable;
  if (g_verbose) {
    Serial.print(F("DEBUG: Flip X "));
    Serial.println(enable ? F("ON") : F("OFF"));
  }
}

void debug_set_flip_y(bool enable) {
  extern bool g_flip_y;
  g_flip_y = enable;
  if (g_verbose) {
    Serial.print(F("DEBUG: Flip Y "));
    Serial.println(enable ? F("ON") : F("OFF"));
  }
}

void debug_set_swap_xy(bool enable) {
  extern bool g_swap_xy;
  g_swap_xy = enable;
  if (g_verbose) {
    Serial.print(F("DEBUG: Swap XY "));
    Serial.println(enable ? F("ON") : F("OFF"));
  }
}

void debug_set_verbose(bool enable) {
  extern bool g_verbose;
  g_verbose = enable;
  if (g_verbose) {
    Serial.print(F("DEBUG: Verbose "));
    Serial.println(enable ? F("ON") : F("OFF"));
  }
}

void debug_update_all() {
  debug_set_flip_x(config_get_flag(DEBUG_FLAG_FLIP_X));
  debug_set_flip_y(config_get_flag(DEBUG_FLAG_FLIP_Y));
  debug_set_swap_xy(config_get_flag(DEBUG_FLAG_SWAP_XY));
  debug_set_dac_serial(config_get_flag(DEBUG_FLAG_DAC_SERIAL));
  debug_set_verbose(config_get_flag(DEBUG_FLAG_VERBOSE));
}

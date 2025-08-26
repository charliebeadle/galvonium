#include "communication/serial_cmd.h"
#include "config/config.h"
#include "config/debug.h"
#include "config/globals.h"
#include "core/timer.h"
#include "modes/buffer.h"
#include <Arduino.h>

void setup() {
  config_init();
  buffer_init();
  serial_cmd_init();
  initTimer();
  debug_update_all();
}

void loop() { serial_cmd_poll(); }

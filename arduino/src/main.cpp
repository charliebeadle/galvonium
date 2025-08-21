#include "communication/serial_cmd.h"
#include "config/config.h"
#include "core/timer.h"
#include "modes/buffer.h"
#include <Arduino.h>


void setup() {
  buffer_init();
  serial_cmd_init();
  initTimer();
  config_init();
}

void loop() { serial_cmd_poll(); }

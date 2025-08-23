#include "communication/serial_cmd.h"
#include "config/config.h"
#include "core/timer.h"
#include "modes/buffer.h"
#include <Arduino.h>


void setup() {
  config_init();
  buffer_init();
  serial_cmd_init();
  initTimer();
}

void loop() { serial_cmd_poll(); }

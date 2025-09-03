#include "hardware/hardware.h"
#include "renderer/renderer.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>

void setup() {

  Hardware::init();

  renderer.init();

  Hardware::setDataSource((void *)renderer_data_source);
}

void loop() { renderer.process(); }

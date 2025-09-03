#include "hardware/hardware.h"
#include "renderer/renderer.h"
#include "debug.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>

void setup() {
  DEBUG_INFO("System startup");
  
  Hardware::init();
  renderer.init();
  Hardware::setDataSource((void *)renderer_data_source);
  
  DEBUG_INFO("System ready");
}

void loop() { 
  CHECK_ISR_ERRORS(); // Check for any ISR errors and report them
  renderer.process(); 
}

#include "hardware/hardware.h"
#include "renderer/renderer.h"
#include "debug.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>

void setup() {
  DEBUG_INFO("System startup beginning");
  
  DEBUG_INFO("Initializing hardware subsystem");
  Hardware::init();
  
  DEBUG_INFO("Initializing renderer subsystem");
  renderer.init();
  
  DEBUG_INFO("Setting up data source bridge");
  Hardware::setDataSource((void *)renderer_data_source);
  
  DEBUG_INFO("System startup complete - entering main loop");
}

void loop() { 
  renderer.process(); 
}

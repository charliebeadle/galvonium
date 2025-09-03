#include "debug.h"
#include "hardware/hardware.h"
#include "renderer/renderer.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>


// Define ISR error flags (declared as extern in debug.h)
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
volatile bool isr_error_flag = false;
volatile uint8_t isr_error_code = 0;
#endif

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

#include "hardware/hardware.h"
#include "renderer/renderer.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>

void setup() {

  DEBUG_INFO(F("Setup started"));
  Hardware::init();
  DEBUG_INFO(F("Hardware initialized"));
  renderer.init();
  DEBUG_INFO(F("Renderer initialized"));
  Hardware::setDataSource((void *)renderer_data_source);
  DEBUG_INFO(F("Data source set"));

#if ENABLE_DEBUG_PINS
  pinMode(DEBUG_DAC_PIN, OUTPUT);
  pinMode(DEBUG_ISR_PIN, OUTPUT);
#endif
  DEBUG_ISR_PIN_ON();
  delay(100);
  DEBUG_ISR_PIN_OFF();
}

void loop() {
  DEBUG_DAC_PIN_ON();
  DEBUG_INFO(F("Loop started"));
  renderer.process();
  DEBUG_INFO(F("Loop completed"));

  DEBUG_DAC_PIN_OFF();
}

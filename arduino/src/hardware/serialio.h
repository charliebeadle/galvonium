#pragma once
#include "../config.h"
#include "../debug.h"
#include <Arduino.h>
#include <HardwareSerial.h>

class SerialIO {
public:
  void init();
  void init(uint32_t baud);
  bool available();
  char read();
  void write(char c);
  void print(const char *str);
  void println(const char *str);

private:
  uint32_t baud_rate;
};

void SerialIO::init() {
  baud_rate = DEFAULT_BAUD_RATE;
  VALIDATE_RANGE_CLIP(baud_rate, MIN_BAUD_RATE, MAX_BAUD_RATE);
  Serial.begin(baud_rate);
  DEBUG_INFO("Serial IO initialized");
  DEBUG_INFO_VAL("Baud rate: ", baud_rate);
  Serial.println(F("Galvonium ready."));
}

void SerialIO::init(uint32_t baud) {
  VALIDATE_RANGE_CLIP(baud, MIN_BAUD_RATE, MAX_BAUD_RATE);
  baud_rate = baud;
  Serial.begin(baud_rate);
  DEBUG_INFO("Serial IO initialized with custom baud rate");
  DEBUG_INFO_VAL("Baud rate: ", baud_rate);
  Serial.println(F("Galvonium ready."));
}

bool SerialIO::available() { return Serial.available(); }

char SerialIO::read() { 
  if (!available()) {
    DEBUG_VERBOSE("SerialIO::read called with no data available");
  }
  return Serial.read(); 
}

void SerialIO::write(char c) { Serial.write(c); }

void SerialIO::print(const char *str) { Serial.print(str); }

void SerialIO::println(const char *str) { Serial.println(str); }

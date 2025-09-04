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
  baud_rate = 9600;
  Serial.begin(baud_rate);
  DEBUG_INFO(F("Serial IO initialized"));
  DEBUG_INFO(F("Baud rate: %d"), baud_rate);
  Serial.println(F("Galvonium ready."));
}

void SerialIO::init(uint32_t baud) {
  Serial.begin(baud);
  DEBUG_INFO(F("Serial IO initialized"));
  DEBUG_INFO(F("Baud rate: %d"), baud);
  Serial.println(F("Galvonium ready."));
}

bool SerialIO::available() { return Serial.available(); }

char SerialIO::read() { return Serial.read(); }

void SerialIO::write(char c) { Serial.write(c); }

void SerialIO::print(const char *str) { Serial.print(str); }

void SerialIO::println(const char *str) { Serial.println(str); }

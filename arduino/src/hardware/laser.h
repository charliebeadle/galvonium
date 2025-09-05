#pragma once
#include "../config.h"
#include <Arduino.h>

// Laser control class
// TODO: write directly to port registers instead of using digitalWrite

class Laser {
public:
  Laser() {
    // Constructor - initialization will be done in init()
  }

  void init() {
    laser_pin = g_config.laser.pin;
    pinMode(laser_pin, OUTPUT);
    digitalWrite(laser_pin, LOW);
  }

  void set_pin(uint8_t pin) {
    pinMode(pin, INPUT);
    laser_pin = pin;
    pinMode(laser_pin, OUTPUT);
    digitalWrite(laser_pin, LOW);
  }

  void set_laser(bool enable) {
    if (enable) {
      digitalWrite(laser_pin, HIGH);
    } else {
      digitalWrite(laser_pin, LOW);
    }
  }

  bool is_laser_on() { return digitalRead(laser_pin) == HIGH; }

private:
  uint8_t laser_pin;
};

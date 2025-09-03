#pragma once
#include "../config.h"
#include "../debug.h"
#include <Arduino.h>

// Laser control class
// TODO: write directly to port registers instead of using digitalWrite
class Laser {
public:
  Laser() {
    // Constructor - initialization will be done in init()
  }

  void init() {
    DEBUG_INFO("Laser init");
    pinMode(LASER_PIN, OUTPUT);
    digitalWrite(LASER_PIN, LOW);
    DEBUG_INFO_VAL("Laser ready on pin: ", LASER_PIN);
  }

  void set_laser(bool enable) {
    if (enable) {
      digitalWrite(LASER_PIN, HIGH);
    } else {
      digitalWrite(LASER_PIN, LOW);
    }
  }

  bool is_laser_on() { return digitalRead(LASER_PIN) == HIGH; }
};

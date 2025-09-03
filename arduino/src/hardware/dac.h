#pragma once
#include "../config.h"
#include "../debug.h"
#include "../types.h"
#include <Arduino.h>
#include <SPI.h>

class DAC {
public:
  void init() {
    DEBUG_INFO("DAC initialization starting");
    
    DDRB |= (1 << PB2);   // Set CS pin as output
    PORTB &= ~(1 << PB2); // Set CS low initially
    
    SPI.begin();
    SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE0));
    
    DEBUG_INFO("DAC initialized");
    DEBUG_VERBOSE_VAL("SPI speed: ", SPI_SPEED);
  }

  void output_point(point_q12_4_t *point) {
    VALIDATE_POINTER(point, "point");
    
    // Validate Q12.4 coordinates
    if (point->x < Q12_4_MIN || point->x > Q12_4_MAX) {
      DEBUG_ERROR_VAL2("DAC X coordinate out of range: ", point->x, " Valid range: ");
      DEBUG_ERROR_VAL2("Min: ", Q12_4_MIN, " Max: ");
      DEBUG_ERROR_VAL("", Q12_4_MAX);
      return;
    }
    
    if (point->y < Q12_4_MIN || point->y > Q12_4_MAX) {
      DEBUG_ERROR_VAL2("DAC Y coordinate out of range: ", point->y, " Valid range: ");
      DEBUG_ERROR_VAL2("Min: ", Q12_4_MIN, " Max: ");
      DEBUG_ERROR_VAL("", Q12_4_MAX);
      return;
    }
    
    uint16_t packetX = DAC_FLAGS_A << 8 | point->x >> 4;
    uint16_t packetY = DAC_FLAGS_B << 8 | point->y >> 4;
    
    DEBUG_VERBOSE_VAL2("Outputting point - X: ", point->x, " Y: ");
    DEBUG_VERBOSE_VAL("", point->y);
    
    output(packetX, packetY);
  }

private:
  void output(uint16_t x, uint16_t y) {
    PORTB &= ~(1 << PB2); // Set CS low
    SPI.transfer16(x);
    PORTB |= (1 << PB2); // Set CS high

    PORTB &= ~(1 << PB2); // Set CS low
    SPI.transfer16(y);
    PORTB |= (1 << PB2); // Set CS high
  }
};

#pragma once
#include "../config.h"
#include "../types.h"
#include <Arduino.h>
#include <SPI.h>

class DAC {
public:
  void init() {
    DDRB |= (1 << PB2);   // Set CS pin as output
    PORTB &= ~(1 << PB2); // Set CS low initially
    SPI.begin();
    SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE0));
  }

  void output_point(point_q12_4_t *point) {
    uint16_t packetX = DAC_FLAGS_A << 8 | point->x >> 4;
    uint16_t packetY = DAC_FLAGS_B << 8 | point->y >> 4;
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

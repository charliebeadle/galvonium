#pragma once
#include "../config.h"
#include "../types.h"
#include <Arduino.h>
#include <SPI.h>

class DAC {
public:
  void init() {

    const auto &dac_config = g_config.dac;

    dac_flags_a = dac_config.dac_flags_a;
    dac_flags_b = dac_config.dac_flags_b;

    DDRB |= (1 << PB2);   // Set CS pin as output
    PORTB &= ~(1 << PB2); // Set CS low initially

    SPI.begin();
    SPI.beginTransaction(SPISettings(dac_config.speed, dac_config.bit_order,
                                     dac_config.data_mode));
  }

  void output_point(point_q12_4_t *point) {
    uint16_t packetX = dac_flags_a << 8 | (point->x & 0x0FFF);
    uint16_t packetY = dac_flags_b << 8 | (point->y & 0x0FFF);
    output(packetX, packetY);
  }

private:
  uint8_t dac_flags_a;
  uint8_t dac_flags_b;

  void output(uint16_t x, uint16_t y) {
    PORTB &= ~(1 << PB2); // Set CS low
    SPI.transfer16(x);
    PORTB |= (1 << PB2); // Set CS high

    PORTB &= ~(1 << PB2); // Set CS low
    SPI.transfer16(y);
    PORTB |= (1 << PB2); // Set CS high
  }
};

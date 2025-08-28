#include "dac_output.h"
#include "../core/timer.h"
#include <SPI.h>

void dac_output_init(void) {
  // Initialize SPI and DAC control
  DDRB |= (1 << PB2);   // Set CS pin as output
  PORTB &= ~(1 << PB2); // Set CS low initially
  SPI.begin();
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
}

// Function to output DAC values for X and Y coordinates
void outputDAC(uint16_t x, uint16_t y) {

  if (g_dac_serial && !is_frame_shown_once()) {
    Serial.print(x >> 4 << 4, HEX);
    Serial.print(" ");
    Serial.println(y >> 4 << 4, HEX);
  }

  uint16_t packetX = DAC_FLAGS_A << 8 | x >> 4;
  uint16_t packetY = DAC_FLAGS_B << 8 | y >> 4;

  PORTB &= ~(1 << PB2); // Set CS low
  SPI.transfer16(packetX);
  PORTB |= (1 << PB2); // Set CS high

  PORTB &= ~(1 << PB2); // Set CS low
  SPI.transfer16(packetY);
  PORTB |= (1 << PB2); // Set CS high
}

#pragma once
#include <EEPROM.h>

namespace EEPROMUtils {

// EEPROM utility functions
void write_byte(uint16_t address, uint8_t value) {
  EEPROM.write(address, value);
}

uint8_t read_byte(uint16_t address) { return EEPROM.read(address); }

void update_block(uint16_t address, const uint8_t *data, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    EEPROM.update(address + i, data[i]);
  }
}

void read_block(uint16_t address, uint8_t *data, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    data[i] = EEPROM.read(address + i);
  }
}

void clear_area(uint16_t address, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    EEPROM.write(address + i, 0);
  }
}

} // namespace EEPROMUtils

#pragma once
#include "../config.h"
#include "../debug.h"
#include <EEPROM.h>

namespace EEPROMUtils {

// EEPROM utility functions
void write_byte(uint16_t address, uint8_t value) {
  if (address >= 1024) { // Arduino Uno EEPROM is 1024 bytes
    DEBUG_ERROR_VAL("EEPROM write address out of range: ", address);
    return;
  }
  EEPROM.write(address, value);
}

uint8_t read_byte(uint16_t address) {
  if (address >= 1024) { // Arduino Uno EEPROM is 1024 bytes
    DEBUG_ERROR_VAL("EEPROM read address out of range: ", address);
    return 0;
  }
  return EEPROM.read(address);
}

void update_block(uint16_t address, const uint8_t *data, uint8_t length) {
  if (data == nullptr) {
    DEBUG_ERROR("EEPROM update_block: null data pointer");
    return;
  }

  if (address + length > 1024) {
    DEBUG_ERROR_VAL("EEPROM update block extends beyond memory: ",
                    address + length);
    return;
  }

  for (uint8_t i = 0; i < length; i++) {
    EEPROM.update(address + i, data[i]);
  }
}

void read_block(uint16_t address, uint8_t *data, uint8_t length) {
  if (data == nullptr) {
    DEBUG_ERROR("EEPROM read_block: null data pointer");
    return;
  }

  if (address + length > 1024) {
    DEBUG_ERROR_VAL("EEPROM read block extends beyond memory: ",
                    address + length);
    return;
  }

  for (uint8_t i = 0; i < length; i++) {
    data[i] = EEPROM.read(address + i);
  }
}

void clear_area(uint16_t address, uint8_t length) {
  if (address + length > 1024) {
    DEBUG_ERROR_VAL("EEPROM clear area extends beyond memory: ",
                    address + length);
    return;
  }

  for (uint8_t i = 0; i < length; i++) {
    EEPROM.write(address + i, 0);
  }
}

} // namespace EEPROMUtils

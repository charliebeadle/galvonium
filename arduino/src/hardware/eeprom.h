#pragma once
#include "../debug.h"
#include <EEPROM.h>

namespace EEPROMUtils {

// EEPROM utility functions
void write_byte(uint16_t address, uint8_t value) {
  if (address >= 1024) { // Arduino Uno EEPROM is 1024 bytes
    DEBUG_ERROR_VAL("EEPROM write address out of range: ", address);
    return;
  }
  DEBUG_VERBOSE_VAL2("EEPROM write - addr: ", address, " val: ");
  DEBUG_VERBOSE_VAL("", value);
  EEPROM.write(address, value);
}

uint8_t read_byte(uint16_t address) { 
  if (address >= 1024) { // Arduino Uno EEPROM is 1024 bytes
    DEBUG_ERROR_VAL("EEPROM read address out of range: ", address);
    return 0;
  }
  uint8_t value = EEPROM.read(address);
  DEBUG_VERBOSE_VAL2("EEPROM read - addr: ", address, " val: ");
  DEBUG_VERBOSE_VAL("", value);
  return value;
}

void update_block(uint16_t address, const uint8_t *data, uint8_t length) {
  VALIDATE_POINTER(data, "data");
  
  if (address + length > 1024) {
    DEBUG_ERROR_VAL("EEPROM update block extends beyond memory: ", address + length);
    return;
  }
  
  DEBUG_INFO_VAL2("EEPROM update block - addr: ", address, " len: ");
  DEBUG_INFO_VAL("", length);
  
  for (uint8_t i = 0; i < length; i++) {
    EEPROM.update(address + i, data[i]);
  }
}

void read_block(uint16_t address, uint8_t *data, uint8_t length) {
  VALIDATE_POINTER(data, "data");
  
  if (address + length > 1024) {
    DEBUG_ERROR_VAL("EEPROM read block extends beyond memory: ", address + length);
    return;
  }
  
  DEBUG_INFO_VAL2("EEPROM read block - addr: ", address, " len: ");
  DEBUG_INFO_VAL("", length);
  
  for (uint8_t i = 0; i < length; i++) {
    data[i] = EEPROM.read(address + i);
  }
}

void clear_area(uint16_t address, uint8_t length) {
  if (address + length > 1024) {
    DEBUG_ERROR_VAL("EEPROM clear area extends beyond memory: ", address + length);
    return;
  }
  
  DEBUG_INFO_VAL2("EEPROM clearing area - addr: ", address, " len: ");
  DEBUG_INFO_VAL("", length);
  
  for (uint8_t i = 0; i < length; i++) {
    EEPROM.write(address + i, 0);
  }
}

} // namespace EEPROMUtils

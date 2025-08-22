#include "eeprom.h"
#include <EEPROM.h>

// === CONFIGURATION EEPROM OPERATIONS ===

bool eeprom_load_config(uint8_t *config_data, uint8_t config_size) {
  if (config_data == nullptr || config_size != EEPROM_CONFIG_SIZE) {
    return false;
  }

  // Read configuration from EEPROM byte by byte to avoid alignment issues
  for (uint8_t i = 0; i < config_size; i++) {
    config_data[i] = EEPROM.read(EEPROM_CONFIG_START + i);
  }

  // Check if EEPROM is uninitialized (all 0xFF)
  bool is_uninitialized = true;
  for (uint8_t i = 0; i < config_size; i++) {
    if (config_data[i] != EEPROM_UNINITIALIZED_VALUE) {
      is_uninitialized = false;
      break;
    }
  }

  return !is_uninitialized;
}

bool eeprom_save_config(const uint8_t *config_data, uint8_t config_size) {
  if (config_data == nullptr || config_size != EEPROM_CONFIG_SIZE) {
    return false;
  }

  // Write configuration to EEPROM byte by byte
  for (uint8_t i = 0; i < config_size; i++) {
    EEPROM.update(EEPROM_CONFIG_START + i, config_data[i]);
  }

  return true;
}

void eeprom_clear_config_area(void) {
  // Clear EEPROM area by writing uninitialized state
  for (uint8_t i = 0; i < EEPROM_CONFIG_SIZE; i++) {
    EEPROM.write(EEPROM_CONFIG_START + i, EEPROM_UNINITIALIZED_VALUE);
  }
}

// === UTILITY FUNCTIONS ===

bool eeprom_is_initialized(void) {
  // Check if any byte in the config area is not uninitialized
  for (uint8_t i = 0; i < EEPROM_CONFIG_SIZE; i++) {
    if (EEPROM.read(EEPROM_CONFIG_START + i) != EEPROM_UNINITIALIZED_VALUE) {
      return true;
    }
  }
  return false;
}

void eeprom_write_bytes(uint16_t address, const uint8_t *data, uint8_t length) {
  if (data == nullptr)
    return;

  for (uint8_t i = 0; i < length; i++) {
    EEPROM.update(address + i, data[i]);
  }
}

void eeprom_read_bytes(uint16_t address, uint8_t *data, uint8_t length) {
  if (data == nullptr)
    return;

  for (uint8_t i = 0; i < length; i++) {
    data[i] = EEPROM.read(address + i);
  }
}

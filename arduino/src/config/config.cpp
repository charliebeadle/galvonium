#include "config.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>

// Global configuration instance
GalvoConfig g_config;

// === INITIALIZATION & PERSISTENCE ===

void config_init(void) {
  // Standard Arduino EEPROM doesn't need initialization

  // Always read configuration from EEPROM on startup
  if (!config_load_from_eeprom()) {
    // If reading fails, load defaults
    config_load_defaults();
    // Save defaults to EEPROM
    config_save_to_eeprom();
  }
}

bool config_load_from_eeprom(void) {
  // Check if we have enough EEPROM space
  if (CONFIG_EEPROM_START + sizeof(GalvoConfig) > EEPROM.length()) {
    return false;
  }

  // Read configuration from EEPROM byte by byte to avoid alignment issues
  uint8_t *data = (uint8_t *)&g_config;
  for (size_t i = 0; i < sizeof(GalvoConfig); i++) {
    data[i] = EEPROM.read(CONFIG_EEPROM_START + i);
  }

  // Check if EEPROM is uninitialized (all 0xFF)
  bool is_uninitialized = true;
  for (size_t i = 0; i < sizeof(GalvoConfig); i++) {
    if (data[i] != 0xFF) {
      is_uninitialized = false;
      break;
    }
  }

  if (is_uninitialized) {
    return false;
  }

  // Validate magic number
  if (g_config.magic != CONFIG_MAGIC) {
    return false;
  }

  // Validate version
  if (g_config.version != CONFIG_VERSION) {
    return false;
  }

  // Validate checksum
  uint8_t calculated_checksum = config_calculate_checksum(&g_config);
  if (g_config.checksum != calculated_checksum) {
    return false;
  }

  return true;
}

bool config_save_to_eeprom(void) {
  // Check if we have enough EEPROM space
  if (CONFIG_EEPROM_START + sizeof(GalvoConfig) > EEPROM.length()) {
    return false;
  }

  // Calculate and update checksum
  g_config.checksum = config_calculate_checksum(&g_config);

  // Write configuration to EEPROM byte by byte using update() to save write
  // cycles
  uint8_t *data = (uint8_t *)&g_config;
  for (size_t i = 0; i < sizeof(GalvoConfig); i++) {
    EEPROM.update(CONFIG_EEPROM_START + i, data[i]);
  }

  return true;
}

void config_load_defaults(void) {
  // Set magic and version
  g_config.magic = CONFIG_MAGIC;
  g_config.version = CONFIG_VERSION;

  // Set default operating mode
  g_config.mode = CONFIG_DEFAULT_MODE;

  // Set default points per second
  g_config.pps = CONFIG_DEFAULT_PPS;

  // Clear reserved bytes
  memset(g_config.reserved, 0, sizeof(g_config.reserved));

  // Calculate initial checksum
  g_config.checksum = config_calculate_checksum(&g_config);
}

void config_reset_eeprom(void) {
  // Clear EEPROM area by writing 0xFF (uninitialized state)
  for (size_t i = 0; i < sizeof(GalvoConfig); i++) {
    EEPROM.write(CONFIG_EEPROM_START + i, 0xFF);
  }

  // Load defaults and save
  config_load_defaults();
  config_save_to_eeprom();
}

// === PARAMETER ACCESS ===

uint16_t config_get(ConfigParam param) {
  switch (param) {
  case PARAM_MODE:
    return g_config.mode;
  case PARAM_PPS:
    return g_config.pps;
  default:
    return 0; // Invalid parameter
  }
}

bool config_set(ConfigParam param, uint16_t value) {
  switch (param) {
  case PARAM_MODE:
    if (value <= MODE_DUAL_BUFFER) {
      g_config.mode = (uint8_t)value;
      return true;
    }
    break;
  case PARAM_PPS:
    if (value > 0 && value <= 65535) {
      g_config.pps = (uint16_t)value;
      return true;
    }
    break;
  default:
    return false; // Invalid parameter
  }
  return false; // Invalid value
}

// === UTILITY ===

uint8_t config_calculate_checksum(const GalvoConfig *config) {
  uint8_t checksum = 0;
  const uint8_t *data = (const uint8_t *)config;

  // Calculate checksum for all bytes except the checksum field itself
  for (size_t i = 0; i < sizeof(GalvoConfig) - 1; i++) {
    checksum ^= data[i];
  }

  return checksum;
}

#include "config.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <string.h>

// Global configuration instance
GalvoConfig g_config;

// === PARAMETER NAMES IN PROGMEM ===
static const char *const param_names[] PROGMEM = {
    "MODE", "DEBUG_FLAGS", "PPS", "MAX_BUFFER_INDEX", "MAX_STEP_LENGTH"};

// === PARAMETER ACCESS HELPERS ===

static void config_set_param(ConfigParam param, uint16_t value) {
  switch (param) {
  case PARAM_MODE:
    g_config.mode = (uint8_t)value;
    break;
  case PARAM_DEBUG_FLAGS:
    g_config.debug_flags = (uint8_t)value;
    break;
  case PARAM_PPS:
    g_config.pps = value;
    break;
  case PARAM_MAX_BUFFER_INDEX:
    g_config.max_buffer_index = (uint8_t)value;
    break;
  case PARAM_MAX_STEP_LENGTH:
    g_config.max_step_length = (uint8_t)value;
    break;
  default:
    break;
  }
}

static uint16_t config_get_param_from_struct(const GalvoConfig *config,
                                             ConfigParam param) {
  switch (param) {
  case PARAM_MODE:
    return config->mode;
  case PARAM_DEBUG_FLAGS:
    return config->debug_flags;
  case PARAM_PPS:
    return config->pps;
  case PARAM_MAX_BUFFER_INDEX:
    return config->max_buffer_index;
  case PARAM_MAX_STEP_LENGTH:
    return config->max_step_length;
  default:
    return 0;
  }
}

static uint16_t config_get_default(ConfigParam param) {
  switch (param) {
  case PARAM_MODE:
    return CONFIG_DEFAULT_MODE;
  case PARAM_DEBUG_FLAGS:
    return CONFIG_DEFAULT_DEBUG_FLAGS;
  case PARAM_PPS:
    return CONFIG_DEFAULT_PPS;
  case PARAM_MAX_BUFFER_INDEX:
    return CONFIG_DEFAULT_MAX_BUFFER_INDEX;
  case PARAM_MAX_STEP_LENGTH:
    return CONFIG_DEFAULT_MAX_STEP_LENGTH;
  default:
    return 0;
  }
}

// === INITIALIZATION & PERSISTENCE ===

void config_init(void) {
  if (!config_load_from_eeprom()) {
    config_load_defaults();
    config_save_to_eeprom();
  }
}

bool config_load_from_eeprom(void) {
  GalvoConfig temp_config;

  // Load configuration from EEPROM
  if (!eeprom_load_config((uint8_t *)&temp_config,
                          (uint8_t)sizeof(GalvoConfig))) {
    return false;
  }

  // Validate magic number
  if (temp_config.magic != CONFIG_MAGIC) {
    return false;
  }

  // Validate checksum of old config
  uint8_t calculated_checksum = config_calculate_checksum(&temp_config);
  if (temp_config.checksum != calculated_checksum) {
    return false;
  }

  // === VERSION MIGRATION LOGIC ===

  // Start with current defaults for all parameters
  config_load_defaults();

  // Copy valid parameters from old config based on param_count
  uint8_t valid_params = min(temp_config.param_count, (uint8_t)PARAM_COUNT);

  for (uint8_t param = 0; param < valid_params; param++) {
    uint16_t old_value =
        config_get_param_from_struct(&temp_config, (ConfigParam)param);
    config_set_param((ConfigParam)param, old_value);
  }

  // Update to current version and recalculate checksum
  g_config.magic = CONFIG_MAGIC;
  g_config.param_count = CONFIG_CURRENT_VERSION;
  g_config.checksum = config_calculate_checksum(&g_config);

  return true;
}

bool config_save_to_eeprom(void) {
  // Ensure current version and valid checksum
  g_config.magic = CONFIG_MAGIC;
  g_config.param_count = CONFIG_CURRENT_VERSION;
  g_config.checksum = config_calculate_checksum(&g_config);

  // Verify structure size matches EEPROM allocation
  if (sizeof(GalvoConfig) != EEPROM_CONFIG_SIZE) {
    return false;
  }

  return eeprom_save_config((uint8_t *)&g_config, (uint8_t)sizeof(GalvoConfig));
}

void config_load_defaults(void) {
  // Set magic and version
  g_config.magic = CONFIG_MAGIC;
  g_config.param_count = CONFIG_CURRENT_VERSION;

  // Load all current defaults using enumeration
  for (uint8_t param = 0; param < PARAM_COUNT; param++) {
    config_set_param((ConfigParam)param,
                     config_get_default((ConfigParam)param));
  }

  // Clear reserved bytes
  for (uint8_t i = 0; i < sizeof(g_config.reserved); i++) {
    g_config.reserved[i] = 0;
  }

  // Calculate initial checksum
  g_config.checksum = config_calculate_checksum(&g_config);
}

void config_reset_eeprom(void) {
  // Clear EEPROM area
  eeprom_clear_config_area();

  // Load defaults and save
  config_load_defaults();
  config_save_to_eeprom();
}

// === PARAMETER ACCESS ===

uint16_t config_get(ConfigParam param) {
  switch (param) {
  case PARAM_MODE:
    return g_config.mode;
  case PARAM_DEBUG_FLAGS:
    return g_config.debug_flags;
  case PARAM_PPS:
    return g_config.pps;
  case PARAM_MAX_BUFFER_INDEX:
    return g_config.max_buffer_index;
  case PARAM_MAX_STEP_LENGTH:
    return g_config.max_step_length;
  default:
    return 0; // Invalid parameter
  }
}

bool config_set(ConfigParam param, uint16_t value) {
  switch (param) {
  case PARAM_MODE:
    if (value < MODE_COUNT) {
      g_config.mode = (uint8_t)value;
      return true;
    }
    break;

  case PARAM_DEBUG_FLAGS:
    g_config.debug_flags = (uint8_t)value;
    return true;

  case PARAM_PPS:
    if (value > 0 && value <= 65535) {
      g_config.pps = (uint16_t)value;
      return true;
    }
    break;

  case PARAM_MAX_BUFFER_INDEX:
    if (value <= 255) {
      g_config.max_buffer_index = (uint8_t)value;
      return true;
    }
    break;

  case PARAM_MAX_STEP_LENGTH:
    if (value > 0 && value <= 255) {
      g_config.max_step_length = (uint8_t)value;
      return true;
    }
    break;

  default:
    return false; // Invalid parameter
  }
  return false; // Invalid value
}

// === PARAMETER NAME ACCESS ===

const char *config_get_param_name(ConfigParam param) {
  if (param < PARAM_COUNT) {
    // Read parameter name from PROGMEM
    return (const char *)pgm_read_word(&param_names[param]);
  }
  return "UNKNOWN";
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
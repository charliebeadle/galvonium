#pragma once

#include <stdint.h>

// === CONFIGURATION CONSTANTS ===

#define CONFIG_MAGIC 0x6A17
#define CONFIG_EEPROM_START 0
#define CONFIG_EEPROM_SIZE 32 // Two EEPROM pages (16 bytes each)

// Parameter enumeration for versioning (ADD NEW PARAMS BEFORE PARAM_COUNT)
enum ConfigParam {
  PARAM_MODE = 0,
  PARAM_DEBUG_FLAGS = 1,
  PARAM_PPS = 2,
  PARAM_MAX_BUFFER_INDEX = 3,
  PARAM_MAX_STEP_LENGTH = 4,
  PARAM_COUNT // Always last - this is our version number
};

#define CONFIG_CURRENT_VERSION PARAM_COUNT

// Operating modes
enum SystemMode {
  MODE_DUAL_BUFFER = 0,
  MODE_COUNT
  // Add new modes here as needed
};

// === DEFAULT VALUES ===

#define CONFIG_DEFAULT_MODE 0
#define CONFIG_DEFAULT_DEBUG_FLAGS 0x00
#define CONFIG_DEFAULT_PPS 10000
#define CONFIG_DEFAULT_MAX_BUFFER_INDEX 255
#define CONFIG_DEFAULT_MAX_STEP_LENGTH 5

// === CONFIGURATION STRUCTURE ===

struct GalvoConfig {
  uint16_t magic;      // Magic number for validation
  uint8_t param_count; // Number of valid parameters (version)

  // Parameters in enumeration order
  uint8_t mode;             // PARAM_MODE
  uint8_t debug_flags;      // PARAM_DEBUG_FLAGS
  uint16_t pps;             // PARAM_PPS
  uint8_t max_buffer_index; // PARAM_MAX_BUFFER_INDEX
  uint8_t max_step_length;  // PARAM_MAX_STEP_LENGTH

  uint8_t reserved[22]; // Future expansion
  uint8_t checksum;     // XOR checksum (always last)
};

// === GLOBAL CONFIGURATION ===

extern GalvoConfig g_config;

// === FUNCTION DECLARATIONS ===

// Initialization & persistence
void config_init(void);
bool config_load_from_eeprom(void);
bool config_save_to_eeprom(void);
void config_load_defaults(void);
void config_reset_eeprom(void);

// Parameter access
uint16_t config_get(ConfigParam param);
bool config_set(ConfigParam param, uint16_t value);
const char *config_get_param_name(ConfigParam param);

// Utility
uint8_t config_calculate_checksum(const GalvoConfig *config);
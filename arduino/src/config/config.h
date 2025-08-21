#pragma once

#include <Arduino.h>

// Configuration constants
#define CONFIG_MAGIC 0x6A17   // "GA17" in hex - validates EEPROM data
#define CONFIG_VERSION 1      // Current config structure version
#define CONFIG_EEPROM_START 0 // EEPROM address offset

// Default parameter values
#define CONFIG_DEFAULT_MODE MODE_DUAL_BUFFER
#define CONFIG_DEFAULT_PPS 1000

// Operating modes
enum GalvoMode : uint8_t { MODE_DUAL_BUFFER, MODE_COUNT };

// Parameter enumeration for generic access
enum ConfigParam : uint8_t { PARAM_MODE, PARAM_PPS, PARAM_COUNT };

// Configuration structure - 16 bytes (single EEPROM page)
struct GalvoConfig {
  uint16_t magic;      // Validation marker (CONFIG_MAGIC)
  uint8_t version;     // Structure version
  uint8_t mode;        // Current operating mode (GalvoMode)
  uint16_t pps;        // Points per second
  uint8_t reserved[9]; // Future expansion
  uint8_t checksum;    // Simple validation checksum
} __attribute__((packed));

// Global configuration instance
extern GalvoConfig g_config;

// === INITIALIZATION & PERSISTENCE ===

// Initialize configuration system - call once in setup()
void config_init(void);

// Load configuration from EEPROM
bool config_load_from_eeprom(void);

// Save current configuration to EEPROM
bool config_save_to_eeprom(void);

// Reset to factory defaults
void config_load_defaults(void);

// Reset EEPROM configuration (useful for debugging)
void config_reset_eeprom(void);

// === PARAMETER ACCESS ===

// Generic parameter access
uint16_t config_get(ConfigParam param);
bool config_set(ConfigParam param, uint16_t value);

// === UTILITY ===

// Calculate checksum for validation
uint8_t config_calculate_checksum(const GalvoConfig *config);
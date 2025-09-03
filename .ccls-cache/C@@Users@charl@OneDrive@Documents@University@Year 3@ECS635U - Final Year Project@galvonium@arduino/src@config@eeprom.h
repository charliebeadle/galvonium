#pragma once

#include <stdint.h>

// === EEPROM CONSTANTS ===

#define EEPROM_CONFIG_START 0
#define EEPROM_CONFIG_SIZE 32
#define EEPROM_UNINITIALIZED_VALUE 0xFF

// === FUNCTION DECLARATIONS ===

// Configuration EEPROM operations
bool eeprom_load_config(uint8_t *config_data, uint8_t config_size);
bool eeprom_save_config(const uint8_t *config_data, uint8_t config_size);
void eeprom_clear_config_area(void);

// Utility functions
bool eeprom_is_initialized(void);
void eeprom_write_bytes(uint16_t address, const uint8_t *data, uint8_t length);
void eeprom_read_bytes(uint16_t address, uint8_t *data, uint8_t length);

// === CONVENIENCE FUNCTIONS (CONFIG AREA ONLY) ===
// These functions are optimized for reading from the config area starting at
// EEPROM_CONFIG_START They do NOT work with arbitrary EEPROM addresses - only
// use for config operations

// Read a single byte from the config area
uint8_t eeprom_read_config_byte(uint8_t offset);

// Read a 16-bit word from the config area (little-endian: low byte first)
uint16_t eeprom_read_config_word(uint8_t offset);

// Read a block of config data with bounds checking
bool eeprom_read_config_block(uint8_t *data, uint8_t offset, uint8_t length);

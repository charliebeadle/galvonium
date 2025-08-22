# Configuration Parameters

This document describes the configuration parameters and EEPROM management system used by the Galvonium system.

## Parameter Overview

| Parameter          | Type     | Default | Range             | Description                            |
| ------------------ | -------- | ------- | ----------------- | -------------------------------------- |
| `mode`             | `uint8`  | 0       | (0, MODE_COUNT-1) | Operating mode selection               |
| `debug_flags`      | `uint8`  | 0       | 0-255             | Debug flags (to be determined)         |
| `pps`              | `uint16` | 10000   | 1-65535           | Points per second for galvo movement   |
| `max_buffer_index` | `uint8`  | 255     | 0-255             | Maximum valid buffer index             |
| `max_step_length`  | `uint8`  | 5       | 1-255             | Maximum step length for galvo movement |

## Parameter Details

### `mode` (uint8)

- **Default**: 0
- **Range**: 0 to (MODE_COUNT-1)
- **Description**: Selects the operating mode of the system
- **Current Modes**:
  - 0: DUAL_BUFFER mode

### `debug_flags` (uint8)

- **Default**: 0
- **Range**: 0-255 (full uint8 range)
- **Description**: Bit flags for enabling various debug features
- **Note**: Specific flag meanings to be determined later

### `pps` (uint16)

- **Default**: 10000
- **Range**: 1-65535
- **Description**: Controls the speed of galvo movement in points per second
- **Units**: Points per second

### `max_buffer_index` (uint8)

- **Default**: 255
- **Range**: 0-255
- **Description**: Maximum valid index for buffer access. With uint8 indices, this gives you 256 total buffer elements (indices 0-255)
- **Units**: Buffer index
- **Note**: This represents the maximum index, not the total count. Total buffer size = max_buffer_index + 1

### `max_step_length` (uint8)

- **Default**: 5
- **Range**: 1-255
- **Description**: Maximum step length for galvo movement, limiting how far the galvos can move in a single step
- **Units**: Arbitrary units (depends on DAC resolution)

## EEPROM Management API

The configuration system uses a dedicated EEPROM abstraction layer (`eeprom.cpp`) to manage persistent storage.

### EEPROM Constants

- `EEPROM_CONFIG_START = 0` - Starting address for configuration data
- `EEPROM_CONFIG_SIZE = 32` - Total bytes allocated for configuration (two EEPROM pages)
- `EEPROM_UNINITIALIZED_VALUE = 0xFF` - Value indicating uninitialized EEPROM

### Core EEPROM Functions

#### Configuration Operations

- `eeprom_load_config(config_data, config_size)` - Loads configuration from EEPROM
- `eeprom_save_config(config_data, config_size)` - Saves configuration to EEPROM
- `eeprom_clear_config_area()` - Clears the configuration area

#### Utility Functions

- `eeprom_is_initialized()` - Checks if EEPROM has been initialized
- `eeprom_write_bytes(address, data, length)` - Writes bytes to arbitrary EEPROM location
- `eeprom_read_bytes(address, data, length)` - Reads bytes from arbitrary EEPROM location

### Usage Example

```cpp
#include "config/eeprom.h"

// Load configuration
uint8_t config_buffer[32];
if (eeprom_load_config(config_buffer, 32)) {
  // Configuration loaded successfully
}

// Save configuration
if (eeprom_save_config(config_buffer, 32)) {
  // Configuration saved successfully
}
```

## Configuration Storage

Configuration parameters are stored in EEPROM and include:

- Magic number for validation
- Parameter count for versioning
- All parameter values
- Reserved space for future expansion
- XOR checksum for data integrity

### EEPROM Layout

The configuration uses exactly **32 bytes** (two EEPROM pages of 16 bytes each):

| Field              | Size     | Offset | Description                          |
| ------------------ | -------- | ------ | ------------------------------------ |
| `magic`            | 2 bytes  | 0-1    | Magic number (0x6A17) for validation |
| `param_count`      | 1 byte   | 2      | Number of valid parameters (version) |
| `mode`             | 1 byte   | 3      | Operating mode                       |
| `debug_flags`      | 1 byte   | 4      | Debug flags                          |
| `pps`              | 2 bytes  | 5-6    | Points per second                    |
| `max_buffer_index` | 1 byte   | 7      | Maximum buffer index                 |
| `max_step_length`  | 1 byte   | 8      | Maximum step length                  |
| `reserved`         | 22 bytes | 9-30   | Reserved for future expansion        |
| `checksum`         | 1 byte   | 31     | XOR checksum of all previous bytes   |

**Total: 32 bytes** - Fits exactly in two EEPROM pages

## Parameter Validation

All parameters are validated when set:

- `mode` must be less than `MODE_COUNT`
- `pps` must be greater than 0
- `max_buffer_index` must be between 0 and 255
- `max_step_length` must be between 1 and 255
- `debug_flags` accepts any uint8 value

## Versioning

The configuration system supports automatic version migration. When new parameters are added:

1. New parameters are added before `PARAM_COUNT`
2. Old configurations are automatically migrated
3. New parameters get default values
4. Existing parameters retain their values

## Architecture Benefits

- **Separation of Concerns**: EEPROM operations are abstracted from configuration logic
- **No Magic Numbers**: All constants are properly defined and documented
- **Type Safety**: Uses appropriate Arduino-compatible types throughout
- **Error Handling**: Proper validation and error checking at all levels
- **Maintainability**: Clean interfaces make the system easy to extend and debug

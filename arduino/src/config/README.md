# Configuration Parameters

This document describes the configuration parameters used by the Galvonium system.

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

## Configuration Storage

Configuration parameters are stored in EEPROM and include:

- Magic number for validation
- Parameter count for versioning
- All parameter values
- Reserved space for future expansion
- XOR checksum for data integrity

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

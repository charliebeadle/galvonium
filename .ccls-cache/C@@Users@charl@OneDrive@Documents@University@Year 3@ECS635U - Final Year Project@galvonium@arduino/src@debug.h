#pragma once

/*
 * ============================================================================
 * GALVONIUM DEBUG SYSTEM
 * ============================================================================
 *
 * This file provides a comprehensive debug and validation system optimized for
 * Arduino development. It includes three debug levels and input validation
 * macros designed for minimal RAM usage and fast execution.
 *
 * DEBUG LEVELS:
 * - ERROR (1): Critical errors that break functionality
 * - INFO (2): Important information, warnings, and validation messages
 * - VERBOSE (3): Detailed execution flow and performance monitoring
 *
 * USAGE:
 * 1. Set DEBUG_LEVEL in config.h (0=OFF, 1=ERROR, 2=INFO, 3=VERBOSE)
 * 2. Include this file: #include "debug.h"
 * 3. Use debug macros: DEBUG_ERROR("Critical issue");
 * 4. Use validation macros: VALIDATE_COORD8(x_coord);
 *
 * IMPORTANT WARNINGS:
 * - NEVER use Serial debug macros inside ISRs (Timer, SPI, etc.)
 * - Use DEBUG_ISR_START/END() for ISR timing instead
 * - Debug strings are stored in PROGMEM to save RAM
 * - Validation macros can be compiled out completely in production
 * - Performance monitoring adds overhead - use only in development
 *
 * MEMORY OPTIMIZATION:
 * - All debug strings use F() macro for PROGMEM storage
 * - Macros compile to nothing when debug level is OFF
 * - Zero overhead in production builds
 *
 * MACRO REFERENCE:
 *
 * BASIC DEBUG MACROS:
 *   DEBUG_ERROR(msg)              - Print error message (level 1+)
 *   DEBUG_INFO(msg)               - Print info message (level 2+)
 *   DEBUG_VERBOSE(msg)            - Print verbose message (level 3+)
 *   DEBUG_ERROR_VAL(msg, val)     - Print error with single value
 *   DEBUG_INFO_VAL(msg, val)      - Print info with single value
 *   DEBUG_VERBOSE_VAL(msg, val)   - Print verbose with single value
 *   DEBUG_ERROR_VAL2(msg, v1, v2) - Print error with two values
 *   DEBUG_INFO_VAL2(msg, v1, v2)  - Print info with two values
 *   DEBUG_VERBOSE_VAL2(msg, v1, v2) - Print verbose with two values
 *
 * HARDWARE DEBUG MACROS:
 *   DEBUG_DAC_PIN_ON/OFF()        - Control DAC debug pin
 *   DEBUG_ISR_PIN_ON/OFF()        - Control ISR timing pin
 *   DEBUG_ISR_START/END()         - ISR-safe timing macros
 *
 * VALIDATION MACROS:
 *   VALIDATE_RANGE_CLIP(val, min, max)     - Clip value to range
 *   VALIDATE_RANGE_ERROR(val, min, max)    - Return false if out of range
 *   VALIDATE_RANGE_DEFAULT(val, min, max, def) - Use default if invalid
 *   VALIDATE_POINTER(ptr, name)            - Check for null pointer
 *   VALIDATE_COORD8(val)                   - Validate 8-bit coordinate
 *   VALIDATE_PPS(val)                      - Validate PPS frequency
 *   VALIDATE_BUFFER_INDEX(idx)             - Validate buffer index
 *   VALIDATE_BUFFER_FULL(buf, name)        - Check if buffer is full
 *   VALIDATE_BUFFER_EMPTY(buf, name)       - Check if buffer is empty
 *   VALIDATE_Q12_4_COORD(point)            - Validate Q12.4 coordinate
 *   VALIDATE_STEP_SIZE(size)               - Validate step size
 *   VALIDATE_INTERP_FACTOR(factor)         - Validate interpolation factor
 *   VALIDATE_FLAGS(flags, max)             - Validate flag values
 *   VALIDATE_STATE(state, max)             - Validate state value
 *
 * PERFORMANCE MACROS:
 *   DEBUG_PERF_START()            - Start performance timing
 *   DEBUG_PERF_END(label)         - End timing and print duration
 *   DEBUG_MEM_USAGE(label)        - Print free RAM
 *
 * UTILITY MACROS:
 *   ASSERT(condition, msg)        - Assertion with halt on failure
 *   VALIDATE_IF_ENABLED(validation) - Conditional validation
 *
 * EXAMPLES:
 *   DEBUG_ERROR("System failed");
 *   DEBUG_INFO_VAL("Frequency", freq);
 *   VALIDATE_PPS(frequency);
 *   DEBUG_PERF_START(); ... code ... DEBUG_PERF_END("operation");
 *
 * ============================================================================
 */

#include "config.h"
#include <Arduino.h>

#define ENABLE_DEBUG_PINS 0
#define ENABLE_INTERPOLATION 0
#define DEBUG_LEVEL 3 // 0=silent, 1=errors, 2=info, 3=verbose

// Debug level constants
#define DEBUG_LEVEL_OFF 0
#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_INFO 2
#define DEBUG_LEVEL_VERBOSE 3

// Enhanced debug macros with file and function context
#if DEBUG_LEVEL >= 1
#define DEBUG_ERROR(msg)                                                       \
  do {                                                                         \
    Serial.print(F("[ERROR] "));                                               \
    Serial.print(__FILE__);                                                    \
    Serial.print(F(":"));                                                      \
    Serial.print(__func__);                                                    \
    Serial.print(F(" - "));                                                    \
    Serial.println(F(msg));                                                    \
  } while (0)
#else
#define DEBUG_ERROR(msg)
#endif

#if DEBUG_LEVEL >= 2
#define DEBUG_INFO(msg)                                                        \
  do {                                                                         \
    Serial.print(F("[INFO] "));                                                \
    Serial.print(__FILE__);                                                    \
    Serial.print(F(":"));                                                      \
    Serial.print(__func__);                                                    \
    Serial.print(F(" - "));                                                    \
    Serial.println(F(msg));                                                    \
  } while (0)
#else
#define DEBUG_INFO(msg)
#endif

#if DEBUG_LEVEL >= 3
#define DEBUG_VERBOSE(msg)                                                     \
  do {                                                                         \
    Serial.print(F("[VERBOSE] "));                                             \
    Serial.print(__FILE__);                                                    \
    Serial.print(F(":"));                                                      \
    Serial.print(__func__);                                                    \
    Serial.print(F(" - "));                                                    \
    Serial.println(F(msg));                                                    \
  } while (0)
#else
#define DEBUG_VERBOSE(msg)
#endif

#if ENABLE_DEBUG_PINS
#define DEBUG_DAC_PIN_ON() digitalWrite(DEBUG_DAC_PIN, HIGH)
#define DEBUG_DAC_PIN_OFF() digitalWrite(DEBUG_DAC_PIN, LOW)
#define DEBUG_ISR_PIN_ON() digitalWrite(DEBUG_ISR_PIN, HIGH)
#define DEBUG_ISR_PIN_OFF() digitalWrite(DEBUG_ISR_PIN, LOW)
#else
#define DEBUG_DAC_PIN_ON()
#define DEBUG_DAC_PIN_OFF()
#define DEBUG_ISR_PIN_ON()
#define DEBUG_ISR_PIN_OFF()
#endif

// Enhanced debug macros - optimized for Arduino (minimal RAM, fast execution)
// These extend the existing DEBUG_ERROR, DEBUG_INFO, DEBUG_VERBOSE from
// config.h

#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
#define DEBUG_ERROR_VAL(x, val)                                                \
  do {                                                                         \
    Serial.print(F("[ERROR] "));                                               \
    Serial.print(__FILE__);                                                    \
    Serial.print(F(":"));                                                      \
    Serial.print(__func__);                                                    \
    Serial.print(F(" - "));                                                    \
    Serial.print(F(x));                                                        \
    Serial.println(val);                                                       \
  } while (0)
#define DEBUG_ERROR_VAL2(x, val1, val2)                                        \
  do {                                                                         \
    Serial.print(F("[ERROR] "));                                               \
    Serial.print(__FILE__);                                                    \
    Serial.print(F(":"));                                                      \
    Serial.print(__func__);                                                    \
    Serial.print(F(" - "));                                                    \
    Serial.print(F(x));                                                        \
    Serial.print(val1);                                                        \
    Serial.print(F(" "));                                                      \
    Serial.println(val2);                                                      \
  } while (0)
#else
#define DEBUG_ERROR_VAL(x, val)
#define DEBUG_ERROR_VAL2(x, val1, val2)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
#define DEBUG_INFO_VAL(x, val)                                                 \
  do {                                                                         \
    Serial.print(F("[INFO] "));                                                \
    Serial.print(F(__FILE__));                                                 \
    Serial.print(F(":"));                                                      \
    Serial.print(F(__func__));                                                 \
    Serial.print(F(" - "));                                                    \
    Serial.print(F(x));                                                        \
    Serial.println(val);                                                       \
  } while (0)
#define DEBUG_INFO_VAL2(x, val1, val2)                                         \
  do {                                                                         \
    Serial.print(F("[INFO] "));                                                \
    Serial.print(F(__FILE__));                                                 \
    Serial.print(F(":"));                                                      \
    Serial.print(F(__func__));                                                 \
    Serial.print(F(" - "));                                                    \
    Serial.print(F(x));                                                        \
    Serial.print(val1);                                                        \
    Serial.print(F(" "));                                                      \
    Serial.println(val2);                                                      \
  } while (0)
#else
#define DEBUG_INFO_VAL(x, val)
#define DEBUG_INFO_VAL2(x, val1, val2)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
#define DEBUG_VERBOSE_VAL(x, val)                                              \
  do {                                                                         \
    Serial.print(F("[VERBOSE] "));                                             \
    Serial.print(__FILE__);                                                    \
    Serial.print(F(":"));                                                      \
    Serial.print(__func__);                                                    \
    Serial.print(F(" - "));                                                    \
    Serial.print(F(x));                                                        \
    Serial.println(val);                                                       \
  } while (0)
#define DEBUG_VERBOSE_VAL2(x, val1, val2)                                      \
  do {                                                                         \
    Serial.print(F("[VERBOSE] "));                                             \
    Serial.print(__FILE__);                                                    \
    Serial.print(F(":"));                                                      \
    Serial.print(__func__);                                                    \
    Serial.print(F(" - "));                                                    \
    Serial.print(F(x));                                                        \
    Serial.print(val1);                                                        \
    Serial.print(F(" "));                                                      \
    Serial.println(val2);                                                      \
  } while (0)
#else
#define DEBUG_VERBOSE_VAL(x, val)
#define DEBUG_VERBOSE_VAL2(x, val1, val2)
#endif

// Input validation macros - optimized for Arduino
// Use F() macro to store strings in PROGMEM to save RAM

// Basic range validation with clipping (most common case)
#define VALIDATE_RANGE_CLIP(value, min_val, max_val)                           \
  do {                                                                         \
    if ((value) < (min_val)) {                                                 \
      DEBUG_INFO_VAL2("CLIP: " #value " below min", (value), (min_val));       \
      (value) = (min_val);                                                     \
    } else if ((value) > (max_val)) {                                          \
      DEBUG_INFO_VAL2("CLIP: " #value " above max", (value), (max_val));       \
      (value) = (max_val);                                                     \
    }                                                                          \
  } while (0)

// Range validation with error return (for critical failures)
#define VALIDATE_RANGE_ERROR(value, min_val, max_val)                          \
  do {                                                                         \
    if ((value) < (min_val) || (value) > (max_val)) {                          \
      DEBUG_ERROR_VAL2("ERROR: " #value " out of range", (value), (min_val));  \
      DEBUG_ERROR_VAL("Max allowed", (max_val));                               \
      return false;                                                            \
    }                                                                          \
  } while (0)

// Range validation with default value (for non-critical failures)
#define VALIDATE_RANGE_DEFAULT(value, min_val, max_val, default_val)           \
  do {                                                                         \
    if ((value) < (min_val) || (value) > (max_val)) {                          \
      DEBUG_INFO_VAL2("DEFAULT: " #value " invalid", (value), (default_val));  \
      (value) = (default_val);                                                 \
    }                                                                          \
  } while (0)

// Pointer validation (critical for preventing crashes)
#define VALIDATE_POINTER(ptr, name)                                            \
  do {                                                                         \
    if ((ptr) == nullptr) {                                                    \
      DEBUG_ERROR("ERROR: Null pointer: " name);                               \
      return false;                                                            \
    }                                                                          \
  } while (0)

// Specialized validation macros for common types
#define VALIDATE_COORD8(value) VALIDATE_RANGE_CLIP(value, 0, 255)
#define VALIDATE_COORD8_ERROR(value) VALIDATE_RANGE_ERROR(value, 0, 255)

#define VALIDATE_PPS(value) VALIDATE_RANGE_CLIP(value, MIN_PPS, MAX_PPS)
#define VALIDATE_PPS_ERROR(value) VALIDATE_RANGE_ERROR(value, MIN_PPS, MAX_PPS)

#define VALIDATE_BUFFER_INDEX(index)                                           \
  VALIDATE_RANGE_ERROR(index, 0, MAX_BUFFER_INDEX - 1)

// Buffer state validation
#define VALIDATE_BUFFER_FULL(buffer, name)                                     \
  do {                                                                         \
    if ((buffer).is_full()) {                                                  \
      DEBUG_INFO("WARN: Buffer full: " name);                                  \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define VALIDATE_BUFFER_EMPTY(buffer, name)                                    \
  do {                                                                         \
    if ((buffer).is_empty()) {                                                 \
      DEBUG_INFO("WARN: Buffer empty: " name);                                 \
      return false;                                                            \
    }                                                                          \
  } while (0)

// Q12.4 coordinate validation (after conversion to coord8)
#define VALIDATE_Q12_4_COORD(point)                                            \
  do {                                                                         \
    uint8_t x_coord = (point).x >> 4;                                          \
    uint8_t y_coord = (point).y >> 4;                                          \
    VALIDATE_COORD8(x_coord);                                                  \
    VALIDATE_COORD8(y_coord);                                                  \
  } while (0)

// Step size validation
#define VALIDATE_STEP_SIZE(step_size)                                          \
  VALIDATE_RANGE_CLIP(step_size, MIN_STEP_SIZE, MAX_STEP_SIZE)

// Dwell time validation
#define VALIDATE_DWELL_TIME(dwell)                                             \
  VALIDATE_RANGE_CLIP(dwell, MIN_DWELL_TIME, MAX_DWELL_TIME)

// Interpolation factor validation (0-7 for bit shifts)
#define VALIDATE_INTERP_FACTOR(factor) VALIDATE_RANGE_CLIP(factor, 0, 7)

// Flag validation
#define VALIDATE_FLAGS(flags, max_flags)                                       \
  VALIDATE_RANGE_CLIP(flags, 0, max_flags)

// State validation
#define VALIDATE_STATE(state, max_state)                                       \
  VALIDATE_RANGE_ERROR(state, 0, max_state)

// Performance monitoring macros (only in verbose mode)
#if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
#define DEBUG_PERF_START() uint32_t _perf_start = micros()
#define DEBUG_PERF_END(label)                                                  \
  do {                                                                         \
    uint32_t _perf_end = micros();                                             \
    DEBUG_VERBOSE_VAL2(label " took", (_perf_end - _perf_start), "us");        \
  } while (0)
#else
#define DEBUG_PERF_START()
#define DEBUG_PERF_END(label)
#endif

// Memory usage monitoring (only in verbose mode)
#if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
#define DEBUG_MEM_USAGE(label)                                                 \
  do {                                                                         \
    DEBUG_VERBOSE_VAL2(label " free RAM", freeMemory(), "bytes");              \
  } while (0)
#else
#define DEBUG_MEM_USAGE(label)
#endif

// Helper function for free memory (if not already defined)
#if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
extern "C" char *sbrk(int i);
inline int freeMemory() {
  char top;
  return &top - reinterpret_cast<char *>(sbrk(0));
}
#endif

// ISR error flag system - no Serial calls in ISR
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
extern volatile bool isr_error_flag;
extern volatile uint8_t isr_error_code;
#define ISR_ERROR_NULL_POINTER 1
#define ISR_ERROR_INVALID_DATA 2
#define ISR_ERROR_BUFFER_EMPTY 3

#define DEBUG_ISR_ERROR(code)                                                  \
  do {                                                                         \
    isr_error_flag = true;                                                     \
    isr_error_code = (code);                                                   \
  } while (0)

#define CHECK_ISR_ERRORS()                                                     \
  do {                                                                         \
    if (isr_error_flag) {                                                      \
      switch (isr_error_code) {                                                \
      case ISR_ERROR_NULL_POINTER:                                             \
        DEBUG_ERROR("ISR encountered null pointer");                           \
        break;                                                                 \
      case ISR_ERROR_INVALID_DATA:                                             \
        DEBUG_ERROR("ISR encountered invalid data");                           \
        break;                                                                 \
      case ISR_ERROR_BUFFER_EMPTY:                                             \
        DEBUG_ERROR("ISR encountered empty buffer");                           \
        break;                                                                 \
      default:                                                                 \
        DEBUG_ERROR_VAL("ISR unknown error code: ", isr_error_code);           \
        break;                                                                 \
      }                                                                        \
      isr_error_flag = false;                                                  \
      isr_error_code = 0;                                                      \
    }                                                                          \
  } while (0)
#else
#define DEBUG_ISR_ERROR(code)
#define CHECK_ISR_ERRORS()
#endif

// ISR-safe debug macros (minimal, no Serial calls in ISR)
#if ENABLE_DEBUG_PINS
#define DEBUG_ISR_START() DEBUG_ISR_PIN_ON()
#define DEBUG_ISR_END() DEBUG_ISR_PIN_OFF()
#else
#define DEBUG_ISR_START()
#define DEBUG_ISR_END()
#endif

// Conditional compilation for validation
#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
#define ENABLE_VALIDATION 1
#else
#define ENABLE_VALIDATION 0
#endif

// Conditional validation macros (can be compiled out completely)
#if ENABLE_VALIDATION
#define VALIDATE_IF_ENABLED(validation) validation
#else
#define VALIDATE_IF_ENABLED(validation)
#endif

// Assertion macro (only in debug builds)
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition)) {                                                        \
      DEBUG_ERROR("ASSERTION FAILED: " message);                               \
      while (1) { /* halt */                                                   \
      }                                                                        \
    }                                                                          \
  } while (0)
#else
#define ASSERT(condition, message)
#endif

// Usage examples in comments:
/*
// Basic usage:
VALIDATE_COORD8(x_coord);
VALIDATE_PPS(frequency);
VALIDATE_POINTER(transition, "transition");

// With error handling:
if (!VALIDATE_PPS_ERROR(frequency)) {
  return false;
}

// Performance monitoring:
DEBUG_PERF_START();
// ... some code ...
DEBUG_PERF_END("interpolation");

// Memory monitoring:
DEBUG_MEM_USAGE("After buffer allocation");

// Assertions:
ASSERT(buffer != nullptr, "Buffer must not be null");
*/

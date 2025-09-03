#pragma once

/*
 * ============================================================================
 * GALVONIUM CONFIGURATION
 * ============================================================================
 *
 * This file contains all system configuration parameters, limits, and defaults
 * for the Galvonium laser galvo control system. Parameters are organized by
 * functional area with clear min/max/default values for validation.
 *
 * ============================================================================
 */

// ============================================================================
// SYSTEM CONSTANTS
// ============================================================================

// Clock and timing
#define CLOCK_FREQ 16000000 // Arduino Uno clock frequency (Hz)
#define TIMER_PRESCALER 1   // Timer prescaler (no prescaling)

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================

// Pin assignments
#define LASER_PIN 9     // Laser control pin
#define DEBUG_ISR_PIN 3 // ISR timing debug pin
#define DEBUG_DAC_PIN 4 // DAC output debug pin

// SPI configuration
#define SPI_SPEED 20000000 // SPI clock speed (Hz)
#define SPI_MODE SPI_MODE0 // SPI mode
#define SPI_ORDER MSBFIRST // SPI bit order

// DAC configuration
#define DAC_FLAGS_A 0b00010000 // DAC channel A flags
#define DAC_FLAGS_B 0b10010000 // DAC channel B flags
#define DAC_RESOLUTION 12      // DAC resolution (bits)
#define DAC_MAX_VALUE 4095     // Maximum DAC output value

// ============================================================================
// COORDINATE SYSTEM LIMITS
// ============================================================================

// 8-bit coordinate system (0-255)
#define COORD8_MIN 0       // Minimum 8-bit coordinate
#define COORD8_MAX 255     // Maximum 8-bit coordinate
#define COORD8_DEFAULT 128 // Default 8-bit coordinate (center)

// Q12.4 fixed-point coordinate system
#define Q12_4_MIN -2048         // Minimum Q12.4 value (-128.0)
#define Q12_4_MAX 2047          // Maximum Q12.4 value (127.9375)
#define Q12_4_DEFAULT 0         // Default Q12.4 value (0.0)
#define Q12_4_FRACTIONAL_BITS 4 // Number of fractional bits
#define Q12_4_SCALE_FACTOR 16   // Scale factor (2^4)

// ILDA coordinate system (-32768 to 32767)
#define ILDA_MIN -32768 // Minimum ILDA coordinate
#define ILDA_MAX 32767  // Maximum ILDA coordinate
#define ILDA_DEFAULT 0  // Default ILDA coordinate

// ============================================================================
// BUFFER CONFIGURATION
// ============================================================================

// Main point buffer
#define MAX_BUFFER_INDEX 64               // Maximum buffer index
#define MAX_POINTS (MAX_BUFFER_INDEX - 1) // Maximum points per buffer
#define MIN_POINTS 1                      // Minimum points per buffer
#define DEFAULT_POINTS 64                 // Default points per buffer

// Step ring buffer (hardcoded to 16 for bitwise operations)
#define STEP_RING_BUFFER_SIZE 16 // Step ring buffer size (must be 2^n)
#define STEP_RING_BUFFER_MASK 15 // Bit mask for modulo operations (size-1)
#define MIN_STEP_BUFFER_SIZE 1   // Minimum step buffer size
#define MAX_STEP_BUFFER_SIZE 16  // Maximum step buffer size

// ============================================================================
// TIMING AND FREQUENCY LIMITS
// ============================================================================

// Points per second (PPS) limits
#define MIN_PPS 1         // Minimum PPS frequency
#define MAX_PPS 65535     // Maximum PPS frequency (16-bit timer limit)
#define DEFAULT_PPS 10000 // Default PPS frequency
#define DEBUG_PPS 100     // Debug PPS frequency (slow for testing)

// Laser dwell times (microseconds)
#define LASER_ON_DWELL_TIME 10  // Laser on dwell time
#define LASER_OFF_DWELL_TIME 10 // Laser off dwell time
#define MIN_DWELL_TIME 1        // Minimum dwell time
#define MAX_DWELL_TIME 255      // Maximum dwell time
#define DEFAULT_DWELL_TIME 10   // Default dwell time

// ============================================================================
// INTERPOLATION PARAMETERS
// ============================================================================

// Step size limits
#define MIN_STEP_SIZE 1     // Minimum interpolation step size
#define MAX_STEP_SIZE 50    // Maximum interpolation step size
#define DEFAULT_STEP_SIZE 4 // Default interpolation step size

// Acceleration/deceleration factors (0-7 for bit shifts)
#define MIN_ACC_FACTOR 0     // Minimum acceleration factor
#define MAX_ACC_FACTOR 7     // Maximum acceleration factor
#define DEFAULT_ACC_FACTOR 0 // Default acceleration factor

#define MIN_DEC_FACTOR 0     // Minimum deceleration factor
#define MAX_DEC_FACTOR 7     // Maximum deceleration factor
#define DEFAULT_DEC_FACTOR 0 // Default deceleration factor

// ============================================================================
// SERIAL COMMUNICATION
// ============================================================================

// Baud rate limits
#define MIN_BAUD_RATE 300             // Minimum baud rate
#define MAX_BAUD_RATE 115200          // Maximum baud rate
#define DEFAULT_BAUD_RATE 9600        // Default baud rate
#define SERIAL_BAUD DEFAULT_BAUD_RATE // Alias for compatibility

// ============================================================================
// SYSTEM LIMITS AND VALIDATION
// ============================================================================

// Performance limits
#define MAX_FRAME_RATE 60     // Maximum frame rate (FPS)
#define MIN_FRAME_RATE 1      // Minimum frame rate (FPS)
#define DEFAULT_FRAME_RATE 25 // Default frame rate (FPS)

// Memory limits
#define MAX_MEMORY_USAGE 2048 // Maximum expected memory usage (bytes)
#define MIN_FREE_MEMORY 100   // Minimum free memory threshold (bytes)

// ============================================================================
// LEGACY COMPATIBILITY
// ============================================================================

// Legacy parameter aliases (for backward compatibility)
#define MAX_STEP_LENGTH MAX_STEP_SIZE // Alias for step size

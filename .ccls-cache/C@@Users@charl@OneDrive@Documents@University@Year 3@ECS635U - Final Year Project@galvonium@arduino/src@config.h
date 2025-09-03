#pragma once

// Constants
#define CLOCK_FREQ 16000000
#define SPI_SPEED 20000000
#define DAC_FLAGS_A 0b00010000
#define DAC_FLAGS_B 0b10010000

// Feature flags
#define ENABLE_DEBUG_PINS 0
#define ENABLE_INTERPOLATION 0
#define DEBUG_LEVEL 3 // 0=silent, 1=errors, 2=info, 3=verbose

// Hardware configuration
#define LASER_PIN 9
#define DEBUG_ISR_PIN 3
#define DEBUG_DAC_PIN 4

// System parameters
#define MAX_BUFFER_INDEX 128
#define MAX_STEP_LENGTH 4
#define MIN_PPS 1
#define DEBUG_PPS 100
#define DEFAULT_PPS 10000
#define MAX_PPS 65535

#define SERIAL_BAUD 9600

// Debug macros

#if DEBUG_LEVEL >= 1
#define DEBUG_ERROR(x, ...) Serial.println(x)
#else
#define DEBUG_ERROR(x, ...)
#endif

#if DEBUG_LEVEL >= 2
#define DEBUG_INFO(x, ...) Serial.println(x)
#else
#define DEBUG_INFO(x, ...)
#endif

#if DEBUG_LEVEL >= 3
#define DEBUG_VERBOSE(x, ...) Serial.println(x)
#else
#define DEBUG_VERBOSE(x, ...)
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
#pragma once
#include "../config.h"
#include "../types.h"
#include <Arduino.h>

// Forward declaration for callback
typedef void (*timer_callback_t)(void);

// Forward declaration for data source callback
typedef bool (*data_source_callback_t)(void *point, void *laser_state);

// Forward declaration for hardware output callback
typedef void (*hardware_output_callback_t)(void *point, void *laser_state);

// Forward declaration for Timer class
class Timer;

// Global timer instance for ISR access
extern Timer *g_timer_instance;

class Timer {
public:
  Timer();
  void init();
  void setFrequency(uint32_t frequency);
  void enable();
  void disable();
  uint32_t getFrequency() const;
  void setCallback(timer_callback_t callback);
  void setDataSource(data_source_callback_t data_source);
  void setHardwareOutput(hardware_output_callback_t hardware_output);
  timer_callback_t getCallback() const { return callback; }
  data_source_callback_t getDataSource() const { return data_source; }
  hardware_output_callback_t getHardwareOutput() const {
    return hardware_output;
  }

private:
  uint32_t frequency;
  bool enabled;
  timer_callback_t callback;
  data_source_callback_t data_source;
  hardware_output_callback_t hardware_output;
};

// Define the global timer instance
Timer *g_timer_instance = nullptr;

Timer::Timer() {
  frequency = DEFAULT_PPS;
  enabled = false;
  callback = nullptr;
  data_source = nullptr;
  hardware_output = nullptr;
  g_timer_instance = this;
}

void Timer::init() {
  DEBUG_INFO("Timer initialization starting");
  
  cli();

  // Clear Timer/Counter Control Registers
  TCCR1A = 0;
  TCCR1B = 0;

  // Set CTC mode (Clear Timer on Compare Match)
  TCCR1B |= (1 << WGM12);

  // No prescaling for higher precision
  TCCR1B |= (1 << CS10);

  DEBUG_VERBOSE_VAL("Setting default frequency: ", frequency);
  setFrequency(frequency);

  enable();

  sei();

  DEBUG_INFO("Timer initialization complete");
}

void Timer::setFrequency(uint32_t frequency) {
  DEBUG_VERBOSE_VAL("setFrequency called with: ", frequency);
  
  if (frequency < MIN_PPS || frequency > MAX_PPS) {
    DEBUG_ERROR_VAL2("Invalid frequency out of range: ", frequency, " Valid range: ");
    DEBUG_ERROR_VAL2("Min: ", MIN_PPS, " Max: ");
    DEBUG_ERROR_VAL("", MAX_PPS);
    return;
  }

  this->frequency = frequency;
  uint32_t ocr_value = (CLOCK_FREQ / frequency) - 1;
  
  if (ocr_value > 65535) {
    DEBUG_ERROR_VAL("OCR value overflow: ", ocr_value);
    return;
  }
  
  OCR1A = ocr_value;
  DEBUG_INFO_VAL("Timer frequency set to: ", frequency);
  DEBUG_VERBOSE_VAL("OCR1A value: ", ocr_value);
}

void Timer::enable() {
  TIMSK1 |= (1 << OCIE1A);
  enabled = true;
  DEBUG_INFO(F("Timer enabled"));
}

void Timer::disable() {
  TIMSK1 &= ~(1 << OCIE1A);
  enabled = false;
  DEBUG_INFO(F("Timer disabled"));
}

uint32_t Timer::getFrequency() const { return frequency; }

void Timer::setCallback(timer_callback_t callback) {
  this->callback = callback;
}

void Timer::setDataSource(data_source_callback_t data_source) {
  VALIDATE_POINTER(data_source, "data_source");
  this->data_source = data_source;
  DEBUG_INFO("Timer data source configured");
}

void Timer::setHardwareOutput(hardware_output_callback_t hardware_output) {
  VALIDATE_POINTER(hardware_output, "hardware_output");
  this->hardware_output = hardware_output;
  DEBUG_INFO("Timer hardware output configured");
}

ISR(TIMER1_COMPA_vect) {
  DEBUG_ISR_PIN_ON();

  if (g_timer_instance && g_timer_instance->getDataSource() &&
      g_timer_instance->getHardwareOutput()) {
    // Get data from the registered data source
    point_q12_4_t point;
    bool laser_state;

    if (g_timer_instance->getDataSource()(&point, &laser_state)) {
      // Output the data using the hardware output callback
      g_timer_instance->getHardwareOutput()(&point, &laser_state);
    }
  }

  DEBUG_ISR_PIN_OFF();
}

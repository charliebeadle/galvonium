#pragma once
#include "../config.h"
#include "dac.h"
#include "eeprom.h"
#include "laser.h"
#include "serialio.h"
#include "timer.h"
#include <Arduino.h>

// Forward declaration for data source callback
typedef bool (*data_source_callback_t)(void *point, void *laser_state);

// Forward declaration for hardware output callback
typedef void (*hardware_output_callback_t)(void *point, void *laser_state);

namespace Hardware {

// Hardware context class to encapsulate all hardware components
class HardwareContext {
public:
  // Hardware component instances
  SerialIO serial;
  DAC dac;
  Timer timer;
  Laser laser;

  // Shared state variables
  point_q12_4_t point;
  bool laser_state;

  // Constructor
  HardwareContext() : point(0, 0), laser_state(false) {}

  // Initialization and shutdown methods
  void init();
  void shutdown();

  // Hardware output function for timer ISR
  void hardware_output(void *point, void *laser_state);

  // Set data source for timer ISR
  void setDataSource(void *data_source_func);
};

// Global context instance
HardwareContext context;

// Convenience functions to maintain existing interface
inline void init() { context.init(); }
inline void shutdown() { context.shutdown(); }
inline void setDataSource(void *data_source_func) {
  context.setDataSource(data_source_func);
}

// Convenience accessors for hardware components
inline SerialIO &serial() { return context.serial; }
inline DAC &dac() { return context.dac; }
inline Timer &timer() { return context.timer; }
inline Laser &laser() { return context.laser; }

// HardwareContext method implementations
void HardwareContext::init() {
  serial.init(9600);
  dac.init();
  timer.init();
  laser.init();
  // Set up hardware output callback for timer
  timer.setHardwareOutput([](void *point, void *laser_state) {
    context.hardware_output(point, laser_state);
  });
}

void HardwareContext::shutdown() {
  timer.disable();
  laser.set_laser(false);
  DEBUG_INFO(F("Hardware shutdown complete"));
}

void HardwareContext::setDataSource(void *data_source_func) {
  timer.setDataSource((data_source_callback_t)data_source_func);
}

void HardwareContext::hardware_output(void *point, void *laser_state) {
  dac.output_point((point_q12_4_t *)point);
  laser.set_laser(*(bool *)laser_state);
}

} // namespace Hardware
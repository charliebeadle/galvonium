
#include "buffer.h"
#include "serial_cmd.h"
#include <Arduino.h>
#include <SPI.h>

// Timer configuration
#define PPS 1000 // Points per second - adjust as needed
#define TIMER_PRESCALER 8
#define CLOCK_FREQ 16000000

// DAC configuration
#define DAC_FLAGS_A 0b00010000
#define DAC_FLAGS_B 0b10010000
#define LASER_PIN 3 // Pin for laser control

// State variables
volatile bool readyForFrame = true;
volatile bool swapRequested = false;
volatile int currentStep = 0;

void initTimer() {
  // Calculate the timer compare value based on the desired PPS
  int compareValue = (CLOCK_FREQ / (2 * TIMER_PRESCALER * PPS)) - 1;

  // Configure Timer1
  TCCR1A = 0; // Clear Timer/Counter Control Registers
  TCCR1B = 0;

  // Set CTC mode (Clear Timer on Compare Match)
  TCCR1B |= (1 << WGM12);

  // Set prescaler to 8
  TCCR1B |= (1 << CS11);

  // Set compare value for the desired PPS
  OCR1A = compareValue;

  // Enable Timer1 compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  // Initialize SPI and DAC control
  DDRB |= (1 << PB2);   // Set CS pin as output
  PORTB &= ~(1 << PB2); // Set CS low initially
  SPI.begin();
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  // Initialize laser pin
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  // Initialize state
  readyForFrame = true;
  swapRequested = false;
  currentStep = 0;
}

// Function to output DAC values for X and Y coordinates
void outputDAC(uint8_t x, uint8_t y) {
  uint16_t packetX = DAC_FLAGS_A << 8 | x << 4;
  uint16_t packetY = DAC_FLAGS_B << 8 | y << 4;

  PORTB &= ~(1 << PB2); // Set CS low
  SPI.transfer16(packetX);
  PORTB |= (1 << PB2); // Set CS high

  PORTB &= ~(1 << PB2); // Set CS low
  SPI.transfer16(packetY);
  PORTB |= (1 << PB2); // Set CS high
}

// Function to request buffer swap (called from serial command handler)
void requestBufferSwap() {
  if (readyForFrame) {
    swapRequested = true;
  }
}

ISR(TIMER1_COMPA_vect) {

  if (currentStep >= buffer_active_steps) {
    readyForFrame = true;
    currentStep = 0;
  }

  if (swapRequested && readyForFrame) {
    buffer_swap();
    swapRequested = false;
    readyForFrame = false;
    return;
  }

  // Output current step data
  uint8_t x = buffer_active[currentStep].x;
  uint8_t y = buffer_active[currentStep].y;
  uint8_t flags = buffer_active[currentStep].flags;

  // Extract laser state from flags (assuming bit 0 is laser on/off)
  bool laserOn = (flags & 0x01) != 0;

  // Output to DAC and control laser
  outputDAC(x, y);
  digitalWrite(LASER_PIN, laserOn ? HIGH : LOW);

  currentStep++;
}
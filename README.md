# Galvonium

PC-Arduino laser galvanometer projector control system for real-time vector graphics display.

## Overview

Galvonium is a hybrid control system that coordinates between a PC and Arduino Uno to drive laser galvanometer mirrors via an AT15MINI driver. The system uses a progressive enhancement approach where computational work gradually migrates from PC to Arduino as optimizations are proven.

## Features

- 10,000 points/second vector display capability
- Dual-buffer system for smooth animations
- Real-time parameter adjustment for calibration
- Text-based command protocol (binary protocol planned)
- ISR-driven DAC output for precise timing
- Progressive PC-to-Arduino optimization pipeline

## Hardware Requirements

- Arduino Uno R3 (or compatible)
- AT15MINI galvo driver with XY galvanometers
- 12-bit DAC for galvo control
- Laser module with TTL control
- USB connection to PC

## Software Architecture

### Arduino Component
- Interrupt-driven DAC output at 10kHz
- Dual frame buffer management
- Simple command interpreter
- PROGMEM-optimized for 2KB RAM limit

### Python Component  
- Auto-detecting serial communication
- High-level drawing commands
- Buffer visualization and editing
- Test pattern generation
- Timing calibration tools

## Getting Started

1. **Arduino Setup:**
   ```bash
   cd arduino
   pio run -t upload
   ```

2. **Python Setup:**
   ```bash
   cd python
   pip install -r requirements.txt
   python laser_client.py
   ```

## Development Status

Currently implementing:
- [x] Basic command system with parameter control
- [x] PC-Arduino communication protocol
- [ ] Dual buffer system
- [ ] ISR-based DAC output
- [ ] Python visualization tools
- [ ] Test pattern generators

## Project Structure

```
galvonium/
├── arduino/          # Arduino firmware (PlatformIO)
├── python/           # PC control software
├── docs/             # Documentation and progress reports
└── tests/            # Test patterns and calibration
```

## License

[Your chosen license]

## Author

[Your name/handle]
# Galvonium

PC-Arduino interface for laser galvanometer control.

## Overview

Dual-buffer vector graphics system using Arduino Uno and AT15MINI galvo driver. PC handles complex calculations while Arduino manages timing-critical DAC output.

## Status

Working:

- Dual-buffer system with atomic swapping
- Serial command interface
- Python serial monitor
- PROGMEM-optimized firmware (~1KB RAM usage)

Next:

- ISR-driven 10kHz DAC output
- SPI communication
- Timing parameters
- Test pattern generation

## Hardware

- Arduino Uno R3
- AT15MINI galvo driver
- 12-bit SPI DAC
- TTL laser module

## Commands

```
WRITE idx x y flags [ACTIVE|INACTIVE]  - Write step to buffer
CLEAR [ACTIVE|INACTIVE]                - Clear buffer
SWAP                                   - Swap buffers
DUMP [ACTIVE|INACTIVE]                 - Display buffer contents
SIZE n [ACTIVE|INACTIVE]               - Set step count
HELP                                   - Show commands
```

Default target: inactive buffer. Active buffer modifications require explicit flag.

## Setup

Arduino:

```bash
cd arduino
pio run -t upload
```

Python:

```bash
cd python
pip install -r requirements.txt
python serial_monitor.py  # Set PORT variable first
```

## Structure

```
galvonium/
├── arduino/
│   ├── src/
│   │   ├── main.cpp
│   │   ├── buffer.cpp/h
│   │   └── serial_cmd.cpp/h
│   └── platformio.ini
├── python/
│   ├── serial_monitor.py
│   └── requirements.txt
└── docs/
```

## Technical Details

- 256 steps per buffer, 3 bytes per step (x, y, flags)
- Atomic buffer swapping with interrupt protection
- Serial: 9600 baud, 32-byte buffer
- Switch-based command dispatch
- All strings in PROGMEM

## Repository

github.com/charliebeadle/galvonium

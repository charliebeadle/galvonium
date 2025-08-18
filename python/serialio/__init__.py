"""
Serial I/O module for Galvonium Laser Control System.

Provides serial communication capabilities for interfacing with Arduino-based
galvo control hardware. Includes command generation, parsing, and connection management.

Main Components:
- SerialConnection: Manages serial port connections
- Commands: Command generation and formatting
- Parser: Response parsing and validation

Usage:
    from serialio import SerialConnection, cmd_write, cmd_dump
    from serialio import list_serial_ports, is_eoc
"""

from .connection import SerialConnection, list_serial_ports
from .commands import (
    cmd_write,
    cmd_dump,
    cmd_swap,
    cmd_clear,
    cmd_size,
    build_write_sequence_from_buffer,
)
from .parser import is_eoc, accumulate_dump_lines, parse_dump_text

__all__ = [
    "SerialConnection",
    "list_serial_ports",
    "cmd_write",
    "cmd_dump",
    "cmd_swap",
    "cmd_clear",
    "cmd_size",
    "build_write_sequence_from_buffer",
    "is_eoc",
    "accumulate_dump_lines",
    "parse_dump_text",
]

# Version information
__version__ = "1.0.0"

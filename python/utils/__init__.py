"""
Utility functions for Galvonium Laser Control System.

Provides validation, formatting, and utility functions used throughout the system
for data validation, binary operations, and common operations.

Main Components:
- Validators: Input validation for coordinates, ports, and data
- Binary utilities: Binary string formatting and parsing

Usage:
    from utils import validate_coordinate, format_binary
    from utils import validate_port_name, validate_baud_rate
"""

from .validators import (
    validate_uint8,
    validate_buffer_index,
    validate_coordinate,
    format_binary,
    parse_binary_string,
    validate_port_name,
    validate_baud_rate,
)

__all__ = [
    "validate_uint8",
    "validate_buffer_index",
    "validate_coordinate",
    "format_binary",
    "parse_binary_string",
    "validate_port_name",
    "validate_baud_rate",
]

# Version information
__version__ = "1.0.0"

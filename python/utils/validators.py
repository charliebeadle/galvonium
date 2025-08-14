"""
Input validation utilities for the laser galvo GUI.
"""

from typing import Tuple, Optional


def validate_uint8(value: any) -> Tuple[bool, Optional[int], str]:
    """
    Validate that a value is a valid 8-bit unsigned integer.

    Args:
        value: Value to validate (string, int, or other)

    Returns:
        Tuple of (is_valid, parsed_value, error_message)
        If valid: (True, parsed_int, "")
        If invalid: (False, None, error_description)
    """
    # Handle string input
    if isinstance(value, str):
        value = value.strip()

        # Check for binary format (0b prefix)
        if value.startswith("0b") or value.startswith("0B"):
            try:
                parsed = int(value, 2)
            except ValueError:
                return False, None, f"Invalid binary format: {value}"
        # Check for hex format (0x prefix)
        elif value.startswith("0x") or value.startswith("0X"):
            try:
                parsed = int(value, 16)
            except ValueError:
                return False, None, f"Invalid hex format: {value}"
        # Regular decimal
        else:
            try:
                parsed = int(value)
            except ValueError:
                return False, None, f"Not a valid integer: {value}"

    # Handle direct int input
    elif isinstance(value, int):
        parsed = value

    # Handle float (truncate if whole number)
    elif isinstance(value, float):
        if value.is_integer():
            parsed = int(value)
        else:
            return False, None, f"Must be a whole number, got: {value}"

    # Unsupported type
    else:
        return False, None, f"Unsupported type: {type(value).__name__}"

    # Check range
    if not 0 <= parsed <= 255:
        return False, None, f"Value {parsed} out of range (0-255)"

    return True, parsed, ""


def validate_buffer_index(value: any) -> Tuple[bool, Optional[int], str]:
    """
    Validate buffer index (same as uint8 but with clearer error message).

    Args:
        value: Value to validate

    Returns:
        Tuple of (is_valid, parsed_value, error_message)
    """
    valid, parsed, error = validate_uint8(value)
    if not valid and "out of range" in error:
        return False, None, f"Buffer index must be 0-255, got: {value}"
    return valid, parsed, error


def validate_coordinate(
    value: any, axis: str = "coordinate"
) -> Tuple[bool, Optional[int], str]:
    """
    Validate a coordinate value (X or Y).

    Args:
        value: Value to validate
        axis: Axis name for error messages ("X" or "Y")

    Returns:
        Tuple of (is_valid, parsed_value, error_message)
    """
    valid, parsed, error = validate_uint8(value)
    if not valid:
        if "out of range" in error:
            return False, None, f"{axis} coordinate must be 0-255"
        else:
            return False, None, f"Invalid {axis} value: {error}"
    return valid, parsed, error


def format_binary(value: int, bits: int = 8) -> str:
    """
    Format an integer as binary string with leading zeros.

    Args:
        value: Integer value to format
        bits: Number of bits to display

    Returns:
        Binary string like "0b11110000"
    """
    if not isinstance(value, int):
        raise TypeError(f"Expected int, got {type(value).__name__}")

    if value < 0:
        raise ValueError(f"Cannot format negative value: {value}")

    if value >= (1 << bits):
        raise ValueError(f"Value {value} too large for {bits} bits")

    return f"0b{format(value, f'0{bits}b')}"


def parse_binary_string(binary_str: str) -> Optional[int]:
    """
    Parse a binary string (with or without 0b prefix) to integer.

    Args:
        binary_str: Binary string like "11110000" or "0b11110000"

    Returns:
        Parsed integer value, or None if invalid
    """
    binary_str = binary_str.strip()

    # Remove 0b prefix if present
    if binary_str.startswith("0b") or binary_str.startswith("0B"):
        binary_str = binary_str[2:]

    # Check if valid binary
    if not all(c in "01" for c in binary_str):
        return None

    # Empty string
    if not binary_str:
        return None

    try:
        return int(binary_str, 2)
    except ValueError:
        return None


def validate_port_name(port: str) -> bool:
    """
    Validate serial port name.

    Args:
        port: Port name to validate

    Returns:
        True if valid port name format
    """
    if not port:
        return False

    # Windows COM ports
    if port.upper().startswith("COM"):
        try:
            port_num = int(port[3:])
            return port_num > 0
        except ValueError:
            return False

    # Unix-like ports
    if port.startswith("/dev/"):
        return len(port) > 5

    # Allow any non-empty string for unknown systems
    return True


def validate_baud_rate(baud: any) -> Tuple[bool, Optional[int], str]:
    """
    Validate serial baud rate.

    Args:
        baud: Baud rate to validate

    Returns:
        Tuple of (is_valid, parsed_value, error_message)
    """
    standard_rates = [
        300,
        1200,
        2400,
        4800,
        9600,
        19200,
        38400,
        57600,
        115200,
        230400,
        460800,
        921600,
    ]

    try:
        if isinstance(baud, str):
            baud = int(baud)
        elif not isinstance(baud, int):
            return False, None, f"Baud rate must be an integer"

        if baud <= 0:
            return False, None, f"Baud rate must be positive"

        # Warn if non-standard but allow it
        if baud not in standard_rates:
            # Still valid, just non-standard
            pass

        return True, baud, ""

    except ValueError:
        return False, None, f"Invalid baud rate: {baud}"

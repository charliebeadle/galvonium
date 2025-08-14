"""
Unit tests for validators module.
Run with: python -m unittest test_validators
"""

import unittest
import sys
from pathlib import Path

# Add parent directory to path if needed
sys.path.insert(0, str(Path(__file__).parent))

from utils.validators import (
    validate_uint8,
    validate_buffer_index,
    validate_coordinate,
    format_binary,
    parse_binary_string,
    validate_port_name,
    validate_baud_rate,
)


class TestValidateUint8(unittest.TestCase):
    """Test cases for validate_uint8 function."""

    def test_valid_decimal(self):
        """Test valid decimal inputs."""
        valid, value, error = validate_uint8("128")
        self.assertTrue(valid)
        self.assertEqual(value, 128)
        self.assertEqual(error, "")

        valid, value, error = validate_uint8("0")
        self.assertTrue(valid)
        self.assertEqual(value, 0)

        valid, value, error = validate_uint8("255")
        self.assertTrue(valid)
        self.assertEqual(value, 255)

    def test_valid_hex(self):
        """Test valid hexadecimal inputs."""
        valid, value, error = validate_uint8("0xFF")
        self.assertTrue(valid)
        self.assertEqual(value, 255)

        valid, value, error = validate_uint8("0x00")
        self.assertTrue(valid)
        self.assertEqual(value, 0)

        valid, value, error = validate_uint8("0x80")
        self.assertTrue(valid)
        self.assertEqual(value, 128)

        # Case insensitive
        valid, value, error = validate_uint8("0XFF")
        self.assertTrue(valid)
        self.assertEqual(value, 255)

    def test_valid_binary(self):
        """Test valid binary inputs."""
        valid, value, error = validate_uint8("0b11111111")
        self.assertTrue(valid)
        self.assertEqual(value, 255)

        valid, value, error = validate_uint8("0b00000000")
        self.assertTrue(valid)
        self.assertEqual(value, 0)

        valid, value, error = validate_uint8("0b10101010")
        self.assertTrue(valid)
        self.assertEqual(value, 170)

        # Case insensitive
        valid, value, error = validate_uint8("0B11110000")
        self.assertTrue(valid)
        self.assertEqual(value, 240)

    def test_valid_int_input(self):
        """Test direct integer input."""
        valid, value, error = validate_uint8(128)
        self.assertTrue(valid)
        self.assertEqual(value, 128)

        valid, value, error = validate_uint8(0)
        self.assertTrue(valid)
        self.assertEqual(value, 0)

        valid, value, error = validate_uint8(255)
        self.assertTrue(valid)
        self.assertEqual(value, 255)

    def test_valid_float_input(self):
        """Test float inputs (whole numbers only)."""
        valid, value, error = validate_uint8(128.0)
        self.assertTrue(valid)
        self.assertEqual(value, 128)

        # Non-whole number should fail
        valid, value, error = validate_uint8(128.5)
        self.assertFalse(valid)
        self.assertIn("whole number", error)

    def test_out_of_range(self):
        """Test out of range values."""
        valid, value, error = validate_uint8("256")
        self.assertFalse(valid)
        self.assertIn("out of range", error)

        valid, value, error = validate_uint8("-1")
        self.assertFalse(valid)
        self.assertIn("out of range", error)

        valid, value, error = validate_uint8(1000)
        self.assertFalse(valid)
        self.assertIn("out of range", error)

    def test_invalid_format(self):
        """Test invalid input formats."""
        valid, value, error = validate_uint8("abc")
        self.assertFalse(valid)
        self.assertIn("Not a valid integer", error)

        valid, value, error = validate_uint8("0xGG")
        self.assertFalse(valid)
        self.assertIn("Invalid hex", error)

        valid, value, error = validate_uint8("0b222")
        self.assertFalse(valid)
        self.assertIn("Invalid binary", error)

    def test_whitespace_handling(self):
        """Test that whitespace is properly handled."""
        valid, value, error = validate_uint8("  128  ")
        self.assertTrue(valid)
        self.assertEqual(value, 128)

        valid, value, error = validate_uint8("  0xFF  ")
        self.assertTrue(valid)
        self.assertEqual(value, 255)


class TestValidateBufferIndex(unittest.TestCase):
    """Test cases for validate_buffer_index function."""

    def test_valid_indices(self):
        """Test valid buffer indices."""
        valid, value, error = validate_buffer_index(0)
        self.assertTrue(valid)
        self.assertEqual(value, 0)

        valid, value, error = validate_buffer_index(255)
        self.assertTrue(valid)
        self.assertEqual(value, 255)

        valid, value, error = validate_buffer_index("128")
        self.assertTrue(valid)
        self.assertEqual(value, 128)

    def test_invalid_indices(self):
        """Test invalid buffer indices with specific error message."""
        valid, value, error = validate_buffer_index(256)
        self.assertFalse(valid)
        self.assertIn("Buffer index must be 0-255", error)

        valid, value, error = validate_buffer_index(-1)
        self.assertFalse(valid)
        self.assertIn("Buffer index must be 0-255", error)


class TestValidateCoordinate(unittest.TestCase):
    """Test cases for validate_coordinate function."""

    def test_valid_coordinates(self):
        """Test valid coordinate values."""
        valid, value, error = validate_coordinate(128, "X")
        self.assertTrue(valid)
        self.assertEqual(value, 128)

        valid, value, error = validate_coordinate("255", "Y")
        self.assertTrue(valid)
        self.assertEqual(value, 255)

    def test_invalid_coordinates(self):
        """Test invalid coordinates with axis-specific messages."""
        valid, value, error = validate_coordinate(256, "X")
        self.assertFalse(valid)
        self.assertIn("X coordinate must be 0-255", error)

        valid, value, error = validate_coordinate("abc", "Y")
        self.assertFalse(valid)
        self.assertIn("Invalid Y value", error)


class TestBinaryFormatting(unittest.TestCase):
    """Test cases for binary formatting functions."""

    def test_format_binary(self):
        """Test formatting integers as binary strings."""
        self.assertEqual(format_binary(0), "0b00000000")
        self.assertEqual(format_binary(255), "0b11111111")
        self.assertEqual(format_binary(170), "0b10101010")
        self.assertEqual(format_binary(15), "0b00001111")

        # Custom bit width
        self.assertEqual(format_binary(3, 4), "0b0011")
        self.assertEqual(format_binary(15, 4), "0b1111")

    def test_format_binary_errors(self):
        """Test format_binary error handling."""
        with self.assertRaises(TypeError):
            format_binary("not an int")

        with self.assertRaises(ValueError):
            format_binary(-1)

        with self.assertRaises(ValueError):
            format_binary(256, 8)  # Too large for 8 bits

    def test_parse_binary_string(self):
        """Test parsing binary strings to integers."""
        self.assertEqual(parse_binary_string("11111111"), 255)
        self.assertEqual(parse_binary_string("00000000"), 0)
        self.assertEqual(parse_binary_string("10101010"), 170)

        # With 0b prefix
        self.assertEqual(parse_binary_string("0b11111111"), 255)
        self.assertEqual(parse_binary_string("0B11111111"), 255)

        # With whitespace
        self.assertEqual(parse_binary_string("  11111111  "), 255)
        self.assertEqual(parse_binary_string("  0b11111111  "), 255)

    def test_parse_binary_string_errors(self):
        """Test parse_binary_string with invalid input."""
        self.assertIsNone(parse_binary_string(""))
        self.assertIsNone(parse_binary_string("0b"))
        self.assertIsNone(parse_binary_string("not binary"))
        self.assertIsNone(parse_binary_string("0b11111112"))  # Invalid digit


class TestPortValidation(unittest.TestCase):
    """Test cases for port name validation."""

    def test_valid_windows_ports(self):
        """Test valid Windows COM port names."""
        self.assertTrue(validate_port_name("COM1"))
        self.assertTrue(validate_port_name("COM10"))
        self.assertTrue(validate_port_name("COM256"))
        self.assertTrue(validate_port_name("com4"))  # Case insensitive

    def test_valid_unix_ports(self):
        """Test valid Unix-like port names."""
        self.assertTrue(validate_port_name("/dev/ttyUSB0"))
        self.assertTrue(validate_port_name("/dev/ttyACM0"))
        self.assertTrue(validate_port_name("/dev/serial0"))

    def test_invalid_ports(self):
        """Test invalid port names."""
        self.assertFalse(validate_port_name(""))
        self.assertFalse(validate_port_name("COM"))  # No number
        self.assertFalse(validate_port_name("COM0"))  # Zero not allowed
        self.assertFalse(validate_port_name("/dev/"))  # Too short


class TestBaudRateValidation(unittest.TestCase):
    """Test cases for baud rate validation."""

    def test_standard_rates(self):
        """Test standard baud rates."""
        standard_rates = [9600, 19200, 38400, 57600, 115200]

        for rate in standard_rates:
            valid, value, error = validate_baud_rate(rate)
            self.assertTrue(valid)
            self.assertEqual(value, rate)

            # Also test string input
            valid, value, error = validate_baud_rate(str(rate))
            self.assertTrue(valid)
            self.assertEqual(value, rate)

    def test_non_standard_rates(self):
        """Test non-standard but valid rates."""
        valid, value, error = validate_baud_rate(12345)
        self.assertTrue(valid)  # Non-standard but allowed
        self.assertEqual(value, 12345)

    def test_invalid_rates(self):
        """Test invalid baud rates."""
        valid, value, error = validate_baud_rate(0)
        self.assertFalse(valid)
        self.assertIn("must be positive", error)

        valid, value, error = validate_baud_rate(-9600)
        self.assertFalse(valid)
        self.assertIn("must be positive", error)

        valid, value, error = validate_baud_rate("not a number")
        self.assertFalse(valid)
        self.assertIn("Invalid baud rate", error)

        valid, value, error = validate_baud_rate(3.14)
        self.assertFalse(valid)
        self.assertIn("must be an integer", error)


class TestValidatorIntegration(unittest.TestCase):
    """Integration tests for validators working together."""

    def test_all_input_formats(self):
        """Test that all validators handle various input formats consistently."""
        test_values = ["128", "0xFF", "0b10000000", 128, 128.0]

        for val in test_values[:4]:  # Exclude float for some
            valid, parsed, _ = validate_uint8(val)
            if valid:
                self.assertIn(parsed, [128, 255])  # Depending on format

    def test_error_message_consistency(self):
        """Test that error messages are consistent and helpful."""
        # Out of range errors
        _, _, error1 = validate_uint8(256)
        _, _, error2 = validate_buffer_index(256)
        _, _, error3 = validate_coordinate(256, "X")

        self.assertIn("range", error1.lower())
        self.assertIn("0-255", error2)
        self.assertIn("0-255", error3)


if __name__ == "__main__":
    # Run with verbose output
    unittest.main(verbosity=2)

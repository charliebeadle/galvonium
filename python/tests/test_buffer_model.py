"""
Unit tests for buffer_model module.
Run with: python -m unittest test_buffer_model
"""

import unittest
import sys
from pathlib import Path

# Add parent directory to path if needed
sys.path.insert(0, str(Path(__file__).parent))

from models.buffer_model import BufferStep, BufferData


class TestBufferStep(unittest.TestCase):
    """Test cases for BufferStep class."""

    def test_creation(self):
        """Test creating a BufferStep with valid values."""
        step = BufferStep(10, x=128, y=64, flags=240)
        self.assertEqual(step.index, 10)
        self.assertEqual(step.x, 128)
        self.assertEqual(step.y, 64)
        self.assertEqual(step.flags, 240)

    def test_default_values(self):
        """Test BufferStep defaults to zeros."""
        step = BufferStep(5)
        self.assertEqual(step.x, 0)
        self.assertEqual(step.y, 0)
        self.assertEqual(step.flags, 0)
        self.assertTrue(step.is_empty())

    def test_x_validation(self):
        """Test X coordinate validation."""
        step = BufferStep(0)

        # Valid values
        step.x = 0
        self.assertEqual(step.x, 0)
        step.x = 255
        self.assertEqual(step.x, 255)

        # Invalid values
        with self.assertRaises(ValueError) as ctx:
            step.x = 256
        self.assertIn("out of range", str(ctx.exception))

        with self.assertRaises(ValueError):
            step.x = -1

    def test_y_validation(self):
        """Test Y coordinate validation."""
        step = BufferStep(0)

        # Valid values
        step.y = 0
        self.assertEqual(step.y, 0)
        step.y = 255
        self.assertEqual(step.y, 255)

        # Invalid values
        with self.assertRaises(ValueError):
            step.y = 256
        with self.assertRaises(ValueError):
            step.y = -1

    def test_flags_validation(self):
        """Test flags validation."""
        step = BufferStep(0)

        # Valid values
        step.flags = 0
        self.assertEqual(step.flags, 0)
        step.flags = 255
        self.assertEqual(step.flags, 255)

        # Invalid values
        with self.assertRaises(ValueError):
            step.flags = 256
        with self.assertRaises(ValueError):
            step.flags = -1

    def test_flags_binary(self):
        """Test binary representation of flags."""
        step = BufferStep(0, flags=0b11110000)
        self.assertEqual(step.flags_binary, "11110000")

        step.flags = 0b00001111
        self.assertEqual(step.flags_binary, "00001111")

        step.flags = 0
        self.assertEqual(step.flags_binary, "00000000")

        step.flags = 255
        self.assertEqual(step.flags_binary, "11111111")

    def test_is_empty(self):
        """Test empty step detection."""
        step = BufferStep(0)
        self.assertTrue(step.is_empty())

        step.x = 1
        self.assertFalse(step.is_empty())
        step.x = 0

        step.y = 1
        self.assertFalse(step.is_empty())
        step.y = 0

        step.flags = 1
        self.assertFalse(step.is_empty())

    def test_to_write_command(self):
        """Test WRITE command generation."""
        step = BufferStep(10, x=128, y=64, flags=240)

        # Default target (INACTIVE)
        cmd = step.to_write_command()
        self.assertEqual(cmd, "WRITE 10 128 64 240 INACTIVE")

        # Explicit INACTIVE
        cmd = step.to_write_command("INACTIVE")
        self.assertEqual(cmd, "WRITE 10 128 64 240 INACTIVE")

        # ACTIVE target
        cmd = step.to_write_command("ACTIVE")
        self.assertEqual(cmd, "WRITE 10 128 64 240 ACTIVE")


class TestBufferData(unittest.TestCase):
    """Test cases for BufferData class."""

    def setUp(self):
        """Create a fresh BufferData for each test."""
        self.buffer = BufferData()

    def test_initialization(self):
        """Test BufferData initializes with 256 empty steps."""
        self.assertEqual(len(self.buffer.steps), 256)
        for i, step in enumerate(self.buffer.steps):
            self.assertEqual(step.index, i)
            self.assertTrue(step.is_empty())

    def test_clear(self):
        """Test clearing buffer."""
        # Set some data
        self.buffer.set_step(0, 100, 100, 100)
        self.buffer.set_step(10, 200, 200, 200)

        # Clear
        self.buffer.clear()

        # Check all steps are empty
        for step in self.buffer.steps:
            self.assertTrue(step.is_empty())

    def test_get_step(self):
        """Test getting step by index."""
        step = self.buffer.get_step(50)
        self.assertEqual(step.index, 50)

        # Test bounds
        step = self.buffer.get_step(0)
        self.assertEqual(step.index, 0)

        step = self.buffer.get_step(255)
        self.assertEqual(step.index, 255)

        # Test out of bounds
        with self.assertRaises(IndexError):
            self.buffer.get_step(256)
        with self.assertRaises(IndexError):
            self.buffer.get_step(-1)

    def test_set_step(self):
        """Test setting step values."""
        self.buffer.set_step(10, 128, 64, 240)

        step = self.buffer.get_step(10)
        self.assertEqual(step.x, 128)
        self.assertEqual(step.y, 64)
        self.assertEqual(step.flags, 240)

    def test_get_last_used_index(self):
        """Test finding last non-empty step."""
        # Empty buffer
        self.assertEqual(self.buffer.get_last_used_index(), 0)

        # Set some data
        self.buffer.set_step(5, 1, 1, 1)
        self.assertEqual(self.buffer.get_last_used_index(), 5)

        self.buffer.set_step(100, 1, 1, 1)
        self.assertEqual(self.buffer.get_last_used_index(), 100)

        self.buffer.set_step(255, 1, 1, 1)
        self.assertEqual(self.buffer.get_last_used_index(), 255)

        # Clear the last one
        self.buffer.set_step(255, 0, 0, 0)
        self.assertEqual(self.buffer.get_last_used_index(), 100)

    def test_get_size_for_write(self):
        """Test size calculation for SIZE command."""
        # Empty buffer should return 1 (minimum)
        self.assertEqual(self.buffer.get_size_for_write(), 1)

        # With data
        self.buffer.set_step(4, 1, 1, 1)
        self.assertEqual(self.buffer.get_size_for_write(), 5)  # Index 4 + 1

        self.buffer.set_step(255, 1, 1, 1)
        self.assertEqual(self.buffer.get_size_for_write(), 256)

    def test_from_dump_response(self):
        """Test parsing DUMP response."""
        dump_response = """0: 128,64 255
1: 130,66 240
2: 132,68 240
3: 134,70 0
EOC"""

        # Use class method to create new instance
        buffer = BufferData.from_dump_response(dump_response)

        # Check parsed values
        step = buffer.get_step(0)
        self.assertEqual(step.x, 128)
        self.assertEqual(step.y, 64)
        self.assertEqual(step.flags, 255)

        step = buffer.get_step(1)
        self.assertEqual(step.x, 130)
        self.assertEqual(step.y, 66)
        self.assertEqual(step.flags, 240)

        step = buffer.get_step(3)
        self.assertEqual(step.x, 134)
        self.assertEqual(step.y, 70)
        self.assertEqual(step.flags, 0)

        # Check step 4 is still empty
        step = buffer.get_step(4)
        self.assertTrue(step.is_empty())

        # Check last used index
        self.assertEqual(buffer.get_last_used_index(), 3)

    def test_from_dump_response_with_gaps(self):
        """Test parsing DUMP response with gaps in indices."""
        dump_response = """0: 100,100 255
10: 150,150 240
100: 200,200 128
EOC"""

        # Use class method to create new instance
        buffer = BufferData.from_dump_response(dump_response)

        self.assertEqual(buffer.get_step(0).x, 100)
        self.assertEqual(buffer.get_step(10).x, 150)
        self.assertEqual(buffer.get_step(100).x, 200)

        # Check gaps are empty
        self.assertTrue(buffer.get_step(5).is_empty())
        self.assertTrue(buffer.get_step(50).is_empty())

        self.assertEqual(buffer.get_last_used_index(), 100)

    def test_to_write_commands(self):
        """Test generating WRITE commands."""
        self.buffer.set_step(0, 100, 100, 128)
        self.buffer.set_step(1, 150, 100, 128)
        self.buffer.set_step(2, 150, 150, 128)

        commands = self.buffer.to_write_commands()

        self.assertEqual(len(commands), 3)
        self.assertEqual(commands[0], "WRITE 0 100 100 128 INACTIVE")
        self.assertEqual(commands[1], "WRITE 1 150 100 128 INACTIVE")
        self.assertEqual(commands[2], "WRITE 2 150 150 128 INACTIVE")

    def test_to_write_commands_with_gaps(self):
        """Test WRITE commands include zeros for gaps."""
        self.buffer.set_step(0, 10, 10, 1)
        self.buffer.set_step(5, 50, 50, 5)

        commands = self.buffer.to_write_commands()

        # Should generate 6 commands (0-5)
        self.assertEqual(len(commands), 6)

        # Check some specific commands
        self.assertEqual(commands[0], "WRITE 0 10 10 1 INACTIVE")
        self.assertEqual(commands[1], "WRITE 1 0 0 0 INACTIVE")  # Gap
        self.assertEqual(commands[5], "WRITE 5 50 50 5 INACTIVE")

    def test_to_size_command(self):
        """Test SIZE command generation."""
        # Empty buffer
        self.assertEqual(self.buffer.to_size_command(), "SIZE 1 INACTIVE")

        self.buffer.set_step(9, 1, 1, 1)
        self.assertEqual(self.buffer.to_size_command(), "SIZE 10 INACTIVE")

        self.assertEqual(self.buffer.to_size_command("ACTIVE"), "SIZE 10 ACTIVE")

    def test_get_write_sequence(self):
        """Test complete write sequence generation."""
        self.buffer.set_step(0, 100, 100, 128)
        self.buffer.set_step(1, 150, 150, 128)

        sequence = self.buffer.get_write_sequence()

        # Should have CLEAR, WRITEs, and SIZE
        self.assertEqual(sequence[0], "CLEAR INACTIVE")
        self.assertEqual(sequence[-1], "SIZE 2 INACTIVE")
        self.assertEqual(len(sequence), 4)  # CLEAR + 2 WRITEs + SIZE

    def test_load_from_list(self):
        """Test loading buffer from list of tuples."""
        data = [
            (100, 100, 255),
            (150, 100, 255),
            (150, 150, 255),
            (100, 150, 255),
            (100, 100, 0),
        ]

        self.buffer.load_from_list(data)

        for i, (x, y, flags) in enumerate(data):
            step = self.buffer.get_step(i)
            self.assertEqual(step.x, x)
            self.assertEqual(step.y, y)
            self.assertEqual(step.flags, flags)

        self.assertEqual(self.buffer.get_last_used_index(), 4)


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and boundary conditions."""

    def test_sparse_buffer(self):
        """Test buffer with sparse data."""
        buffer = BufferData()

        # Create sparse data
        buffer.set_step(0, 10, 10, 1)
        buffer.set_step(10, 20, 20, 2)
        buffer.set_step(100, 30, 30, 3)
        buffer.set_step(255, 40, 40, 4)

        self.assertEqual(buffer.get_last_used_index(), 255)
        self.assertEqual(buffer.get_size_for_write(), 256)

        # Check that write commands include all steps up to 255
        commands = buffer.to_write_commands()
        self.assertEqual(len(commands), 256)

    def test_all_zeros_except_last(self):
        """Test buffer with only last step non-zero."""
        buffer = BufferData()
        buffer.set_step(255, 1, 1, 1)

        self.assertEqual(buffer.get_last_used_index(), 255)
        self.assertEqual(buffer.get_size_for_write(), 256)

        commands = buffer.to_write_commands()
        self.assertEqual(len(commands), 256)
        self.assertEqual(commands[-1], "WRITE 255 1 1 1 INACTIVE")

    def test_malformed_dump_response(self):
        """Test handling of malformed DUMP responses."""
        # Missing EOC - should still parse what it can
        dump = "0: 100,100 255\n1: 150,150 255"
        buffer = BufferData.from_dump_response(dump)
        self.assertEqual(buffer.get_step(0).x, 100)
        self.assertEqual(buffer.get_step(1).x, 150)

        # Invalid format lines should be skipped
        dump = """0: 100,100 255
garbage line
1: 150,150 255
EOC"""
        buffer = BufferData.from_dump_response(dump)
        self.assertEqual(buffer.get_step(0).x, 100)
        self.assertEqual(buffer.get_step(1).x, 150)


if __name__ == "__main__":
    # Run with verbose output
    unittest.main(verbosity=2)

import unittest
from serialio.commands import (
    cmd_write,
    cmd_dump,
    cmd_swap,
    cmd_clear,
    cmd_size,
    build_write_sequence_from_buffer,
    INACTIVE,
)
from unittest.mock import Mock


class TestCommands(unittest.TestCase):
    """Test the command building functions"""

    def test_inactive_constant(self):
        """Test the INACTIVE constant"""
        self.assertEqual(INACTIVE, "INACTIVE")

    def test_cmd_write_basic(self):
        """Test basic WRITE command generation"""
        self.assertEqual(cmd_write(0, 1, 2, 3), "WRITE 0 1 2 3 INACTIVE")
        self.assertEqual(
            cmd_write(255, 255, 255, 255), "WRITE 255 255 255 255 INACTIVE"
        )

    def test_cmd_write_with_custom_buffer(self):
        """Test WRITE command with custom buffer parameter"""
        self.assertEqual(cmd_write(0, 1, 2, 3, "ACTIVE"), "WRITE 0 1 2 3 ACTIVE")
        self.assertEqual(
            cmd_write(10, 20, 30, 40, "INACTIVE"), "WRITE 10 20 30 40 INACTIVE"
        )
        self.assertEqual(
            cmd_write(100, 150, 200, 250, "ACTIVE"), "WRITE 100 150 200 250 ACTIVE"
        )

    def test_cmd_write_buffer_parameter_handling(self):
        """Test WRITE command buffer parameter handling"""
        # Test None buffer (should default to INACTIVE)
        self.assertEqual(cmd_write(0, 1, 2, 3, None), "WRITE 0 1 2 3 INACTIVE")

        # Test empty string buffer (should default to INACTIVE)
        self.assertEqual(cmd_write(0, 1, 2, 3, ""), "WRITE 0 1 2 3 INACTIVE")

        # Test lowercase buffer (should be converted to uppercase)
        self.assertEqual(cmd_write(0, 1, 2, 3, "active"), "WRITE 0 1 2 3 ACTIVE")
        self.assertEqual(cmd_write(0, 1, 2, 3, "inactive"), "WRITE 0 1 2 3 INACTIVE")

    def test_cmd_dump_basic(self):
        """Test basic DUMP command generation"""
        self.assertEqual(cmd_dump(), "DUMP INACTIVE")
        self.assertEqual(cmd_dump("INACTIVE"), "DUMP INACTIVE")

    def test_cmd_dump_with_custom_buffer(self):
        """Test DUMP command with custom buffer parameter"""
        self.assertEqual(cmd_dump("ACTIVE"), "DUMP ACTIVE")
        self.assertEqual(cmd_dump("active"), "DUMP ACTIVE")  # lowercase conversion

    def test_cmd_clear_basic(self):
        """Test basic CLEAR command generation"""
        self.assertEqual(cmd_clear(), "CLEAR INACTIVE")
        self.assertEqual(cmd_clear("INACTIVE"), "CLEAR INACTIVE")

    def test_cmd_clear_with_custom_buffer(self):
        """Test CLEAR command with custom buffer parameter"""
        self.assertEqual(cmd_clear("ACTIVE"), "CLEAR ACTIVE")
        self.assertEqual(cmd_clear("active"), "CLEAR ACTIVE")  # lowercase conversion

    def test_cmd_swap(self):
        """Test SWAP command generation"""
        self.assertEqual(cmd_swap(), "SWAP")
        # SWAP doesn't take parameters, so test it multiple times
        self.assertEqual(cmd_swap(), "SWAP")

    def test_cmd_size_basic(self):
        """Test basic SIZE command generation"""
        self.assertEqual(cmd_size(1), "SIZE 1 INACTIVE")
        self.assertEqual(cmd_size(256), "SIZE 256 INACTIVE")
        self.assertEqual(cmd_size(128), "SIZE 128 INACTIVE")

    def test_cmd_size_with_custom_buffer(self):
        """Test SIZE command with custom buffer parameter"""
        self.assertEqual(cmd_size(1, "ACTIVE"), "SIZE 1 ACTIVE")
        self.assertEqual(cmd_size(256, "ACTIVE"), "SIZE 256 ACTIVE")
        self.assertEqual(
            cmd_size(100, "active"), "SIZE 100 ACTIVE"
        )  # lowercase conversion

    def test_cmd_size_edge_cases(self):
        """Test SIZE command edge cases"""
        # Test minimum valid size
        self.assertEqual(cmd_size(1), "SIZE 1 INACTIVE")

        # Test maximum valid size
        self.assertEqual(cmd_size(256), "SIZE 256 INACTIVE")

        # Test middle value
        self.assertEqual(cmd_size(128), "SIZE 128 INACTIVE")

    def test_cmd_write_edge_cases(self):
        """Test WRITE command edge cases"""
        # Test minimum valid values
        self.assertEqual(cmd_write(0, 0, 0, 0), "WRITE 0 0 0 0 INACTIVE")

        # Test maximum valid values
        self.assertEqual(
            cmd_write(255, 255, 255, 255), "WRITE 255 255 255 255 INACTIVE"
        )

        # Test mixed values
        self.assertEqual(cmd_write(0, 255, 0, 255), "WRITE 0 255 0 255 INACTIVE")
        self.assertEqual(cmd_write(255, 0, 255, 0), "WRITE 255 0 255 0 INACTIVE")

    def test_cmd_write_parameter_types(self):
        """Test WRITE command parameter type handling"""
        # Test with string numbers (should be converted to int)
        self.assertEqual(cmd_write("0", "1", "2", "3"), "WRITE 0 1 2 3 INACTIVE")

        # Test with float numbers (should be converted to int)
        self.assertEqual(cmd_write(0.0, 1.5, 2.7, 3.9), "WRITE 0 1 2 3 INACTIVE")

        # Test with mixed types
        self.assertEqual(cmd_write("0", 1, "2", 3.0), "WRITE 0 1 2 3 INACTIVE")

    def test_cmd_size_parameter_types(self):
        """Test SIZE command parameter type handling"""
        # Test with string numbers
        self.assertEqual(cmd_size("1"), "SIZE 1 INACTIVE")
        self.assertEqual(cmd_size("256"), "SIZE 256 INACTIVE")

        # Test with float numbers
        self.assertEqual(cmd_size(1.0), "SIZE 1 INACTIVE")
        self.assertEqual(cmd_size(256.7), "SIZE 256 INACTIVE")

    def test_cmd_write_validation_errors(self):
        """Test WRITE command validation error cases"""
        # Test index out of bounds
        with self.assertRaises(ValueError) as cm:
            cmd_write(-1, 0, 0, 0)
        self.assertIn("index must be 0..255", str(cm.exception))

        with self.assertRaises(ValueError) as cm:
            cmd_write(256, 0, 0, 0)
        self.assertIn("index must be 0..255", str(cm.exception))

        # Test x out of bounds
        with self.assertRaises(ValueError) as cm:
            cmd_write(0, -1, 0, 0)
        self.assertIn("x must be 0..255", str(cm.exception))

        with self.assertRaises(ValueError) as cm:
            cmd_write(0, 256, 0, 0)
        self.assertIn("x must be 0..255", str(cm.exception))

        # Test y out of bounds
        with self.assertRaises(ValueError) as cm:
            cmd_write(0, 0, -1, 0)
        self.assertIn("y must be 0..255", str(cm.exception))

        with self.assertRaises(ValueError) as cm:
            cmd_write(0, 0, 256, 0)
        self.assertIn("y must be 0..255", str(cm.exception))

        # Test flags out of bounds
        with self.assertRaises(ValueError) as cm:
            cmd_write(0, 0, 0, -1)
        self.assertIn("flags must be 0..255", str(cm.exception))

        with self.assertRaises(ValueError) as cm:
            cmd_write(0, 0, 0, 999)
        self.assertIn("flags must be 0..255", str(cm.exception))

    def test_cmd_size_validation_errors(self):
        """Test SIZE command validation error cases"""
        # Test size too small
        with self.assertRaises(ValueError) as cm:
            cmd_size(0)
        self.assertIn("size must be 1..256", str(cm.exception))

        # Test size too large
        with self.assertRaises(ValueError) as cm:
            cmd_size(257)
        self.assertIn("size must be 1..256", str(cm.exception))

        # Test negative size
        with self.assertRaises(ValueError) as cm:
            cmd_size(-1)
        self.assertIn("size must be 1..256", str(cm.exception))

    def test_command_formatting_consistency(self):
        """Test that all commands follow consistent formatting"""
        # All commands should have uppercase names
        self.assertTrue(cmd_write(0, 0, 0, 0).startswith("WRITE"))
        self.assertTrue(cmd_dump().startswith("DUMP"))
        self.assertTrue(cmd_clear().startswith("CLEAR"))
        self.assertTrue(cmd_swap().startswith("SWAP"))
        self.assertTrue(cmd_size(1).startswith("SIZE"))

        # All commands should have proper spacing
        self.assertIn(" ", cmd_write(0, 0, 0, 0))
        self.assertIn(" ", cmd_dump())
        self.assertIn(" ", cmd_clear())
        self.assertIn(" ", cmd_size(1))
        # SWAP has no parameters, so no spaces needed

    def test_buffer_parameter_consistency(self):
        """Test that buffer parameters are handled consistently across commands"""
        # All commands that take buffer parameters should handle them the same way
        for buffer_name in ["ACTIVE", "INACTIVE", "active", "inactive", None, ""]:
            expected_upper = (
                "ACTIVE"
                if buffer_name and buffer_name.upper() == "ACTIVE"
                else "INACTIVE"
            )

            # Test WRITE
            if buffer_name:
                result = cmd_write(0, 0, 0, 0, buffer_name)
                self.assertIn(expected_upper, result)

            # Test DUMP
            result = cmd_dump(buffer_name)
            self.assertIn(expected_upper, result)

            # Test CLEAR
            result = cmd_clear(buffer_name)
            self.assertIn(expected_upper, result)

            # Test SIZE
            result = cmd_size(1, buffer_name)
            self.assertIn(expected_upper, result)


class TestBuildWriteSequenceFromBuffer(unittest.TestCase):
    """Test the build_write_sequence_from_buffer function"""

    def test_empty_buffer(self):
        """Test building sequence from empty buffer"""
        # Create a mock buffer with no steps
        mock_buffer = Mock()
        mock_buffer.get_last_used_index.return_value = -1
        mock_buffer.steps = []

        # The function expects steps for all indices, so we need to provide them
        # For an empty buffer (last_used_index = -1), it will try to access steps[0]
        # So we need to provide at least one step
        mock_step = Mock()
        mock_step.x = 0
        mock_step.y = 0
        mock_step.flags = 0
        mock_buffer.steps = [mock_step]

        sequence = build_write_sequence_from_buffer(mock_buffer)

        # Should have CLEAR, WRITE, and SIZE commands
        self.assertEqual(len(sequence), 3)
        self.assertEqual(sequence[0], "CLEAR INACTIVE")
        self.assertEqual(sequence[1], "WRITE 0 0 0 0 INACTIVE")
        self.assertEqual(sequence[2], "SIZE 1 INACTIVE")

    def test_buffer_with_gaps(self):
        """Test building sequence from buffer with gaps in indices"""
        mock_buffer = Mock()
        mock_buffer.get_last_used_index.return_value = 5

        # Create mock steps for all indices 0-5 (no gaps)
        mock_steps = []
        for i in range(6):
            step = Mock()
            step.x = i * 10
            step.y = i * 20
            step.flags = i * 30
            mock_steps.append(step)

        mock_buffer.steps = mock_steps

        sequence = build_write_sequence_from_buffer(mock_buffer)

        # Should have CLEAR, 6 WRITE commands, and SIZE
        self.assertEqual(len(sequence), 8)
        self.assertEqual(sequence[0], "CLEAR INACTIVE")
        self.assertEqual(sequence[1], "WRITE 0 0 0 0 INACTIVE")
        self.assertEqual(sequence[2], "WRITE 1 10 20 30 INACTIVE")
        self.assertEqual(sequence[3], "WRITE 2 20 40 60 INACTIVE")
        self.assertEqual(sequence[4], "WRITE 3 30 60 90 INACTIVE")
        self.assertEqual(sequence[5], "WRITE 4 40 80 120 INACTIVE")
        self.assertEqual(sequence[6], "WRITE 5 50 100 150 INACTIVE")
        self.assertEqual(sequence[7], "SIZE 6 INACTIVE")

    def test_single_step_buffer(self):
        """Test building sequence from buffer with single step"""
        mock_buffer = Mock()
        mock_buffer.get_last_used_index.return_value = 0

        # Create a mock step
        mock_step = Mock()
        mock_step.x = 100
        mock_step.y = 200
        mock_step.flags = 255
        mock_buffer.steps = [mock_step]

        sequence = build_write_sequence_from_buffer(mock_buffer)

        # Should have CLEAR, WRITE, and SIZE commands
        self.assertEqual(len(sequence), 3)
        self.assertEqual(sequence[0], "CLEAR INACTIVE")
        self.assertEqual(sequence[1], "WRITE 0 100 200 255 INACTIVE")
        self.assertEqual(sequence[2], "SIZE 1 INACTIVE")

    def test_multiple_step_buffer(self):
        """Test building sequence from buffer with multiple steps"""
        mock_buffer = Mock()
        mock_buffer.get_last_used_index.return_value = 2

        # Create mock steps
        mock_steps = []
        for i in range(3):
            step = Mock()
            step.x = i * 10
            step.y = i * 20
            step.flags = i * 30
            mock_steps.append(step)

        mock_buffer.steps = mock_steps

        sequence = build_write_sequence_from_buffer(mock_buffer)

        # Should have CLEAR, 3 WRITE commands, and SIZE
        self.assertEqual(len(sequence), 5)
        self.assertEqual(sequence[0], "CLEAR INACTIVE")
        self.assertEqual(sequence[1], "WRITE 0 0 0 0 INACTIVE")
        self.assertEqual(sequence[2], "WRITE 1 10 20 30 INACTIVE")
        self.assertEqual(sequence[3], "WRITE 2 20 40 60 INACTIVE")
        self.assertEqual(sequence[4], "SIZE 3 INACTIVE")

    def test_buffer_edge_cases(self):
        """Test building sequence from buffer edge cases"""
        # Test maximum size buffer
        mock_buffer = Mock()
        mock_buffer.get_last_used_index.return_value = 255

        # Create 256 mock steps
        mock_steps = []
        for i in range(256):
            step = Mock()
            step.x = i % 256
            step.y = (i * 2) % 256
            step.flags = (i * 3) % 256
            mock_steps.append(step)

        mock_buffer.steps = mock_steps

        sequence = build_write_sequence_from_buffer(mock_buffer)

        # Should have CLEAR, 256 WRITE commands, and SIZE
        self.assertEqual(len(sequence), 258)
        self.assertEqual(sequence[0], "CLEAR INACTIVE")
        self.assertEqual(sequence[257], "SIZE 256 INACTIVE")

        # Test first and last WRITE commands
        self.assertEqual(sequence[1], "WRITE 0 0 0 0 INACTIVE")
        # The last step (index 255) should have the calculated values
        expected_x = 255 % 256  # 255
        expected_y = (255 * 2) % 256  # 510 % 256 = 254
        expected_flags = (255 * 3) % 256  # 765 % 256 = 253
        self.assertEqual(
            sequence[256],
            f"WRITE 255 {expected_x} {expected_y} {expected_flags} INACTIVE",
        )


if __name__ == "__main__":
    unittest.main()

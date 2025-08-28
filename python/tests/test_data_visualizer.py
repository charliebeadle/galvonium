import unittest
from unittest.mock import Mock, patch
import sys
import os

# Add the parent directory to the path so we can import the modules
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from gui.data_visualizer import DataVisualizer


class TestDataVisualizer(unittest.TestCase):
    """Test cases for the DataVisualizer widget."""

    def setUp(self):
        """Set up test fixtures."""
        # Create a mock QApplication instance
        from PyQt5.QtWidgets import QApplication

        self.app = QApplication.instance()
        if self.app is None:
            self.app = QApplication([])

        self.visualizer = DataVisualizer()

    def test_hex_q88_to_decimal(self):
        """Test Q8.8 hex to decimal conversion."""
        # Test case: 0x0100 = 1.0 (1 integer, 0 fractional)
        result = self.visualizer._hex_q88_to_decimal("0100")
        self.assertEqual(result, 1.0)

        # Test case: 0x0080 = 0.5 (0 integer, 128/256 fractional)
        result = self.visualizer._hex_q88_to_decimal("0080")
        self.assertEqual(result, 0.5)

        # Test case: 0x0200 = 2.0 (2 integer, 0 fractional)
        result = self.visualizer._hex_q88_to_decimal("0200")
        self.assertEqual(result, 2.0)

        # Test case: 0x00FF = 0.996 (0 integer, 255/256 fractional)
        result = self.visualizer._hex_q88_to_decimal("00FF")
        self.assertAlmostEqual(result, 0.996, places=3)

        # Test case: 0x0000 = 0.0 (0 integer, 0 fractional) - short hex
        result = self.visualizer._hex_q88_to_decimal("0")
        self.assertEqual(result, 0.0)

        # Test case: 0x0001 = 0.004 (0 integer, 1/256 fractional) - short hex
        result = self.visualizer._hex_q88_to_decimal("1")
        self.assertAlmostEqual(result, 0.004, places=3)

        # Test case: 0x0100 = 1.0 (1 integer, 0 fractional) - short hex
        result = self.visualizer._hex_q88_to_decimal("100")
        self.assertEqual(result, 1.0)

    def test_process_data_start_marker(self):
        """Test processing START marker."""
        self.visualizer.is_collecting = True
        self.visualizer.x_coords = [1.0, 2.0]  # Some existing data
        self.visualizer.y_coords = [1.0, 2.0]

        self.visualizer.process_data("START")

        # Should clear existing data
        self.assertEqual(len(self.visualizer.x_coords), 0)
        self.assertEqual(len(self.visualizer.y_coords), 0)

    def test_process_data_end_marker(self):
        """Test processing END marker."""
        self.visualizer.is_collecting = True
        self.visualizer.x_coords = [1.0, 2.0]
        self.visualizer.y_coords = [1.0, 2.0]

        self.visualizer.process_data("END")

        # Should enable export button
        self.assertTrue(self.visualizer.export_btn.isEnabled())

    def test_process_data_coordinates(self):
        """Test processing coordinate data."""
        self.visualizer.is_collecting = True

        # Test valid coordinate data with full hex
        self.visualizer.process_data("0100 0200")  # X=1.0, Y=2.0

        self.assertEqual(len(self.visualizer.x_coords), 1)
        self.assertEqual(len(self.visualizer.y_coords), 1)
        self.assertEqual(self.visualizer.x_coords[0], 1.0)
        self.assertEqual(self.visualizer.y_coords[0], 2.0)

        # Test coordinates with short hex values (leading zeros omitted)
        self.visualizer.process_data("0 100")  # X=0.0, Y=1.0
        self.visualizer.process_data("80 0")  # X=0.5, Y=0.0

        self.assertEqual(len(self.visualizer.x_coords), 3)
        self.assertEqual(len(self.visualizer.y_coords), 3)
        self.assertEqual(self.visualizer.x_coords[1], 0.0)
        self.assertEqual(self.visualizer.y_coords[1], 1.0)
        self.assertEqual(self.visualizer.x_coords[2], 0.5)
        self.assertEqual(self.visualizer.y_coords[2], 0.0)

    def test_process_data_invalid_format(self):
        """Test processing invalid coordinate data."""
        self.visualizer.is_collecting = True
        initial_count = len(self.visualizer.x_coords)

        # Test invalid data
        self.visualizer.process_data("invalid data")
        self.visualizer.process_data("G 100")  # Invalid hex character
        self.visualizer.process_data("100 G")  # Invalid hex character
        self.visualizer.process_data("10000 200")  # Too large for Q8.8 format
        self.visualizer.process_data("200 10000")  # Too large for Q8.8 format

        # Should not add any coordinates
        self.assertEqual(len(self.visualizer.x_coords), initial_count)

    def test_clear_data(self):
        """Test clearing data."""
        self.visualizer.x_coords = [1.0, 2.0]
        self.visualizer.y_coords = [1.0, 2.0]
        self.visualizer.export_btn.setEnabled(True)

        self.visualizer._clear_data()

        self.assertEqual(len(self.visualizer.x_coords), 0)
        self.assertEqual(len(self.visualizer.y_coords), 0)
        self.assertFalse(self.visualizer.export_btn.isEnabled())

    def test_toggle_collection(self):
        """Test toggling data collection."""
        # Start with collection off
        self.assertFalse(self.visualizer.is_collecting)
        self.assertEqual(self.visualizer.start_stop_btn.text(), "Start Collection")

        # Toggle on
        self.visualizer._toggle_collection()
        self.assertTrue(self.visualizer.is_collecting)
        self.assertEqual(self.visualizer.start_stop_btn.text(), "Stop Collection")

        # Toggle off
        self.visualizer._toggle_collection()
        self.assertFalse(self.visualizer.is_collecting)
        self.assertEqual(self.visualizer.start_stop_btn.text(), "Start Collection")


if __name__ == "__main__":
    unittest.main()

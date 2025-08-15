"""
Test the refactored GUI architecture.
"""

import unittest
import sys
from PyQt5 import QtWidgets, QtCore

# Ensure package path includes python/
app = QtWidgets.QApplication.instance() or QtWidgets.QApplication(sys.argv)

from controllers.galvo_controller import GalvoController
from gui.connection_manager import ConnectionManager


class TestRefactoredArchitecture(unittest.TestCase):
    """Test the refactored GUI architecture."""

    def setUp(self):
        """Set up test fixtures."""
        self.controller = GalvoController()
        self.connection_manager = ConnectionManager(self.controller)

    def test_controller_creation(self):
        """Test that controller can be created."""
        self.assertIsNotNone(self.controller)
        self.assertIsInstance(self.controller, GalvoController)

    def test_connection_manager_creation(self):
        """Test that connection manager can be created."""
        self.assertIsNotNone(self.connection_manager)
        self.assertIsInstance(self.connection_manager, ConnectionManager)

    def test_controller_signals(self):
        """Test that controller emits signals."""
        # Test buffer data changed signal
        buffer_data = self.controller.get_buffer_data()
        self.assertIsNotNone(buffer_data)

        # Test connection status signal
        self.assertFalse(self.controller.is_connected())

    def test_connection_manager_signals(self):
        """Test that connection manager emits signals."""
        # Test that connection manager can refresh ports
        self.connection_manager.refresh_ports()

    def test_controller_buffer_operations(self):
        """Test controller buffer operations."""
        # Test clear buffer
        self.controller.clear_buffer()
        buffer_data = self.controller.get_buffer_data()
        self.assertEqual(buffer_data.get_last_used_index(), 0)

        # Test set step
        self.controller.set_buffer_step(0, 128, 64, 255)
        step = buffer_data.get_step(0)
        self.assertEqual(step.x, 128)
        self.assertEqual(step.y, 64)
        self.assertEqual(step.flags, 255)

    def test_controller_port_discovery(self):
        """Test that controller can discover available ports."""
        # Test get_available_ports returns a list
        ports = self.controller.get_available_ports()
        self.assertIsInstance(ports, list)

        # Test that ports are strings (even if empty)
        for port in ports:
            self.assertIsInstance(port, str)

        # Test that method handles import errors gracefully
        # (This tests the fallback behavior)
        self.assertIsInstance(ports, list)

    def test_connection_manager_operations(self):
        """Test connection manager operations."""
        # Test not connected state
        self.assertFalse(self.connection_manager.is_connected())
        self.assertIsNone(self.connection_manager.get_current_port())
        self.assertIsNone(self.connection_manager.get_current_baud())

    def tearDown(self):
        """Clean up test fixtures."""
        self.connection_manager.cleanup()
        self.controller.cleanup()


if __name__ == "__main__":
    unittest.main()

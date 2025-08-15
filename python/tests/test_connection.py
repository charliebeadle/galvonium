import unittest
import queue
import threading
import time
from unittest.mock import Mock, patch, MagicMock

from PyQt5.QtCore import QCoreApplication, QThread
from PyQt5.QtTest import QTest

from serialio.connection import SerialConnection, _ReaderWorker
from serialio.serial_io import SerialIOError


class FakeSerialPort:
    """Mock serial port for testing SerialConnection - matches ThreadSafeSerialIO interface"""

    def __init__(self, port="COM_TEST", baud=9600, timeout=0.1):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.closed = False
        self._read_buffer = []
        self._write_buffer = []
        self._read_index = 0

    def readline(self):
        if self._read_index < len(self._read_buffer):
            result = self._read_buffer[self._read_index]
            self._read_index += 1
            return result
        return b""  # Timeout

    def write(self, data):
        if self.closed:
            raise Exception("Port is closed")
        self._write_buffer.append(data)
        return len(data)

    def close(self):
        self.closed = True

    def add_read_data(self, *data_items):
        """Add data to the read buffer for testing"""
        for item in data_items:
            if isinstance(item, str):
                item = item.encode("utf-8")
            self._read_buffer.append(item)

    def get_written_data(self):
        """Get all data written to the port"""
        return self._write_buffer.copy()

    # Add the methods that ThreadSafeSerialIO expects
    def request_shutdown(self):
        """Signal that any ongoing read operations should stop."""
        pass  # For testing, just a no-op


def fake_serial_factory(*args, **kwargs):
    """Factory function that returns a fresh FakeSerialPort for each test"""
    return FakeSerialPort(*args, **kwargs)


class TestReaderWorker(unittest.TestCase):
    """Test the _ReaderWorker class in isolation"""

    def setUp(self):
        self.app = QCoreApplication([])
        # Create a mock ThreadSafeSerialIO instead of FakeSerialPort
        self.fake_serial_io = Mock()
        self.worker = _ReaderWorker(self.fake_serial_io)

    def tearDown(self):
        if hasattr(self, "worker"):
            self.worker.stop()

    def test_initialization(self):
        """Test _ReaderWorker initialization"""
        self.assertEqual(self.worker._serial_io, self.fake_serial_io)

    def test_start_stop(self):
        """Test start and stop methods"""
        # Mock the readline method to return some data then None (shutdown)
        self.fake_serial_io.readline.side_effect = ["test line", None]

        # Start the worker
        self.worker.start()

        # Should have called readline twice
        self.assertEqual(self.fake_serial_io.readline.call_count, 2)

        # Stop the worker
        self.worker.stop()

        # Should have requested shutdown
        self.fake_serial_io.request_shutdown.assert_called_once()

    def test_line_received_signal(self):
        """Test that line_received signal is emitted"""
        received_lines = []
        self.worker.line_received.connect(received_lines.append)

        # Mock readline to return data then None
        self.fake_serial_io.readline.side_effect = ["line 1", "line 2", None]

        # Start and wait
        self.worker.start()

        # Should have received the lines
        self.assertEqual(len(received_lines), 2)
        self.assertEqual(received_lines[0], "line 1")
        self.assertEqual(received_lines[1], "line 2")

    def test_error_signal_on_read_error(self):
        """Test error signal is emitted on read errors"""
        errors = []
        self.worker.error.connect(errors.append)

        # Create a mock serial I/O that will raise an error
        bad_serial_io = Mock()
        bad_serial_io.readline.side_effect = SerialIOError("Read error")
        worker = _ReaderWorker(bad_serial_io)
        worker.error.connect(errors.append)

        # Start the worker
        worker.start()

        # Should have received error
        self.assertEqual(len(errors), 1)
        self.assertIn("Read error", errors[0])

        worker.stop()


class TestSerialConnection(unittest.TestCase):
    """Test the SerialConnection class"""

    @classmethod
    def setUpClass(cls):
        cls.app = QCoreApplication([])

    def setUp(self):
        self.connection = SerialConnection(serial_class=fake_serial_factory)
        self.received_data = []
        self.connection_status = []
        self.errors = []

        # Connect signals to capture them
        self.connection.data_received.connect(self.received_data.append)
        self.connection.connection_status.connect(self.connection_status.append)
        self.connection.error_occurred.connect(self.errors.append)

    def tearDown(self):
        if self.connection.is_connected():
            self.connection.disconnect()

    def test_initial_state(self):
        """Test initial state of SerialConnection"""
        self.assertFalse(self.connection.is_connected())
        self.assertEqual(len(self.connection_status), 0)

    def test_connect_success(self):
        """Test successful connection"""
        result = self.connection.connect("COM3", 115200)

        self.assertTrue(result)
        self.assertTrue(self.connection.is_connected())
        # Should emit both disconnect (from previous state) and connect signals
        self.assertEqual(len(self.connection_status), 2)
        self.assertFalse(self.connection_status[0])  # disconnect
        self.assertTrue(self.connection_status[1])  # connect

    def test_connect_failure(self):
        """Test connection failure"""
        # Create a connection with a bad serial class that will fail during connect
        bad_serial_class = Mock()
        bad_serial_class.side_effect = Exception("Port not found")

        # Create a new connection instance with the bad serial class
        bad_connection = SerialConnection(serial_class=bad_serial_class)
        bad_connection.error_occurred.connect(self.errors.append)
        bad_connection.connection_status.connect(self.connection_status.append)

        result = bad_connection.connect("INVALID", 9600)

        self.assertFalse(result)
        self.assertFalse(bad_connection.is_connected())
        # Should emit disconnect (from initial state) then False (from failed connect)
        self.assertEqual(len(self.connection_status), 2)
        self.assertFalse(self.connection_status[0])  # initial disconnect
        self.assertFalse(self.connection_status[1])  # failed connect
        self.assertEqual(len(self.errors), 1)
        self.assertIn("Failed to open INVALID @ 9600", self.errors[0])

    def test_disconnect(self):
        """Test disconnection"""
        self.connection.connect("COM3", 9600)
        self.assertTrue(self.connection.is_connected())

        self.connection.disconnect()

        self.assertFalse(self.connection.is_connected())
        # Should have disconnect + connect + disconnect signals
        self.assertEqual(len(self.connection_status), 3)
        self.assertFalse(self.connection_status[2])  # final disconnect

    def test_send_command_success(self):
        """Test successful command sending"""
        self.connection.connect("COM3", 9600)

        result = self.connection.send_command("DUMP INACTIVE")

        self.assertTrue(result)
        # The command should have been written to the underlying serial port
        # We can't easily test this without exposing the internal serial_io

    def test_send_command_not_connected(self):
        """Test sending command when not connected"""
        result = self.connection.send_command("DUMP INACTIVE")

        self.assertFalse(result)
        self.assertEqual(len(self.errors), 1)
        self.assertIn("Not connected", self.errors[0])

    def test_data_received_signal(self):
        """Test that data_received signal is emitted when data arrives"""
        self.connection.connect("COM3", 9600)

        # Simulate data arrival by directly calling the signal
        # In a real scenario, this would come from the reader thread
        self.connection.data_received.emit("test data")

        self.assertEqual(len(self.received_data), 1)
        self.assertEqual(self.received_data[0], "test data")

    def test_connection_status_signal(self):
        """Test connection_status signal behavior"""
        # Connect
        self.connection.connect("COM3", 9600)
        self.assertEqual(len(self.connection_status), 2)  # disconnect + connect
        self.assertTrue(self.connection_status[1])  # connect signal

        # Disconnect
        self.connection.disconnect()
        self.assertEqual(
            len(self.connection_status), 3
        )  # disconnect + connect + disconnect
        self.assertFalse(self.connection_status[2])  # disconnect signal

    def test_error_signal(self):
        """Test error_occurred signal"""
        # Simulate an error
        self.connection.error_occurred.emit("Test error message")

        self.assertEqual(len(self.errors), 1)
        self.assertEqual(self.errors[0], "Test error message")

    def test_available_ports(self):
        """Test available_ports static method"""
        ports = self.connection.available_ports()

        # Should return a list (actual content depends on system)
        self.assertIsInstance(ports, list)

    def test_multiple_connect_disconnect(self):
        """Test multiple connect/disconnect cycles"""
        for i in range(3):
            result = self.connection.connect(f"COM{i}", 9600)
            self.assertTrue(result)
            self.assertTrue(self.connection.is_connected())

            self.connection.disconnect()
            self.assertFalse(self.connection.is_connected())

    def test_connect_with_existing_connection(self):
        """Test connecting when already connected (should disconnect first)"""
        self.connection.connect("COM1", 9600)
        self.assertTrue(self.connection.is_connected())

        # Connect to different port
        result = self.connection.connect("COM2", 115200)
        self.assertTrue(result)
        self.assertTrue(self.connection.is_connected())

        # Should have emitted disconnect then connect signals
        self.assertEqual(len(self.connection_status), 4)
        self.assertFalse(self.connection_status[0])  # first disconnect
        self.assertTrue(self.connection_status[1])  # first connect
        self.assertFalse(self.connection_status[2])  # second disconnect
        self.assertTrue(self.connection_status[3])  # second connect

    def test_thread_safety_send_command(self):
        """Test thread safety of send_command"""
        self.connection.connect("COM3", 9600)

        def send_commands():
            for i in range(10):
                try:
                    self.connection.send_command(f"command_{i}")
                except Exception:
                    pass

        threads = [threading.Thread(target=send_commands) for _ in range(3)]

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()

        # Should not crash
        self.assertTrue(self.connection.is_connected())

    def test_custom_serial_class_injection(self):
        """Test custom serial class injection for testing"""
        custom_port = FakeSerialPort()
        custom_connection = SerialConnection(
            serial_class=lambda *args, **kwargs: custom_port
        )

        result = custom_connection.connect("COM_TEST", 9600)

        self.assertTrue(result)
        self.assertTrue(custom_connection.is_connected())

        custom_connection.disconnect()


class TestSerialConnectionIntegration(unittest.TestCase):
    """Integration tests for SerialConnection with actual threading"""

    @classmethod
    def setUpClass(cls):
        cls.app = QCoreApplication([])

    def setUp(self):
        self.connection = SerialConnection(serial_class=fake_serial_factory)
        self.received_data = []
        self.connection_status = []
        self.errors = []

        self.connection.data_received.connect(self.received_data.append)
        self.connection.connection_status.connect(self.connection_status.append)
        self.connection.error_occurred.connect(self.errors.append)

    def tearDown(self):
        if self.connection.is_connected():
            self.connection.disconnect()

    def test_full_connection_lifecycle(self):
        """Test complete connection lifecycle with data exchange"""
        # Connect
        result = self.connection.connect("COM3", 9600)
        self.assertTrue(result)
        self.assertTrue(self.connection.is_connected())

        # Send command
        result = self.connection.send_command("DUMP INACTIVE")
        self.assertTrue(result)

        # Wait a bit for any background processing
        QTest.qWait(100)

        # Disconnect
        self.connection.disconnect()
        self.assertFalse(self.connection.is_connected())

        # Verify signal sequence: disconnect + connect + disconnect
        self.assertEqual(len(self.connection_status), 3)
        self.assertFalse(self.connection_status[0])  # initial disconnect
        self.assertTrue(self.connection_status[1])  # connect
        self.assertFalse(self.connection_status[2])  # final disconnect

    def test_reader_thread_cleanup(self):
        """Test that reader thread is properly cleaned up on disconnect"""
        self.connection.connect("COM3", 9600)

        # Verify thread is running
        self.assertIsNotNone(self.connection._reader_thread)
        self.assertTrue(self.connection._reader_thread.isRunning())

        # Disconnect
        self.connection.disconnect()

        # Verify thread is stopped
        self.assertIsNone(self.connection._reader_thread)
        self.assertIsNone(self.connection._reader)


if __name__ == "__main__":
    unittest.main()

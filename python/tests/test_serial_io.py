import unittest
import threading
import time
from unittest.mock import Mock, patch, MagicMock

from serialio.serial_io import ThreadSafeSerialIO, SerialIOError


class FakeSerialPort:
    """Mock serial port for testing ThreadSafeSerialIO"""

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


class TestThreadSafeSerialIO(unittest.TestCase):
    """Test the ThreadSafeSerialIO class"""

    def setUp(self):
        self.serial_io = ThreadSafeSerialIO()

    def tearDown(self):
        if self.serial_io.is_connected():
            self.serial_io.disconnect()

    def test_initial_state(self):
        """Test initial state of ThreadSafeSerialIO"""
        self.assertFalse(self.serial_io.is_connected())
        self.assertIsNone(self.serial_io.get_connection_info()[0])
        self.assertIsNone(self.serial_io.get_connection_info()[1])

    def test_connect_success(self):
        """Test successful connection"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        result = serial_io.connect("COM3", 115200)

        self.assertTrue(result)
        self.assertTrue(serial_io.is_connected())
        self.assertEqual(serial_io.get_connection_info(), ("COM3", 115200))
        mock_serial_class.assert_called_once_with("COM3", 115200, timeout=0.1)

        serial_io.disconnect()

    def test_connect_failure(self):
        """Test connection failure"""
        # Create a mock serial class that raises an exception
        mock_serial_class = Mock()
        mock_serial_class.side_effect = Exception("Port not found")

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        with self.assertRaises(SerialIOError) as cm:
            serial_io.connect("INVALID", 9600)

        self.assertIn("Failed to open INVALID @ 9600", str(cm.exception))
        self.assertFalse(serial_io.is_connected())

    def test_connect_with_custom_serial_class(self):
        """Test connection with custom serial class"""
        custom_serial = FakeSerialPort
        serial_io = ThreadSafeSerialIO(serial_class=custom_serial)

        result = serial_io.connect("COM4", 57600)

        self.assertTrue(result)
        self.assertTrue(serial_io.is_connected())
        self.assertEqual(serial_io.get_connection_info(), ("COM4", 57600))

        serial_io.disconnect()

    def test_disconnect(self):
        """Test disconnection"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)
        self.assertTrue(serial_io.is_connected())

        serial_io.disconnect()
        self.assertFalse(serial_io.is_connected())
        self.assertTrue(mock_port.closed)

    def test_write_when_not_connected(self):
        """Test write operation when not connected"""
        with self.assertRaises(SerialIOError) as cm:
            self.serial_io.write(b"test data")
        self.assertEqual(str(cm.exception), "Not connected")

    def test_write_success(self):
        """Test successful write operation"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)

        result = serial_io.write(b"test data")

        self.assertEqual(result, 9)  # Length of "test data"
        self.assertEqual(mock_port.get_written_data(), [b"test data"])

        serial_io.disconnect()

    def test_write_line(self):
        """Test write_line method"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)

        result = serial_io.write_line("test command")

        self.assertEqual(result, 13)  # Length of "test command\n"
        self.assertEqual(mock_port.get_written_data(), [b"test command\n"])

        serial_io.disconnect()

    def test_readline_success(self):
        """Test successful readline operation"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_port.add_read_data(b"test line\r\n", b"another line\n")
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)

        line1 = serial_io.readline()
        line2 = serial_io.readline()

        self.assertEqual(line1, "test line")
        self.assertEqual(line2, "another line")

        serial_io.disconnect()

    def test_readline_empty(self):
        """Test readline with no data (timeout)"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)

        line = serial_io.readline()

        self.assertEqual(line, "")  # Empty string for timeout

        serial_io.disconnect()

    def test_readline_when_not_connected(self):
        """Test readline when not connected"""
        with self.assertRaises(SerialIOError) as cm:
            self.serial_io.readline()
        self.assertEqual(str(cm.exception), "Not connected")

    def test_shutdown_request(self):
        """Test shutdown request functionality"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)

        # Request shutdown
        serial_io.request_shutdown()

        # readline should return None after shutdown
        line = serial_io.readline()
        self.assertIsNone(line)

        serial_io.disconnect()

    def test_thread_safety_connect_disconnect(self):
        """Test thread safety of connect/disconnect operations"""

        def connect_worker():
            for i in range(10):
                try:
                    self.serial_io.connect(f"COM{i}", 9600)
                    time.sleep(0.01)
                    self.serial_io.disconnect()
                except Exception:
                    pass

        threads = [threading.Thread(target=connect_worker) for _ in range(3)]

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()

        # Should not crash and should end in disconnected state
        self.assertFalse(self.serial_io.is_connected())

    def test_thread_safety_write(self):
        """Test thread safety of write operations"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)

        def write_worker():
            for i in range(100):
                try:
                    serial_io.write(f"data_{i}".encode())
                except Exception:
                    pass

        threads = [threading.Thread(target=write_worker) for _ in range(3)]

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()

        # Should have written data without crashes
        written_data = mock_port.get_written_data()
        self.assertGreater(len(written_data), 0)

        serial_io.disconnect()

    def test_connection_info_after_disconnect(self):
        """Test connection info is cleared after disconnect"""
        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class)

        serial_io.connect("COM3", 9600)
        self.assertEqual(serial_io.get_connection_info(), ("COM3", 9600))

        serial_io.disconnect()
        self.assertEqual(serial_io.get_connection_info(), (None, None))

    def test_custom_timeout(self):
        """Test custom timeout setting"""
        serial_io = ThreadSafeSerialIO(read_timeout=0.5)

        # Create a mock serial class that returns our fake port
        mock_serial_class = Mock()
        mock_port = FakeSerialPort()
        mock_serial_class.return_value = mock_port

        # Create a new instance with the mock class
        serial_io = ThreadSafeSerialIO(serial_class=mock_serial_class, read_timeout=0.5)

        serial_io.connect("COM3", 9600)

        # Verify the custom timeout was used
        mock_serial_class.assert_called_once_with("COM3", 9600, timeout=0.5)

        serial_io.disconnect()


if __name__ == "__main__":
    unittest.main()

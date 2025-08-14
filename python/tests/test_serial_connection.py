import unittest
import queue

from PyQt5.QtCore import QCoreApplication
from PyQt5.QtTest import QTest

from serialio.connection import SerialConnection


# A tiny fake serial port for tests
class _FakeSerial:
    def __init__(self, *args, timeout=0.1, **kwargs):
        self._in = queue.Queue()
        self._out = []
        self.timeout = timeout
        self.closed = False

    def readline(self):
        try:
            return self._in.get(timeout=self.timeout)
        except queue.Empty:
            return b""

    def write(self, data: bytes):
        self._out.append(data)
        return len(data)

    def close(self):
        self.closed = True


def _fake_serial_factory(*args, **kwargs):
    # Return a fresh fake and stash a reference on the factory for the test to poke
    fs = _FakeSerial(*args, **kwargs)
    _fake_serial_factory.instance = fs
    return fs


class TestSerialConnection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QCoreApplication([])

    def test_connect_read_write_disconnect(self):
        conn = SerialConnection(serial_class=_fake_serial_factory)
        got = []

        conn.data_received.connect(got.append)
        ok = conn.connect("COM_TEST", 9600)
        self.assertTrue(ok)
        self.assertTrue(conn.is_connected())

        # Simulate an incoming line from the device
        fake = _fake_serial_factory.instance
        fake._in.put(b"HELLO\r\n")

        # Let the reader thread emit and Qt deliver the signal
        QTest.qWait(50)
        self.assertIn("HELLO", got[0])

        # Write a command
        self.assertTrue(conn.send_command("DUMP INACTIVE"))
        self.assertTrue(fake._out[-1].endswith(b"\n"))

        # Disconnect
        conn.disconnect()
        self.assertFalse(conn.is_connected())
        self.assertTrue(fake.closed)


if __name__ == "__main__":
    unittest.main()

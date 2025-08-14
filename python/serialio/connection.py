from __future__ import annotations

import threading
from typing import Optional, Callable, List

from PyQt5.QtCore import QObject, pyqtSignal, QThread

# NOTE: we intentionally import pyserial here (not our own package).
# Our package is named 'serialio' to avoid shadowing 'serial'.
import serial as pyserial
from serial.tools import list_ports as pyserial_list_ports


def list_serial_ports() -> List[str]:
    """
    Return a list of available serial port device names (e.g., COM3, COM4).
    """
    return [p.device for p in pyserial_list_ports.comports()]


class _ReaderWorker(QObject):
    """
    Runs in its own QThread. Continuously reads lines from the serial port
    and emits them via a Qt signal.
    """

    line_received = pyqtSignal(str)
    error = pyqtSignal(str)

    def __init__(self, ser: pyserial.Serial, parent: Optional[QObject] = None):
        super().__init__(parent)
        self._ser = ser
        self._running = False

    def start(self):
        self._running = True
        try:
            while self._running:
                try:
                    line = self._ser.readline()
                except Exception as e:
                    self.error.emit(f"Serial read error: {e!r}")
                    break

                if not self._running:
                    break

                if line:
                    try:
                        text = line.decode("utf-8", errors="replace").rstrip("\r\n")
                    except Exception as e:
                        self.error.emit(f"Decode error: {e!r}")
                        continue
                    self.line_received.emit(text)
        finally:
            # no cleanup here; port closed by SerialConnection
            pass

    def stop(self):
        self._running = False


class SerialConnection(QObject):
    """
    Qt-friendly serial connection with a QThread reader and thread-safe writes.

    Signals:
        data_received(str): a single decoded line received from the port
        connection_status(bool): True when connected, False when disconnected
        error_occurred(str): error text suitable for terminal/status bar
    """

    data_received = pyqtSignal(str)
    connection_status = pyqtSignal(bool)
    error_occurred = pyqtSignal(str)

    def __init__(
        self,
        parent: Optional[QObject] = None,
        *,
        serial_class: Callable[..., pyserial.Serial] = pyserial.Serial,
        read_timeout: float = 0.1,
    ):
        """
        serial_class is injectable for tests (e.g., a FakeSerial).
        """
        super().__init__(parent)
        self._serial_class = serial_class
        self._read_timeout = read_timeout

        self._ser: Optional[pyserial.Serial] = None
        self._reader_thread: Optional[QThread] = None
        self._reader: Optional[_ReaderWorker] = None

        self._write_lock = threading.Lock()

    # ---- Lifecycle ----
    def connect(self, port: str, baud: int = 9600) -> bool:
        """
        Open the serial port and start the reader thread.
        Returns True if connected, False otherwise.
        """
        self.disconnect()  # idempotent

        try:
            self._ser = self._serial_class(port, baud, timeout=self._read_timeout)
        except Exception as e:
            self._ser = None
            self.error_occurred.emit(f"Failed to open {port} @ {baud}: {e}")
            self.connection_status.emit(False)
            return False

        # Set up reader worker + thread
        self._reader_thread = QThread()
        self._reader = _ReaderWorker(self._ser)
        self._reader.moveToThread(self._reader_thread)

        # Plumb signals
        self._reader.line_received.connect(self.data_received)
        self._reader.error.connect(self.error_occurred)
        self._reader_thread.started.connect(self._reader.start)

        # Start
        self._reader_thread.start()
        self.connection_status.emit(True)
        return True

    def disconnect(self):
        """
        Stop reader and close the serial port.
        """
        # Stop reader
        if self._reader:
            try:
                self._reader.stop()
            except Exception:
                pass

        # Quit thread
        if self._reader_thread:
            self._reader_thread.quit()
            self._reader_thread.wait(1000)
        self._reader_thread = None
        self._reader = None

        # Close port
        if self._ser:
            try:
                self._ser.close()
            except Exception:
                pass
        self._ser = None

        self.connection_status.emit(False)

    def is_connected(self) -> bool:
        return self._ser is not None

    # ---- Writing ----
    def send_command(self, cmd: str) -> bool:
        """
        Thread-safe write of a single command. Appends '\n'.
        Returns True on success, False if not connected or on error.

        NOTE: Per your protocol, we do not force uppercase here; build commands
        uppercase via command_builder. We also do not block for responses here
        (GUI can manage flow). A synchronous helper can be added later.
        """
        if not self._ser:
            self.error_occurred.emit("Not connected.")
            return False

        data = (cmd + "\n").encode("utf-8")
        try:
            with self._write_lock:
                self._ser.write(data)
            return True
        except Exception as e:
            self.error_occurred.emit(f"Write failed: {e}")
            return False

    # ---- Convenience ----
    @staticmethod
    def available_ports() -> List[str]:
        return list_serial_ports()

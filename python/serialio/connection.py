from __future__ import annotations

from typing import Optional, Callable, List

from PyQt5.QtCore import QObject, pyqtSignal, QThread

from .serial_io import ThreadSafeSerialIO, SerialIOError, list_serial_ports


class _ReaderWorker(QObject):
    """
    Runs in its own QThread. Continuously reads lines from the serial I/O
    and emits them via a Qt signal.
    """

    line_received = pyqtSignal(str)
    error = pyqtSignal(str)

    def __init__(self, serial_io: ThreadSafeSerialIO, parent: Optional[QObject] = None):
        super().__init__(parent)
        self._serial_io = serial_io

    def start(self):
        """Main reading loop - runs in the worker thread."""
        try:
            while True:
                try:
                    line = self._serial_io.readline()
                    if line is None:  # Shutdown requested
                        break
                    if line:  # Non-empty line
                        self.line_received.emit(line)
                except SerialIOError as e:
                    self.error.emit(str(e))
                    break
        except Exception as e:
            self.error.emit(f"Unexpected error in reader: {e!r}")

    def stop(self):
        """Request the reader to stop."""
        self._serial_io.request_shutdown()


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
        serial_class=None,  # For backwards compatibility with tests
        read_timeout: float = 0.1,
    ):
        """
        serial_class is injectable for tests (e.g., a FakeSerial).
        """
        super().__init__(parent)

        # Create the thread-safe serial I/O handler
        if serial_class is not None:
            # For tests - pass the serial class to ThreadSafeSerialIO
            self._serial_io = ThreadSafeSerialIO(
                serial_class=serial_class, read_timeout=read_timeout
            )
        else:
            self._serial_io = ThreadSafeSerialIO(read_timeout=read_timeout)

        self._reader_thread: Optional[QThread] = None
        self._reader: Optional[_ReaderWorker] = None

    # ---- Lifecycle ----
    def connect(self, port: str, baud: int = 9600) -> bool:
        """
        Open the serial port and start the reader thread.
        Returns True if connected, False otherwise.
        """
        self.disconnect()  # idempotent

        try:
            success = self._serial_io.connect(port, baud)
            if not success:
                self.connection_status.emit(False)
                return False
        except SerialIOError as e:
            self.error_occurred.emit(str(e))
            self.connection_status.emit(False)
            return False

        # Set up reader worker + thread
        self._reader_thread = QThread()
        self._reader = _ReaderWorker(self._serial_io)
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
        # Stop reader first
        if self._reader:
            self._reader.stop()

        # Quit thread and wait for it to finish
        if self._reader_thread:
            self._reader_thread.quit()
            self._reader_thread.wait(2000)  # Wait up to 2 seconds

        self._reader_thread = None
        self._reader = None

        # Close the serial I/O (this will signal shutdown and close the port)
        self._serial_io.disconnect()

        self.connection_status.emit(False)

    def is_connected(self) -> bool:
        return self._serial_io.is_connected()

    # ---- Writing ----
    def send_command(self, cmd: str) -> bool:
        """
        Thread-safe write of a single command. Appends '\n'.
        Returns True on success, False if not connected or on error.

        NOTE: Per your protocol, we do not force uppercase here; build commands
        uppercase via commands module. We also do not block for responses here
        (GUI can manage flow). A synchronous helper can be added later.
        """
        try:
            self._serial_io.write_line(cmd)
            return True
        except SerialIOError as e:
            self.error_occurred.emit(str(e))
            return False

    def write(self, data: str) -> bool:
        """
        Thread-safe write of data. Appends '\n'.
        Returns True on success, False if not connected or on error.

        This is an alias for send_command for compatibility with existing code.
        """
        return self.send_command(data)

    # ---- Convenience ----
    @staticmethod
    def available_ports() -> List[str]:
        return list_serial_ports()

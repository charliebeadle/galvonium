from __future__ import annotations

import threading
from typing import Optional, Callable, List

import serial as pyserial
from serial.tools import list_ports as pyserial_list_ports


def list_serial_ports() -> List[str]:
    """
    Return a list of available serial port device names (e.g., COM3, COM4).
    """
    return [p.device for p in pyserial_list_ports.comports()]


class SerialIOError(Exception):
    """Exception raised for serial I/O operations."""
    pass


class ThreadSafeSerialIO:
    """
    Thread-safe, low-level serial I/O operations.
    
    Handles port connection, reading, and writing with proper synchronization.
    Does not handle Qt signals or threading - that's the responsibility of higher layers.
    """
    
    def __init__(
        self, 
        serial_class: Callable[..., pyserial.Serial] = pyserial.Serial,
        read_timeout: float = 0.1
    ):
        """
        Args:
            serial_class: Injectable for tests (e.g., a FakeSerial)
            read_timeout: Timeout for read operations
        """
        self._serial_class = serial_class
        self._read_timeout = read_timeout
        
        # Thread synchronization
        self._connection_lock = threading.RLock()
        self._shutdown_event = threading.Event()
        
        # Connection state
        self._serial_port: Optional[pyserial.Serial] = None
        self._port_name: Optional[str] = None
        self._baud_rate: Optional[int] = None
    
    def connect(self, port: str, baud: int = 9600) -> bool:
        """
        Open the serial port.
        
        Args:
            port: Serial port name (e.g., "COM3")
            baud: Baud rate
            
        Returns:
            True if connected successfully, False otherwise
            
        Raises:
            SerialIOError: If connection fails
        """
        with self._connection_lock:
            # Close existing connection if any
            self._close_port()
            
            try:
                self._serial_port = self._serial_class(port, baud, timeout=self._read_timeout)
                self._port_name = port
                self._baud_rate = baud
                self._shutdown_event.clear()
                return True
            except Exception as e:
                self._serial_port = None
                self._port_name = None
                self._baud_rate = None
                raise SerialIOError(f"Failed to open {port} @ {baud}: {e}") from e
    
    def disconnect(self):
        """Close the serial port and signal shutdown to any readers."""
        with self._connection_lock:
            self._shutdown_event.set()
            self._close_port()
    
    def _close_port(self):
        """Internal method to close the port. Must be called with _connection_lock held."""
        if self._serial_port:
            try:
                self._serial_port.close()
            except Exception:
                pass  # Best effort cleanup
            finally:
                self._serial_port = None
                self._port_name = None
                self._baud_rate = None
    
    def is_connected(self) -> bool:
        """Check if the serial port is connected."""
        with self._connection_lock:
            return self._serial_port is not None and not self._shutdown_event.is_set()
    
    def write(self, data: bytes) -> int:
        """
        Write data to the serial port.
        
        Args:
            data: Bytes to write
            
        Returns:
            Number of bytes written
            
        Raises:
            SerialIOError: If not connected or write fails
        """
        with self._connection_lock:
            if not self._serial_port or self._shutdown_event.is_set():
                raise SerialIOError("Not connected")
            
            try:
                return self._serial_port.write(data)
            except Exception as e:
                raise SerialIOError(f"Write failed: {e}") from e
    
    def write_line(self, text: str, encoding: str = 'utf-8') -> int:
        """
        Write a text line to the serial port (appends newline).
        
        Args:
            text: Text to write
            encoding: Text encoding
            
        Returns:
            Number of bytes written
        """
        data = (text + '\n').encode(encoding)
        return self.write(data)
    
    def readline(self, encoding: str = 'utf-8') -> Optional[str]:
        """
        Read a line from the serial port.
        
        Args:
            encoding: Text encoding for decoding
            
        Returns:
            Decoded line with newlines stripped, or None if shutdown requested
            
        Raises:
            SerialIOError: If not connected or read fails
        """
        with self._connection_lock:
            if not self._serial_port:
                raise SerialIOError("Not connected")
            
            # Check shutdown outside the lock to allow interruption
            if self._shutdown_event.is_set():
                return None
        
        try:
            # Don't hold the lock during the blocking read operation
            line = self._serial_port.readline()
            
            # Check shutdown again after read
            if self._shutdown_event.is_set():
                return None
            
            if line:
                return line.decode(encoding, errors='replace').rstrip('\r\n')
            return ""  # Empty string for timeout/no data
            
        except Exception as e:
            raise SerialIOError(f"Read failed: {e}") from e
    
    def get_connection_info(self) -> tuple[Optional[str], Optional[int]]:
        """Get current connection information."""
        with self._connection_lock:
            return self._port_name, self._baud_rate
    
    def request_shutdown(self):
        """Signal that any ongoing read operations should stop."""
        self._shutdown_event.set()

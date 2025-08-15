"""
Main controller for Galvonium Laser GUI.
Coordinates between GUI components and serial communication.
"""

from typing import Optional, List, Callable
from PyQt5.QtCore import QObject, pyqtSignal

from models.buffer_model import BufferData
from serialio.connection import SerialConnection
from serialio.commands import (
    cmd_write,
    cmd_dump,
    cmd_swap,
    cmd_clear,
    cmd_size,
    build_write_sequence_from_buffer,
)
from serialio.parser import is_eoc, accumulate_dump_lines, parse_dump_text


class GalvoController(QObject):
    """
    Main controller that coordinates between GUI and serial communication.

    Signals:
        buffer_data_changed: Emitted when buffer data is updated
        connection_status_changed: Emitted when connection status changes
        operation_progress: Emitted during long operations
        error_occurred: Emitted when errors occur
        status_message: Emitted for status updates
    """

    # Buffer and data signals
    buffer_data_changed = pyqtSignal(BufferData)
    connection_status_changed = pyqtSignal(bool)

    # Operation signals
    operation_progress = pyqtSignal(int, str)  # progress, message
    error_occurred = pyqtSignal(str)
    status_message = pyqtSignal(str, int)  # message, timeout_ms

    # Raw data signals
    data_received = pyqtSignal(str)  # raw line received from Arduino

    def __init__(self, parent=None):
        super().__init__(parent)

        # Core components
        self._serial_conn = SerialConnection(parent=self)
        self._buffer_data = BufferData()
        self._is_connected = False

        # Response accumulation for multi-line responses
        self._response_lines: List[str] = []
        self._current_operation: Optional[str] = None

        # Wire serial connection signals
        self._wire_serial_signals()

    def _wire_serial_signals(self):
        """Connect serial connection signals to internal handlers."""
        self._serial_conn.data_received.connect(self._on_data_received)
        self._serial_conn.connection_status.connect(self._on_connection_status)
        self._serial_conn.error_occurred.connect(self._on_serial_error)

    # === Connection Management ===

    def connect_to_device(self, port: str, baud: int) -> bool:
        """
        Connect to the Arduino device.

        Args:
            port: Serial port name (e.g., "COM3")
            baud: Baud rate

        Returns:
            True if connection successful, False otherwise
        """
        try:
            self.status_message.emit(f"Connecting to {port} @ {baud}...", 0)
            success = self._serial_conn.connect(port, baud)

            if success:
                self.status_message.emit(f"Connected to {port}", 3000)
            else:
                self.status_message.emit("Connection failed", 3000)

            return success

        except Exception as e:
            self.error_occurred.emit(f"Connection error: {e}")
            self.status_message.emit("Connection failed", 3000)
            return False

    def disconnect_from_device(self):
        """Disconnect from the Arduino device."""
        if self._is_connected:
            self._serial_conn.disconnect()
            self.status_message.emit("Disconnected", 3000)

    def is_connected(self) -> bool:
        """Check if currently connected to device."""
        return self._is_connected

    def get_available_ports(self) -> List[str]:
        """Get list of available serial ports."""
        try:
            from serialio.connection import list_serial_ports

            return list_serial_ports()
        except ImportError:
            return []

    # === Buffer Operations ===

    def get_buffer_data(self) -> BufferData:
        """Get current buffer data."""
        return self._buffer_data

    def clear_buffer(self):
        """Clear the current buffer data."""
        self._buffer_data.clear()
        self.buffer_data_changed.emit(self._buffer_data)
        self.status_message.emit("Buffer cleared", 2000)

    def set_buffer_step(self, index: int, x: int, y: int, flags: int):
        """
        Set a single buffer step.

        Args:
            index: Step index (0-255)
            x: X coordinate (0-255)
            y: Y coordinate (0-255)
            flags: Control flags (0-255)
        """
        try:
            self._buffer_data.set_step(index, x, y, flags)
            self.buffer_data_changed.emit(self._buffer_data)
        except (ValueError, IndexError) as e:
            self.error_occurred.emit(f"Invalid step data: {e}")

    def load_buffer_from_device(self, buffer_name: str = "INACTIVE") -> bool:
        """
        Load buffer data from Arduino device.

        Args:
            buffer_name: Buffer to load ("ACTIVE" or "INACTIVE")

        Returns:
            True if successful, False otherwise
        """
        if not self._is_connected:
            self.error_occurred.emit("Not connected to device")
            return False

        try:
            self._current_operation = "load"
            self._response_lines.clear()
            self.operation_progress.emit(0, f"Loading {buffer_name} buffer...")

            # Send DUMP command
            command = cmd_dump(buffer_name)
            if not self._serial_conn.write(command):
                self.error_occurred.emit("Failed to send DUMP command")
                return False

            return True

        except Exception as e:
            self.error_occurred.emit(f"Load error: {e}")
            return False

    def write_buffer_to_device(self, buffer_name: str = "INACTIVE") -> bool:
        """
        Write current buffer data to Arduino device.

        Args:
            buffer_name: Buffer to write to ("ACTIVE" or "INACTIVE")

        Returns:
            True if successful, False otherwise
        """
        if not self._is_connected:
            self.error_occurred.emit("Not connected to device")
            return False

        try:
            self._current_operation = "write"
            self.operation_progress.emit(0, f"Writing to {buffer_name} buffer...")

            # Generate write sequence
            commands = self._buffer_data.get_write_sequence(buffer_name)
            total_commands = len(commands)

            # Send commands with progress updates
            for i, command in enumerate(commands):
                if not self._serial_conn.write(command):
                    self.error_occurred.emit(f"Failed to send command: {command}")
                    return False

                progress = int((i + 1) / total_commands * 100)
                self.operation_progress.emit(
                    progress, f"Sent {i + 1}/{total_commands} commands"
                )

            self.operation_progress.emit(100, f"Buffer written to {buffer_name}")
            self.status_message.emit(f"Buffer written to {buffer_name}", 3000)
            return True

        except Exception as e:
            self.error_occurred.emit(f"Write error: {e}")
            return False

    def swap_buffers(self) -> bool:
        """
        Swap active and inactive buffers on Arduino.

        Returns:
            True if successful, False otherwise
        """
        if not self._is_connected:
            self.error_occurred.emit("Not connected to device")
            return False

        try:
            command = cmd_swap()
            if self._serial_conn.write(command):
                self.status_message.emit("Buffers swapped", 2000)
                return True
            else:
                self.error_occurred.emit("Failed to send SWAP command")
                return False

        except Exception as e:
            self.error_occurred.emit(f"Swap error: {e}")
            return False

    # === Command Execution ===

    def send_command(self, command: str) -> bool:
        """
        Send a raw command to the Arduino.

        Args:
            command: Command string to send

        Returns:
            True if sent successfully, False otherwise
        """
        if not self._is_connected:
            self.error_occurred.emit("Not connected to device")
            return False

        try:
            if self._serial_conn.write(command):
                return True
            else:
                self.error_occurred.emit("Failed to send command")
                return False

        except Exception as e:
            self.error_occurred.emit(f"Command error: {e}")
            return False

    # === Serial Event Handlers ===

    def _on_data_received(self, line: str):
        """Handle data received from serial connection."""
        # Always emit raw data for terminal display
        self.data_received.emit(line.strip())

        if self._current_operation == "load":
            self._handle_load_response(line)
        else:
            # For other operations, just emit the data
            self.status_message.emit(f"Received: {line.strip()}", 1000)

    def _handle_load_response(self, line: str):
        """Handle response during buffer load operation."""
        if is_eoc(line):
            # End of communication - process accumulated response
            if self._response_lines:
                try:
                    response_text = accumulate_dump_lines(self._response_lines)
                    self._buffer_data = parse_dump_text(response_text)
                    self.buffer_data_changed.emit(self._buffer_data)
                    self.operation_progress.emit(100, "Buffer loaded successfully")
                    self.status_message.emit("Buffer loaded from device", 3000)
                except Exception as e:
                    self.error_occurred.emit(f"Failed to parse buffer data: {e}")
                finally:
                    self._response_lines.clear()
                    self._current_operation = None
        else:
            # Accumulate response lines
            self._response_lines.append(line)
            progress = min(len(self._response_lines) * 2, 90)  # Rough progress estimate
            self.operation_progress.emit(
                progress, f"Loading... ({len(self._response_lines)} lines)"
            )

    def _on_connection_status(self, connected: bool):
        """Handle connection status changes."""
        self._is_connected = connected
        self.connection_status_changed.emit(connected)

        if connected:
            self.status_message.emit("Device connected", 2000)
        else:
            self.status_message.emit("Device disconnected", 2000)

    def _on_serial_error(self, error: str):
        """Handle serial connection errors."""
        self.error_occurred.emit(f"Serial error: {error}")
        self._is_connected = False
        self.connection_status_changed.emit(False)

    # === Cleanup ===

    def cleanup(self):
        """Clean up resources before shutdown."""
        if self._is_connected:
            self.disconnect_from_device()

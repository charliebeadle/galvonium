"""
Connection manager for GUI components.
Handles connection state and coordinates between controller and UI.
"""

from typing import Optional, List
from PyQt5.QtCore import QObject, pyqtSignal, QTimer

from controllers.galvo_controller import GalvoController


class ConnectionManager(QObject):
    """
    Manages connection state and coordinates between controller and GUI.

    Signals:
        connection_status_changed: Emitted when connection status changes
        ports_updated: Emitted when available ports list is updated
        status_message: Emitted for status updates
        error_occurred: Emitted when errors occur
    """

    connection_status_changed = pyqtSignal(bool)
    ports_updated = pyqtSignal(list)
    status_message = pyqtSignal(str, int)  # message, timeout_ms
    error_occurred = pyqtSignal(str)

    def __init__(self, controller: GalvoController, parent=None):
        super().__init__(parent)
        self._controller = controller
        self._is_connected = False
        self._current_port: Optional[str] = None
        self._current_baud: Optional[int] = None

        # Port refresh timer
        self._port_refresh_timer = QTimer(self)
        self._port_refresh_timer.timeout.connect(self._refresh_ports)
        self._port_refresh_timer.start(5000)  # Refresh every 5 seconds

        # Wire controller signals
        self._wire_controller_signals()

        # Initial port refresh
        self._refresh_ports()

    def _wire_controller_signals(self):
        """Connect controller signals to internal handlers."""
        self._controller.connection_status_changed.connect(self._on_connection_status)
        self._controller.status_message.connect(self.status_message)
        self._controller.error_occurred.connect(self.error_occurred)

    # === Connection Management ===

    def connect_to_device(self, port: str, baud: int) -> bool:
        """
        Connect to device using the specified port and baud rate.

        Args:
            port: Serial port name
            baud: Baud rate

        Returns:
            True if connection initiated successfully
        """
        if self._is_connected:
            self.error_occurred.emit("Already connected to device")
            return False

        try:
            # Extract port name from display string (e.g., "COM3 — Arduino Uno" -> "COM3")
            if " — " in port:
                port = port.split(" — ")[0]

            self._current_port = port
            self._current_baud = baud

            return self._controller.connect_to_device(port, baud)

        except Exception as e:
            self.error_occurred.emit(f"Connection error: {e}")
            return False

    def disconnect_from_device(self):
        """Disconnect from the current device."""
        if self._is_connected:
            self._controller.disconnect_from_device()

    def is_connected(self) -> bool:
        """Check if currently connected."""
        return self._is_connected

    def get_current_port(self) -> Optional[str]:
        """Get the currently connected port."""
        return self._current_port

    def get_current_baud(self) -> Optional[int]:
        """Get the current baud rate."""
        return self._current_baud

    # === Port Management ===

    def refresh_ports(self):
        """Manually refresh the available ports list."""
        self._refresh_ports()

    def _refresh_ports(self):
        """Refresh the list of available ports."""
        try:
            ports = self._controller.get_available_ports()
            if ports:
                # Format ports for display
                formatted_ports = []
                for port in ports:
                    # Try to get a descriptive name
                    description = self._get_port_description(port)
                    formatted_ports.append(f"{port} — {description}")

                self.ports_updated.emit(formatted_ports)
            else:
                # Fallback for when no ports are detected
                fallback_ports = ["COM3 — Arduino Uno (example)"]
                self.ports_updated.emit(fallback_ports)

        except Exception as e:
            # Fallback on error
            fallback_ports = ["COM3 — Arduino Uno (example)"]
            self.ports_updated.emit(fallback_ports)

    def _get_port_description(self, port: str) -> str:
        """Get a descriptive name for a port."""
        try:
            from serial.tools import list_ports

            for p in list_ports.comports():
                if p.device == port:
                    return p.description or "Unknown Device"
        except ImportError:
            pass

        # Fallback descriptions
        if "COM" in port.upper():
            return "Serial Device"
        elif "USB" in port.upper():
            return "USB Device"
        else:
            return "Unknown Device"

    # === Controller Operations ===

    def get_controller(self) -> GalvoController:
        """Get the underlying controller instance."""
        return self._controller

    def send_command(self, command: str) -> bool:
        """Send a command through the controller."""
        return self._controller.send_command(command)

    def load_buffer_from_device(self, buffer_name: str = "INACTIVE") -> bool:
        """Load buffer from device through the controller."""
        return self._controller.load_buffer_from_device(buffer_name)

    def write_buffer_to_device(self, buffer_name: str = "INACTIVE") -> bool:
        """Write buffer to device through the controller."""
        return self._controller.write_buffer_to_device(buffer_name)

    def swap_buffers(self) -> bool:
        """Swap buffers through the controller."""
        return self._controller.swap_buffers()

    def clear_buffer(self):
        """Clear buffer through the controller."""
        self._controller.clear_buffer()

    # === Event Handlers ===

    def _on_connection_status(self, connected: bool):
        """Handle connection status changes from controller."""
        self._is_connected = connected
        self.connection_status_changed.emit(connected)

        if not connected:
            self._current_port = None
            self._current_baud = None

    # === Cleanup ===

    def cleanup(self):
        """Clean up resources before shutdown."""
        self._port_refresh_timer.stop()
        if self._is_connected:
            self.disconnect_from_device()

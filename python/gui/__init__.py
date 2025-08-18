"""
GUI module for Galvonium Laser Control System.

This module provides the main user interface components for controlling
laser galvo systems through a PyQt5-based graphical interface.

Main Components:
- MainWindow: Primary application window
- ControlPanel: Connection and buffer operation controls
- BufferTableWidget: Data table for buffer management
- TerminalWidget: Command input/output terminal
- ConnectionManager: Connection state management

Usage:
    from gui import MainWindow, ControlPanel, BufferTableWidget
    from gui import TerminalWidget, ConnectionManager
"""

# Main GUI components
from .main_window import MainWindow
from .control_panel import ControlPanel, StatusLED
from .buffer_table import BufferTableWidget, BufferTableModel
from .terminal_widget import TerminalWidget
from .connection_manager import ConnectionManager

# Re-export main classes for convenient access
__all__ = [
    "MainWindow",
    "ControlPanel",
    "StatusLED",
    "BufferTableWidget",
    "BufferTableModel",
    "TerminalWidget",
    "ConnectionManager",
]

# Version information
__version__ = "1.0.0"



"""
Galvonium Laser Control System - Python Interface

A comprehensive Python application for controlling laser galvo systems through
a graphical user interface with serial communication capabilities.

Main Entry Point:
    from galvonium import main

Key Modules:
    - gui: User interface components
    - controllers: Business logic and coordination
    - models: Data structures and models
    - serialio: Serial communication and commands
    - utils: Utility functions and validators

Usage:
    # Run the main application
    from galvonium import main
    main()

    # Import specific components
    from galvonium.gui import MainWindow
    from galvonium.controllers import GalvoController
    from galvonium.models import BufferData
"""

# Main application entry point
from .laser_gui import main

# Core module imports for convenient access
from . import gui
from . import controllers
from . import models
from . import serialio
from . import utils

# Re-export main entry point and key classes
__all__ = ["main", "gui", "controllers", "models", "serialio", "utils"]

# Application metadata
__version__ = "1.0.0"
__author__ = "Galvonium Team"
__description__ = "Laser Galvo Control System - Python Interface"



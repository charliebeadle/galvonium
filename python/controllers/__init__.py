"""
Controllers package for Galvonium Laser Control System.

Contains business logic and coordination between GUI and serial communication.
Manages the core application state and coordinates between different system components.

Main Components:
- GalvoController: Main controller for galvo system operations

Usage:
    from controllers import GalvoController
    controller = GalvoController()
"""

from .galvo_controller import GalvoController

__all__ = ["GalvoController"]

# Version information
__version__ = "1.0.0"

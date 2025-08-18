"""
Models module for Galvonium Laser Control System.

Provides data structures and models for representing laser galvo system state,
buffer data, and system configuration.

Main Components:
- BufferStep: Individual step in a buffer sequence
- BufferData: Complete buffer containing multiple steps

Usage:
    from models import BufferStep, BufferData
    buffer = BufferData()
    step = BufferStep(x=100, y=200, flags=0)
"""

from .buffer_model import BufferStep, BufferData

__all__ = ["BufferStep", "BufferData"]

# Version information
__version__ = "1.0.0"

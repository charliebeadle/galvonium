"""
Buffer data model for laser galvo control system.
Represents the 256-step buffer with validation.
"""

from typing import List, Tuple, Optional, ClassVar
import re


class BufferStep:
    """Single step in the galvo buffer."""

    def __init__(self, index: int, x: int = 0, y: int = 0, flags: int = 0):
        """
        Initialize a buffer step.

        Args:
            index: Step index (0-255)
            x: X coordinate (0-255)
            y: Y coordinate (0-255)
            flags: Control flags (0-255)
        """
        self.index = index
        self._x = 0
        self._y = 0
        self._flags = 0

        # Use properties to validate
        self.x = x
        self.y = y
        self.flags = flags

    @property
    def x(self) -> int:
        return self._x

    @x.setter
    def x(self, value: int):
        if not 0 <= value <= 255:
            raise ValueError(f"X value {value} out of range (0-255)")
        self._x = value

    @property
    def y(self) -> int:
        return self._y

    @y.setter
    def y(self, value: int):
        if not 0 <= value <= 255:
            raise ValueError(f"Y value {value} out of range (0-255)")
        self._y = value

    @property
    def flags(self) -> int:
        return self._flags

    @flags.setter
    def flags(self, value: int):
        if not 0 <= value <= 255:
            raise ValueError(f"Flags value {value} out of range (0-255)")
        self._flags = value

    @property
    def flags_binary(self) -> str:
        """Return flags as 8-bit binary string."""
        return format(self._flags, "08b")

    def is_empty(self) -> bool:
        """Check if this step is empty (all zeros)."""
        return self._x == 0 and self._y == 0 and self._flags == 0

    def to_write_command(self, target: str = "INACTIVE") -> str:
        """
        Generate WRITE command for this step.

        Args:
            target: Buffer target ("ACTIVE" or "INACTIVE")

        Returns:
            Command string like "WRITE 0 128 64 255 INACTIVE"
        """
        return f"WRITE {self.index} {self.x} {self.y} {self.flags} {target}"

    def __str__(self) -> str:
        return f"Step[{self.index}]: ({self.x}, {self.y}) flags={self.flags_binary}"

    def __repr__(self) -> str:
        return f"BufferStep({self.index}, {self.x}, {self.y}, {self.flags})"


class BufferData:
    """Complete buffer data model with 256 steps."""

    def __init__(self):
        """Initialize empty buffer with 256 steps."""
        self.steps: List[BufferStep] = [BufferStep(i) for i in range(256)]

    def clear(self):
        """Clear all steps to (0, 0, 0)."""
        for step in self.steps:
            step.x = 0
            step.y = 0
            step.flags = 0

    def get_step(self, index: int) -> BufferStep:
        """
        Get step at index.

        Args:
            index: Step index (0-255)

        Returns:
            BufferStep at index

        Raises:
            IndexError: If index out of range
        """
        if not 0 <= index <= 255:
            raise IndexError(f"Index {index} out of range (0-255)")
        return self.steps[index]

    def set_step(self, index: int, x: int, y: int, flags: int):
        """
        Set step values.

        Args:
            index: Step index (0-255)
            x: X coordinate (0-255)
            y: Y coordinate (0-255)
            flags: Control flags (0-255)
        """
        step = self.get_step(index)
        step.x = x
        step.y = y
        step.flags = flags

    def get_last_used_index(self) -> int:
        """
        Find the highest index with non-zero data.

        Returns:
            Index of last non-empty step, or 0 if all empty
        """
        for i in range(255, -1, -1):
            if not self.steps[i].is_empty():
                return i
        return 0

    def get_size_for_write(self) -> int:
        """
        Get the size to use for SIZE command.

        Returns:
            Last used index + 1, minimum 1
        """
        return self.get_last_used_index() + 1

    @classmethod
    def from_dump_response(cls, response: str) -> "BufferData":
        """
        Parse Arduino DUMP response into buffer data.

        Expected format:
        0: 128,64 255
        1: 130,66 255
        ...
        EOC

        Args:
            response: Multi-line DUMP response from Arduino
        """
        # Clear existing data
        buf = cls()

        # Parse each line until EOC
        lines = response.strip().split("\n")

        # Pattern to match: "index: x,y flags"
        pattern = r"(\d+):\s*(\d+),(\d+)\s+(\d+)"

        for line in lines:
            if line.strip() == "EOC":
                break

            match = re.match(pattern, line.strip())
            if match:
                index = int(match.group(1))
                x = int(match.group(2))
                y = int(match.group(3))
                flags = int(match.group(4))

                if 0 <= index <= 255:
                    buf.set_step(index, x, y, flags)

        return buf

    def to_write_commands(self, target: str = "INACTIVE") -> List[str]:
        """
        Generate all WRITE commands for non-empty steps.

        Args:
            target: Buffer target ("ACTIVE" or "INACTIVE")

        Returns:
            List of WRITE command strings
        """
        commands = []
        last_index = self.get_last_used_index()

        for i in range(last_index + 1):
            step = self.steps[i]
            commands.append(step.to_write_command(target))

        return commands

    def to_size_command(self, target: str = "INACTIVE") -> str:
        """
        Generate SIZE command based on last used index.

        Args:
            target: Buffer target ("ACTIVE" or "INACTIVE")

        Returns:
            SIZE command string
        """
        size = self.get_size_for_write()
        return f"SIZE {size} {target}"

    def get_write_sequence(self, target: str = "INACTIVE") -> List[str]:
        """
        Get complete command sequence to write buffer to Arduino.

        Args:
            target: Buffer target ("ACTIVE" or "INACTIVE")

        Returns:
            List of commands: CLEAR, WRITE commands, SIZE
        """
        commands = [f"CLEAR {target}"]
        commands.extend(self.to_write_commands(target))
        commands.append(self.to_size_command(target))
        return commands

    def load_from_list(self, data: List[Tuple[int, int, int]]):
        """
        Load buffer from list of (x, y, flags) tuples.

        Args:
            data: List of (x, y, flags) tuples, indexed by position
        """
        self.clear()
        for i, (x, y, flags) in enumerate(data[:256]):
            self.set_step(i, x, y, flags)

    def __str__(self) -> str:
        last = self.get_last_used_index()
        return f"BufferData: {last + 1} steps used"

    def __repr__(self) -> str:
        return f"BufferData(size={self.get_size_for_write()})"

from __future__ import annotations

from typing import Iterable, List

# Keep these uppercase; firmware expects uppercase commands.
INACTIVE = "INACTIVE"


def _check_uint8(name: str, v: int):
    if not (0 <= int(v) <= 255):
        raise ValueError(f"{name} must be 0..255, got {v}")


def _check_size(name: str, v: int):
    # Your current plan uses up to 256; minimum 1 to keep firmware happy on empty.
    if not (1 <= int(v) <= 256):
        raise ValueError(f"{name} must be 1..256, got {v}")


def cmd_write(idx: int, x: int, y: int, flags: int, buffer: str = INACTIVE) -> str:
    _check_uint8("index", idx)
    _check_uint8("x", x)
    _check_uint8("y", y)
    _check_uint8("flags", flags)
    buf = (buffer or INACTIVE).upper()
    return f"WRITE {int(idx)} {int(x)} {int(y)} {int(flags)} {buf}"


def cmd_dump(buffer: str = INACTIVE) -> str:
    buf = (buffer or INACTIVE).upper()
    return f"DUMP {buf}"


def cmd_swap() -> str:
    return "SWAP"


def cmd_clear(buffer: str = INACTIVE) -> str:
    buf = (buffer or INACTIVE).upper()
    return f"CLEAR {buf}"


def cmd_size(n: int, buffer: str = INACTIVE) -> str:
    _check_size("size", n)
    buf = (buffer or INACTIVE).upper()
    return f"SIZE {int(n)} {buf}"


# Optional helper if you want the serial layer to build a full sequence
# directly from your BufferData (if present).
def build_write_sequence_from_buffer(buffer_data) -> List[str]:
    """
    Build a full 'CLEAR -> WRITE* -> SIZE' sequence for the INACTIVE buffer
    using your BufferData instance (which should already be validated).

    buffer_data must provide:
        - get_last_used_index() -> int
        - steps list/iterable (0..255) with .x, .y, .flags
    """
    cmds: List[str] = [cmd_clear(INACTIVE)]

    last = int(buffer_data.get_last_used_index())  # may be -1 for empty
    n = max(1, last + 1)

    for i in range(n):
        step = buffer_data.steps[i]
        cmds.append(cmd_write(i, step.x, step.y, step.flags, INACTIVE))

    cmds.append(cmd_size(n, INACTIVE))
    return cmds

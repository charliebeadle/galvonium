from __future__ import annotations

from typing import Iterable, List

# We delegate actual DUMP parsing to your BufferData model,
# per your progress report design.
from models.buffer_model import BufferData


def is_eoc(line: str) -> bool:
    return line.strip().upper() == "EOC"


def accumulate_dump_lines(lines: Iterable[str]) -> str:
    """
    Join lines until an EOC is seen (EOC is NOT included).
    Returns a single text blob suitable for BufferData.from_dump_response().
    """
    collected: List[str] = []
    for line in lines:
        if is_eoc(line):
            break
        collected.append(line)
    return "\n".join(collected)


def parse_dump_text(text: str) -> BufferData:
    """
    Create BufferData from DUMP text (no EOC line).
    """
    return BufferData.from_dump_response(text)

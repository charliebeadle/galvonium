# SerialIO Module

The `serialio` module provides a robust, thread-safe interface for communicating with Arduino-based galvo control hardware via serial communication. It's designed to work seamlessly with PyQt5 applications while maintaining clean separation of concerns.

## Overview

The module consists of several layers that work together to provide a complete serial communication solution:

- **Low-level I/O**: `ThreadSafeSerialIO` handles raw serial port operations
- **Qt Integration**: `SerialConnection` provides Qt-friendly signals and threading
- **Command Generation**: `commands.py` creates properly formatted commands for the Arduino
- **Response Parsing**: `parser.py` handles incoming data from the Arduino

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   GUI Layer     │    │  SerialConnection│    │ ThreadSafeSerial│
│   (PyQt5)       │◄──►│  (Qt Wrapper)    │◄──►│  IO (Low-level) │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │                        │
                              ▼                        ▼
                       ┌──────────────────┐    ┌─────────────────┐
                       │   Commands       │    │   Parser        │
                       │   (Protocol)     │    │   (Responses)   │
                       └──────────────────┘    └─────────────────┘
```

## Key Features

- **Thread-safe operations** with proper synchronization
- **Qt integration** with signals for data reception and error handling
- **Automatic port discovery** and management
- **Robust error handling** with custom exception types
- **Test-friendly design** with injectable dependencies
- **Protocol compliance** with Arduino firmware expectations

## Module Components

### SerialConnection (Qt Interface)

High-level Qt-friendly interface that manages the connection lifecycle and provides signals for data reception.

**Signals:**

- `data_received(str)`: Emitted when a line of data is received
- `connection_status(bool)`: Emitted when connection status changes
- `error_occurred(str)`: Emitted when errors occur

**Key Methods:**

- `connect(port: str, baud: int = 9600) -> bool`: Open serial port and start reader thread
- `disconnect()`: Close serial port and stop reader thread
- `write(data: str) -> bool`: Send data to Arduino (thread-safe)
- `write_lines(lines: List[str]) -> bool`: Send multiple lines of data
- `is_connected() -> bool`: Return current connection status

**Constructor:**

```python
def __init__(
    self,
    parent: Optional[QObject] = None,
    *,
    serial_class=None,  # For testing
    read_timeout: float = 0.1,
):
```

### ThreadSafeSerialIO (Low-level I/O)

Thread-safe, low-level serial I/O operations with proper synchronization.

**Key Methods:**

- `connect(port: str, baud: int = 9600) -> bool`: Open serial port
- `disconnect()`: Close port and signal shutdown
- `write(data: str) -> bool`: Send string data
- `write_bytes(data: bytes) -> bool`: Send raw bytes
- `readline() -> Optional[str]`: Read a line from serial port
- `read_bytes(size: int = 1) -> Optional[bytes]`: Read specified bytes
- `request_shutdown()`: Signal shutdown request
- `is_shutdown_requested() -> bool`: Check shutdown status

**Thread Safety:**

- Uses `threading.RLock()` for connection state changes
- Uses `threading.Event()` for graceful shutdown
- All I/O operations are properly synchronized

### Commands (Protocol)

Functions for generating properly formatted commands to communicate with Arduino firmware.

**Command Functions:**

- `cmd_write(idx: int, x: int, y: int, flags: int, buffer: str = INACTIVE) -> str`
- `cmd_dump(buffer: str = INACTIVE) -> str`
- `cmd_swap() -> str`
- `cmd_clear(buffer: str = INACTIVE) -> str`
- `cmd_size(n: int, buffer: str = INACTIVE) -> str`

**Helper Functions:**

- `build_write_sequence_from_buffer(buffer_data) -> List[str]`: Build complete command sequence

**Constants:**

- `INACTIVE = "INACTIVE"`: Default buffer identifier

**Parameter Validation:**

- All parameters automatically converted to integers
- Index, coordinates, flags: 0-255 range
- Size: 1-256 range
- Buffer names automatically converted to uppercase

### Parser (Response Handling)

Utilities for parsing responses from Arduino firmware.

**Functions:**

- `is_eoc(line: str) -> bool`: Check if line indicates end of communication
- `accumulate_dump_lines(lines: Iterable[str]) -> str`: Collect lines until EOC marker
- `parse_dump_text(text: str) -> BufferData`: Parse DUMP response text into BufferData

**Response Format:**

```
line1
line2
line3
...
EOC
```

**DUMP Response Format:**

```
index x y flags
index x y flags
...
EOC
```

## Protocol Details

The module communicates with Arduino firmware using a simple text-based protocol:

- **WRITE**: Set galvo position at specific buffer index
- **DUMP**: Retrieve buffer contents
- **SWAP**: Swap active and inactive buffers
- **CLEAR**: Clear specified buffer
- **SIZE**: Set buffer size

All commands are sent as plain text with parameters separated by spaces.

## Error Handling

### SerialIOError

Custom exception class for serial I/O operations.

### Error Scenarios

- **Connection failures**: Port not found, access denied, etc.
- **I/O failures**: Write errors, read timeouts, etc.
- **Resource errors**: Port already in use, insufficient permissions

### Error Handling Patterns

```python
try:
    if not conn.connect("COM3", 9600):
        return False
    # ... operations
except SerialIOError as e:
    print(f"Connection error: {e}")
    return False
```

## Threading Model

The module uses a producer-consumer pattern:

1. **Main thread**: Handles UI and sends commands
2. **Reader thread**: Continuously reads from serial port
3. **Thread-safe I/O**: Synchronized access to serial port
4. **Signal-based communication**: Qt signals for cross-thread communication

**Thread Safety:**

- Write operations: Thread-safe and can be called from any thread
- Connection management: Thread-safe connection/disconnection
- Signal emission: Automatically handled by Qt's event system

## Testing

The module is designed with testing in mind:

**Injectable Dependencies:**

```python
class MockSerial:
    def __init__(self, port, baud, timeout):
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.written_data = []
        self.read_queue = ["OK\n", "DATA\n"]

    def write(self, data):
        self.written_data.append(data)
        return len(data)

    def readline(self):
        if self.read_queue:
            return self.read_queue.pop(0).encode()
        return b""

    def close(self):
        pass

# Use in tests
conn = SerialConnection(serial_class=MockSerial)
```

**Test Patterns:**

```python
def test_command_generation():
    assert cmd_write(0, 128, 128, 0) == "WRITE 0 128 128 0 INACTIVE"
    assert cmd_dump("ACTIVE") == "DUMP ACTIVE"
    assert cmd_swap() == "SWAP"

def test_parser_functions():
    assert is_eoc("EOC") == True
    assert is_eoc("DUMP") == False

    lines = ["line1", "line2", "EOC"]
    result = accumulate_dump_lines(lines)
    assert result == "line1\nline2"
```

## Usage Examples

### Basic Connection

```python
from serialio import SerialConnection

conn = SerialConnection()
conn.data_received.connect(print)
conn.connection_status.connect(lambda x: print(f"Connected: {x}"))

if conn.connect("COM3", 9600):
    conn.write("DUMP INACTIVE")
    # Data will be printed via signal
```

### Command Generation

```python
from serialio.commands import cmd_write, cmd_dump, cmd_swap

write_cmd = cmd_write(0, 128, 128, 0)  # "WRITE 0 128 128 0 INACTIVE"
dump_cmd = cmd_dump()                    # "DUMP INACTIVE"
swap_cmd = cmd_swap()                    # "SWAP"
```

### Response Parsing

```python
from serialio.parser import is_eoc, accumulate_dump_lines

class ArduinoHandler:
    def __init__(self):
        self.conn = SerialConnection()
        self.response_lines = []
        self.conn.data_received.connect(self.on_data_received)

    def on_data_received(self, line):
        self.response_lines.append(line)
        if is_eoc(line):
            self.process_response()

    def process_response(self):
        response_text = accumulate_dump_lines(self.response_lines)
        print(f"Complete response: {response_text}")
        self.response_lines = []
```

### Complete Workflow

```python
from serialio import SerialConnection, cmd_write, cmd_dump
from serialio.parser import is_eoc, accumulate_dump_lines

class GalvoController:
    def __init__(self):
        self.conn = SerialConnection()
        self.response_lines = []
        self.conn.data_received.connect(self.on_data_received)

    def on_data_received(self, line):
        self.response_lines.append(line)
        if is_eoc(line):
            self.process_response()

    def process_response(self):
        response_text = accumulate_dump_lines(self.response_lines)
        if response_text:
            print("Buffer contents:")
            for line in response_text.split('\n'):
                if line.strip():
                    parts = line.split()
                    if len(parts) == 4:
                        idx, x, y, flags = parts
                        print(f"  Step {idx}: ({x}, {y}) flags={flags}")
        self.response_lines = []

    def upload_pattern(self, positions):
        if not self.conn.is_connected():
            return False

        # Clear buffer
        if not self.conn.write("CLEAR INACTIVE"):
            return False

        # Upload positions
        for i, (x, y, flags) in enumerate(positions):
            cmd = cmd_write(i, x, y, flags)
            if not self.conn.write(cmd):
                return False

        # Set buffer size
        size_cmd = f"SIZE {len(positions)} INACTIVE"
        return self.conn.write(size_cmd)
```

## Performance Considerations

### Timeout Settings

- **Low timeout (0.1s)**: Responsive UI, higher CPU usage
- **High timeout (1.0s)**: Lower CPU usage, less responsive
- **Default (0.1s)**: Good balance for most applications

### Memory Management

- **Response accumulation**: Lines stored until EOC
- **Buffer cleanup**: Always disconnect to free resources
- **Signal handling**: Connect signals early to avoid missing data

### Threading

- **Write operations**: Thread-safe
- **Read operations**: Handled in separate thread
- **Signal emission**: Automatic cross-thread communication

## Common Issues and Solutions

### Connection Fails

- Check if port exists and is available
- Verify baud rate matches Arduino firmware
- Ensure no other application is using the port
- Check permissions and access rights

### No Data Received

- Verify Arduino is sending data
- Check if signals are properly connected
- Ensure connection is established before sending commands
- Check Arduino firmware and wiring

### Threading Issues

- Don't call Qt methods from non-main threads
- Use signals for cross-thread communication
- Avoid blocking operations in signal handlers
- Always disconnect in finally blocks

### Memory Leaks

- Always call disconnect() when done
- Don't store references to connection objects
- Use weak references for long-lived objects
- Monitor signal connections and disconnect properly

## Dependencies

- `PyQt5`: For Qt integration and threading
- `pyserial`: For low-level serial communication
- `typing`: For type hints and annotations

## Integration Notes

### With Buffer Models

The parser integrates with `BufferData` models from `models.buffer_model`:

```python
from models.buffer_model import BufferData
buffer_data = parse_dump_text(response_text)
```

### With GUI Components

Designed to work with PyQt5 applications:

- Signals automatically handled by Qt's event system
- Thread-safe operations for responsive UI
- Proper cleanup on application shutdown

### With Test Frameworks

Injectable serial classes for easy mocking:

- Unit tests can use MockSerial classes
- Integration tests can use real hardware
- Clean separation of concerns for testing

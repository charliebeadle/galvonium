# SerialIO Module API Reference

This document provides a comprehensive reference for all public APIs in the `serialio` module, focused on integration and technical details.

## Module Imports

```python
from serialio import (
    SerialConnection,
    list_serial_ports,
    cmd_write,
    cmd_dump,
    cmd_swap,
    cmd_clear,
    cmd_size,
    build_write_sequence_from_buffer,
    is_eoc,
    accumulate_dump_lines,
    parse_dump_text,
)
```

## SerialConnection

### Class Definition

```python
class SerialConnection(QObject):
    """Qt-friendly serial connection with a QThread reader and thread-safe writes."""
```

### Constructor

```python
def __init__(
    self,
    parent: Optional[QObject] = None,
    *,
    serial_class=None,
    read_timeout: float = 0.1,
):
```

**Parameters:**

- `parent`: Optional QObject parent
- `serial_class`: Injectable serial class for testing
- `read_timeout`: Read timeout in seconds (default: 0.1)

### Signals

```python
data_received = pyqtSignal(str)           # Emitted when data is received
connection_status = pyqtSignal(bool)      # Emitted when connection status changes
error_occurred = pyqtSignal(str)          # Emitted when errors occur
```

### Methods

#### Connection Management

```python
def connect(self, port: str, baud: int = 9600) -> bool:
    """Open serial port and start reader thread."""

def disconnect(self) -> None:
    """Close serial port and stop reader thread."""

def is_connected(self) -> bool:
    """Return current connection status."""
```

#### Data Communication

```python
def write(self, data: str) -> bool:
    """Send data to Arduino."""

def write_lines(self, lines: List[str]) -> bool:
    """Send multiple lines of data."""
```

## ThreadSafeSerialIO

### Class Definition

```python
class ThreadSafeSerialIO:
    """Thread-safe, low-level serial I/O operations."""
```

### Constructor

```python
def __init__(
    self,
    serial_class: Callable[..., pyserial.Serial] = pyserial.Serial,
    read_timeout: float = 0.1,
):
```

**Parameters:**

- `serial_class`: Injectable serial class constructor
- `read_timeout`: Read timeout in seconds

### Methods

#### Connection Management

```python
def connect(self, port: str, baud: int = 9600) -> bool:
    """Open serial port."""

def disconnect(self) -> None:
    """Close serial port and signal shutdown."""

def is_connected(self) -> bool:
    """Return connection status."""

def get_port_info(self) -> Optional[Tuple[str, int]]:
    """Get current port name and baud rate."""
```

#### Data I/O

```python
def write(self, data: str) -> bool:
    """Send string data."""

def write_bytes(self, data: bytes) -> bool:
    """Send raw bytes."""

def readline(self) -> Optional[str]:
    """Read a line from serial port."""

def read_bytes(self, size: int = 1) -> Optional[bytes]:
    """Read specified number of bytes."""
```

#### Threading Control

```python
def request_shutdown(self) -> None:
    """Signal shutdown request."""

def is_shutdown_requested(self) -> bool:
    """Check if shutdown was requested."""
```

## Commands

### Functions

#### Command Generation

```python
def cmd_write(idx: int, x: int, y: int, flags: int, buffer: str = INACTIVE) -> str:
    """Generate WRITE command."""

def cmd_dump(buffer: str = INACTIVE) -> str:
    """Generate DUMP command."""

def cmd_swap() -> str:
    """Generate SWAP command."""

def cmd_clear(buffer: str = INACTIVE) -> str:
    """Generate CLEAR command."""

def cmd_size(n: int, buffer: str = INACTIVE) -> str:
    """Generate SIZE command."""
```

#### Helper Functions

```python
def build_write_sequence_from_buffer(buffer_data) -> List[str]:
    """Build complete command sequence from buffer data."""
```

### Constants

```python
INACTIVE = "INACTIVE"  # Default buffer identifier
```

## Parser

### Functions

```python
def is_eoc(line: str) -> bool:
    """Check if line indicates end of communication."""

def accumulate_dump_lines(lines: Iterable[str]) -> str:
    """Collect lines until EOC marker."""

def parse_dump_text(text: str) -> BufferData:
    """Parse DUMP response text into BufferData."""
```

## Exceptions

### SerialIOError

```python
class SerialIOError(Exception):
    """Exception raised for serial I/O operations."""
    pass
```

## Utility Functions

### Port Discovery

```python
def list_serial_ports() -> List[str]:
    """Return list of available serial port device names."""
```

**Returns:**

- List of port names (e.g., ["COM3", "COM4", "/dev/ttyUSB0"])

## Complete Integration Example

```python
from serialio import SerialConnection, cmd_dump
from serialio.parser import is_eoc, accumulate_dump_lines

class ArduinoController:
    def __init__(self):
        self.conn = SerialConnection()
        self.response_lines = []

        # Connect signals
        self.conn.data_received.connect(self.on_data_received)
        self.conn.connection_status.connect(self.on_connection_changed)
        self.conn.error_occurred.connect(self.on_error)

    def on_data_received(self, line):
        """Handle incoming data."""
        self.response_lines.append(line)

        if is_eoc(line):
            self.process_response()

    def process_response(self):
        """Process complete response."""
        response_text = accumulate_dump_lines(self.response_lines)
        print(f"Complete response: {response_text}")
        self.response_lines = []

    def on_connection_changed(self, connected):
        """Handle connection status changes."""
        print(f"Connection: {'Connected' if connected else 'Disconnected'}")

    def on_error(self, error_msg):
        """Handle errors."""
        print(f"Error: {error_msg}")

    def connect_and_dump(self, port="COM3"):
        """Connect and request buffer dump."""
        if self.conn.connect(port, 9600):
            self.response_lines = []  # Clear previous response
            self.conn.write(cmd_dump())
        else:
            print("Failed to connect")

    def disconnect(self):
        """Disconnect from Arduino."""
        self.conn.disconnect()
```

## Error Handling Patterns

### Connection Errors

```python
try:
    if not conn.connect("COM3", 9600):
        print("Connection failed")
        return
except SerialIOError as e:
    print(f"Connection error: {e}")
    return
```

### Communication Errors

```python
def robust_write(conn, data):
    """Write data with error handling."""
    if not conn.is_connected():
        print("Not connected")
        return False

    try:
        return conn.write(data)
    except Exception as e:
        print(f"Write error: {e}")
        return False
```

### Response Parsing Errors

```python
def safe_parse_response(lines):
    """Safely parse response."""
    try:
        if not lines:
            return None

        if not any(is_eoc(line) for line in lines):
            return None  # Incomplete response

        response_text = accumulate_dump_lines(lines)
        return response_text

    except Exception as e:
        print(f"Parsing error: {e}")
        return None
```

## Testing Patterns

### Mock Serial Class

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

### Testing Commands

```python
def test_command_generation():
    """Test command generation."""
    assert cmd_write(0, 128, 128, 0) == "WRITE 0 128 128 0 INACTIVE"
    assert cmd_dump("ACTIVE") == "DUMP ACTIVE"
    assert cmd_swap() == "SWAP"
```

### Testing Parser

```python
def test_parser_functions():
    """Test parser functions."""
    assert is_eoc("EOC") == True
    assert is_eoc("DUMP") == False

    lines = ["line1", "line2", "EOC"]
    result = accumulate_dump_lines(lines)
    assert result == "line1\nline2"
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

**Problem:** Cannot connect to serial port
**Solutions:**

- Check if port exists and is available
- Verify baud rate matches Arduino firmware
- Ensure no other application is using the port
- Check permissions and access rights

### No Data Received

**Problem:** Connected but no data from Arduino
**Solutions:**

- Verify Arduino is sending data
- Check if signals are properly connected
- Ensure connection is established before sending commands
- Check Arduino firmware and wiring

### Threading Issues

**Problem:** Application freezes or crashes
**Solutions:**

- Don't call Qt methods from non-main threads
- Use signals for cross-thread communication
- Avoid blocking operations in signal handlers
- Always disconnect in finally blocks

### Memory Leaks

**Problem:** Application memory usage increases over time
**Solutions:**

- Always call disconnect() when done
- Don't store references to connection objects
- Use weak references for long-lived objects
- Monitor signal connections and disconnect properly

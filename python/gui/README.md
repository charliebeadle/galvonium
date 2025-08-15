# GUI Module Documentation

## Overview

The GUI module provides a PyQt5-based graphical interface for the Galvonium Laser system, implementing a three-panel layout with real-time serial communication capabilities. The module is designed with a clean separation of concerns, using the Model-View-Controller pattern to coordinate between UI components, serial communication, and data management.

## Architecture

### Core Components

- **MainWindow**: Central application window orchestrating all GUI components
- **ControlPanel**: Connection management and buffer operation controls
- **BufferTableWidget**: 256-row table for editing galvo position data in binary format
- **TerminalWidget**: Command-line interface for direct Arduino communication
- **ConnectionManager**: Coordinates between GUI and GalvoController

### Design Patterns

- **MVC Architecture**: Clear separation between data models, view components, and control logic
- **Signal-Slot Communication**: Qt-based event system for loose coupling between components
- **Observer Pattern**: Components subscribe to state changes via signals
- **Factory Pattern**: Delegate creation for specialized table editing

## Component Specifications

### MainWindow

**Purpose**: Central application container and signal coordinator

**Key Responsibilities**:

- Layout management with persistent geometry and splitter states
- Signal routing between all GUI components
- Menu system and application lifecycle management
- Status bar updates and error handling

**Signals Connected**:

- `buffer_table.validation_error` → `_on_validation_error`
- `control_panel.clear_table_clicked` → `_on_clear_table`
- `terminal.command_submitted` → `_on_terminal_command`
- `connection_manager.connection_status_changed` → `_on_connection_status`
- `controller.buffer_data_changed` → `_on_buffer_data_changed`

**State Persistence**:

- Window geometry via `QSettings`
- Vertical splitter positions
- Automatic restoration on application restart

### ControlPanel

**Purpose**: Connection management and buffer operation controls

**Components**:

- **StatusLED**: Visual connection indicator (grey=disconnected, green=connected, amber=connecting)
- **Port Selection**: COM port dropdown with auto-refresh every 5 seconds
- **Baud Rate**: Standard serial rates (9600-115200)
- **Operation Buttons**: Write, Load, Swap, Clear, Pattern (future)

**Connection States**:

- **Disconnected**: Port/baud enabled, operation buttons disabled
- **Connected**: Port/baud disabled, operation buttons enabled
- **Connecting**: LED amber, buttons in transition state

**Button Operations**:

- **Write to Buffer**: Sends current table data to Arduino INACTIVE buffer
- **Load from Arduino**: Retrieves buffer data from device
- **Swap Buffers**: Exchanges ACTIVE/INACTIVE buffer states
- **Clear Table**: Empties local buffer data

### BufferTableWidget

**Purpose**: 256-row table for editing galvo position data

**Data Model**:

- **Index**: Row number (0-255, read-only)
- **X**: 8-bit binary X coordinate (editable)
- **Y**: 8-bit binary Y coordinate (editable)
- **Flags**: 8-bit binary control flags (editable)

**Editing Features**:

- **Binary Input Validation**: Only accepts 1-8 bit binary strings
- **Copy/Paste Support**: Tab-separated values, binary format preservation
- **Visual Feedback**: Invalid cells highlighted in light red
- **Monospace Font**: Consolas for bit alignment

**Validation Rules**:

- Input must match regex `^[01]{1,8}$`
- Leading zeros preserved for 8-bit alignment
- Invalid entries rejected with error highlighting
- Copy operation converts binary to decimal for external use

**Delegate System**:

- **BinaryDelegate**: Custom editor for X, Y, Flags columns
- **Input Validation**: Real-time regex validation during editing
- **Placeholder Text**: "00000000" for empty cells

### TerminalWidget

**Purpose**: Command-line interface for direct Arduino communication

**Features**:

- **Command History**: Up/Down arrow navigation (100 command limit)
- **Auto-scroll**: Automatic viewport management
- **Line Limiting**: 1000 line maximum with automatic trimming
- **Color Coding**: Blue (sent), Black (received), Red (error)
- **Timestamps**: HH:MM:SS format, configurable

**Input Handling**:

- **Enter Key**: Submit command
- **Send Button**: Alternative submission method
- **History Navigation**: Up/Down arrows cycle through previous commands
- **Command Echo**: Local display before transmission

**Output Management**:

- **Message Types**: sent, received, error with color differentiation
- **Timestamp Prefix**: Optional [HH:MM:SS] format
- **Auto-scroll**: Ensures latest output visible
- **Memory Management**: Automatic cleanup of old lines

### ConnectionManager

**Purpose**: Coordinates between GUI components and GalvoController

**Connection Lifecycle**:

- **Port Discovery**: Automatic refresh every 5 seconds
- **Connection Initiation**: Port/baud validation and controller coordination
- **State Management**: Connection status tracking and UI synchronization
- **Cleanup**: Resource management and disconnection handling

**Port Management**:

- **Auto-refresh**: 5-second timer for port list updates
- **Port Formatting**: "COM3 — Arduino Uno" display format
- **Fallback Handling**: Graceful degradation when port detection fails
- **Device Preference**: Uno-like devices prioritized in selection

**Signal Coordination**:

- **Controller Integration**: Direct access to GalvoController methods
- **Error Propagation**: Centralized error handling and status updates
- **State Synchronization**: UI state updates based on connection status

## Data Flow

### Serial Communication Path

```
TerminalWidget → ConnectionManager → GalvoController → SerialConnection → Arduino
Arduino → SerialConnection → GalvoController → ConnectionManager → TerminalWidget
```

### Buffer Data Flow

```
BufferTableWidget ↔ BufferTableModel ↔ BufferData
BufferData ↔ GalvoController ↔ SerialConnection ↔ Arduino
```

### Connection State Flow

```
ControlPanel → ConnectionManager → GalvoController → SerialConnection
SerialConnection → GalvoController → ConnectionManager → ControlPanel
```

## Integration Points

### Controller Interface

- **GalvoController**: Main business logic and serial coordination
- **BufferData**: Data model for galvo position information
- **SerialConnection**: Low-level serial communication

### External Dependencies

- **PyQt5**: Core GUI framework
- **pyserial**: Serial port enumeration and communication
- **typing**: Type hints for development and integration

### Signal Interface

All components communicate via Qt signals, enabling loose coupling:

- **connection_status_changed(bool)**: Connection state updates
- **buffer_data_changed(BufferData)**: Buffer content modifications
- **validation_error(str)**: Input validation failures
- **command_submitted(str)**: Terminal command submissions

## Error Handling

### Validation Errors

- **Binary Input**: Rejected with visual feedback and error signals
- **Port Selection**: Fallback to example ports when detection fails
- **Connection Failures**: User notification with timeout-based status messages

### Exception Handling

- **Serial Errors**: Graceful degradation with user feedback
- **Port Enumeration**: Fallback handling for missing pyserial
- **Controller Failures**: Error propagation through signal chain

## Configuration

### Default Settings

- **Window Size**: 1100x800 pixels
- **Port Refresh**: 5-second intervals
- **Terminal Lines**: 1000 maximum with auto-trim
- **Command History**: 100 entries maximum
- **Timestamps**: Enabled by default

### Persistent State

- **Window Geometry**: Automatic save/restore
- **Splitter Positions**: Vertical divider state preservation
- **User Preferences**: QSettings-based configuration storage

## Performance Considerations

### Memory Management

- **Terminal Output**: Automatic line limiting prevents memory growth
- **Command History**: Fixed-size deque with automatic overflow handling
- **Buffer Data**: Efficient model updates with minimal redraws

### UI Responsiveness

- **Async Operations**: Serial communication non-blocking
- **Progress Updates**: Real-time operation status feedback
- **State Synchronization**: Immediate UI updates for user actions

## Extension Points

### Future Enhancements

- **Pattern Loading**: Predefined galvo movement patterns
- **Keyboard Shortcuts**: F5 for load, Ctrl+G for write operations
- **Help System**: Firmware command reference and examples
- **Buffer Validation**: Cross-reference checking for movement constraints

### Plugin Architecture

- **Custom Delegates**: Specialized editors for different data types
- **Additional Panels**: Expandable control sections
- **Data Export**: CSV, binary, or custom format support
- **Logging Integration**: Comprehensive operation logging

## Testing Considerations

### Unit Testing

- **Model Validation**: BufferTableModel data integrity
- **Signal Emission**: Component communication verification
- **Input Validation**: Binary format acceptance/rejection

### Integration Testing

- **Controller Communication**: End-to-end signal flow
- **Serial Mocking**: Arduino communication simulation
- **UI State Management**: Connection state transitions

### Performance Testing

- **Large Buffer Handling**: 256-row table performance
- **Memory Usage**: Long-running session monitoring
- **Response Time**: UI responsiveness under load

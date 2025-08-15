# Galvonium Laser System Architecture

## System Overview

The Galvonium Laser system consists of a Python-based GUI application that communicates with an Arduino device via serial communication. The system is built using a modular architecture with clear separation of concerns between GUI components, business logic, and communication layers.

## High-Level Architecture

```mermaid
graph TB
    subgraph "Python Application"
        subgraph "GUI Layer"
            MW[MainWindow]
            CP[ControlPanel]
            BT[BufferTableWidget]
            TW[TerminalWidget]
            CM[ConnectionManager]
        end

        subgraph "Controller Layer"
            GC[GalvoController]
        end

        subgraph "Serial Layer"
            SC[SerialConnection]
        end

        subgraph "Data Layer"
            BD[BufferData]
            BM[BufferTableModel]
        end
    end

    subgraph "Hardware"
        AR[Arduino Device]
    end

    MW --> CP
    MW --> BT
    MW --> TW
    MW --> CM

    CM --> GC
    GC --> SC
    SC --> AR

    BT --> BM
    BM --> BD
    GC --> BD
```

## Component Relationships

```mermaid
classDiagram
    class MainWindow {
        -GalvoController _controller
        -ConnectionManager _connection_manager
        -ControlPanel control_panel
        -BufferTableWidget buffer_table
        -TerminalWidget terminal
        +_wire_signals()
        +_on_connection_status()
        +_on_buffer_data_changed()
    }

    class ControlPanel {
        -ConnectionManager _connection_manager
        -StatusLED led
        -QComboBox port_combo
        -QComboBox baud_combo
        -QPushButton connect_btn
        +clear_table_clicked
        +refresh_ports()
        +_on_connect_clicked()
    }

    class BufferTableWidget {
        -BufferTableModel _model
        -BufferTableView _view
        +validation_error
        +load_buffer_data()
        +get_buffer_data()
        +clear_all()
    }

    class TerminalWidget {
        -QTextEdit output
        -QLineEdit input
        -QPushButton send_btn
        -deque history
        +command_submitted
        +append_output()
        +clear_terminal()
    }

    class ConnectionManager {
        -GalvoController _controller
        -bool _is_connected
        -QTimer _port_refresh_timer
        +connection_status_changed
        +ports_updated
        +status_message
        +error_occurred
        +connect_to_device()
        +disconnect_from_device()
        +refresh_ports()
    }

    class GalvoController {
        -SerialConnection _serial_conn
        -BufferData _buffer_data
        -bool _is_connected
        +buffer_data_changed
        +connection_status_changed
        +operation_progress
        +error_occurred
        +status_message
        +data_received
        +connect_to_device()
        +send_command()
    }

    class BufferTableModel {
        -BufferData _buffer
        -Set _invalid_cells
        +load_buffer()
        +clear_all()
        +setData()
        +data()
    }

    class BufferData {
        -List _steps
        +get_step()
        +set_step()
        +clear()
    }

    MainWindow --> ControlPanel
    MainWindow --> BufferTableWidget
    MainWindow --> TerminalWidget
    MainWindow --> ConnectionManager
    MainWindow --> GalvoController

    ControlPanel --> ConnectionManager
    BufferTableWidget --> BufferTableModel
    BufferTableModel --> BufferData
    ConnectionManager --> GalvoController
    GalvoController --> BufferData
```

## Data Flow Architecture

```mermaid
flowchart TD
    subgraph "User Input"
        UI[User Interface Actions]
        CP[Control Panel Buttons]
        TW[Terminal Commands]
        BT[Buffer Table Edits]
    end

    subgraph "GUI Processing"
        MW[MainWindow Signal Routing]
        CM[Connection Manager]
        VAL[Input Validation]
    end

    subgraph "Business Logic"
        GC[Galvo Controller]
        BD[Buffer Data Model]
    end

    subgraph "Communication"
        SC[Serial Connection]
        SER[Serial Protocol]
    end

    subgraph "Hardware"
        AR[Arduino Device]
        FW[Firmware]
    end

    UI --> MW
    CP --> MW
    TW --> MW
    BT --> VAL

    MW --> CM
    VAL --> CM
    CM --> GC

    GC --> BD
    GC --> SC

    SC --> SER
    SER --> AR
    AR --> FW

    FW --> SER
    SER --> SC
    SC --> GC
    GC --> CM
    CM --> MW
    MW --> UI
```

## Signal Communication Flow

```mermaid
sequenceDiagram
    participant User
    participant CP as ControlPanel
    participant CM as ConnectionManager
    participant GC as GalvoController
    participant SC as SerialConnection
    participant AR as Arduino

    Note over User,AR: Connection Sequence
    User->>CP: Click Connect
    CP->>CM: connect_to_device(port, baud)
    CM->>GC: connect_to_device(port, baud)
    GC->>SC: connect(port, baud)
    SC->>AR: Open serial connection
    AR-->>SC: Connection established
    SC-->>GC: connection_status_changed(true)
    GC-->>CM: connection_status_changed(true)
    CM-->>CP: connection_status_changed(true)
    CP->>CP: Update UI state (LED green, buttons enabled)

    Note over User,AR: Buffer Operations
    User->>CP: Click Write to Buffer
    CP->>CM: write_buffer_to_device("INACTIVE")
    CM->>GC: write_buffer_to_device("INACTIVE")
    GC->>SC: Send write commands
    SC->>AR: Serial commands
    AR-->>SC: Acknowledgment
    SC-->>GC: data_received
    GC-->>CM: status_message
    CM-->>CP: Update progress/status
```

## Connection State Machine

```mermaid
stateDiagram-v2
    [*] --> Disconnected

    Disconnected --> Connecting : connect_to_device()
    Connecting --> Connected : Serial connection successful
    Connecting --> Disconnected : Connection failed

    Connected --> Disconnected : disconnect_from_device()
    Connected --> Disconnected : Serial error/timeout

    state Connected {
        [*] --> Idle
        Idle --> Writing : write_buffer_to_device()
        Idle --> Loading : load_buffer_from_device()
        Idle --> Swapping : swap_buffers()

        Writing --> Idle : Operation complete
        Loading --> Idle : Operation complete
        Swapping --> Idle : Operation complete
    }

    state Disconnected {
        [*] --> PortSelection
        PortSelection --> PortSelection : refresh_ports()
    }
```

## Buffer Data Structure

```mermaid
graph LR
    subgraph "Buffer Table (256 rows)"
        R0[Row 0]
        R1[Row 1]
        R2[Row 2]
        R255[Row 255]
    end

    subgraph "Each Row Contains"
        IDX[Index<br/>0-255]
        X[X Coordinate<br/>8-bit binary]
        Y[Y Coordinate<br/>8-bit binary]
        FLG[Flags<br/>8-bit binary]
    end

    subgraph "Data Model"
        BD[BufferData]
        ST[Step Objects]
        VAL[Validation]
    end

    R0 --> IDX
    R0 --> X
    R0 --> Y
    R0 --> FLG

    BD --> ST
    ST --> VAL
```

## Error Handling Flow

```mermaid
flowchart TD
    subgraph "Input Validation"
        IN[User Input]
        VAL[Validation Check]
        REJ[Reject Input]
        ACC[Accept Input]
    end

    subgraph "Connection Errors"
        CON[Connection Attempt]
        FAIL[Connection Failed]
        TIMEOUT[Timeout]
        PORT[Port Error]
    end

    subgraph "Serial Errors"
        SER[Serial Operation]
        SERR[Serial Error]
        PARSE[Parse Error]
        COMM[Command Error]
    end

    subgraph "Error Response"
        UI[UI Update]
        MSG[Status Message]
        LOG[Error Logging]
        REC[Recovery Action]
    end

    IN --> VAL
    VAL --> REJ
    VAL --> ACC

    CON --> FAIL
    FAIL --> TIMEOUT
    FAIL --> PORT

    SER --> SERR
    SERR --> PARSE
    SERR --> COMM

    REJ --> UI
    FAIL --> MSG
    SERR --> LOG
    TIMEOUT --> REC
```

## Performance Characteristics

```mermaid
graph TB
    subgraph "Memory Management"
        TL[Terminal Lines: 1000 max]
        CH[Command History: 100 max]
        BT[Buffer Table: 256 rows]
        BU[Buffer Updates: Efficient]
    end

    subgraph "UI Responsiveness"
        AS[Async Serial Operations]
        PU[Progress Updates]
        SU[State Synchronization]
        IM[Immediate UI Updates]
    end

    subgraph "Communication"
        PR[Port Refresh: 5s intervals]
        SC[Serial Commands: Non-blocking]
        AC[Auto-reconnection]
        FB[Fallback Handling]
    end

    subgraph "Data Processing"
        BV[Binary Validation: Regex]
        CP[Copy/Paste: Tab-separated]
        EF[Efficient Model Updates]
        CA[Cell-level Change Detection]
    end
```

## Extension Points

```mermaid
graph LR
    subgraph "Current Implementation"
        GUI[GUI Module]
        SER[Serial Module]
        CON[Controller]
    end

    subgraph "Future Extensions"
        PAT[Pattern Loading]
        EXP[Data Export]
        LOG[Logging System]
        VAL[Advanced Validation]
        PLG[Plugin System]
    end

    subgraph "Integration Points"
        API[External APIs]
        DB[Database Storage]
        NET[Network Communication]
        UI[Custom UI Components]
    end

    GUI --> PAT
    GUI --> EXP
    GUI --> LOG
    GUI --> VAL
    GUI --> PLG

    PAT --> API
    EXP --> DB
    LOG --> NET
    PLG --> UI
```

## Testing Architecture

```mermaid
graph TB
    subgraph "Unit Testing"
        UT[Unit Tests]
        MT[Model Tests]
        VT[Validation Tests]
        ST[Signal Tests]
    end

    subgraph "Integration Testing"
        IT[Integration Tests]
        CT[Controller Tests]
        GT[GUI Tests]
        ST[Serial Tests]
    end

    subgraph "Performance Testing"
        PT[Performance Tests]
        MT[Memory Tests]
        RT[Response Tests]
        LT[Load Tests]
    end

    subgraph "Test Infrastructure"
        MO[Mock Objects]
        SI[Serial Simulation]
        UI[UI Automation]
        CI[CI/CD Pipeline]
    end

    UT --> MO
    IT --> SI
    PT --> UI
    UT --> CI
    IT --> CI
    PT --> CI
```

## Deployment Architecture

```mraph
graph TB
    subgraph "Development Environment"
        DEV[Developer Machine]
        VENV[Virtual Environment]
        DEPS[Dependencies]
        TESTS[Test Suite]
    end

    subgraph "Production Environment"
        PROD[Production Machine]
        PYTHON[Python Runtime]
        QT[PyQt5 Installation]
        SERIAL[pyserial]
    end

    subgraph "Hardware Requirements"
        USB[USB Port]
        AR[Arduino Device]
        LASER[Laser System]
        GALVO[Galvo Mirrors]
    end

    DEV --> VENV
    VENV --> DEPS
    DEPS --> TESTS

    PROD --> PYTHON
    PYTHON --> QT
    PYTHON --> SERIAL

    PROD --> USB
    USB --> AR
    AR --> LASER
    LASER --> GALVO
```

This architecture documentation provides a comprehensive view of your Galvonium Laser system, showing how all the components interact, how data flows through the system, and where the extension points are for future development.

from PyQt5 import QtCore, QtGui, QtWidgets

try:
    from serial.tools import list_ports
except Exception:  # pyserial may not be installed yet during Phase 1
    list_ports = None


class StatusLED(QtWidgets.QLabel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(14, 14)
        self.setToolTip("Connection status")
        self.set_off()

    def _set_color(self, color: str):
        self.setStyleSheet(
            f"border-radius:7px;background:{color};border:1px solid #888;"
        )

    def set_off(self):
        self._set_color("#bbb")  # grey

    def set_on(self):
        self._set_color("#4caf50")  # green

    def set_busy(self):
        self._set_color("#ffa000")  # amber


class ControlPanel(QtWidgets.QGroupBox):
    clear_table_clicked = QtCore.pyqtSignal()

    def __init__(self, connection_manager, parent=None):
        super().__init__("Control Panel", parent)
        self._connection_manager = connection_manager
        layout = QtWidgets.QGridLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setHorizontalSpacing(12)
        row = 0

        # Connection group
        conn_box = QtWidgets.QGroupBox("Connection", self)
        conn_grid = QtWidgets.QGridLayout(conn_box)

        self.port_combo = QtWidgets.QComboBox(conn_box)
        self.refresh_btn = QtWidgets.QPushButton("Refresh", conn_box)
        self.baud_combo = QtWidgets.QComboBox(conn_box)
        self.baud_combo.addItems(
            ["9600", "19200", "38400", "57600", "115200"]
        )  # default 9600
        self.connect_btn = QtWidgets.QPushButton("Connect", conn_box)
        self.led = StatusLED(conn_box)

        conn_grid.addWidget(QtWidgets.QLabel("Port:"), 0, 0)
        conn_grid.addWidget(self.port_combo, 0, 1)
        conn_grid.addWidget(self.refresh_btn, 0, 2)
        conn_grid.addWidget(QtWidgets.QLabel("Baud:"), 1, 0)
        conn_grid.addWidget(self.baud_combo, 1, 1)
        conn_grid.addWidget(self.connect_btn, 1, 2)
        conn_grid.addWidget(self.led, 0, 3, 2, 1)

        layout.addWidget(conn_box, row, 0, 1, 2)
        row += 1

        # Buffer ops
        ops_box = QtWidgets.QGroupBox("Buffer Operations", self)
        ops_grid = QtWidgets.QGridLayout(ops_box)

        self.write_btn = QtWidgets.QPushButton("Write to Buffer", ops_box)
        self.load_btn = QtWidgets.QPushButton("Load from Arduino", ops_box)
        self.swap_btn = QtWidgets.QPushButton("Swap Buffers", ops_box)
        self.clear_btn = QtWidgets.QPushButton("Clear Table", ops_box)
        self.pattern_btn = QtWidgets.QPushButton("Load Pattern", ops_box)
        self.pattern_btn.setEnabled(False)  # future

        self.progress = QtWidgets.QProgressBar(ops_box)
        self.progress.setValue(0)
        self.progress.setFormat("%p%")
        self.progress.setTextVisible(True)

        ops_grid.addWidget(self.write_btn, 0, 0)
        ops_grid.addWidget(self.load_btn, 0, 1)
        ops_grid.addWidget(self.swap_btn, 0, 2)
        ops_grid.addWidget(self.clear_btn, 1, 0)
        ops_grid.addWidget(self.pattern_btn, 1, 1)
        ops_grid.addWidget(self.progress, 1, 2)

        layout.addWidget(ops_box, row, 0, 1, 2)
        row += 1

        # Status line
        self.status_lbl = QtWidgets.QLabel("Ready.", self)
        layout.addWidget(self.status_lbl, row, 0, 1, 2)

        # Wire signals
        self.refresh_btn.clicked.connect(self.refresh_ports)
        self.connect_btn.clicked.connect(self._on_connect_clicked)
        self.clear_btn.clicked.connect(self.clear_table_clicked)
        self.write_btn.clicked.connect(self._on_write_clicked)
        self.load_btn.clicked.connect(self._on_load_clicked)
        self.swap_btn.clicked.connect(self._on_swap_clicked)

        # Wire connection manager signals
        self._connection_manager.ports_updated.connect(self._on_ports_updated)
        self._connection_manager.connection_status_changed.connect(
            self._on_connection_status
        )

        # Initial state
        self.refresh_ports()

    def refresh_ports(self):
        """Refresh the list of available ports."""
        self._connection_manager.refresh_ports()

    def populate_ports(self):
        """Legacy method - now delegates to refresh_ports."""
        self.refresh_ports()

    def _on_ports_updated(self, ports):
        """Handle ports list updates from connection manager."""
        self.port_combo.clear()
        for port in ports:
            self.port_combo.addItem(port, port)

        # Prefer Uno-like descriptions if present
        idx = next((i for i, port in enumerate(ports) if "uno" in port.lower()), 0)
        self.port_combo.setCurrentIndex(idx)

    def _on_connection_status(self, connected: bool):
        """Handle connection status changes."""
        if connected:
            self.led.set_on()
            self.connect_btn.setText("Disconnect")
            self.connect_btn.clicked.disconnect()
            self.connect_btn.clicked.connect(self._on_disconnect_clicked)
            self._update_button_states(True)
        else:
            self.led.set_off()
            self.connect_btn.setText("Connect")
            self.connect_btn.clicked.disconnect()
            self.connect_btn.clicked.connect(self._on_connect_clicked)
            self._update_button_states(False)

    def _on_connect_clicked(self):
        """Handle connect button click."""
        port = self.port_combo.currentText()
        baud = int(self.baud_combo.currentText())

        if self._connection_manager.connect_to_device(port, baud):
            self.led.set_busy()

    def _on_disconnect_clicked(self):
        """Handle disconnect button click."""
        self._connection_manager.disconnect_from_device()

    def _on_write_clicked(self):
        """Handle write button click."""
        if not self._connection_manager.is_connected():
            return

        # For now, write to INACTIVE buffer
        self._connection_manager.write_buffer_to_device("INACTIVE")

    def _on_load_clicked(self):
        """Handle load button click."""
        if not self._connection_manager.is_connected():
            return

        # For now, load from INACTIVE buffer
        self._connection_manager.load_buffer_from_device("INACTIVE")

    def _on_swap_clicked(self):
        """Handle swap button click."""
        if not self._connection_manager.is_connected():
            return

        self._connection_manager.swap_buffers()

    def _update_button_states(self, connected: bool):
        """Update button states based on connection status."""
        self.write_btn.setEnabled(connected)
        self.load_btn.setEnabled(connected)
        self.swap_btn.setEnabled(connected)
        self.port_combo.setEnabled(not connected)
        self.baud_combo.setEnabled(not connected)
        self.refresh_btn.setEnabled(not connected)

    def update_progress(self, progress: int, message: str):
        """Update progress bar and status."""
        self.progress.setValue(progress)
        self.status_lbl.setText(message)

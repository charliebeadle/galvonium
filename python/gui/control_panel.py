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

    def __init__(self, parent=None):
        super().__init__("Control Panel", parent)
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

        # Wire
        self.refresh_btn.clicked.connect(self.populate_ports)
        self.clear_btn.clicked.connect(self.clear_table_clicked)

        # Initial state
        self.populate_ports()

    def populate_ports(self):
        self.port_combo.clear()
        candidates = []
        if list_ports is not None:
            try:
                ports = list(list_ports.comports())
                for p in ports:
                    disp = f"{p.device} — {p.description}"
                    candidates.append((p.device, disp))
            except Exception:
                candidates = []
        # Fallback sample on Windows
        if not candidates:
            candidates = [("COM3", "COM3 — Arduino Uno (example)")]
        for dev, disp in candidates:
            self.port_combo.addItem(disp, dev)
        # Prefer Uno-like descriptions if present
        idx = next((i for i, (_, d) in enumerate(candidates) if "uno" in d.lower()), 0)
        self.port_combo.setCurrentIndex(idx)

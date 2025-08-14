from PyQt5 import QtCore, QtGui, QtWidgets
from .buffer_table import BufferTableWidget
from .terminal_widget import TerminalWidget
from .control_panel import ControlPanel

from models.buffer_model import BufferData  # uses your implemented model


class MainWindow(QtWidgets.QMainWindow):
    """Main application window composed of ControlPanel (top),
    BufferTable (middle), Terminal (bottom). Persists geometry & splitter.
    """

    GEOM_KEY = "main/geometry"
    SPLIT_VERT_KEY = "main/splitter_vertical"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Galvonium Laser GUI")
        self.resize(1100, 800)

        # Central layout container
        central = QtWidgets.QWidget(self)
        vbox = QtWidgets.QVBoxLayout(central)
        vbox.setContentsMargins(6, 6, 6, 6)
        vbox.setSpacing(6)

        # Top: Control Panel (fixed-ish height)
        self.control_panel = ControlPanel(self)
        vbox.addWidget(self.control_panel, 0)

        # Middle/Bottom with splitter
        self.splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical, self)
        vbox.addWidget(self.splitter, 1)

        # Middle: Buffer Table
        self.buffer_data = BufferData()  # start empty 256 steps
        self.buffer_table = BufferTableWidget(self)
        self.buffer_table.load_buffer_data(self.buffer_data)
        self.splitter.addWidget(self.buffer_table)

        # Bottom: Terminal
        self.terminal = TerminalWidget(self)
        self.splitter.addWidget(self.terminal)
        self.splitter.setStretchFactor(0, 3)
        self.splitter.setStretchFactor(1, 1)

        self.setCentralWidget(central)

        # Status bar
        self.status = self.statusBar()
        self._set_status("Disconnected")

        # Menus (stubs / TODOs)
        self._build_menu()

        # Wire signals (no serial yet)
        self._wire_signals()

        # Restore state
        self._restore_ui_state()

    # ── Menu bar ────────────────────────────────────────────────────────────
    def _build_menu(self):
        menubar = self.menuBar()

        file_menu = menubar.addMenu("&File")
        exit_act = QtWidgets.QAction("E&xit", self)
        exit_act.triggered.connect(self.close)
        file_menu.addAction(exit_act)

        edit_menu = menubar.addMenu("&Edit")
        clear_tbl = QtWidgets.QAction("Clear &Table", self)
        clear_tbl.triggered.connect(self.buffer_table.clear_all)
        edit_menu.addAction(clear_tbl)

        tools_menu = menubar.addMenu("&Tools")
        # TODO: shortcuts like F5 load, Ctrl+G write

        help_menu = menubar.addMenu("&Help")
        about_act = QtWidgets.QAction("&About", self)
        about_act.triggered.connect(self._show_about)
        help_menu.addAction(about_act)

    def _show_about(self):
        QtWidgets.QMessageBox.information(
            self,
            "About Galvonium Laser GUI",
            "Laser galvo control GUI. Phase 1 scaffold (no serial).\n\n"
            "Commands default to INACTIVE buffer.\n"
            "TODO: Help sheet with firmware commands.",
        )

    # ── Signals ─────────────────────────────────────────────────────────────
    def _wire_signals(self):
        self.buffer_table.validation_error.connect(self._on_validation_error)
        self.control_panel.clear_table_clicked.connect(self.buffer_table.clear_all)

        # Terminal: echo sent command locally (no serial yet)
        def on_cmd(text: str):
            if not text.strip():
                return
            self.terminal.append_output(text, msg_type="sent")
            # In future, we will forward to SerialConnection

        self.terminal.command_submitted.connect(on_cmd)

    def _on_validation_error(self, message: str):
        self._set_status(message, timeout_ms=3000)

    # ── Status & persistence ────────────────────────────────────────────────
    def _set_status(self, text: str, timeout_ms: int | None = None):
        if timeout_ms:
            self.status.showMessage(text, timeout_ms)
        else:
            self.status.showMessage(text)

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        self._save_ui_state()
        super().closeEvent(event)

    def _save_ui_state(self):
        settings = QtCore.QSettings()
        settings.setValue(self.GEOM_KEY, self.saveGeometry())
        settings.setValue(self.SPLIT_VERT_KEY, self.splitter.saveState())

    def _restore_ui_state(self):
        settings = QtCore.QSettings()
        geom = settings.value(self.GEOM_KEY)
        if geom is not None:
            self.restoreGeometry(geom)
        split = settings.value(self.SPLIT_VERT_KEY)
        if split is not None:
            self.splitter.restoreState(split)

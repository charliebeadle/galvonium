from PyQt5 import QtCore, QtGui, QtWidgets
from .buffer_table import BufferTableWidget
from .terminal_widget import TerminalWidget
from .control_panel import ControlPanel
from .connection_manager import ConnectionManager

from controllers.galvo_controller import GalvoController


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

        # Initialize controller and connection manager
        self._controller = GalvoController(self)
        self._connection_manager = ConnectionManager(self._controller, self)

        # Central layout container
        central = QtWidgets.QWidget(self)
        vbox = QtWidgets.QVBoxLayout(central)
        vbox.setContentsMargins(6, 6, 6, 6)
        vbox.setSpacing(6)

        # Top: Control Panel (fixed-ish height)
        self.control_panel = ControlPanel(self._connection_manager, self)
        vbox.addWidget(self.control_panel, 0)

        # Middle/Bottom with splitter
        self.splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical, self)
        vbox.addWidget(self.splitter, 1)

        # Middle: Buffer Table
        self.buffer_table = BufferTableWidget(self)
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

        # Wire signals
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
        # Buffer table signals
        self.buffer_table.validation_error.connect(self._on_validation_error)

        # Control panel signals
        self.control_panel.clear_table_clicked.connect(self._on_clear_table)

        # Terminal signals
        self.terminal.command_submitted.connect(self._on_terminal_command)

        # Connection manager signals
        self._connection_manager.connection_status_changed.connect(
            self._on_connection_status
        )
        self._connection_manager.status_message.connect(self._on_status_message)
        self._connection_manager.error_occurred.connect(self._on_error_occurred)

        # Controller signals
        self._controller.buffer_data_changed.connect(self._on_buffer_data_changed)
        self._controller.operation_progress.connect(self._on_operation_progress)
        self._controller.data_received.connect(self._on_arduino_data_received)

        # Load initial buffer data
        self._on_buffer_data_changed(self._controller.get_buffer_data())

    def _on_validation_error(self, message: str):
        self._set_status(message, timeout_ms=3000)

    def _on_clear_table(self):
        """Handle clear table request from control panel."""
        self._connection_manager.clear_buffer()

    def _on_terminal_command(self, command: str):
        """Handle command submission from terminal."""
        if not command.strip():
            return

        # Echo command locally
        self.terminal.append_output(command, msg_type="sent")

        # Send command through connection manager
        if self._connection_manager.is_connected():
            self._connection_manager.send_command(command)
        else:
            self.terminal.append_output("Not connected to device", msg_type="error")

    def _on_connection_status(self, connected: bool):
        """Handle connection status changes."""
        if connected:
            self._set_status("Connected")
        else:
            self._set_status("Disconnected")

    def _on_status_message(self, message: str, timeout_ms: int):
        """Handle status messages from connection manager."""
        self._set_status(message, timeout_ms)

    def _on_error_occurred(self, error: str):
        """Handle errors from connection manager."""
        self._set_status(f"Error: {error}", timeout_ms=5000)

    def _on_buffer_data_changed(self, buffer_data):
        """Handle buffer data changes from controller."""
        self.buffer_table.load_buffer_data(buffer_data)

    def _on_operation_progress(self, progress: int, message: str):
        """Handle operation progress updates."""
        self.control_panel.update_progress(progress, message)

    def _on_arduino_data_received(self, data: str):
        """Handle raw data received from Arduino."""
        # Display in terminal widget
        self.terminal.append_output(data, msg_type="received")

    # ── Status & persistence ────────────────────────────────────────────────
    def _set_status(self, text: str, timeout_ms: int | None = None):
        if timeout_ms:
            self.status.showMessage(text, timeout_ms)
        else:
            self.status.showMessage(text)

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        # Clean up resources
        self._connection_manager.cleanup()
        self._controller.cleanup()

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

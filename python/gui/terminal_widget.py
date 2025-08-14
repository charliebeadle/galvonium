from PyQt5 import QtCore, QtGui, QtWidgets
from collections import deque
import datetime as _dt


class TerminalWidget(QtWidgets.QWidget):
    """Simple console: output view + input line + Send button.
    - Auto-scroll, 1000 line cap
    - History with Up/Down
    - Enter to send
    - Color coding: sent=blue, recv=black, error=red
    - Timestamps ON by default
    """

    command_submitted = QtCore.pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        vbox = QtWidgets.QVBoxLayout(self)
        vbox.setContentsMargins(0, 0, 0, 0)
        vbox.setSpacing(6)

        self.output = QtWidgets.QTextEdit(self)
        self.output.setReadOnly(True)
        self.output.setLineWrapMode(QtWidgets.QTextEdit.NoWrap)
        self.output.setFont(QtGui.QFont("Consolas"))

        hbox = QtWidgets.QHBoxLayout()
        self.input = QtWidgets.QLineEdit(self)
        self.input.setPlaceholderText("Enter command (e.g., DUMP INACTIVE)")
        self.send_btn = QtWidgets.QPushButton("Send", self)
        hbox.addWidget(self.input, 1)
        hbox.addWidget(self.send_btn, 0)

        vbox.addWidget(self.output, 1)
        vbox.addLayout(hbox)

        self.history = deque(maxlen=100)
        self._hist_idx = -1
        self._limit = 1000
        self._timestamps = True

        # Signals
        self.send_btn.clicked.connect(self._send)
        self.input.returnPressed.connect(self._send)
        self.input.installEventFilter(self)

    # History navigation
    def eventFilter(self, obj, ev):
        if obj is self.input and ev.type() == QtCore.QEvent.KeyPress:
            if ev.key() == QtCore.Qt.Key_Up:
                self._hist_prev()
                return True
            if ev.key() == QtCore.Qt.Key_Down:
                self._hist_next()
                return True
        return super().eventFilter(obj, ev)

    def _hist_prev(self):
        if not self.history:
            return
        if self._hist_idx == -1:
            self._hist_idx = len(self.history) - 1
        else:
            self._hist_idx = max(0, self._hist_idx - 1)
        self.input.setText(self.history[self._hist_idx])

    def _hist_next(self):
        if not self.history:
            return
        if self._hist_idx == -1:
            return
        self._hist_idx = min(len(self.history) - 1, self._hist_idx + 1)
        self.input.setText(self.history[self._hist_idx])

    def _send(self):
        text = self.input.text()
        if not text.strip():
            return
        self.history.append(text)
        self._hist_idx = -1
        self.append_output(text, msg_type="sent")
        self.command_submitted.emit(text)
        self.input.clear()

    def append_output(self, text: str, msg_type: str = "recv"):
        """Append line with optional type coloring and timestamps."""
        color = {
            "sent": "#1e88e5",  # blue
            "recv": "#000000",  # black
            "error": "#d32f2f",  # red
        }.get(msg_type, "#000000")
        ts = _dt.datetime.now().strftime("%H:%M:%S") if self._timestamps else ""
        prefix = f"[{ts}] " if ts else ""

        cursor = self.output.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End)

        fmt = QtGui.QTextCharFormat()
        fmt.setForeground(QtGui.QBrush(QtGui.QColor(color)))
        cursor.setCharFormat(fmt)
        cursor.insertText(prefix + text + "\n")
        self.output.setTextCursor(cursor)
        self.output.ensureCursorVisible()

        # Trim to _limit lines
        doc = self.output.document()
        while doc.blockCount() > self._limit:
            b = doc.firstBlock()
            cursor = QtGui.QTextCursor(b)
            cursor.select(QtGui.QTextCursor.BlockUnderCursor)
            cursor.removeSelectedText()
            cursor.deleteChar()

    def clear_terminal(self):
        self.output.clear()

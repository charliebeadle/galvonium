from __future__ import annotations
from PyQt5 import QtCore, QtGui, QtWidgets
from typing import Optional, Tuple, Set
import re

from models.buffer_model import BufferData  # your implemented data model


BIN_QRE = QtCore.QRegularExpression(r"^[01]{1,8}$")
BIN_FULL_RE = re.compile(r"^[01]{1,8}$")


class BinaryDelegate(QtWidgets.QStyledItemDelegate):
    """Editor that accepts only up to 8 binary digits. No prefixes. Pads on display."""

    def createEditor(self, parent, option, index):
        editor = QtWidgets.QLineEdit(parent)
        validator = QtGui.QRegularExpressionValidator(BIN_QRE, editor)
        editor.setValidator(validator)
        editor.setMaxLength(8)
        editor.setPlaceholderText("00000000")
        # Monospace makes bits readable
        editor.setFont(QtGui.QFont("Consolas"))
        return editor

    def setEditorData(self, editor, index):
        if not isinstance(editor, QtWidgets.QLineEdit):
            return super().setEditorData(editor, index)
        val = index.model().data(index, QtCore.Qt.EditRole)
        editor.setText(val)

    def setModelData(self, editor, model, index):
        if not isinstance(editor, QtWidgets.QLineEdit):
            return super().setModelData(editor, model, index)
        text = editor.text()
        model.setData(index, text, role=QtCore.Qt.EditRole)


class BufferTableModel(QtCore.QAbstractTableModel):
    COLUMNS = ("Index", "X", "Y", "Flags")

    def __init__(self, buffer: Optional[BufferData] = None, parent=None):
        super().__init__(parent)
        self._buffer: BufferData = buffer if buffer is not None else BufferData()
        self._invalid_cells: Set[Tuple[int, int]] = set()

    # ── Qt model API ───────────────────────────────────────────────────────
    def rowCount(self, parent=QtCore.QModelIndex()):
        return 256

    def columnCount(self, parent=QtCore.QModelIndex()):
        return 4

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role != QtCore.Qt.DisplayRole:
            return None
        if orientation == QtCore.Qt.Horizontal:
            return self.COLUMNS[section]
        return section

    def flags(self, index: QtCore.QModelIndex):
        if not index.isValid():
            return QtCore.Qt.NoItemFlags
        base = QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled
        if index.column() == 0:
            return base  # Index column read-only
        return base | QtCore.Qt.ItemIsEditable

    def data(self, index, role=QtCore.Qt.DisplayRole):
        if not index.isValid():
            return None
        row, col = index.row(), index.column()

        if role == QtCore.Qt.DisplayRole or role == QtCore.Qt.EditRole:
            if col == 0:
                return row  # Index decimal
            step = self._buffer.get_step(row)
            if col == 1:
                return f"{step.x:08b}"
            if col == 2:
                return f"{step.y:08b}"
            if col == 3:
                return f"{step.flags:08b}"

        if role == QtCore.Qt.TextAlignmentRole:
            if col == 0:
                return QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter
            return QtCore.Qt.AlignCenter

        if role == QtCore.Qt.BackgroundRole:
            if (row, col) in self._invalid_cells:
                return QtGui.QBrush(QtGui.QColor("#ffcccc"))  # light red
        return None

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        if role != QtCore.Qt.EditRole or not index.isValid():
            return False
        row, col = index.row(), index.column()
        if col == 0:
            return False

        text = str(value).strip()
        if not BIN_FULL_RE.fullmatch(text):
            self._invalid_cells.add((row, col))
            self.dataChanged.emit(index, index, [QtCore.Qt.BackgroundRole])
            return False

        try:
            intval = int(text, 2)
        except ValueError:
            self._invalid_cells.add((row, col))
            self.dataChanged.emit(index, index, [QtCore.Qt.BackgroundRole])
            return False
        step = self._buffer.get_step(row)
        if col == 1:
            step.x = intval
        elif col == 2:
            step.y = intval
        elif col == 3:
            step.flags = intval
        else:
            return False

        # Clear error state if any
        self._invalid_cells.discard((row, col))
        self.dataChanged.emit(index, index)
        return True

    # ── Public API for widget wrapper ──────────────────────────────────────
    def load_buffer(self, buffer: BufferData):
        self.beginResetModel()
        self._buffer = buffer
        self._invalid_cells.clear()
        self.endResetModel()

    def clear_all(self):
        self.beginResetModel()
        self._buffer.clear()
        self._invalid_cells.clear()
        self.endResetModel()

    # Optional helper for external validation
    @staticmethod
    def validate_input(text: str) -> Tuple[bool, Optional[int], Optional[str]]:
        if not BIN_FULL_RE.fullmatch(text or ""):
            return False, None, "Binary only (1–8 bits, e.g. 01010101)"
        return True, int(text, 2), None


class BufferTableView(QtWidgets.QTableView):
    """QTableView with copy/paste support and binary-only editing.

    Copy OUT: plain decimal numbers (no 0x/0b), tab-separated.
    Paste IN: binary strings only, reject invalid entries and keep originals.
    """

    validation_error = QtCore.pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectItems)
        self.setSelectionMode(QtWidgets.QAbstractItemView.ContiguousSelection)
        self.setAlternatingRowColors(True)
        self.setShowGrid(True)
        self.verticalHeader().setDefaultSectionSize(22)
        self.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Fixed)
        self.setFont(QtGui.QFont("Consolas"))

    # Copy: decimals (Index as decimal, others as decimal values)
    def keyPressEvent(self, event: QtGui.QKeyEvent) -> None:
        if event.matches(QtGui.QKeySequence.Copy):
            self._copy_selection()
            return
        if event.matches(QtGui.QKeySequence.Paste):
            self._paste_into_selection()
            return
        super().keyPressEvent(event)

    def _copy_selection(self):
        model = self.model()
        if model is None:
            return
        sel = self.selectionModel().selectedIndexes()
        if not sel:
            return
        # Sort by row, col
        sel_sorted = sorted(sel, key=lambda i: (i.row(), i.column()))
        top_left = sel_sorted[0]
        bottom_right = sel_sorted[-1]
        r0, c0 = top_left.row(), top_left.column()
        r1, c1 = bottom_right.row(), bottom_right.column()
        rows = []
        for r in range(r0, r1 + 1):
            cols = []
            for c in range(c0, c1 + 1):
                idx = model.index(r, c)
                if c == 0:
                    cols.append(str(r))
                else:
                    # convert current display binary to decimal on copy
                    bin_str = model.data(idx, QtCore.Qt.DisplayRole)
                    try:
                        cols.append(str(int(bin_str, 2)))
                    except Exception:
                        cols.append("")
            rows.append("\t".join(cols))
        QtWidgets.QApplication.clipboard().setText("\n".join(rows))

    def _paste_into_selection(self):
        model: BufferTableModel = self.model()  # type: ignore
        if model is None:
            return
        sel = self.selectionModel().selectedIndexes()
        if not sel:
            return
        # Anchor at the top-left selected cell
        top_left = sorted(sel, key=lambda i: (i.row(), i.column()))[0]
        r0, c0 = top_left.row(), top_left.column()
        if c0 == 0:
            c0 = 1  # can't edit index column

        text = QtWidgets.QApplication.clipboard().text()
        if not text:
            return
        lines = [ln for ln in text.splitlines() if ln.strip() != ""]
        err_count = 0
        for dr, line in enumerate(lines):
            cols = [c for c in line.split("\t")]
            for dc, token in enumerate(cols):
                col = c0 + dc
                if col > 3:
                    continue
                idx = model.index(r0 + dr, col)
                ok, val, err = BufferTableModel.validate_input(token.strip())
                if not ok:
                    err_count += 1
                    continue
                # Set EditRole with original binary text to preserve leading zeros length
                model.setData(idx, token.strip(), role=QtCore.Qt.EditRole)
        if err_count:
            self.validation_error.emit("Some cells rejected: binary only (1–8 bits).")


class BufferTableWidget(QtWidgets.QWidget):
    """Composite widget that wraps a QTableView + model + delegate,
    exposing the spec'd API: load_buffer_data, get_buffer_data, clear_all, etc.
    """

    validation_error = QtCore.pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        vbox = QtWidgets.QVBoxLayout(self)
        vbox.setContentsMargins(0, 0, 0, 0)

        self._model = BufferTableModel()
        self._view = BufferTableView(self)
        self._view.setModel(self._model)
        # Binary delegate for X, Y, Flags
        delegate = BinaryDelegate(self._view)
        self._view.setItemDelegateForColumn(1, delegate)
        self._view.setItemDelegateForColumn(2, delegate)
        self._view.setItemDelegateForColumn(3, delegate)

        # Column sizing
        self._view.setColumnWidth(0, 60)  # Index
        for c in (1, 2, 3):
            self._view.setColumnWidth(c, 110)

        # Connect validation errors upwards
        self._view.validation_error.connect(self.validation_error)

        # NOTE/TODO: Frozen first column (index) left-locked — future enhancement

        vbox.addWidget(self._view)

    # Public API per spec
    def load_buffer_data(self, buffer_data: BufferData):
        self._model.load_buffer(buffer_data)

    def get_buffer_data(self) -> BufferData:
        return self._model._buffer

    def clear_all(self):
        self._model.clear_all()

    def validate_input(self, row: int, col: int, value: str):
        return BufferTableModel.validate_input(value)

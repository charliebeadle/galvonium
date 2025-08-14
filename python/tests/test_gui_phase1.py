import unittest
import sys
from PyQt5 import QtWidgets, QtCore

# Ensure package path includes python/
# (Adjust if your runner differs)

app = QtWidgets.QApplication.instance() or QtWidgets.QApplication(sys.argv)

from gui.buffer_table import BufferTableWidget, BufferTableModel
from models.buffer_model import BufferData


class TestBufferTableModel(unittest.TestCase):
    def setUp(self):
        self.buf = BufferData()
        self.widget = BufferTableWidget()
        self.widget.load_buffer_data(self.buf)
        self.model = self.widget._model  # type: ignore

    def test_row_col_counts(self):
        self.assertEqual(self.model.rowCount(), 256)
        self.assertEqual(self.model.columnCount(), 4)

    def test_display_binary_padding(self):
        # default zeros => "00000000"
        idx = self.model.index(0, 1)
        self.assertEqual(self.model.data(idx, QtCore.Qt.DisplayRole), "00000000")

    def test_edit_accepts_binary_only(self):
        idx_x = self.model.index(0, 1)
        ok = self.model.setData(idx_x, "1010", role=QtCore.Qt.EditRole)
        self.assertTrue(ok)
        # now value should display padded
        self.assertEqual(self.model.data(idx_x, QtCore.Qt.DisplayRole), "00001010")

        # reject non-binary
        idx_y = self.model.index(0, 2)
        ok2 = self.model.setData(idx_y, "12", role=QtCore.Qt.EditRole)
        self.assertFalse(ok2)

    def test_clear_all_sets_zero(self):
        self.model.setData(self.model.index(5, 1), "11111111", QtCore.Qt.EditRole)
        self.model.clear_all()
        self.assertEqual(
            self.model.data(self.model.index(5, 1), QtCore.Qt.DisplayRole), "00000000"
        )


if __name__ == "__main__":
    unittest.main()

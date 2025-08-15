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

    def test_display_decimal_for_xy(self):
        # X and Y columns should display in decimal
        idx_x = self.model.index(0, 1)
        idx_y = self.model.index(0, 2)
        self.assertEqual(self.model.data(idx_x, QtCore.Qt.DisplayRole), "0")
        self.assertEqual(self.model.data(idx_y, QtCore.Qt.DisplayRole), "0")

    def test_display_binary_for_flags(self):
        # Flags column should still display in binary
        idx_flags = self.model.index(0, 3)
        self.assertEqual(self.model.data(idx_flags, QtCore.Qt.DisplayRole), "00000000")

    def test_edit_accepts_binary_only(self):
        idx_x = self.model.index(0, 1)
        ok = self.model.setData(idx_x, "1010", role=QtCore.Qt.EditRole)
        self.assertTrue(ok)
        # now value should display as decimal
        self.assertEqual(self.model.data(idx_x, QtCore.Qt.DisplayRole), "10")

        # reject non-binary
        idx_y = self.model.index(0, 2)
        ok2 = self.model.setData(idx_y, "12", role=QtCore.Qt.EditRole)
        self.assertFalse(ok2)

    def test_clear_all_sets_zero(self):
        self.model.setData(self.model.index(5, 1), "11111111", QtCore.Qt.EditRole)
        self.model.clear_all()
        self.assertEqual(
            self.model.data(self.model.index(5, 1), QtCore.Qt.DisplayRole), "0"
        )

    def test_invalid_cells_highlighted_red(self):
        # Test that invalid input highlights cells in red
        idx = self.model.index(0, 1)

        # Try to set invalid data
        ok = self.model.setData(idx, "invalid", role=QtCore.Qt.EditRole)
        self.assertFalse(ok)

        # Check that the cell is now in the invalid cells set
        self.assertIn((0, 1), self.model._invalid_cells)

        # Check that the background role returns red color
        background = self.model.data(idx, QtCore.Qt.BackgroundRole)
        self.assertIsNotNone(background)
        self.assertEqual(background.color().name(), "#ffcccc")  # light red

    def test_copy_paste_with_decimal_display(self):
        # Test that copy/paste works correctly with decimal display
        # Set some values
        self.model.setData(self.model.index(0, 1), "1010", QtCore.Qt.EditRole)  # X = 10
        self.model.setData(self.model.index(0, 2), "1111", QtCore.Qt.EditRole)  # Y = 15

        # Verify display shows decimal
        self.assertEqual(
            self.model.data(self.model.index(0, 1), QtCore.Qt.DisplayRole), "10"
        )
        self.assertEqual(
            self.model.data(self.model.index(0, 2), QtCore.Qt.DisplayRole), "15"
        )


if __name__ == "__main__":
    unittest.main()

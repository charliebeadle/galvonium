from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QFileDialog
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import numpy as np
import csv
import re
from typing import List, Tuple, Optional


class DataVisualizer(QtWidgets.QDialog):
    """Dialog window for visualizing real-time coordinate data from Arduino."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Data Visualizer")
        self.setGeometry(100, 100, 800, 600)

        # Data storage
        self.x_coords: List[float] = []
        self.y_coords: List[float] = []
        self.is_collecting = False

        # Setup UI
        self._setup_ui()
        self._setup_plot()

    def _setup_ui(self):
        """Setup the user interface."""
        layout = QVBoxLayout(self)

        # Control buttons
        button_layout = QHBoxLayout()

        self.start_stop_btn = QPushButton("Start Collection")
        self.start_stop_btn.clicked.connect(self._toggle_collection)

        self.clear_btn = QPushButton("Clear Data")
        self.clear_btn.clicked.connect(self._clear_data)

        self.export_btn = QPushButton("Export to CSV")
        self.export_btn.clicked.connect(self._export_csv)
        self.export_btn.setEnabled(False)

        button_layout.addWidget(self.start_stop_btn)
        button_layout.addWidget(self.clear_btn)
        button_layout.addWidget(self.export_btn)
        button_layout.addStretch()

        layout.addLayout(button_layout)

        # Status label
        self.status_label = QLabel("Ready. Click 'Start Collection' to begin.")
        layout.addWidget(self.status_label)

        # Matplotlib canvas
        self.figure = Figure(figsize=(8, 6))
        self.canvas = FigureCanvas(self.figure)
        layout.addWidget(self.canvas)

        # Coordinate display label
        self.coord_label = QLabel("Coordinates: None")
        layout.addWidget(self.coord_label)

    def _setup_plot(self):
        """Setup the matplotlib plot."""
        self.ax = self.figure.add_subplot(111)
        self.ax.set_xlabel("X Coordinate")
        self.ax.set_ylabel("Y Coordinate")
        self.ax.set_title("Real-time Coordinate Data")
        self.ax.grid(True, alpha=0.3)

        # Initialize empty plot
        (self.line,) = self.ax.plot([], [], "r-", linewidth=1, alpha=0.7)
        self.points = self.ax.scatter([], [], c="black", s=20, alpha=0.8)

        # Set fixed axis limits (0-256 for Q8.8 format)
        self.ax.set_xlim(0, 256)
        self.ax.set_ylim(0, 256)

        # Connect mouse motion for coordinate display
        self.canvas.mpl_connect("motion_notify_event", self._on_mouse_move)

    def _toggle_collection(self):
        """Toggle data collection on/off."""
        if self.is_collecting:
            self.is_collecting = False
            self.start_stop_btn.setText("Start Collection")
            self.status_label.setText("Data collection stopped.")
        else:
            self.is_collecting = True
            self.start_stop_btn.setText("Stop Collection")
            self.status_label.setText(
                "Data collection active. Waiting for START marker..."
            )

    def _clear_data(self):
        """Clear all collected data."""
        self.x_coords.clear()
        self.y_coords.clear()
        self._update_plot()
        self.status_label.setText("Data cleared.")
        self.export_btn.setEnabled(False)

    def _export_csv(self):
        """Export collected data to CSV file."""
        if not self.x_coords:
            QtWidgets.QMessageBox.warning(self, "No Data", "No data to export.")
            return

        filename, _ = QFileDialog.getSaveFileName(
            self, "Export to CSV", "", "CSV Files (*.csv)"
        )

        if filename:
            try:
                with open(filename, "w", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerow(["X", "Y"])
                    for x, y in zip(self.x_coords, self.y_coords):
                        writer.writerow([x, y])

                QtWidgets.QMessageBox.information(
                    self, "Success", f"Data exported to {filename}"
                )
            except Exception as e:
                QtWidgets.QMessageBox.critical(
                    self, "Error", f"Failed to export: {str(e)}"
                )

    def _on_mouse_move(self, event):
        """Handle mouse movement over the plot."""
        if event.inaxes != self.ax:
            self.coord_label.setText("Coordinates: None")
            return

        # Find closest point
        if self.x_coords and self.y_coords:
            x_data = np.array(self.x_coords)
            y_data = np.array(self.y_coords)

            # Calculate distance to each point
            distances = np.sqrt(
                (x_data - event.xdata) ** 2 + (y_data - event.ydata) ** 2
            )
            closest_idx = np.argmin(distances)

            # If close enough to a point, show its coordinates
            if distances[closest_idx] < 0.1:  # Threshold for "close enough"
                x = self.x_coords[closest_idx]
                y = self.y_coords[closest_idx]
                self.coord_label.setText(f"Coordinates: X={x:.3f}, Y={y:.3f}")
            else:
                self.coord_label.setText(
                    f"Mouse: X={event.xdata:.3f}, Y={event.ydata:.3f}"
                )
        else:
            self.coord_label.setText("Coordinates: None")

    def process_data(self, data: str):
        """Process incoming data from Arduino."""
        if not self.is_collecting:
            return

        # Check for START marker
        if "START" in data:
            self._clear_data()
            self.status_label.setText(
                "Data collection active. Receiving coordinates (plot will update when complete)..."
            )
            return

        # Check for END marker
        if "END" in data:
            self.status_label.setText(
                f"Data collection complete. {len(self.x_coords)} points collected."
            )
            self.export_btn.setEnabled(True)
            # Update plot now that all data is received
            self._update_plot()
            return

        # Try to parse coordinate data (two hex values separated by space)
        # Expected format: "X Y" where X and Y are hex values (leading zeros may be omitted)
        coord_match = re.match(r"^([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)$", data.strip())
        if coord_match:
            try:
                x_hex = coord_match.group(1)
                y_hex = coord_match.group(2)

                # Validate that hex values are reasonable for Q8.8 format (max 0xFFFF)
                x_int = int(x_hex, 16)
                y_int = int(y_hex, 16)

                if x_int > 0xFFFF or y_int > 0xFFFF:
                    # Values too large for Q8.8 format, ignore
                    return

                # Convert Q8.8 hex to decimal
                x_decimal = self._hex_q88_to_decimal(x_hex)
                y_decimal = self._hex_q88_to_decimal(y_hex)

                self.x_coords.append(x_decimal)
                self.y_coords.append(y_decimal)

                # Don't update plot in real-time to avoid lag
                self.status_label.setText(
                    f"Received coordinate: X={x_decimal:.3f}, Y={y_decimal:.3f}"
                )

            except ValueError as e:
                # Invalid hex data, ignore
                pass

    def _hex_q88_to_decimal(self, hex_str: str) -> float:
        """Convert Q8.8 hex format to decimal.

        Q8.8 format: 8 integer bits + 8 fractional bits
        Example: 0x0100 = 1.0, 0x0080 = 0.5

        Note: hex_str may be shorter than 4 digits (e.g., "0" instead of "0000")
        """
        # Pad hex string to 4 digits if needed (e.g., "0" -> "0000")
        padded_hex = hex_str.zfill(4)

        # Convert hex to integer
        value = int(padded_hex, 16)

        # Extract integer part (bits 8-15)
        integer_part = (value >> 8) & 0xFF

        # Extract fractional part (bits 0-7)
        fractional_part = value & 0xFF

        # Convert to decimal
        decimal_value = integer_part + (fractional_part / 256.0)

        return decimal_value

    def _update_plot(self):
        """Update the plot with current data."""
        if not self.x_coords:
            # Clear plot if no data
            self.line.set_data([], [])
            self.points.set_offsets(np.column_stack([[], []]))
        else:
            # Update line plot
            self.line.set_data(self.x_coords, self.y_coords)

            # Update scatter plot
            self.points.set_offsets(np.column_stack([self.x_coords, self.y_coords]))

            # Axes are fixed at 0-256, no need to adjust

        self.canvas.draw()

    def closeEvent(self, event):
        """Handle window close event."""
        self.is_collecting = False
        super().closeEvent(event)

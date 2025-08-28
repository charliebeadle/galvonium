#!/usr/bin/env python3
"""
Demo script for the Data Visualizer widget.
This script demonstrates how the data visualizer processes Arduino data.
"""

import sys
from PyQt5.QtWidgets import QApplication
from gui.data_visualizer import DataVisualizer


def main():
    """Main demo function."""
    app = QApplication(sys.argv)

    # Create the data visualizer
    visualizer = DataVisualizer()
    visualizer.show()

    # Start collection
    visualizer._toggle_collection()

    # Simulate Arduino data stream
    print("Simulating Arduino data stream...")
    print("Data format: START -> coordinates -> END")
    print("Note: Plot will only update after END marker is received")

    # Sample data in Q8.8 format (including short hex values)
    sample_data = [
        "START",
        "0100 0200",  # X=1.0, Y=2.0
        "0200 0300",  # X=2.0, Y=3.0
        "0300 0100",  # X=3.0, Y=1.0
        "100 0",  # X=1.0, Y=0.0 (short hex)
        "0 100",  # X=0.0, Y=1.0 (short hex)
        "80 80",  # X=0.5, Y=0.5 (short hex)
        "END",
    ]

    # Process each line with a delay to simulate real-time
    import time

    for i, data in enumerate(sample_data):
        print(f"Processing: {data}")
        visualizer.process_data(data)
        time.sleep(0.5)  # Half second delay between data points

    print("Demo complete! The visualizer should now show the plotted coordinates.")
    print("Note: The plot only appeared after the END marker was processed.")
    print("You can hover over points to see exact coordinates.")
    print("Use the Export to CSV button to save the data.")

    # Run the application
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()

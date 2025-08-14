from PyQt5 import QtWidgets
from gui.main_window import MainWindow
import sys

APP_ORG = "Galvonium"
APP_NAME = "LaserGUI"


def main():
    # Single QApplication instance
    app = QtWidgets.QApplication.instance() or QtWidgets.QApplication(sys.argv)
    app.setOrganizationName(APP_ORG)
    app.setApplicationName(APP_NAME)

    win = MainWindow()
    win.show()

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()

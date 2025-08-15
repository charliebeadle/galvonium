from PyQt5 import QtWidgets
import sys

APP_ORG = "Galvonium"
APP_NAME = "LaserGUI"


def main():
    # Single QApplication instance
    app = QtWidgets.QApplication.instance() or QtWidgets.QApplication(sys.argv)
    app.setOrganizationName(APP_ORG)
    app.setApplicationName(APP_NAME)

    try:
        from gui.main_window import MainWindow
        win = MainWindow()
        win.show()
    except ImportError as e:
        # Show error dialog for missing dependencies
        error_dialog = QtWidgets.QMessageBox()
        error_dialog.setIcon(QtWidgets.QMessageBox.Critical)
        error_dialog.setWindowTitle("Import Error")
        error_dialog.setText("Failed to import required modules")
        error_dialog.setInformativeText(str(e))
        error_dialog.setDetailedText(
            "This usually means some required packages are not installed.\n"
            "Please check that all dependencies are properly installed."
        )
        error_dialog.exec_()
        return 1
    except Exception as e:
        # Show error dialog for other errors
        error_dialog = QtWidgets.QMessageBox()
        error_dialog.setIcon(QtWidgets.QMessageBox.Critical)
        error_dialog.setWindowTitle("Error")
        error_dialog.setText("Failed to start application")
        error_dialog.setInformativeText(str(e))
        error_dialog.exec_()
        return 1

    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())

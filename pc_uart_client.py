#!/usr/bin/env python3
"""PyQt5 GUI to send images and display the UART detection stream."""

import sys

import serial
import cv2
import numpy as np
from PyQt5 import QtCore, QtGui, QtWidgets

import pc_uart_utils as utils


class StreamThread(QtCore.QThread):
    """Background thread reading frames from the MCU."""

    frame_received = QtCore.pyqtSignal(np.ndarray)

    def __init__(self, ser: serial.Serial):
        super().__init__()
        self.ser = ser
        self._running = True

    def run(self) -> None:
        self.ser.reset_input_buffer()
        while self._running:
            frame, _, _ = utils.read_frame(self.ser)
            if frame is None:
                continue
            _, dets = utils.read_detections(self.ser)
            frame = utils.draw_detections(frame, dets)
            self.frame_received.emit(frame)

    def stop(self) -> None:
        self._running = False
        self.wait()


class App(QtWidgets.QMainWindow):
    """Main application window."""

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("UART Object Detection Client")

        central = QtWidgets.QWidget()
        self.setCentralWidget(central)

        layout = QtWidgets.QGridLayout(central)

        layout.addWidget(QtWidgets.QLabel("Port"), 0, 0)
        self.port_edit = QtWidgets.QLineEdit("COM3")
        layout.addWidget(self.port_edit, 0, 1)

        layout.addWidget(QtWidgets.QLabel("Baud"), 1, 0)
        self.baud_edit = QtWidgets.QLineEdit(str(921600 * 8))
        layout.addWidget(self.baud_edit, 1, 1)

        layout.addWidget(QtWidgets.QLabel("Width"), 2, 0)
        self.width_edit = QtWidgets.QLineEdit("128")
        layout.addWidget(self.width_edit, 2, 1)

        layout.addWidget(QtWidgets.QLabel("Height"), 3, 0)
        self.height_edit = QtWidgets.QLineEdit("128")
        layout.addWidget(self.height_edit, 3, 1)

        self.connect_btn = QtWidgets.QPushButton("Connect")
        self.connect_btn.clicked.connect(self.connect)
        layout.addWidget(self.connect_btn, 4, 0)

        self.disconnect_btn = QtWidgets.QPushButton("Disconnect")
        self.disconnect_btn.setEnabled(False)
        self.disconnect_btn.clicked.connect(self.disconnect)
        layout.addWidget(self.disconnect_btn, 4, 1)

        self.send_btn = QtWidgets.QPushButton("Send Images")
        self.send_btn.setEnabled(False)
        self.send_btn.clicked.connect(self.send_images)
        layout.addWidget(self.send_btn, 5, 0, 1, 2)

        self.image_label = QtWidgets.QLabel()
        self.image_label.setFixedSize(640, 480)
        self.image_label.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(self.image_label, 6, 0, 1, 2)

        self.ser: serial.Serial | None = None
        self.stream_thread: StreamThread | None = None

    # ------------------------------------------------------------------
    def connect(self) -> None:
        try:
            self.ser = serial.Serial(
                self.port_edit.text(), int(self.baud_edit.text()), timeout=1
            )
        except Exception as exc:  # pragma: no cover - UI feedback only
            QtWidgets.QMessageBox.critical(self, "Connection error", str(exc))
            return

        self.connect_btn.setEnabled(False)
        self.disconnect_btn.setEnabled(True)
        self.send_btn.setEnabled(True)

        self.start_stream()

    # ------------------------------------------------------------------
    def start_stream(self) -> None:
        if not self.ser:
            return
        self.stream_thread = StreamThread(self.ser)
        self.stream_thread.frame_received.connect(self.update_frame)
        self.stream_thread.start()

    # ------------------------------------------------------------------
    def disconnect(self) -> None:
        if self.stream_thread:
            self.stream_thread.stop()
            self.stream_thread = None
        if self.ser:
            self.ser.close()
            self.ser = None
        self.connect_btn.setEnabled(True)
        self.disconnect_btn.setEnabled(False)
        self.send_btn.setEnabled(False)

    # ------------------------------------------------------------------
    def send_images(self) -> None:
        if not self.ser:
            QtWidgets.QMessageBox.warning(
                self,
                "Not connected",
                "Connect first",
            )
            return
        files, _ = QtWidgets.QFileDialog.getOpenFileNames(
            self, "Select image files"
        )
        if not files:
            return

        # pause streaming while sending
        was_streaming = self.stream_thread is not None
        if was_streaming:
            self.stream_thread.stop()
            self.stream_thread = None

        for img in files:
            utils.send_image(
                self.ser,
                img,
                (int(self.width_edit.text()), int(self.height_edit.text())),
                display=False,
                rx=False,
                preview=True,
            )

        if was_streaming:
            self.start_stream()

    # ------------------------------------------------------------------
    def update_frame(self, frame: np.ndarray) -> None:
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, _ = rgb.shape
        qimg = QtGui.QImage(
            rgb.data, w, h, 3 * w, QtGui.QImage.Format.Format_RGB888
        )
        pix = QtGui.QPixmap.fromImage(qimg).scaled(
            self.image_label.size(), QtCore.Qt.AspectRatioMode.KeepAspectRatio
        )
        self.image_label.setPixmap(pix)

    # ------------------------------------------------------------------
    def closeEvent(self, event: QtGui.QCloseEvent) -> None:  # pragma: no cover
        self.disconnect()
        event.accept()


def main() -> None:
    app = QtWidgets.QApplication(sys.argv)
    window = App()
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()

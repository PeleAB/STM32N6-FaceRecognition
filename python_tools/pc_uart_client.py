#!/usr/bin/env python3
"""PySide6 GUI to send images and display the UART detection stream."""

import sys
import re
from pathlib import Path
import time

import serial
from serial.tools import list_ports
import cv2
import numpy as np
from PySide6 import QtCore, QtGui, QtWidgets
import onnxruntime as ort

from centerface import CenterFace

import pc_uart_utils as utils


def crop_align(image: np.ndarray, box: np.ndarray, left_eye: np.ndarray,
               right_eye: np.ndarray, size=(96, 112)) -> np.ndarray:
    """Crop and align face using eye landmarks without squashing."""
    h, w, _ = image.shape
    x_center = (box[0] + box[2]) / 2 * w
    y_center = (box[1] + box[3]) / 2 * h
    width = (box[2] - box[0]) * w
    height = (box[3] - box[1]) * h
    lx = left_eye[0] * w
    ly = left_eye[1] * h
    rx = right_eye[0] * w
    ry = right_eye[1] * h

    angle = -np.arctan2(ry - ly, rx - lx)
    cos_a = np.cos(angle)
    sin_a = np.sin(angle)

    dst_w, dst_h = size
    dst_full = max(dst_w, dst_h)
    off_x = (dst_full - dst_w) / 2.0
    off_y = (dst_full - dst_h) / 2.0

    out = np.zeros((dst_h, dst_w, 3), dtype=image.dtype)
    for y in range(dst_h):
        ny = ((y + off_y) + 0.5) / dst_full - 0.5
        for x in range(dst_w):
            nx = ((x + off_x) + 0.5) / dst_full - 0.5
            src_x = x_center + (nx * width) * cos_a + (ny * height) * sin_a
            src_y = y_center + (ny * height) * cos_a - (nx * width) * sin_a
            src_x = np.clip(src_x, 0, w - 1)
            src_y = np.clip(src_y, 0, h - 1)
            out[y, x] = image[int(src_y), int(src_x)]
    return out


def inflate_box(box: np.ndarray, factor: float = 1.2) -> np.ndarray:
    """Return *box* scaled by *factor* around its center."""
    cx = (box[0] + box[2]) / 2
    cy = (box[1] + box[3]) / 2
    w = (box[2] - box[0]) * factor
    h = (box[3] - box[1]) * factor
    half = np.array([w / 2, h / 2], dtype=box.dtype)
    new_box = np.array([cx - half[0], cy - half[1], cx + half[0], cy + half[1]],
                       dtype=box.dtype)
    new_box = np.clip(new_box, 0.0, 1.0)
    return new_box


def compute_embedding(img_path: Path, detector: CenterFace, sess: ort.InferenceSession,
                      input_name: str, output_name: str) -> np.ndarray | None:
    """Return L2-normalized embedding for *img_path* or None on failure."""
    img = cv2.imread(str(img_path))
    if img is None:
        print(f"Failed to read {img_path}")
        return None

    h0, w0, _ = img.shape
    crop_size = min(h0, w0)
    off_x = (w0 - crop_size) // 2
    off_y = (h0 - crop_size) // 2
    img_sq = img[off_y : off_y + crop_size, off_x : off_x + crop_size]

    dets, lms = detector.inference(img_sq, threshold=0.5)
    if len(dets) == 0:
        print(f"No face detected in {img_path}")
        return None

    det = dets[0]
    lm = lms[0]
    box = np.array([
        det[0] / img_sq.shape[1],
        det[1] / img_sq.shape[0],
        det[2] / img_sq.shape[1],
        det[3] / img_sq.shape[0],
    ], dtype=np.float32)
    box = inflate_box(box)
    left_eye = np.array([lm[0] / img_sq.shape[1], lm[1] / img_sq.shape[0]], dtype=np.float32)
    right_eye = np.array([lm[2] / img_sq.shape[1], lm[3] / img_sq.shape[0]], dtype=np.float32)
    aligned = crop_align(img_sq, box, left_eye, right_eye, size=(96, 112))

    face_rgb = cv2.cvtColor(aligned, cv2.COLOR_BGR2RGB).astype(np.int16)
    face_rgb -= 128
    face = np.transpose(face_rgb.astype(np.int8), (2, 0, 1))[None, ...]

    onnx_out = sess.run([output_name], {input_name: face})[0]
    embedding = onnx_out.astype(np.int8).flatten() / 128.0

    norm = np.linalg.norm(embedding)
    if norm > 0:
        embedding /= norm
    return embedding


def load_target_embedding() -> np.ndarray | None:
    """Parse the target embedding array from the firmware source."""
    path = Path(__file__).resolve().parents[1] / "Src" / "target_embedding.c"
    if not path.exists():
        return None
    text = path.read_text()
    m = re.search(r"\{([^}]+)\}", text)
    if not m:
        return None
    try:
        numbers = [float(v) for v in m.group(1).split(',') if v.strip()]
    except ValueError:
        return None
    return np.array(numbers, dtype=np.float32)

class StreamThread(QtCore.QThread):
    """Background thread reading frames from the MCU."""

    frame_received = QtCore.Signal(np.ndarray)
    aligned_received = QtCore.Signal(np.ndarray)
    fps_updated = QtCore.Signal(float)

    def __init__(self, ser: serial.Serial, target_emb: np.ndarray | None = None):
        super().__init__()
        self.ser = ser
        self._running = True
        self.target_emb = target_emb

    def run(self) -> None:
        self.ser.reset_input_buffer()
        last = time.time()
        count = 0
        while self._running:
            tag, frame, _, _ = utils.read_frame(self.ser)
            if frame is None:
                continue
            if tag == "ALN":
                emb = utils.read_embedding(self.ser)
                if emb and self.target_emb is not None and len(emb) == len(self.target_emb):
                    e = np.array(emb, dtype=np.float32)
                    a = e / (np.linalg.norm(e) + 1e-6)
                    b = self.target_emb / (np.linalg.norm(self.target_emb) + 1e-6)
                    sim = float(np.dot(a, b))
                    print(f"Similarity: {sim:.4f}")
                self.aligned_received.emit(frame)
            else:
                _, dets = utils.read_detections(self.ser)
                frame = utils.draw_detections(frame, dets)
                self.frame_received.emit(frame)
            count += 1
            now = time.time()
            if now - last >= 1.0:
                self.fps_updated.emit(count / (now - last))
                count = 0
                last = now

    def stop(self) -> None:
        self._running = False
        # Flush any pending data so the MCU can resume without overflowing
        try:
            self.ser.reset_input_buffer()
        except serial.SerialException:
            pass
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
        self.port_combo = QtWidgets.QComboBox()
        layout.addWidget(self.port_combo, 0, 1)
        refresh_btn = QtWidgets.QPushButton("Refresh")
        refresh_btn.clicked.connect(self.refresh_ports)
        layout.addWidget(refresh_btn, 0, 2)
        self.refresh_ports()

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

        self.enroll_btn = QtWidgets.QPushButton("Enroll")
        self.enroll_btn.clicked.connect(self.enroll)
        layout.addWidget(self.enroll_btn, 6, 0, 1, 2)

        self.image_label = QtWidgets.QLabel()
        self.image_label.setFixedSize(640, 480)
        self.image_label.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(self.image_label, 7, 0, 1, 2)

        self.aligned_label = QtWidgets.QLabel()
        self.aligned_label.setFixedSize(224, 224)
        self.aligned_label.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(self.aligned_label, 8, 0, 1, 2)

        self.fps_label = QtWidgets.QLabel("FPS: 0.0")
        layout.addWidget(self.fps_label, 9, 0, 1, 2)

        self.progress = QtWidgets.QProgressBar()
        self.progress.setVisible(False)
        layout.addWidget(self.progress, 10, 0, 1, 2)

        self.ser: serial.Serial | None = None
        self.stream_thread: StreamThread | None = None

    def refresh_ports(self) -> None:
        self.port_combo.clear()
        for p in list_ports.comports():
            self.port_combo.addItem(p.device)

    # ------------------------------------------------------------------
    def connect(self) -> None:
        try:
            self.ser = serial.Serial(
                self.port_combo.currentText(), int(self.baud_edit.text()), timeout=1
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
        target = load_target_embedding()
        self.stream_thread = StreamThread(self.ser, target)
        self.stream_thread.frame_received.connect(self.update_frame)
        self.stream_thread.aligned_received.connect(self.update_aligned)
        self.stream_thread.fps_updated.connect(self.update_fps)
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

        self.progress.setVisible(True)
        self.progress.setMaximum(len(files))
        self.progress.setValue(0)
        QtWidgets.QApplication.processEvents()

        for i, img in enumerate(files, 1):
            utils.send_image(
                self.ser,
                img,
                (int(self.width_edit.text()), int(self.height_edit.text())),
                display=False,
                rx=False,
                preview=True,
            )
            self.progress.setValue(i)
            QtWidgets.QApplication.processEvents()

        self.progress.setVisible(False)

        if was_streaming:
            self.start_stream()

    # ------------------------------------------------------------------
    def enroll(self) -> None:
        dir_path = QtWidgets.QFileDialog.getExistingDirectory(
            self, "Select directory with face images"
        )
        if not dir_path:
            return

        model_dir = Path(__file__).resolve().parent / "models"
        det_model = model_dir / "centerface_1x3xHxW_integer_quant.tflite"
        rec_model = model_dir / "mobilefacenet_integer_quant_1_OE_3_2_0.onnx"
        detector = CenterFace(str(det_model))
        sess = ort.InferenceSession(str(rec_model))
        input_name = sess.get_inputs()[0].name
        output_name = sess.get_outputs()[0].name

        embeddings = []
        for ext in ("*.jpg", "*.jpeg", "*.png"):
            for img_path in Path(dir_path).glob(ext):
                emb = compute_embedding(img_path, detector, sess, input_name, output_name)
                if emb is not None:
                    embeddings.append(emb)

        if not embeddings:
            QtWidgets.QMessageBox.information(self, "Enroll", "No embeddings generated")
            return

        avg = np.mean(np.stack(embeddings), axis=0)
        norm = np.linalg.norm(avg)
        if norm > 0:
            avg /= norm

        print("Enrollment embedding:")
        print(" ".join(f"{x:.6f}" for x in avg))

        emb_line = (
            "float target_embedding[EMBEDDING_SIZE] = {"
            + ", ".join(f"{x:.6f}" for x in avg)
            + "};\n"
        )
        c_path = Path(__file__).resolve().parents[1] / "Src" / "target_embedding.c"
        lines = c_path.read_text().splitlines(keepends=True)
        for i, line in enumerate(lines):
            if line.strip().startswith("float target_embedding"):
                lines[i] = emb_line
                break
        c_path.write_text("".join(lines))
        QtWidgets.QMessageBox.information(self, "Enroll", f"Updated {c_path}")

        if self.stream_thread is not None:
            self.stream_thread.target_emb = avg

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
    def update_aligned(self, frame: np.ndarray) -> None:
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, _ = rgb.shape
        qimg = QtGui.QImage(
            rgb.data, w, h, 3 * w, QtGui.QImage.Format.Format_RGB888
        )
        pix = QtGui.QPixmap.fromImage(qimg).scaled(
            self.aligned_label.size(), QtCore.Qt.AspectRatioMode.KeepAspectRatio
        )
        self.aligned_label.setPixmap(pix)

    # ------------------------------------------------------------------
    def update_fps(self, fps: float) -> None:
        self.fps_label.setText(f"FPS: {fps:.1f}")

    # ------------------------------------------------------------------
    def closeEvent(self, event: QtGui.QCloseEvent) -> None:  # pragma: no cover
        self.disconnect()
        event.accept()


def main() -> None:
    app = QtWidgets.QApplication(sys.argv)
    window = App()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()

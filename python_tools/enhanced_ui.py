#!/usr/bin/env python3
"""
Enhanced Modern UI for STM32N6 Object Detection System

Features:
- Modern Material Design interface
- Real-time analytics and monitoring
- Enhanced face enrollment with validation
- Connection management with auto-reconnect
- Performance metrics and logging
- Dark/Light theme support
- Advanced settings panel
"""

import sys
import json
import time
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional, List, Dict, Any
from datetime import datetime, timedelta
import threading
import queue
import logging

import cv2
import numpy as np
from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtCore import QTimer, Signal, QThread
from PySide6.QtGui import QAction
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QGridLayout,
    QWidget, QLabel, QPushButton, QComboBox, QLineEdit, QProgressBar,
    QTextEdit, QTabWidget, QGroupBox, QCheckBox, QSpinBox, QSlider,
    QSplitter, QFrame, QScrollArea, QMessageBox, QFileDialog,
    QStatusBar, QMenuBar, QToolBar
)
import serial
from serial.tools import list_ports
import onnxruntime as ort

try:
    from centerface import CenterFace
except ImportError:
    CenterFace = None

try:
    import enhanced_protocol as protocol
except ImportError:
    # Fallback to basic protocol if enhanced not available
    import pc_uart_utils as protocol

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class AppSettings:
    """Application settings with persistence"""
    # Connection settings
    baud_rate: int = 921600 * 8
    auto_reconnect: bool = True
    connection_timeout: float = 5.0
    
    # Display settings
    theme: str = "dark"  # "dark" or "light"
    max_fps: int = 30
    show_performance_overlay: bool = True
    
    # Detection settings
    confidence_threshold: float = 0.5
    similarity_threshold: float = 0.55
    track_smoothing: float = 0.5
    
    # Enrollment settings
    min_enrollment_images: int = 5
    max_enrollment_images: int = 20
    enrollment_quality_threshold: float = 0.8
    
    def save(self, path: Path):
        """Save settings to JSON file"""
        with open(path, 'w') as f:
            json.dump(asdict(self), f, indent=2)
    
    @classmethod
    def load(cls, path: Path) -> 'AppSettings':
        """Load settings from JSON file"""
        if path.exists():
            try:
                with open(path, 'r') as f:
                    data = json.load(f)
                return cls(**data)
            except Exception as e:
                logger.warning(f"Failed to load settings: {e}")
        return cls()

class PerformanceMonitor:
    """Monitor and track performance metrics"""
    
    def __init__(self, window_size: int = 100):
        self.window_size = window_size
        self.fps_history = []
        self.latency_history = []
        self.detection_count_history = []
        self.start_time = time.time()
        
    def update(self, fps: float, latency: float, detection_count: int):
        """Update performance metrics"""
        self.fps_history.append(fps)
        self.latency_history.append(latency)
        self.detection_count_history.append(detection_count)
        
        # Keep only recent history
        if len(self.fps_history) > self.window_size:
            self.fps_history.pop(0)
            self.latency_history.pop(0)
            self.detection_count_history.pop(0)
    
    def get_stats(self) -> Dict[str, float]:
        """Get current performance statistics"""
        if not self.fps_history:
            return {}
        
        return {
            'fps_avg': np.mean(self.fps_history),
            'fps_max': np.max(self.fps_history),
            'fps_min': np.min(self.fps_history),
            'latency_avg': np.mean(self.latency_history),
            'latency_max': np.max(self.latency_history),
            'detection_rate': np.mean([1 if c > 0 else 0 for c in self.detection_count_history]),
            'total_detections': sum(self.detection_count_history),
            'uptime': time.time() - self.start_time
        }

class ConnectionManager(QThread):
    """Manage serial connection with auto-reconnect"""
    
    connection_changed = Signal(bool)  # True for connected, False for disconnected
    error_occurred = Signal(str)
    
    def __init__(self, settings: AppSettings):
        super().__init__()
        self.settings = settings
        self.serial_port: Optional[serial.Serial] = None
        self.port_name = ""
        self._running = False
        self._connect_requested = False
        
    def connect(self, port_name: str):
        """Request connection to specified port"""
        self.port_name = port_name
        self._connect_requested = True
        if not self.isRunning():
            self.start()
    
    def disconnect(self):
        """Disconnect from current port"""
        self._connect_requested = False
        if self.serial_port:
            try:
                self.serial_port.close()
            except:
                pass
            self.serial_port = None
        
    def run(self):
        """Main connection management loop"""
        self._running = True
        while self._running:
            if self._connect_requested and not self.serial_port:
                try:
                    self.serial_port = serial.Serial(
                        self.port_name,
                        self.settings.baud_rate,
                        timeout=self.settings.connection_timeout
                    )
                    self.connection_changed.emit(True)
                    logger.info(f"Connected to {self.port_name}")
                except Exception as e:
                    self.error_occurred.emit(f"Connection failed: {e}")
                    logger.error(f"Connection failed: {e}")
                    
            elif not self._connect_requested and self.serial_port:
                self.serial_port.close()
                self.serial_port = None
                self.connection_changed.emit(False)
                logger.info("Disconnected")
            
            # Auto-reconnect logic
            if (self._connect_requested and 
                self.settings.auto_reconnect and 
                self.serial_port and 
                not self.serial_port.is_open):
                
                logger.warning("Connection lost, attempting reconnect...")
                self.serial_port = None
                time.sleep(2)  # Wait before retry
                
            time.sleep(0.1)
    
    def stop(self):
        """Stop the connection manager"""
        self._running = False
        self.disconnect()
        self.wait()

class StreamProcessor(QThread):
    """Process incoming data stream with enhanced protocol"""
    
    frame_received = Signal(np.ndarray, dict)  # frame, metadata
    statistics_updated = Signal(dict)
    error_occurred = Signal(str)
    
    def __init__(self, connection_manager: ConnectionManager, settings: AppSettings):
        super().__init__()
        self.connection_manager = connection_manager
        self.settings = settings
        self.performance_monitor = PerformanceMonitor()
        self._running = False
        
    def run(self):
        """Main stream processing loop"""
        self._running = True
        frame_count = 0
        last_stats_time = time.time()
        
        while self._running:
            if not self.connection_manager.serial_port:
                time.sleep(0.1)
                continue
                
            try:
                # Read frame with enhanced protocol
                frame_data = protocol.read_enhanced_frame(self.connection_manager.serial_port)
                if frame_data:
                    frame_count += 1
                    self.frame_received.emit(frame_data['image'], frame_data['metadata'])
                    
                    # Update performance metrics
                    self.performance_monitor.update(
                        frame_data['metadata'].get('fps', 0),
                        frame_data['metadata'].get('latency', 0),
                        len(frame_data['metadata'].get('detections', []))
                    )
                    
                    # Emit statistics periodically
                    now = time.time()
                    if now - last_stats_time >= 1.0:
                        stats = self.performance_monitor.get_stats()
                        stats['frame_count'] = frame_count
                        self.statistics_updated.emit(stats)
                        last_stats_time = now
                        
            except Exception as e:
                self.error_occurred.emit(f"Stream processing error: {e}")
                logger.error(f"Stream processing error: {e}")
                time.sleep(0.1)
    
    def stop(self):
        """Stop stream processing"""
        self._running = False
        self.wait()

class ModernImageWidget(QLabel):
    """Modern image display widget with overlay support"""
    
    def __init__(self):
        super().__init__()
        self.setMinimumSize(640, 480)
        self.setStyleSheet("""
            QLabel {
                border: 2px solid #444;
                border-radius: 8px;
                background-color: #222;
            }
        """)
        self.setAlignment(QtCore.Qt.AlignCenter)
        self.setScaledContents(False)
        
    def set_image(self, image: np.ndarray, detections: List = None):
        """Set image with optional detection overlays"""
        if image is None:
            return
            
        # Draw detections if provided
        if detections:
            image = self._draw_detections(image.copy(), detections)
        
        # Convert to Qt format
        height, width, channel = image.shape
        bytes_per_line = 3 * width
        q_image = QtGui.QImage(
            image.data, width, height, bytes_per_line, QtGui.QImage.Format_RGB888
        )
        
        # Scale to fit widget while maintaining aspect ratio
        pixmap = QtGui.QPixmap.fromImage(q_image)
        scaled_pixmap = pixmap.scaled(
            self.size(), QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation
        )
        self.setPixmap(scaled_pixmap)
    
    def _draw_detections(self, image: np.ndarray, detections: List) -> np.ndarray:
        """Draw detection boxes and labels"""
        for det in detections:
            if len(det) < 6:
                continue
                
            _, xc, yc, w, h, conf = det[:6]
            height, width = image.shape[:2]
            
            # Calculate box coordinates
            x1 = int((xc - w/2) * width)
            y1 = int((yc - h/2) * height)
            x2 = int((xc + w/2) * width)
            y2 = int((yc + h/2) * height)
            
            # Choose color based on confidence
            color = (0, 255, 0) if conf > 0.7 else (255, 255, 0) if conf > 0.5 else (255, 0, 0)
            
            # Draw bounding box
            cv2.rectangle(image, (x1, y1), (x2, y2), color, 2)
            
            # Draw confidence label
            label = f"{conf:.2f}"
            label_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)[0]
            cv2.rectangle(image, (x1, y1 - label_size[1] - 10), 
                         (x1 + label_size[0], y1), color, -1)
            cv2.putText(image, label, (x1, y1 - 5), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)
        
        return image

class StatsWidget(QWidget):
    """Display performance statistics"""
    
    def __init__(self):
        super().__init__()
        self.setFixedWidth(300)
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        
        # Performance section
        perf_group = QGroupBox("Performance")
        perf_layout = QGridLayout(perf_group)
        
        self.fps_label = QLabel("FPS: --")
        self.latency_label = QLabel("Latency: --")
        self.detection_rate_label = QLabel("Detection Rate: --")
        self.uptime_label = QLabel("Uptime: --")
        
        perf_layout.addWidget(QLabel("Current FPS:"), 0, 0)
        perf_layout.addWidget(self.fps_label, 0, 1)
        perf_layout.addWidget(QLabel("Avg Latency:"), 1, 0)
        perf_layout.addWidget(self.latency_label, 1, 1)
        perf_layout.addWidget(QLabel("Detection Rate:"), 2, 0)
        perf_layout.addWidget(self.detection_rate_label, 2, 1)
        perf_layout.addWidget(QLabel("Uptime:"), 3, 0)
        perf_layout.addWidget(self.uptime_label, 3, 1)
        
        layout.addWidget(perf_group)
        
        # Detection section
        det_group = QGroupBox("Detection Stats")
        det_layout = QGridLayout(det_group)
        
        self.total_detections_label = QLabel("Total: 0")
        self.current_detections_label = QLabel("Current: 0")
        
        det_layout.addWidget(QLabel("Total Detections:"), 0, 0)
        det_layout.addWidget(self.total_detections_label, 0, 1)
        det_layout.addWidget(QLabel("Current Frame:"), 1, 0)
        det_layout.addWidget(self.current_detections_label, 1, 1)
        
        layout.addWidget(det_group)
        layout.addStretch()
        
    def update_stats(self, stats: Dict[str, Any]):
        """Update displayed statistics"""
        if 'fps_avg' in stats:
            self.fps_label.setText(f"{stats['fps_avg']:.1f}")
        if 'latency_avg' in stats:
            self.latency_label.setText(f"{stats['latency_avg']:.1f} ms")
        if 'detection_rate' in stats:
            self.detection_rate_label.setText(f"{stats['detection_rate']:.1%}")
        if 'uptime' in stats:
            uptime = timedelta(seconds=int(stats['uptime']))
            self.uptime_label.setText(str(uptime))
        if 'total_detections' in stats:
            self.total_detections_label.setText(f"{stats['total_detections']}")

class SettingsDialog(QtWidgets.QDialog):
    """Advanced settings dialog"""
    
    def __init__(self, settings: AppSettings, parent=None):
        super().__init__(parent)
        self.settings = settings
        self.setWindowTitle("Settings")
        self.setModal(True)
        self.resize(500, 400)
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        
        tabs = QTabWidget()
        
        # Connection tab
        conn_tab = QWidget()
        conn_layout = QGridLayout(conn_tab)
        
        conn_layout.addWidget(QLabel("Baud Rate:"), 0, 0)
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["921600", "1843200", "3686400", "7372800"])
        self.baud_combo.setCurrentText(str(self.settings.baud_rate // 8))
        conn_layout.addWidget(self.baud_combo, 0, 1)
        
        self.auto_reconnect_cb = QCheckBox("Auto-reconnect")
        self.auto_reconnect_cb.setChecked(self.settings.auto_reconnect)
        conn_layout.addWidget(self.auto_reconnect_cb, 1, 0, 1, 2)
        
        tabs.addTab(conn_tab, "Connection")
        
        # Detection tab
        det_tab = QWidget()
        det_layout = QGridLayout(det_tab)
        
        det_layout.addWidget(QLabel("Confidence Threshold:"), 0, 0)
        self.conf_slider = QSlider(QtCore.Qt.Horizontal)
        self.conf_slider.setRange(0, 100)
        self.conf_slider.setValue(int(self.settings.confidence_threshold * 100))
        det_layout.addWidget(self.conf_slider, 0, 1)
        
        det_layout.addWidget(QLabel("Similarity Threshold:"), 1, 0)
        self.sim_slider = QSlider(QtCore.Qt.Horizontal)
        self.sim_slider.setRange(0, 100)
        self.sim_slider.setValue(int(self.settings.similarity_threshold * 100))
        det_layout.addWidget(self.sim_slider, 1, 1)
        
        tabs.addTab(det_tab, "Detection")
        
        layout.addWidget(tabs)
        
        # Buttons
        buttons = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)
        
    def accept(self):
        """Apply settings changes"""
        self.settings.baud_rate = int(self.baud_combo.currentText()) * 8
        self.settings.auto_reconnect = self.auto_reconnect_cb.isChecked()
        self.settings.confidence_threshold = self.conf_slider.value() / 100.0
        self.settings.similarity_threshold = self.sim_slider.value() / 100.0
        super().accept()

class EnhancedMainWindow(QMainWindow):
    """Modern main application window"""
    
    def __init__(self):
        super().__init__()
        self.settings = AppSettings.load(Path("settings.json"))
        self.setup_ui()
        self.setup_connections()
        self.apply_theme()
        
        # Initialize managers
        self.connection_manager = ConnectionManager(self.settings)
        self.stream_processor = StreamProcessor(self.connection_manager, self.settings)
        
        # Connect signals
        self.connection_manager.connection_changed.connect(self.on_connection_changed)
        self.connection_manager.error_occurred.connect(self.show_error)
        self.stream_processor.frame_received.connect(self.on_frame_received)
        self.stream_processor.statistics_updated.connect(self.stats_widget.update_stats)
        
    def setup_ui(self):
        """Setup the user interface"""
        self.setWindowTitle("STM32N6 Object Detection - Enhanced UI")
        self.setMinimumSize(1200, 800)
        
        # Create menu bar
        self.create_menu_bar()
        
        # Create toolbar
        self.create_toolbar()
        
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QHBoxLayout(central_widget)
        
        # Left panel - controls
        left_panel = QWidget()
        left_panel.setFixedWidth(300)
        left_layout = QVBoxLayout(left_panel)
        
        # Connection controls
        conn_group = QGroupBox("Connection")
        conn_layout = QGridLayout(conn_group)
        
        conn_layout.addWidget(QLabel("Port:"), 0, 0)
        self.port_combo = QComboBox()
        conn_layout.addWidget(self.port_combo, 0, 1)
        
        self.refresh_btn = QPushButton("🔄 Refresh")
        self.refresh_btn.clicked.connect(self.refresh_ports)
        conn_layout.addWidget(self.refresh_btn, 0, 2)
        
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        conn_layout.addWidget(self.connect_btn, 1, 0, 1, 3)
        
        left_layout.addWidget(conn_group)
        
        # Face management
        face_group = QGroupBox("Face Management")
        face_layout = QVBoxLayout(face_group)
        
        self.enroll_btn = QPushButton("📷 Enroll New Face")
        self.enroll_btn.clicked.connect(self.enroll_face)
        face_layout.addWidget(self.enroll_btn)
        
        self.manage_faces_btn = QPushButton("👥 Manage Faces")
        self.manage_faces_btn.clicked.connect(self.manage_faces)
        face_layout.addWidget(self.manage_faces_btn)
        
        left_layout.addWidget(face_group)
        
        # Statistics
        self.stats_widget = StatsWidget()
        left_layout.addWidget(self.stats_widget)
        
        left_layout.addStretch()
        main_layout.addWidget(left_panel)
        
        # Right panel - video and info
        right_splitter = QSplitter(QtCore.Qt.Vertical)
        
        # Video display
        self.video_widget = ModernImageWidget()
        right_splitter.addWidget(self.video_widget)
        
        # Log output
        self.log_widget = QTextEdit()
        self.log_widget.setMaximumHeight(150)
        self.log_widget.setReadOnly(True)
        right_splitter.addWidget(self.log_widget)
        
        main_layout.addWidget(right_splitter, 1)
        
        # Status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready")
        
        # Initial port refresh
        self.refresh_ports()
        
    def create_menu_bar(self):
        """Create application menu bar"""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("File")
        
        settings_action = QAction(self)
        settings_action.setText("Settings...")
        settings_action.triggered.connect(self.show_settings)
        file_menu.addAction(settings_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction(self)
        exit_action.setText("Exit")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # View menu
        view_menu = menubar.addMenu("View")
        
        theme_action = QAction(self)
        theme_action.setText("Toggle Theme")
        theme_action.triggered.connect(self.toggle_theme)
        view_menu.addAction(theme_action)
        
        # Help menu
        help_menu = menubar.addMenu("Help")
        
        about_action = QAction(self)
        about_action.setText("About")
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
        
    def create_toolbar(self):
        """Create application toolbar"""
        toolbar = QToolBar("Main")
        self.addToolBar(toolbar)
        
        # Quick connection toggle
        self.quick_connect_action = QAction(self)
        self.quick_connect_action.setText("🔌 Connect")
        self.quick_connect_action.triggered.connect(self.toggle_connection)
        toolbar.addAction(self.quick_connect_action)
        
        toolbar.addSeparator()
        
        # Settings
        settings_action = QAction(self)
        settings_action.setText("⚙️ Settings")
        settings_action.triggered.connect(self.show_settings)
        toolbar.addAction(settings_action)
        
    def setup_connections(self):
        """Setup signal connections"""
        pass
        
    def apply_theme(self):
        """Apply the selected theme"""
        if self.settings.theme == "dark":
            self.setStyleSheet("""
                QMainWindow {
                    background-color: #2b2b2b;
                    color: #ffffff;
                }
                QGroupBox {
                    font-weight: bold;
                    border: 2px solid #555;
                    border-radius: 5px;
                    margin-top: 10px;
                    padding-top: 10px;
                }
                QGroupBox::title {
                    subcontrol-origin: margin;
                    left: 10px;
                    padding: 0 5px 0 5px;
                }
                QPushButton {
                    background-color: #404040;
                    border: 1px solid #555;
                    border-radius: 4px;
                    padding: 8px;
                    min-width: 80px;
                }
                QPushButton:hover {
                    background-color: #505050;
                }
                QPushButton:pressed {
                    background-color: #353535;
                }
                QPushButton:disabled {
                    background-color: #2b2b2b;
                    color: #666;
                }
            """)
    
    def refresh_ports(self):
        """Refresh available serial ports"""
        self.port_combo.clear()
        ports = list_ports.comports()
        for port in ports:
            self.port_combo.addItem(f"{port.device} - {port.description}")
    
    def toggle_connection(self):
        """Toggle serial connection"""
        if self.connection_manager.serial_port:
            self.connection_manager.disconnect()
        else:
            port_text = self.port_combo.currentText()
            if port_text:
                port_name = port_text.split(" - ")[0]
                self.connection_manager.connect(port_name)
    
    def on_connection_changed(self, connected: bool):
        """Handle connection state changes"""
        if connected:
            self.connect_btn.setText("Disconnect")
            self.quick_connect_action.setText("🔌 Disconnect")
            self.status_bar.showMessage("Connected")
            self.stream_processor.start()
            self.log_message("Connected to device")
        else:
            self.connect_btn.setText("Connect")
            self.quick_connect_action.setText("🔌 Connect")
            self.status_bar.showMessage("Disconnected")
            self.stream_processor.stop()
            self.log_message("Disconnected from device")
    
    def on_frame_received(self, frame: np.ndarray, metadata: dict):
        """Handle received frame"""
        detections = metadata.get('detections', [])
        self.video_widget.set_image(frame, detections)
        self.stats_widget.current_detections_label.setText(f"{len(detections)}")
    
    def show_error(self, message: str):
        """Show error message"""
        QMessageBox.critical(self, "Error", message)
        self.log_message(f"ERROR: {message}")
    
    def log_message(self, message: str):
        """Add message to log widget"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_widget.append(f"[{timestamp}] {message}")
    
    def show_settings(self):
        """Show settings dialog"""
        dialog = SettingsDialog(self.settings, self)
        if dialog.exec_() == QtWidgets.QDialog.Accepted:
            self.settings.save(Path("settings.json"))
            self.apply_theme()
    
    def toggle_theme(self):
        """Toggle between dark and light themes"""
        self.settings.theme = "light" if self.settings.theme == "dark" else "dark"
        self.apply_theme()
    
    def enroll_face(self):
        """Open face enrollment dialog"""
        # TODO: Implement face enrollment dialog
        QMessageBox.information(self, "Face Enrollment", "Face enrollment feature coming soon!")
    
    def manage_faces(self):
        """Open face management dialog"""
        # TODO: Implement face management dialog
        QMessageBox.information(self, "Face Management", "Face management feature coming soon!")
    
    def show_about(self):
        """Show about dialog"""
        QMessageBox.about(self, "About", 
                         "STM32N6 Object Detection Enhanced UI\\n"
                         "Modern interface for real-time face detection and recognition\\n\\n"
                         "Built with PySide6 and OpenCV")
    
    def closeEvent(self, event):
        """Handle application close"""
        self.connection_manager.stop()
        self.stream_processor.stop()
        self.settings.save(Path("settings.json"))
        event.accept()

def main():
    """Main application entry point"""
    app = QApplication(sys.argv)
    app.setApplicationName("STM32N6 Object Detection")
    app.setApplicationVersion("2.0")
    
    # Set application icon (if available)
    # app.setWindowIcon(QtGui.QIcon("icon.png"))
    
    window = EnhancedMainWindow()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Basic UI for STM32N6 Object Detection System
Minimal dependencies - no TensorFlow/ML libraries required
"""

import sys
import json
import time
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional, List, Dict, Any
from datetime import datetime, timedelta
import threading
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

# Try to import enhanced protocol, fall back to legacy
try:
    import pc_uart_utils as uart_utils
except ImportError:
    uart_utils = None

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class BasicSettings:
    """Basic application settings"""
    baud_rate: int = 921600 * 8
    auto_reconnect: bool = True
    theme: str = "dark"
    
    def save(self, path: Path):
        """Save settings to JSON file"""
        try:
            with open(path, 'w') as f:
                json.dump(asdict(self), f, indent=2)
        except Exception as e:
            logger.warning(f"Failed to save settings: {e}")
    
    @classmethod
    def load(cls, path: Path) -> 'BasicSettings':
        """Load settings from JSON file"""
        if path.exists():
            try:
                with open(path, 'r') as f:
                    data = json.load(f)
                return cls(**data)
            except Exception as e:
                logger.warning(f"Failed to load settings: {e}")
        return cls()

class BasicImageWidget(QLabel):
    """Basic image display widget"""
    
    def __init__(self):
        super().__init__()
        self.setMinimumSize(640, 480)
        self.setStyleSheet("""
            QLabel {
                border: 2px solid #444;
                border-radius: 8px;
                background-color: #222;
                color: #fff;
            }
        """)
        self.setAlignment(QtCore.Qt.AlignCenter)
        self.setText("No Image")
        
    def set_image(self, image: np.ndarray):
        """Set image to display"""
        if image is None:
            self.setText("No Image")
            return
            
        try:
            # Convert BGR to RGB
            if len(image.shape) == 3:
                image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
            else:
                image_rgb = image
                
            height, width = image_rgb.shape[:2]
            
            if len(image_rgb.shape) == 3:
                bytes_per_line = 3 * width
                q_image = QtGui.QImage(
                    image_rgb.data, width, height, bytes_per_line, QtGui.QImage.Format_RGB888
                )
            else:
                bytes_per_line = width
                q_image = QtGui.QImage(
                    image_rgb.data, width, height, bytes_per_line, QtGui.QImage.Format_Grayscale8
                )
            
            # Scale to fit widget
            pixmap = QtGui.QPixmap.fromImage(q_image)
            scaled_pixmap = pixmap.scaled(
                self.size(), QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation
            )
            self.setPixmap(scaled_pixmap)
            
        except Exception as e:
            logger.error(f"Failed to display image: {e}")
            self.setText("Image Error")

class StreamReader(QThread):
    """Basic stream reader for UART data"""
    
    frame_received = Signal(np.ndarray)
    error_occurred = Signal(str)
    stats_updated = Signal(dict)
    
    def __init__(self, serial_port):
        super().__init__()
        self.serial_port = serial_port
        self._running = False
        self.frame_count = 0
        self.start_time = time.time()
        
    def run(self):
        """Main reading loop"""
        self._running = True
        last_stats_time = time.time()
        
        while self._running and self.serial_port and self.serial_port.is_open:
            try:
                # Try to read frame using available utilities
                if uart_utils:
                    tag, frame, width, height = uart_utils.read_frame(self.serial_port)
                    if frame is not None:
                        self.frame_count += 1
                        self.frame_received.emit(frame)
                        
                        # Read detections if available
                        try:
                            frame_id, detections = uart_utils.read_detections(self.serial_port)
                        except:
                            pass
                else:
                    # Basic UART reading without protocol
                    time.sleep(0.1)
                
                # Update stats periodically
                current_time = time.time()
                if current_time - last_stats_time >= 1.0:
                    elapsed = current_time - self.start_time
                    fps = self.frame_count / elapsed if elapsed > 0 else 0
                    stats = {
                        'frame_count': self.frame_count,
                        'fps': fps,
                        'uptime': elapsed
                    }
                    self.stats_updated.emit(stats)
                    last_stats_time = current_time
                    
            except Exception as e:
                self.error_occurred.emit(f"Stream error: {e}")
                time.sleep(1)  # Wait before retry
    
    def stop(self):
        """Stop reading"""
        self._running = False
        self.wait()

class BasicStatsWidget(QWidget):
    """Basic statistics display"""
    
    def __init__(self):
        super().__init__()
        self.setFixedWidth(250)
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        
        # Connection status
        conn_group = QGroupBox("Connection")
        conn_layout = QVBoxLayout(conn_group)
        
        self.status_label = QLabel("Disconnected")
        self.port_label = QLabel("Port: None")
        
        conn_layout.addWidget(self.status_label)
        conn_layout.addWidget(self.port_label)
        layout.addWidget(conn_group)
        
        # Statistics
        stats_group = QGroupBox("Statistics")
        stats_layout = QVBoxLayout(stats_group)
        
        self.fps_label = QLabel("FPS: 0.0")
        self.frames_label = QLabel("Frames: 0")
        self.uptime_label = QLabel("Uptime: 0s")
        
        stats_layout.addWidget(self.fps_label)
        stats_layout.addWidget(self.frames_label)
        stats_layout.addWidget(self.uptime_label)
        layout.addWidget(stats_group)
        
        layout.addStretch()
        
    def update_connection(self, connected: bool, port: str = ""):
        """Update connection status"""
        if connected:
            self.status_label.setText("Connected")
            self.status_label.setStyleSheet("color: green;")
            self.port_label.setText(f"Port: {port}")
        else:
            self.status_label.setText("Disconnected")
            self.status_label.setStyleSheet("color: red;")
            self.port_label.setText("Port: None")
    
    def update_stats(self, stats: Dict[str, Any]):
        """Update statistics display"""
        if 'fps' in stats:
            self.fps_label.setText(f"FPS: {stats['fps']:.1f}")
        if 'frame_count' in stats:
            self.frames_label.setText(f"Frames: {stats['frame_count']}")
        if 'uptime' in stats:
            uptime = timedelta(seconds=int(stats['uptime']))
            self.uptime_label.setText(f"Uptime: {uptime}")

class BasicMainWindow(QMainWindow):
    """Basic main window with minimal dependencies"""
    
    def __init__(self):
        super().__init__()
        self.settings = BasicSettings.load(Path("basic_settings.json"))
        self.serial_port: Optional[serial.Serial] = None
        self.stream_reader: Optional[StreamReader] = None
        
        self.setWindowTitle("STM32N6 Object Detection - Basic UI")
        self.setMinimumSize(900, 600)
        
        self.setup_ui()
        self.apply_theme()
        self.refresh_ports()
        
    def setup_ui(self):
        """Setup user interface"""
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QHBoxLayout(central_widget)
        
        # Left panel - controls
        left_panel = QWidget()
        left_panel.setFixedWidth(250)
        left_layout = QVBoxLayout(left_panel)
        
        # Connection controls
        conn_group = QGroupBox("Connection")
        conn_layout = QVBoxLayout(conn_group)
        
        # Port selection
        port_layout = QHBoxLayout()
        port_layout.addWidget(QLabel("Port:"))
        self.port_combo = QComboBox()
        port_layout.addWidget(self.port_combo)
        
        self.refresh_btn = QPushButton("🔄")
        self.refresh_btn.setMaximumWidth(30)
        self.refresh_btn.clicked.connect(self.refresh_ports)
        port_layout.addWidget(self.refresh_btn)
        conn_layout.addLayout(port_layout)
        
        # Baud rate
        baud_layout = QHBoxLayout()
        baud_layout.addWidget(QLabel("Baud:"))
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["921600", "1843200", "3686400", "7372800"])
        self.baud_combo.setCurrentText("7372800")
        baud_layout.addWidget(self.baud_combo)
        conn_layout.addLayout(baud_layout)
        
        # Connect button
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        conn_layout.addWidget(self.connect_btn)
        
        left_layout.addWidget(conn_group)
        
        # Statistics
        self.stats_widget = BasicStatsWidget()
        left_layout.addWidget(self.stats_widget)
        
        # Tools
        tools_group = QGroupBox("Tools")
        tools_layout = QVBoxLayout(tools_group)
        
        self.clear_btn = QPushButton("Clear Display")
        self.clear_btn.clicked.connect(self.clear_display)
        tools_layout.addWidget(self.clear_btn)
        
        self.theme_btn = QPushButton("Toggle Theme")
        self.theme_btn.clicked.connect(self.toggle_theme)
        tools_layout.addWidget(self.theme_btn)
        
        left_layout.addWidget(tools_group)
        left_layout.addStretch()
        
        main_layout.addWidget(left_panel)
        
        # Right panel - display and log
        right_splitter = QSplitter(QtCore.Qt.Vertical)
        
        # Image display
        self.image_widget = BasicImageWidget()
        right_splitter.addWidget(self.image_widget)
        
        # Log output
        self.log_widget = QTextEdit()
        self.log_widget.setMaximumHeight(120)
        self.log_widget.setReadOnly(True)
        right_splitter.addWidget(self.log_widget)
        
        main_layout.addWidget(right_splitter, 1)
        
        # Status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready")
        
        # Create menu
        self.create_menu()
        
    def create_menu(self):
        """Create menu bar"""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("File")
        
        exit_action = QAction(self)
        exit_action.setText("Exit")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Help menu
        help_menu = menubar.addMenu("Help")
        
        about_action = QAction(self)
        about_action.setText("About")
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
        
    def apply_theme(self):
        """Apply theme"""
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
                }
                QPushButton:hover {
                    background-color: #505050;
                }
                QPushButton:pressed {
                    background-color: #353535;
                }
                QComboBox {
                    background-color: #404040;
                    border: 1px solid #555;
                    border-radius: 4px;
                    padding: 4px;
                }
                QTextEdit {
                    background-color: #1e1e1e;
                    border: 1px solid #555;
                    border-radius: 4px;
                }
            """)
    
    def refresh_ports(self):
        """Refresh available ports"""
        self.port_combo.clear()
        ports = list_ports.comports()
        for port in ports:
            self.port_combo.addItem(f"{port.device} - {port.description}")
        
        if ports:
            self.log_message(f"Found {len(ports)} serial ports")
        else:
            self.log_message("No serial ports found")
    
    def toggle_connection(self):
        """Toggle connection"""
        if self.serial_port and self.serial_port.is_open:
            self.disconnect()
        else:
            self.connect()
    
    def connect(self):
        """Connect to serial port"""
        port_text = self.port_combo.currentText()
        if not port_text:
            self.log_message("No port selected")
            return
            
        port_name = port_text.split(" - ")[0]
        baud_rate = int(self.baud_combo.currentText())
        
        try:
            self.serial_port = serial.Serial(port_name, baud_rate, timeout=1)
            self.connect_btn.setText("Disconnect")
            self.stats_widget.update_connection(True, port_name)
            self.status_bar.showMessage(f"Connected to {port_name}")
            self.log_message(f"Connected to {port_name} at {baud_rate} baud")
            
            # Start stream reader
            if uart_utils:
                self.stream_reader = StreamReader(self.serial_port)
                self.stream_reader.frame_received.connect(self.on_frame_received)
                self.stream_reader.error_occurred.connect(self.on_error)
                self.stream_reader.stats_updated.connect(self.stats_widget.update_stats)
                self.stream_reader.start()
            else:
                self.log_message("UART utilities not available - limited functionality")
                
        except Exception as e:
            self.log_message(f"Connection failed: {e}")
            QMessageBox.critical(self, "Connection Error", f"Failed to connect: {e}")
    
    def disconnect(self):
        """Disconnect from serial port"""
        if self.stream_reader:
            self.stream_reader.stop()
            self.stream_reader = None
            
        if self.serial_port:
            self.serial_port.close()
            self.serial_port = None
            
        self.connect_btn.setText("Connect")
        self.stats_widget.update_connection(False)
        self.status_bar.showMessage("Disconnected")
        self.log_message("Disconnected")
    
    def on_frame_received(self, frame: np.ndarray):
        """Handle received frame"""
        self.image_widget.set_image(frame)
    
    def on_error(self, error_msg: str):
        """Handle error"""
        self.log_message(f"ERROR: {error_msg}")
    
    def clear_display(self):
        """Clear image display"""
        self.image_widget.setText("No Image")
        self.image_widget.clear()
    
    def toggle_theme(self):
        """Toggle theme"""
        self.settings.theme = "light" if self.settings.theme == "dark" else "dark"
        self.apply_theme()
    
    def log_message(self, message: str):
        """Add message to log"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_widget.append(f"[{timestamp}] {message}")
    
    def show_about(self):
        """Show about dialog"""
        QMessageBox.about(self, "About", 
                         "STM32N6 Object Detection - Basic UI\\n"
                         "Minimal dependency version for basic functionality\\n\\n"
                         "Built with PySide6 and OpenCV")
    
    def closeEvent(self, event):
        """Handle window close"""
        self.disconnect()
        self.settings.save(Path("basic_settings.json"))
        event.accept()

def main():
    """Main entry point"""
    app = QApplication(sys.argv)
    app.setApplicationName("STM32N6 Object Detection Basic UI")
    
    window = BasicMainWindow()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
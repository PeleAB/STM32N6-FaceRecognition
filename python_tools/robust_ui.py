#!/usr/bin/env python3
"""
Robust UI using the new binary protocol for STM32N6 Object Detection
Implements reliable communication with message buffering and parsing
"""

import sys
import json
import time
import threading
import struct
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional, List, Dict, Any
from datetime import datetime, timedelta
import logging

import cv2
import numpy as np
from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtCore import QTimer, Signal, QThread, QMutex, QMutexLocker
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

from robust_protocol import (
    RobustProtocolParser, MessageType, ProtocolMessage, 
    FrameDataParser, DetectionDataParser, EmbeddingDataParser
)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class RobustSettings:
    """Robust UI settings"""
    baud_rate: int = 921600 * 8
    auto_reconnect: bool = True
    theme: str = "dark"
    protocol_stats: bool = True
    
    def save(self, path: Path):
        """Save settings to JSON file"""
        try:
            with open(path, 'w') as f:
                json.dump(asdict(self), f, indent=2)
        except Exception as e:
            logger.warning(f"Failed to save settings: {e}")
    
    @classmethod
    def load(cls, path: Path) -> 'RobustSettings':
        """Load settings from JSON file"""
        if path.exists():
            try:
                with open(path, 'r') as f:
                    data = json.load(f)
                return cls(**data)
            except Exception as e:
                logger.warning(f"Failed to load settings: {e}")
        return cls()

class RobustImageWidget(QLabel):
    """Image display widget optimized for robust protocol"""
    
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
        self.setScaledContents(False)
        
        # Statistics
        self.frames_received = 0
        self.last_frame_time = 0
        self.frame_rate = 0.0
        
    def set_image(self, image: np.ndarray, frame_type: str = ""):
        """Set image to display"""
        if image is None:
            self.setText("No Image")
            return
            
        try:
            # Update statistics
            current_time = time.time()
            if self.last_frame_time > 0:
                interval = current_time - self.last_frame_time
                if interval > 0:
                    self.frame_rate = 0.9 * self.frame_rate + 0.1 * (1.0 / interval)
            self.last_frame_time = current_time
            self.frames_received += 1
            
            # Convert BGR to RGB
            if len(image.shape) == 3:
                image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
            else:
                image_rgb = image
                
            height, width = image_rgb.shape[:2]
            
            # Create QImage
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

class RobustStatsWidget(QWidget):
    """Statistics display for robust protocol"""
    
    def __init__(self):
        super().__init__()
        self.setFixedWidth(280)
        self.setup_ui()
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        
        # Connection status
        conn_group = QGroupBox("Connection")
        conn_layout = QVBoxLayout(conn_group)
        
        self.status_label = QLabel("Disconnected")
        self.port_label = QLabel("Port: None")
        self.protocol_label = QLabel("Protocol: Robust Binary")
        
        conn_layout.addWidget(self.status_label)
        conn_layout.addWidget(self.port_label)
        conn_layout.addWidget(self.protocol_label)
        layout.addWidget(conn_group)
        
        # Protocol statistics
        protocol_group = QGroupBox("Protocol Stats")
        protocol_layout = QVBoxLayout(protocol_group)
        
        self.messages_label = QLabel("Messages: 0")
        self.bytes_label = QLabel("Bytes: 0")
        self.sync_errors_label = QLabel("Sync Errors: 0")
        self.checksum_errors_label = QLabel("Header Errors: 0")
        self.crc_errors_label = QLabel("CRC32 Errors: 0")
        self.dropped_label = QLabel("Dropped: 0")
        self.parse_errors_label = QLabel("Parse Errors: 0")
        
        # Error rate indicators
        self.sync_rate_label = QLabel("Sync Error Rate: 0.0%")
        self.checksum_rate_label = QLabel("Header Error Rate: 0.0%")
        self.crc_rate_label = QLabel("CRC32 Error Rate: 0.0%")
        
        # Performance indicators
        self.throughput_label = QLabel("Throughput: 0.0 Mbps")
        self.parse_time_label = QLabel("Parse Time: 0 ms")
        
        protocol_layout.addWidget(self.messages_label)
        protocol_layout.addWidget(self.bytes_label)
        protocol_layout.addWidget(self.throughput_label)
        protocol_layout.addWidget(self.sync_errors_label)
        protocol_layout.addWidget(self.sync_rate_label)
        protocol_layout.addWidget(self.checksum_errors_label)
        protocol_layout.addWidget(self.checksum_rate_label)
        protocol_layout.addWidget(self.crc_errors_label)
        protocol_layout.addWidget(self.crc_rate_label)
        protocol_layout.addWidget(self.parse_errors_label)
        protocol_layout.addWidget(self.parse_time_label)
        protocol_layout.addWidget(self.dropped_label)
        layout.addWidget(protocol_group)
        
        # Frame statistics
        frame_group = QGroupBox("Frame Stats")
        frame_layout = QVBoxLayout(frame_group)
        
        self.frame_count_label = QLabel("Frames: 0")
        self.frame_rate_label = QLabel("FPS: 0.0")
        self.detections_label = QLabel("Detections: 0")
        self.embeddings_label = QLabel("Embeddings: 0")
        
        frame_layout.addWidget(self.frame_count_label)
        frame_layout.addWidget(self.frame_rate_label)
        frame_layout.addWidget(self.detections_label)
        frame_layout.addWidget(self.embeddings_label)
        layout.addWidget(frame_group)
        
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
    
    def update_protocol_stats(self, stats: Dict[str, Any]):
        """Update protocol statistics"""
        messages = stats.get('messages_received', 0)
        bytes_received = stats.get('bytes_received', 0)
        sync_errors = stats.get('sync_errors', 0)
        checksum_errors = stats.get('checksum_errors', 0)
        crc_errors = stats.get('crc_errors', 0)
        parse_errors = stats.get('parse_errors', 0)
        dropped = stats.get('messages_dropped', 0)
        
        self.messages_label.setText(f"Messages: {messages}")
        self.bytes_label.setText(f"Bytes: {bytes_received}")
        
        # Performance metrics
        throughput = stats.get('throughput_mbps', 0.0)
        parse_time = stats.get('parse_time_ms', 0)
        self.throughput_label.setText(f"Throughput: {throughput:.1f} Mbps")
        self.parse_time_label.setText(f"Parse Time: {parse_time} ms")
        
        self.sync_errors_label.setText(f"Sync Errors: {sync_errors}")
        self.checksum_errors_label.setText(f"Header Errors: {checksum_errors}")
        self.crc_errors_label.setText(f"CRC32 Errors: {crc_errors}")
        self.parse_errors_label.setText(f"Parse Errors: {parse_errors}")
        self.dropped_label.setText(f"Dropped: {dropped}")
        
        # Calculate error rates
        if bytes_received > 0:
            sync_rate = (sync_errors / bytes_received) * 100
            checksum_rate = (checksum_errors / max(messages + checksum_errors, 1)) * 100
            crc_rate = (crc_errors / max(messages + crc_errors, 1)) * 100
            
            # Color code error rates
            sync_color = "red" if sync_rate > 5 else "orange" if sync_rate > 1 else "green"
            checksum_color = "red" if checksum_rate > 5 else "orange" if checksum_rate > 1 else "green"
            crc_color = "red" if crc_rate > 5 else "orange" if crc_rate > 1 else "green"
            
            self.sync_rate_label.setText(f"Sync Error Rate: {sync_rate:.2f}%")
            self.sync_rate_label.setStyleSheet(f"color: {sync_color};")
            
            self.checksum_rate_label.setText(f"Header Error Rate: {checksum_rate:.2f}%")
            self.checksum_rate_label.setStyleSheet(f"color: {checksum_color};")
            
            self.crc_rate_label.setText(f"CRC32 Error Rate: {crc_rate:.2f}%")
            self.crc_rate_label.setStyleSheet(f"color: {crc_color};")
    
    def update_frame_stats(self, frame_count: int, frame_rate: float, detections: int, embeddings: int):
        """Update frame statistics"""
        self.frame_count_label.setText(f"Frames: {frame_count}")
        self.frame_rate_label.setText(f"FPS: {frame_rate:.1f}")
        self.detections_label.setText(f"Detections: {detections}")
        self.embeddings_label.setText(f"Embeddings: {embeddings}")

class RobustSerialReader(QThread):
    """Serial reader using robust protocol"""
    
    frame_received = Signal(np.ndarray, str)  # image, frame_type
    detections_received = Signal(int, list)   # frame_id, detections
    embedding_received = Signal(list)         # embedding
    stats_updated = Signal(dict)              # protocol stats
    error_occurred = Signal(str)              # error message
    
    def __init__(self, serial_port):
        super().__init__()
        self.serial_port = serial_port
        self._running = False
        self.protocol_parser = RobustProtocolParser()
        
        # Register message handlers
        self.protocol_parser.register_handler(MessageType.FRAME_DATA, self._handle_frame_data)
        self.protocol_parser.register_handler(MessageType.DETECTION_RESULTS, self._handle_detections)
        self.protocol_parser.register_handler(MessageType.EMBEDDING_DATA, self._handle_embedding)
        self.protocol_parser.register_handler(MessageType.PERFORMANCE_METRICS, self._handle_performance_metrics)
        self.protocol_parser.register_handler(MessageType.HEARTBEAT, self._handle_heartbeat)
        
        # Statistics
        self.detection_count = 0
        self.embedding_count = 0
        
    def run(self):
        """Main reading loop"""
        self._running = True
        last_stats_time = time.time()
        
        logger.info("Robust serial reader started")
        
        while self._running and self.serial_port and self.serial_port.is_open:
            try:
                # Read available data in larger chunks for efficiency
                bytes_waiting = self.serial_port.in_waiting
                if bytes_waiting > 0:
                    # Read up to 64KB at once for better throughput
                    chunk_size = min(bytes_waiting, 65536)
                    data = self.serial_port.read(chunk_size)
                    if data:
                        self.protocol_parser.add_data(data)
                        
                # Process more messages per iteration for higher throughput
                processed = self.protocol_parser.process_messages(max_messages=100)
                
                # Update statistics periodically
                current_time = time.time()
                if current_time - last_stats_time >= 1.0:
                    stats = self.protocol_parser.get_stats()
                    self.stats_updated.emit(stats)
                    last_stats_time = current_time
                
                # Adaptive sleep based on activity
                if processed == 0 and bytes_waiting == 0:
                    time.sleep(0.005)  # Reduced sleep for better responsiveness
                elif processed > 50:
                    # High activity, don't sleep
                    pass
                else:
                    time.sleep(0.001)  # Minimal sleep for moderate activity
                    
            except Exception as e:
                self.error_occurred.emit(f"Serial read error: {e}")
                time.sleep(0.1)
                
        logger.info("Robust serial reader stopped")
    
    def stop(self):
        """Stop reading with timeout"""
        logger.info("Stopping robust serial reader...")
        self._running = False
        if not self.wait(3000):  # 3 second timeout
            logger.warning("Serial reader thread did not stop gracefully")
            self.terminate()
            self.wait(1000)
    
    def _handle_frame_data(self, message: ProtocolMessage):
        """Handle frame data message with optimized decoding"""
        try:
            frame_data = FrameDataParser.parse_frame_fast(message.payload)
            if frame_data:
                frame_type, image, width, height = frame_data
                self.frame_received.emit(image, frame_type)
        except Exception as e:
            logger.error(f"Error handling frame data: {e}")
    
    def _handle_detections(self, message: ProtocolMessage):
        """Handle detection results message"""
        try:
            detection_data = DetectionDataParser.parse_detections(message.payload)
            if detection_data:
                frame_id, detections = detection_data
                self.detection_count += len(detections)
                self.detections_received.emit(frame_id, detections)
        except Exception as e:
            logger.error(f"Error handling detections: {e}")
    
    def _handle_embedding(self, message: ProtocolMessage):
        """Handle embedding data message"""
        try:
            embedding = EmbeddingDataParser.parse_embedding(message.payload)
            if embedding:
                self.embedding_count += 1
                self.embedding_received.emit(embedding)
        except Exception as e:
            logger.error(f"Error handling embedding: {e}")
    
    def _handle_performance_metrics(self, message: ProtocolMessage):
        """Handle performance metrics message"""
        try:
            if len(message.payload) >= 28:  # Size of performance_metrics_t
                # Just log for now, could emit signal if needed
                logger.debug(f"Performance metrics received: {len(message.payload)} bytes")
        except Exception as e:
            logger.error(f"Error handling performance metrics: {e}")
    
    def _handle_heartbeat(self, message: ProtocolMessage):
        """Handle heartbeat message"""
        try:
            if len(message.payload) >= 4:  # Timestamp
                timestamp = struct.unpack('<I', message.payload[:4])[0]
                logger.debug(f"Heartbeat received: timestamp={timestamp}")
        except Exception as e:
            logger.error(f"Error handling heartbeat: {e}")

class RobustMainWindow(QMainWindow):
    """Main window using robust protocol"""
    
    def __init__(self):
        super().__init__()
        self.settings = RobustSettings.load(Path("robust_settings.json"))
        self.serial_port: Optional[serial.Serial] = None
        self.serial_reader: Optional[RobustSerialReader] = None
        
        # Statistics
        self.frame_count = 0
        self.detection_count = 0
        self.embedding_count = 0
        
        self.setWindowTitle("STM32N6 Object Detection - Robust Protocol UI")
        self.setMinimumSize(1000, 700)
        
        self.setup_ui()
        self.apply_theme()
        self.refresh_ports()
        
        # Update timer for display refresh
        self.update_timer = QTimer()
        self.update_timer.timeout.connect(self.update_display_stats)
        self.update_timer.start(1000)  # 1 second updates
        
    def setup_ui(self):
        """Setup user interface"""
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QHBoxLayout(central_widget)
        
        # Left panel - controls and stats
        left_panel = QWidget()
        left_panel.setFixedWidth(280)
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
        
        # Statistics widget
        self.stats_widget = RobustStatsWidget()
        left_layout.addWidget(self.stats_widget)
        
        # Tools
        tools_group = QGroupBox("Tools")
        tools_layout = QVBoxLayout(tools_group)
        
        self.clear_btn = QPushButton("Clear Display")
        self.clear_btn.clicked.connect(self.clear_display)
        tools_layout.addWidget(self.clear_btn)
        
        self.reset_stats_btn = QPushButton("Reset Statistics")
        self.reset_stats_btn.clicked.connect(self.reset_statistics)
        tools_layout.addWidget(self.reset_stats_btn)
        
        self.theme_btn = QPushButton("Toggle Theme")
        self.theme_btn.clicked.connect(self.toggle_theme)
        tools_layout.addWidget(self.theme_btn)
        
        left_layout.addWidget(tools_group)
        left_layout.addStretch()
        
        main_layout.addWidget(left_panel)
        
        # Right panel - display and log
        right_splitter = QSplitter(QtCore.Qt.Vertical)
        
        # Single image display for all frame types
        image_container = QWidget()
        image_layout = QVBoxLayout(image_container)
        
        # Main stream display (all raw grayscale frames)
        main_stream_group = QGroupBox("Frame Stream (Raw Grayscale)")
        main_stream_layout = QVBoxLayout(main_stream_group)
        self.main_image_widget = RobustImageWidget()
        self.main_image_widget.setMinimumSize(640, 480)
        main_stream_layout.addWidget(self.main_image_widget)
        image_layout.addWidget(main_stream_group)
        
        right_splitter.addWidget(image_container)
        
        # Log output
        self.log_widget = QTextEdit()
        self.log_widget.setMaximumHeight(150)
        self.log_widget.setReadOnly(True)
        right_splitter.addWidget(self.log_widget)
        
        main_layout.addWidget(right_splitter, 1)
        
        # Status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready - Robust Protocol")
        
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
        
        # View menu
        view_menu = menubar.addMenu("View")
        
        stats_action = QAction(self)
        stats_action.setText("Protocol Statistics")
        stats_action.setCheckable(True)
        stats_action.setChecked(self.settings.protocol_stats)
        stats_action.triggered.connect(self.toggle_protocol_stats)
        view_menu.addAction(stats_action)
        
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
            self.serial_port = serial.Serial(port_name, baud_rate, timeout=0.1)
            self.connect_btn.setText("Disconnect")
            self.stats_widget.update_connection(True, port_name)
            self.status_bar.showMessage(f"Connected to {port_name} - Robust Protocol Active")
            self.log_message(f"Connected to {port_name} at {baud_rate} baud (Robust Protocol)")
            
            # Start serial reader
            self.serial_reader = RobustSerialReader(self.serial_port)
            self.serial_reader.frame_received.connect(self.on_frame_received)
            self.serial_reader.detections_received.connect(self.on_detections_received)
            self.serial_reader.embedding_received.connect(self.on_embedding_received)
            self.serial_reader.stats_updated.connect(self.on_stats_updated)
            self.serial_reader.error_occurred.connect(self.on_error)
            self.serial_reader.start()
                
        except Exception as e:
            self.log_message(f"Connection failed: {e}")
            QMessageBox.critical(self, "Connection Error", f"Failed to connect: {e}")
    
    def disconnect(self):
        """Disconnect from serial port"""
        self.log_message("Disconnecting...")
        
        # Stop serial reader
        if self.serial_reader:
            self.serial_reader.stop()
            self.serial_reader = None
            
        # Close serial port
        if self.serial_port:
            try:
                self.serial_port.close()
            except:
                pass
            self.serial_port = None
            
        self.connect_btn.setText("Connect")
        self.stats_widget.update_connection(False)
        self.status_bar.showMessage("Disconnected")
        self.log_message("Disconnected")
    
    def on_frame_received(self, image: np.ndarray, frame_type: str):
        """Handle received frame - all frame types go to single display"""
        self.frame_count += 1
        
        # All frames go to the main display (now single display for all types)
        self.main_image_widget.set_image(image, frame_type)
    
    def on_detections_received(self, frame_id: int, detections: List):
        """Handle received detections"""
        self.detection_count += len(detections)
        det_info = []
        for det in detections:
            class_id, x, y, w, h, confidence, keypoints = det
            det_info.append(f"cls:{class_id} conf:{confidence:.2f}")
        
        detection_details = ", ".join(det_info) if det_info else "none"
        self.log_message(f"Frame {frame_id}: {len(detections)} detections [{detection_details}]")
    
    def on_embedding_received(self, embedding: List[float]):
        """Handle received embedding"""
        self.embedding_count += 1
        self.log_message(f"Embedding received: {len(embedding)} values")
    
    def on_stats_updated(self, stats: Dict[str, Any]):
        """Handle protocol statistics update"""
        self.stats_widget.update_protocol_stats(stats)
    
    def on_error(self, error_msg: str):
        """Handle error"""
        self.log_message(f"ERROR: {error_msg}")
    
    def update_display_stats(self):
        """Update display statistics"""
        # Get stats from single display
        frames_received = getattr(self.main_image_widget, 'frames_received', 0)
        frame_rate = getattr(self.main_image_widget, 'frame_rate', 0.0)
        
        self.stats_widget.update_frame_stats(
            frames_received,
            frame_rate,
            self.detection_count,
            self.embedding_count
        )
    
    def clear_display(self):
        """Clear image display"""
        self.main_image_widget.setText("No Image")
        self.main_image_widget.clear()
    
    def reset_statistics(self):
        """Reset all statistics"""
        self.frame_count = 0
        self.detection_count = 0
        self.embedding_count = 0
        
        # Reset stats for display
        if hasattr(self.main_image_widget, 'frames_received'):
            self.main_image_widget.frames_received = 0
            self.main_image_widget.frame_rate = 0.0
        
        if self.serial_reader:
            self.serial_reader.protocol_parser.clear_stats()
            self.serial_reader.detection_count = 0
            self.serial_reader.embedding_count = 0
        
        self.log_message("Statistics reset")
    
    def toggle_protocol_stats(self, checked: bool):
        """Toggle protocol statistics display"""
        self.settings.protocol_stats = checked
    
    def toggle_theme(self):
        """Toggle theme"""
        self.settings.theme = "light" if self.settings.theme == "dark" else "dark"
        self.apply_theme()
    
    def log_message(self, message: str):
        """Add message to log"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_widget.append(f"[{timestamp}] {message}")
        
        # Limit log lines
        document = self.log_widget.document()
        if document.blockCount() > 100:
            cursor = self.log_widget.textCursor()
            cursor.movePosition(QtGui.QTextCursor.MoveOperation.Start)
            cursor.select(QtGui.QTextCursor.SelectionType.BlockUnderCursor)
            cursor.removeSelectedText()
    
    def show_about(self):
        """Show about dialog"""
        QMessageBox.about(self, "About", 
                         "STM32N6 Object Detection - Robust Protocol UI\\n"
                         "Advanced binary protocol with message framing\\n"
                         "Features: Checksums, Buffering, Error Recovery\\n\\n"
                         "Built with PySide6 and OpenCV")
    
    def closeEvent(self, event):
        """Handle window close"""
        self.update_timer.stop()
        self.disconnect()
        self.settings.save(Path("robust_settings.json"))
        event.accept()

def main():
    """Main entry point"""
    app = QApplication(sys.argv)
    app.setApplicationName("STM32N6 Object Detection Robust UI")
    
    window = RobustMainWindow()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
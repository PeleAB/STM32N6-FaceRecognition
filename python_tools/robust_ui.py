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
from types import FrameType
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
    QStatusBar, QMenuBar, QToolBar, QDialog
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

class FaceEnlargementDialog(QDialog):
    """Dialog to show enlarged face detection"""
    
    def __init__(self, face_image: np.ndarray, detection_info: str, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Face Detection Details")
        self.setModal(True)
        self.resize(400, 500)
        
        layout = QVBoxLayout(self)
        
        # Detection info
        info_label = QLabel(detection_info)
        info_label.setStyleSheet("font-weight: bold; font-size: 14px; padding: 10px;")
        info_label.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(info_label)
        
        # Enlarged image
        image_label = QLabel()
        image_label.setAlignment(QtCore.Qt.AlignCenter)
        image_label.setStyleSheet("""
            QLabel {
                border: 2px solid #4CAF50;
                border-radius: 8px;
                background-color: #222;
                padding: 10px;
            }
        """)
        
        # Convert and scale the image
        if len(face_image.shape) == 3:
            height, width, channel = face_image.shape
            bytes_per_line = 3 * width
            q_image = QtGui.QImage(face_image.data, width, height, bytes_per_line, QtGui.QImage.Format_RGB888)
        else:
            height, width = face_image.shape
            bytes_per_line = width
            q_image = QtGui.QImage(face_image.data, width, height, bytes_per_line, QtGui.QImage.Format_Grayscale8)
        
        pixmap = QtGui.QPixmap.fromImage(q_image)
        scaled_pixmap = pixmap.scaled(350, 350, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
        image_label.setPixmap(scaled_pixmap)
        
        layout.addWidget(image_label)
        
        # Close button
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.accept)
        layout.addWidget(close_btn)

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

class ALNDetectionWidget(QWidget):
    """Vertical film strip display for last 5 ALN face detections"""
    
    def __init__(self):
        super().__init__()
        self.setFixedWidth(200)  # Fixed width for vertical layout
        self.detections = []  # Store last 5 detection images with timestamps and confidence
        self.max_detections = 5
        self.setup_ui()
        
        # Timer for updating timestamps
        self.timestamp_timer = QTimer()
        self.timestamp_timer.timeout.connect(self.update_timestamps)
        self.timestamp_timer.start(1000)  # Update every second
        
    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(5)
        
        # Title
        title_label = QLabel("ALN Detections")
        title_label.setStyleSheet("font-weight: bold; color: #4CAF50; font-size: 14px;")
        title_label.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(title_label)
        
        # Create 5 slots for detection thumbnails - vertical layout
        self.detection_slots = []
        self.timestamp_labels = []
        for i in range(self.max_detections):
            # Container for each detection
            container = QWidget()
            container_layout = QVBoxLayout(container)
            container_layout.setContentsMargins(2, 2, 2, 2)
            container_layout.setSpacing(2)
            
            # Face thumbnail
            slot = QLabel()
            slot.setFixedSize(180, 180)  # Square thumbnails for face crops
            slot.setStyleSheet("""
                QLabel {
                    border: 2px solid #444;
                    border-radius: 8px;
                    background-color: #1a1a1a;
                    color: #666;
                    font-size: 12px;
                    font-weight: bold;
                }
            """)
            slot.setAlignment(QtCore.Qt.AlignCenter)
            slot.setScaledContents(True)
            slot.setCursor(QtCore.Qt.PointingHandCursor)
            
            # Make slot clickable
            slot.mousePressEvent = lambda event, idx=i: self.on_slot_clicked(idx)
            
            # Timestamp label
            timestamp_label = QLabel("")
            timestamp_label.setAlignment(QtCore.Qt.AlignCenter)
            timestamp_label.setStyleSheet("""
                QLabel {
                    color: #888;
                    font-size: 10px;
                    background-color: transparent;
                    border: none;
                    padding: 2px;
                }
            """)
            timestamp_label.setFixedHeight(15)
            
            container_layout.addWidget(slot)
            container_layout.addWidget(timestamp_label)
            
            self.detection_slots.append(slot)
            self.timestamp_labels.append(timestamp_label)
            layout.addWidget(container)
        
        layout.addStretch()  # Push everything to the top
        
    def add_detection(self, face_crop: np.ndarray, detection_info: str = ""):
        """Add new ALN face detection to the film strip"""
        try:
            # Extract confidence from detection_info
            confidence = 0.0
            if "conf=" in detection_info:
                conf_str = detection_info.split("conf=")[1].split()[0]
                try:
                    confidence = float(conf_str)
                except:
                    confidence = 0.0
            
            # Add to detections list (newest first) with timestamp and confidence
            detection_data = {
                'image': face_crop.copy(),
                'info': detection_info,
                'timestamp': time.time(),
                'confidence': confidence
            }
            self.detections.insert(0, detection_data)
            
            # Keep only last 5
            if len(self.detections) > self.max_detections:
                self.detections = self.detections[:self.max_detections]
            
            # Update display slots
            self.update_display()
            
        except Exception as e:
            logger.error(f"Error adding ALN detection: {e}")
    
    def update_display(self):
        """Update the vertical film strip display"""
        try:
            # Clear all slots first
            for i, slot in enumerate(self.detection_slots):
                slot.clear()
                slot.setText(f"FACE[N-{i}]")
                slot.setStyleSheet("""
                    QLabel {
                        border: 2px solid #444;
                        border-radius: 8px;
                        background-color: #1a1a1a;
                        color: #666;
                        font-size: 12px;
                        font-weight: bold;
                    }
                """)
                # Clear timestamp labels
                if i < len(self.timestamp_labels):
                    self.timestamp_labels[i].setText("")
            
            # Fill slots with detections (newest to oldest)
            for i, detection_data in enumerate(self.detections):
                if i < len(self.detection_slots):
                    slot = self.detection_slots[i]
                    face_crop = detection_data['image']
                    info = detection_data['info']
                    confidence = detection_data['confidence']
                    
                    # Convert face crop to QPixmap
                    if len(face_crop.shape) == 3:
                        height, width, channel = face_crop.shape
                        bytes_per_line = 3 * width
                        q_image = QtGui.QImage(face_crop.data, width, height, bytes_per_line, QtGui.QImage.Format_RGB888)
                    else:
                        height, width = face_crop.shape
                        bytes_per_line = width
                        q_image = QtGui.QImage(face_crop.data, width, height, bytes_per_line, QtGui.QImage.Format_Grayscale8)
                    
                    pixmap = QtGui.QPixmap.fromImage(q_image)
                    scaled_pixmap = pixmap.scaled(176, 176, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
                    
                    slot.setPixmap(scaled_pixmap)
                    
                    # Color-code border based on confidence
                    if confidence >= 0.8:
                        border_color = "#4CAF50"  # Green - high confidence
                    elif confidence >= 0.6:
                        border_color = "#FF9800"  # Orange - medium confidence
                    else:
                        border_color = "#F44336"  # Red - low confidence
                    
                    slot.setStyleSheet(f"""
                        QLabel {{
                            border: 3px solid {border_color};
                            border-radius: 8px;
                            background-color: #222;
                        }}
                    """)
                    
                    # Set tooltip with detection info and confidence
                    tooltip_text = f"Face {i+1}: {info}\nConfidence: {confidence:.2f}"
                    slot.setToolTip(tooltip_text)
            
            # Update timestamps
            self.update_timestamps()
            
        except Exception as e:
            logger.error(f"Error updating ALN display: {e}")
    
    def update_timestamps(self):
        """Update timestamp labels with age of detections"""
        try:
            current_time = time.time()
            for i, detection_data in enumerate(self.detections):
                if i < len(self.timestamp_labels):
                    age_seconds = current_time - detection_data['timestamp']
                    
                    # Format age nicely
                    if age_seconds < 60:
                        age_text = f"{int(age_seconds)}s ago"
                    elif age_seconds < 3600:
                        age_text = f"{int(age_seconds/60)}m ago"
                    else:
                        age_text = f"{int(age_seconds/3600)}h ago"
                    
                    self.timestamp_labels[i].setText(age_text)
        except Exception as e:
            logger.error(f"Error updating timestamps: {e}")
    
    def on_slot_clicked(self, slot_index):
        """Handle click on detection slot"""
        try:
            if slot_index < len(self.detections):
                detection_data = self.detections[slot_index]
                face_image = detection_data['image']
                info = detection_data['info']
                confidence = detection_data['confidence']
                
                # Create detailed info for dialog
                age_seconds = time.time() - detection_data['timestamp']
                if age_seconds < 60:
                    age_text = f"{int(age_seconds)}s ago"
                elif age_seconds < 3600:
                    age_text = f"{int(age_seconds/60)}m ago"
                else:
                    age_text = f"{int(age_seconds/3600)}h ago"
                
                detailed_info = f"{info}\nConfidence: {confidence:.2f}\nDetected: {age_text}"
                
                # Show enlargement dialog
                dialog = FaceEnlargementDialog(face_image, detailed_info, self)
                dialog.exec()
        except Exception as e:
            logger.error(f"Error handling slot click: {e}")
    
    def clear_detections(self):
        """Clear all detections"""
        self.detections.clear()
        self.update_display()

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
    aln_detection_received = Signal(np.ndarray, str)  # face_crop, detection_info
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
        
        # Store current frame for ALN detection
        self.current_frame = None
        self.current_faces = []
        
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
                # Store current frame for ALN detection
                if frame_type == "RAW":
                    self.current_frame = image.copy() if image is not None else None
                if frame_type == "ALN":
                    self.current_faces.append(image.copy())
                    if len(self.current_faces) > 5:
                        self.current_faces = self.current_faces[1:]
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
                
                # Check for ALN detections (assuming class_id 0 is ALN)
                aln_detections = [det for det in detections if det[0] == 0]  # class_id == 0
                if aln_detections and self.current_frame is not None:
                    # Process each ALN detection separately
                    for i, det in enumerate(aln_detections):
                        class_id, x, y, w, h, confidence, keypoints = det
                        
                        # Crop face from current frame
                        face_crop = self.current_faces[-1]
                        if face_crop is not None:
                            aln_info = f"Frame {frame_id}: ALN conf={confidence:.2f}"
                            self.aln_detection_received.emit(face_crop, aln_info)
                    
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
    
    def _crop_face(self, image: np.ndarray, x: int, y: int, w: int, h: int) -> Optional[np.ndarray]:
        """Crop face from image using bounding box coordinates"""
        try:
            if image is None:
                return None
                
            img_height, img_width = image.shape[:2]
            
            # Ensure coordinates are within image bounds
            x = max(0, min(x, img_width - 1))
            y = max(0, min(y, img_height - 1))
            w = max(1, min(w, img_width - x))
            h = max(1, min(h, img_height - y))
            
            # Crop the face region
            face_crop = image[y:y+h, x:x+w]
            
            # Add some padding if the crop is too small
            if face_crop.shape[0] < 50 or face_crop.shape[1] < 50:
                # Add padding to make it at least 50x50
                pad_h = max(0, 50 - face_crop.shape[0])
                pad_w = max(0, 50 - face_crop.shape[1])
                if len(face_crop.shape) == 3:
                    face_crop = np.pad(face_crop, ((0, pad_h), (0, pad_w), (0, 0)), mode='constant', constant_values=0)
                else:
                    face_crop = np.pad(face_crop, ((0, pad_h), (0, pad_w)), mode='constant', constant_values=0)
            
            return face_crop
            
        except Exception as e:
            logger.error(f"Error cropping face: {e}")
            return None

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
        self.setMinimumSize(1400, 800)  # Larger window to accommodate wider layout
        
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
        left_panel.setFixedWidth(280)  # Back to original width
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
        
        self.clear_aln_btn = QPushButton("Clear ALN History")
        self.clear_aln_btn.clicked.connect(self.clear_aln_detections)
        tools_layout.addWidget(self.clear_aln_btn)
        
        self.reset_stats_btn = QPushButton("Reset Statistics")
        self.reset_stats_btn.clicked.connect(self.reset_statistics)
        tools_layout.addWidget(self.reset_stats_btn)
        
        self.theme_btn = QPushButton("Toggle Theme")
        self.theme_btn.clicked.connect(self.toggle_theme)
        tools_layout.addWidget(self.theme_btn)
        
        left_layout.addWidget(tools_group)
        left_layout.addStretch()
        
        main_layout.addWidget(left_panel)
        
        # Center panel - display and log
        center_splitter = QSplitter(QtCore.Qt.Vertical)
        
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
        
        center_splitter.addWidget(image_container)
        
        # Log output
        self.log_widget = QTextEdit()
        self.log_widget.setMaximumHeight(150)
        self.log_widget.setReadOnly(True)
        center_splitter.addWidget(self.log_widget)
        
        main_layout.addWidget(center_splitter, 1)
        
        # Right panel - ALN Detection film strip
        self.aln_widget = ALNDetectionWidget()
        main_layout.addWidget(self.aln_widget)
        
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
            self.serial_reader.aln_detection_received.connect(self.on_aln_detection_received)
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
        """Handle received frame - only RAW frames go to main display"""
        self.frame_count += 1
        
        # Only show RAW frames in the main display, skip ALN detection frames
        if frame_type.upper() == "RAW" or frame_type.upper() == "GRAYSCALE":
            self.main_image_widget.set_image(image, frame_type)
        else:
            # Log other frame types but don't display them in main view
            self.log_message(f"Received {frame_type} frame (not displayed in main view)")
    
    def on_detections_received(self, frame_id: int, detections: List):
        """Handle received detections"""
        self.detection_count += len(detections)
        det_info = []
        for det in detections:
            class_id, x, y, w, h, confidence, keypoints = det
            det_info.append(f"cls:{class_id} conf:{confidence:.2f}")
        
        detection_details = ", ".join(det_info) if det_info else "none"
        self.log_message(f"Frame {frame_id}: {len(detections)} detections [{detection_details}]")
    
    def on_aln_detection_received(self, face_crop: np.ndarray, detection_info: str):
        """Handle received ALN detection"""
        self.aln_widget.add_detection(face_crop, detection_info)
        self.log_message(f"ALN Detection: {detection_info}")
    
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
    
    def clear_aln_detections(self):
        """Clear ALN detection history"""
        self.aln_widget.clear_detections()
        self.log_message("ALN detection history cleared")
    
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
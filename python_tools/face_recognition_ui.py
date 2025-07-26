#!/usr/bin/env python3
"""
STM32N6 Face Recognition UI using DearPyGui
Modern, responsive interface for face recognition system monitoring
"""

import time
import threading
import logging
import serial
import numpy as np
import cv2
from collections import deque
from dataclasses import dataclass
from typing import Optional, List, Dict, Any
from pathlib import Path
from serial.tools import list_ports

import dearpygui.dearpygui as dpg

from serial_protocol import (
    SerialProtocolParser, MessageType, ProtocolMessage,
    FrameDataParser, DetectionDataParser, EmbeddingDataParser
)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

@dataclass
class UIState:
    """UI state management"""
    connected: bool = False
    port_name: str = ""
    baud_rate: int = 7372800
    auto_reconnect: bool = True
    show_detections: bool = True
    show_embeddings: bool = True
    frame_count: int = 0
    fps: float = 0.0
    last_frame_time: float = 0.0

class FaceRecognitionUI:
    """Main UI application class"""
    
    def __init__(self):
        self.state = UIState()
        self.serial_conn: Optional[serial.Serial] = None
        self.protocol_parser = SerialProtocolParser()
        self.running = False
        self.worker_thread: Optional[threading.Thread] = None
        
        # Data storage
        self.current_frame: Optional[np.ndarray] = None
        self.current_detections: List = []
        self.recent_embeddings = deque(maxlen=10)
        self.frame_history = deque(maxlen=100)
        
        # UI elements
        self.texture_id = None
        self.frame_width = 320
        self.frame_height = 240
        
        # Setup protocol handlers
        self.setup_protocol_handlers()
        
    def setup_protocol_handlers(self):
        """Setup message handlers for the protocol parser"""
        self.protocol_parser.register_handler(MessageType.FRAME_DATA, self.handle_frame_data)
        self.protocol_parser.register_handler(MessageType.DETECTION_RESULTS, self.handle_detection_results)
        self.protocol_parser.register_handler(MessageType.EMBEDDING_DATA, self.handle_embedding_data)
        self.protocol_parser.register_handler(MessageType.PERFORMANCE_METRICS, self.handle_performance_metrics)
        self.protocol_parser.register_handler(MessageType.HEARTBEAT, self.handle_heartbeat)
        
    def handle_frame_data(self, message: ProtocolMessage):
        """Handle incoming frame data"""
        try:
            frame_data = FrameDataParser.parse_frame(message.payload)
            if frame_data:
                frame_type, frame, width, height, data_size, compression_ratio = frame_data
                
                # Log frame received for debugging
                logger.info(f"Frame received: {frame_type} {width}x{height}, {data_size} bytes, ratio: {compression_ratio}%")
                
                # Only update main display for main stream frames (JPG, RAW) not ALN frames
                if frame_type in ['JPG', 'RAW']:
                    # Update frame info for main stream
                    self.current_frame = frame
                    self.frame_width = width
                    self.frame_height = height
                    
                    # Calculate FPS
                    current_time = time.time()
                    if self.state.last_frame_time > 0:
                        frame_interval = current_time - self.state.last_frame_time
                        if frame_interval > 0:
                            self.state.fps = 1.0 / frame_interval
                    self.state.last_frame_time = current_time
                    self.state.frame_count += 1
                    
                    # Update texture for display
                    self.update_frame_texture(frame)
                elif frame_type == 'ALN':
                    # ALN frames are aligned/cropped faces - just log them
                    logger.info(f"ALN frame (face crop): {width}x{height}")
                
                # Store all frames in history
                self.frame_history.append({
                    'timestamp': time.time(),
                    'frame_type': frame_type,
                    'size': data_size,
                    'compression_ratio': compression_ratio,
                    'dimensions': f"{width}x{height}"
                })
                
        except Exception as e:
            logger.error(f"Error handling frame data: {e}")
    
    def handle_detection_results(self, message: ProtocolMessage):
        """Handle detection results"""
        try:
            detection_data = DetectionDataParser.parse_detections(message.payload)
            if detection_data:
                frame_id, detections = detection_data
                self.current_detections = detections
                logger.debug(f"Detections received: {len(detections)} faces")
        except Exception as e:
            logger.error(f"Error handling detection results: {e}")
    
    def handle_embedding_data(self, message: ProtocolMessage):
        """Handle embedding data"""
        try:
            embedding = EmbeddingDataParser.parse_embedding(message.payload)
            if embedding:
                self.recent_embeddings.append({
                    'timestamp': time.time(),
                    'embedding': embedding,
                    'size': len(embedding)
                })
                logger.debug(f"Embedding received: {len(embedding)} dimensions")
        except Exception as e:
            logger.error(f"Error handling embedding data: {e}")
    
    def handle_performance_metrics(self, message: ProtocolMessage):
        """Handle performance metrics"""
        try:
            # Performance metrics structure from C code
            if len(message.payload) >= 28:  # 7 * 4 bytes
                metrics = np.frombuffer(message.payload[:28], dtype=np.float32)
                logger.debug(f"Performance metrics: FPS={metrics[0]:.1f}, Inference={metrics[1]:.1f}ms")
        except Exception as e:
            logger.error(f"Error handling performance metrics: {e}")
    
    def handle_heartbeat(self, message: ProtocolMessage):
        """Handle heartbeat messages"""
        logger.debug("Heartbeat received")
    
    def update_frame_texture(self, frame: np.ndarray):
        """Update the frame texture for display"""
        try:
            if frame is None:
                return
                
            # Resize frame for display
            display_frame = cv2.resize(frame, (640, 480))
            
            # Draw detections if enabled
            if self.state.show_detections and self.current_detections:
                self.draw_detections(display_frame)
            
            # Convert BGR to RGB for DearPyGui
            display_frame = cv2.cvtColor(display_frame, cv2.COLOR_BGR2RGB)
            
            # Normalize to 0-1 range
            texture_data = display_frame.astype(np.float32) / 255.0
            
            # Update texture
            if self.texture_id is None:
                # Create new texture
                with dpg.texture_registry():
                    self.texture_id = dpg.add_raw_texture(
                        640, 480, texture_data, format=dpg.mvFormat_Float_rgb
                    )
            else:
                # Update existing texture
                dpg.set_value(self.texture_id, texture_data)
                
        except Exception as e:
            logger.error(f"Error updating frame texture: {e}")
    
    def draw_detections(self, frame: np.ndarray):
        """Draw detection boxes on frame"""
        try:
            height, width = frame.shape[:2]
            
            for detection in self.current_detections:
                if len(detection) >= 6:
                    class_id, x, y, w, h, confidence = detection[:6]
                    
                    # Convert normalized coordinates to pixel coordinates
                    x1 = int((x - w/2) * width)
                    y1 = int((y - h/2) * height)
                    x2 = int((x + w/2) * width)
                    y2 = int((y + h/2) * height)
                    
                    # Clamp coordinates
                    x1 = max(0, min(width-1, x1))
                    y1 = max(0, min(height-1, y1))
                    x2 = max(0, min(width-1, x2))
                    y2 = max(0, min(height-1, y2))
                    
                    # Draw rectangle
                    color = (0, 255, 0) if confidence > 0.7 else (255, 255, 0)
                    cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                    
                    # Draw confidence
                    label = f"{confidence:.2f}"
                    cv2.putText(frame, label, (x1, y1-5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)
                    
        except Exception as e:
            logger.error(f"Error drawing detections: {e}")
    
    def get_available_ports(self) -> List[str]:
        """Get list of available serial ports"""
        try:
            ports = list_ports.comports()
            available_ports = []
            
            for port in ports:
                try:
                    # Try to access the port to check permissions
                    test_serial = serial.Serial(port.device, timeout=0.1)
                    test_serial.close()
                    available_ports.append(port.device)
                    logger.info(f"Found accessible port: {port.device} - {port.description}")
                except (serial.SerialException, PermissionError) as e:
                    logger.warning(f"Port {port.device} exists but not accessible: {e}")
                    # Still add it to list but with a note
                    available_ports.append(f"{port.device} (permission denied)")
            
            # Also add manual entries for common STM32 ports
            common_ports = ["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0", "/dev/ttyUSB1"]
            for port in common_ports:
                if port not in [p.split()[0] for p in available_ports]:
                    try:
                        import os
                        if os.path.exists(port):
                            available_ports.append(f"{port} (needs permissions)")
                    except:
                        pass
            
            return available_ports if available_ports else ["No ports found"]
            
        except Exception as e:
            logger.error(f"Error getting serial ports: {e}")
            return ["Error detecting ports"]
    
    def connect_serial(self):
        """Connect to serial port"""
        try:
            if self.serial_conn and self.serial_conn.is_open:
                self.disconnect_serial()
            
            self.serial_conn = serial.Serial(
                port=self.state.port_name,
                baudrate=self.state.baud_rate,
                timeout=0.1
            )
            
            self.state.connected = True
            logger.info(f"Connected to {self.state.port_name} at {self.state.baud_rate} baud")
            
            # Start worker thread
            if not self.running:
                self.running = True
                self.worker_thread = threading.Thread(target=self.serial_worker, daemon=True)
                self.worker_thread.start()
                
        except Exception as e:
            logger.error(f"Failed to connect to serial port: {e}")
            self.state.connected = False
    
    def disconnect_serial(self):
        """Disconnect from serial port"""
        try:
            self.running = False
            
            if self.worker_thread:
                self.worker_thread.join(timeout=1.0)
            
            if self.serial_conn and self.serial_conn.is_open:
                self.serial_conn.close()
            
            self.state.connected = False
            logger.info("Disconnected from serial port")
            
        except Exception as e:
            logger.error(f"Error disconnecting: {e}")
    
    def serial_worker(self):
        """Worker thread for reading serial data"""
        consecutive_errors = 0
        
        while self.running and self.serial_conn and self.serial_conn.is_open:
            try:
                if self.serial_conn.in_waiting > 0:
                    data = self.serial_conn.read(self.serial_conn.in_waiting)
                    if data:
                        self.protocol_parser.add_data(data)
                        self.protocol_parser.process_messages()
                        consecutive_errors = 0
                
                time.sleep(0.001)  # Small sleep to prevent CPU spinning
                
            except Exception as e:
                consecutive_errors += 1
                logger.error(f"Serial worker error: {e}")
                
                if consecutive_errors > 10:
                    logger.error("Too many consecutive errors, stopping worker")
                    break
                    
                time.sleep(0.1)
        
        logger.info("Serial worker thread stopped")
    
    def create_ui(self):
        """Create the main UI"""
        dpg.create_context()
        dpg.create_viewport(title="STM32N6 Face Recognition Monitor", width=1200, height=800)
        
        with dpg.window(label="STM32N6 Face Recognition", tag="main_window"):
            
            # Connection panel
            with dpg.group(horizontal=True):
                with dpg.child_window(label="Connection", width=300, height=150):
                    dpg.add_text("Serial Connection")
                    dpg.add_separator()
                    
                    # Port selection
                    ports = self.get_available_ports()
                    dpg.add_combo(
                        items=ports,
                        label="Port",
                        tag="port_combo",
                        default_value=ports[0] if ports else "",
                        callback=self.on_port_selected,
                        width=200
                    )
                    
                    # Baud rate
                    dpg.add_input_int(
                        label="Baud Rate",
                        tag="baud_input",
                        default_value=self.state.baud_rate,
                        callback=self.on_baud_changed,
                        width=200
                    )
                    
                    # Connection buttons
                    with dpg.group(horizontal=True):
                        dpg.add_button(
                            label="Connect",
                            tag="connect_btn",
                            callback=self.on_connect_clicked,
                            width=70
                        )
                        dpg.add_button(
                            label="Disconnect",
                            tag="disconnect_btn",
                            callback=self.on_disconnect_clicked,
                            width=70,
                            enabled=False
                        )
                
                # Status panel
                with dpg.child_window(label="Status", width=300, height=150):
                    dpg.add_text("System Status")
                    dpg.add_separator()
                    
                    dpg.add_text("Disconnected", tag="connection_status", color=(255, 100, 100))
                    dpg.add_text("Frames: 0", tag="frame_counter")
                    dpg.add_text("FPS: 0.0", tag="fps_display")
                    dpg.add_text("Detections: 0", tag="detection_counter")
                    dpg.add_text("Embeddings: 0", tag="embedding_counter")
            
            dpg.add_separator()
            
            # Main content area
            with dpg.group(horizontal=True):
                
                # Video display
                with dpg.child_window(label="Video Feed", width=660, height=500):
                    dpg.add_text("Video Feed")
                    dpg.add_separator()
                    
                    # Video display group
                    with dpg.group(tag="video_display_group"):
                        dpg.add_text("No video signal", tag="video_placeholder")
                    
                    # Video display options
                    with dpg.group(horizontal=True):
                        dpg.add_checkbox(
                            label="Show Detections",
                            tag="show_detections_cb",
                            default_value=self.state.show_detections,
                            callback=self.on_show_detections_changed
                        )
                        dpg.add_checkbox(
                            label="Show Embeddings",
                            tag="show_embeddings_cb",
                            default_value=self.state.show_embeddings,
                            callback=self.on_show_embeddings_changed
                        )
                
                # Data panels
                with dpg.child_window(label="Data", width=520, height=500):
                    
                    with dpg.tab_bar():
                        
                        # Protocol statistics
                        with dpg.tab(label="Protocol Stats"):
                            dpg.add_text("Protocol Statistics")
                            dpg.add_separator()
                            
                            with dpg.table(header_row=True, borders_innerH=True, borders_outerH=True,
                                         borders_innerV=True, borders_outerV=True):
                                dpg.add_table_column(label="Metric")
                                dpg.add_table_column(label="Value")
                                
                                with dpg.table_row():
                                    dpg.add_text("Messages Received")
                                    dpg.add_text("0", tag="stats_messages")
                                
                                with dpg.table_row():
                                    dpg.add_text("Bytes Received")
                                    dpg.add_text("0", tag="stats_bytes")
                                
                                with dpg.table_row():
                                    dpg.add_text("CRC Errors")
                                    dpg.add_text("0", tag="stats_crc_errors")
                                
                                with dpg.table_row():
                                    dpg.add_text("Sync Errors")
                                    dpg.add_text("0", tag="stats_sync_errors")
                                
                                with dpg.table_row():
                                    dpg.add_text("Throughput (Mbps)")
                                    dpg.add_text("0.0", tag="stats_throughput")
                        
                        # Frame history
                        with dpg.tab(label="Frame History"):
                            dpg.add_text("Recent Frames")
                            dpg.add_separator()
                            
                            with dpg.child_window(height=400, tag="frame_history_window"):
                                dpg.add_text("No frames received", tag="frame_history_list")
                        
                        # Embeddings
                        with dpg.tab(label="Embeddings"):
                            dpg.add_text("Recent Embeddings")
                            dpg.add_separator()
                            
                            with dpg.child_window(height=400, tag="embeddings_window"):
                                dpg.add_text("No embeddings received", tag="embeddings_list")
        
        # Set main window as primary
        dpg.set_primary_window("main_window", True)
    
    def update_ui_elements(self):
        """Update UI elements manually"""
        try:
            # Update status display
            if self.state.connected:
                dpg.set_value("connection_status", "Connected")
                dpg.configure_item("connection_status", color=(100, 255, 100))
                dpg.configure_item("connect_btn", enabled=False)
                dpg.configure_item("disconnect_btn", enabled=True)
            else:
                dpg.set_value("connection_status", "Disconnected")
                dpg.configure_item("connection_status", color=(255, 100, 100))
                dpg.configure_item("connect_btn", enabled=True)
                dpg.configure_item("disconnect_btn", enabled=False)
            
            # Update counters
            dpg.set_value("frame_counter", f"Frames: {self.state.frame_count}")
            dpg.set_value("fps_display", f"FPS: {self.state.fps:.1f}")
            dpg.set_value("detection_counter", f"Detections: {len(self.current_detections)}")
            dpg.set_value("embedding_counter", f"Embeddings: {len(self.recent_embeddings)}")
            
            # Update protocol statistics
            stats = self.protocol_parser.get_stats()
            dpg.set_value("stats_messages", str(stats['messages_received']))
            dpg.set_value("stats_bytes", f"{stats['bytes_received']:,}")
            dpg.set_value("stats_crc_errors", str(stats['crc_errors']))
            dpg.set_value("stats_sync_errors", str(stats['sync_errors']))
            dpg.set_value("stats_throughput", f"{stats['throughput_mbps']:.2f}")
            
            # Update frame history
            self.update_frame_history_display()
            
            # Update embeddings display
            self.update_embeddings_display()
            
            # Show/hide video based on frame availability
            if self.current_frame is not None and self.texture_id is not None:
                if dpg.does_item_exist("video_image"):
                    dpg.configure_item("video_image", texture_tag=self.texture_id)
                else:
                    if dpg.does_item_exist("video_placeholder"):
                        dpg.delete_item("video_placeholder")
                    dpg.add_image(self.texture_id, tag="video_image", parent="video_display_group")
            
        except Exception as e:
            logger.error(f"Error updating UI: {e}")
    
    def update_frame_history_display(self):
        """Update frame history display"""
        try:
            if not self.frame_history:
                return
                
            # Build history text
            history_text = ""
            for i, frame_info in enumerate(list(self.frame_history)[-10:]):  # Show last 10 frames
                timestamp = time.strftime("%H:%M:%S", time.localtime(frame_info['timestamp']))
                history_text += f"{timestamp} - {frame_info['frame_type']} {frame_info['dimensions']} "
                history_text += f"({frame_info['size']} bytes, {frame_info['compression_ratio']}% ratio)\\n"
            
            dpg.set_value("frame_history_list", history_text)
            
        except Exception as e:
            logger.error(f"Error updating frame history: {e}")
    
    def update_embeddings_display(self):
        """Update embeddings display"""
        try:
            if not self.recent_embeddings:
                return
                
            # Build embeddings text
            embeddings_text = ""
            for i, emb_info in enumerate(list(self.recent_embeddings)[-5:]):  # Show last 5 embeddings
                timestamp = time.strftime("%H:%M:%S", time.localtime(emb_info['timestamp']))
                embeddings_text += f"{timestamp} - {emb_info['size']} dimensions\\n"
                
                # Show first few values
                embedding = emb_info['embedding']
                preview = ", ".join([f"{val:.3f}" for val in embedding[:5]])
                embeddings_text += f"  [{preview}...]\\n\\n"
            
            dpg.set_value("embeddings_list", embeddings_text)
            
        except Exception as e:
            logger.error(f"Error updating embeddings display: {e}")
    
    # UI Callbacks
    def on_port_selected(self, sender, app_data):
        """Handle port selection"""
        # Extract just the port name if it has extra info
        port_name = app_data.split()[0] if app_data else ""
        self.state.port_name = port_name
        logger.info(f"Selected port: {port_name}")
    
    def on_baud_changed(self, sender, app_data):
        """Handle baud rate change"""
        self.state.baud_rate = app_data
    
    def on_connect_clicked(self):
        """Handle connect button click"""
        self.connect_serial()
    
    def on_disconnect_clicked(self):
        """Handle disconnect button click"""
        self.disconnect_serial()
    
    def on_show_detections_changed(self, sender, app_data):
        """Handle show detections checkbox"""
        self.state.show_detections = app_data
    
    def on_show_embeddings_changed(self, sender, app_data):
        """Handle show embeddings checkbox"""
        self.state.show_embeddings = app_data
    
    def run(self):
        """Run the application"""
        try:
            self.create_ui()
            
            dpg.setup_dearpygui()
            dpg.show_viewport()
            
            # Main loop with manual UI updates
            frame_count = 0
            while dpg.is_dearpygui_running():
                dpg.render_dearpygui_frame()
                
                # Update UI every 10 frames (~6 times per second at 60 FPS)
                frame_count += 1
                if frame_count % 10 == 0:
                    self.update_ui_elements()
                
                time.sleep(0.016)  # ~60 FPS
                
        except KeyboardInterrupt:
            logger.info("Application interrupted by user")
        except Exception as e:
            logger.error(f"Application error: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Cleanup resources"""
        try:
            self.disconnect_serial()
            dpg.destroy_context()
            logger.info("Application cleanup completed")
        except Exception as e:
            logger.error(f"Error during cleanup: {e}")

def main():
    """Main entry point"""
    try:
        app = FaceRecognitionUI()
        app.run()
    except Exception as e:
        logger.error(f"Failed to start application: {e}")

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Simple diagnostic script to test embedded streaming data
Logs raw data and attempts basic parsing to identify corruption source
"""

import time
import struct
import logging
import cv2
import numpy as np
import serial
from pathlib import Path
from serial.tools import list_ports

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('stream_diagnostics.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class StreamDiagnostics:
    def __init__(self):
        self.stats = {
            'bytes_received': 0,
            'frames_found': 0,
            'frames_decoded': 0,
            'parse_errors': 0,
            'sof_markers_found': 0,
            'valid_headers': 0,
            'invalid_headers': 0
        }
        self.raw_data_buffer = bytearray()
        self.output_dir = Path("diagnostic_frames")
        self.output_dir.mkdir(exist_ok=True)
        
    def analyze_raw_data(self, data: bytes):
        """Analyze incoming raw data for patterns"""
        self.stats['bytes_received'] += len(data)
        self.raw_data_buffer.extend(data)
        
        # Look for SOF markers (0xAA)
        sof_count = data.count(0xAA)
        self.stats['sof_markers_found'] += sof_count
        
        if sof_count > 0:
            logger.info(f"Found {sof_count} SOF markers in {len(data)} bytes")
            
        # Try to find and parse frame headers
        self.find_and_parse_frames()
        
        # Keep buffer manageable (last 1MB)
        if len(self.raw_data_buffer) > 1024 * 1024:
            self.raw_data_buffer = self.raw_data_buffer[-512*1024:]
            
    def find_and_parse_frames(self):
        """Look for complete frames in buffer"""
        buffer = self.raw_data_buffer
        pos = 0
        
        while pos < len(buffer) - 4:
            # Look for SOF byte (0xAA)
            sof_pos = buffer.find(0xAA, pos)
            if sof_pos == -1:
                break
                
            # Check if we have enough data for header
            if sof_pos + 4 > len(buffer):
                break
                
            # Parse header: SOF(1) + PayloadSize(2) + Checksum(1)
            try:
                header = buffer[sof_pos:sof_pos + 4]
                sof, payload_size_low, payload_size_high, checksum = header
                payload_size = payload_size_low | (payload_size_high << 8)
                
                # Validate header
                if payload_size > 0 and payload_size < 64*1024:
                    # Calculate header checksum
                    expected_checksum = sof ^ payload_size_low ^ payload_size_high
                    
                    if checksum == expected_checksum:
                        self.stats['valid_headers'] += 1
                        logger.info(f"Valid header at pos {sof_pos}: payload_size={payload_size}")
                        
                        # Check if we have the complete message
                        message_end = sof_pos + 4 + payload_size
                        if message_end <= len(buffer):
                            self.parse_complete_message(buffer[sof_pos:message_end])
                            pos = message_end
                        else:
                            # Incomplete message, wait for more data
                            break
                    else:
                        self.stats['invalid_headers'] += 1
                        logger.warning(f"Invalid checksum at pos {sof_pos}: got {checksum}, expected {expected_checksum}")
                        pos = sof_pos + 1
                else:
                    logger.warning(f"Invalid payload size at pos {sof_pos}: {payload_size}")
                    pos = sof_pos + 1
                    
            except Exception as e:
                logger.error(f"Error parsing header at pos {sof_pos}: {e}")
                pos = sof_pos + 1
                
    def parse_complete_message(self, message: bytes):
        """Parse a complete message"""
        try:
            # Skip frame header (4 bytes)
            payload = message[4:]
            
            if len(payload) < 3:
                return
                
            # Parse message header: MessageType(1) + SequenceId(2)
            msg_type, seq_id_low, seq_id_high = payload[:3]
            seq_id = seq_id_low | (seq_id_high << 8)
            
            logger.info(f"Message: type={msg_type}, seq={seq_id}, payload_len={len(payload)-3}")
            
            # If it's a frame data message (type 1), try to parse it
            if msg_type == 1:  # FRAME_DATA
                self.parse_frame_data(payload[3:])
                
        except Exception as e:
            logger.error(f"Error parsing message: {e}")
            self.stats['parse_errors'] += 1
            
    def parse_frame_data(self, frame_payload: bytes):
        """Parse frame data payload"""
        try:
            if len(frame_payload) < 12:
                logger.warning("Frame payload too short")
                return
                
            # Parse frame header: FrameType(4) + Width(4) + Height(4) + ImageData(...)
            frame_type = frame_payload[:4].decode('ascii').rstrip('\x00')
            width = struct.unpack('<I', frame_payload[4:8])[0]
            height = struct.unpack('<I', frame_payload[8:12])[0]
            
            image_data = frame_payload[12:]
            
            logger.info(f"Frame: type='{frame_type}', size={width}x{height}, jpeg_len={len(image_data)}")
            self.stats['frames_found'] += 1
            
            # Try to decode JPEG
            if image_data:
                self.decode_and_save_frame(image_data, frame_type, width, height)
                
        except Exception as e:
            logger.error(f"Error parsing frame data: {e}")
            
    def decode_and_save_frame(self, jpeg_data: bytes, frame_type: str, width: int, height: int):
        """Decode JPEG and save for analysis"""
        try:
            # Decode JPEG
            img_array = np.frombuffer(jpeg_data, dtype=np.uint8)
            frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
            
            if frame is not None:
                self.stats['frames_decoded'] += 1
                
                # Save frame for analysis
                timestamp = int(time.time() * 1000)
                filename = f"frame_{timestamp}_{frame_type}_{width}x{height}.jpg"
                filepath = self.output_dir / filename
                
                cv2.imwrite(str(filepath), frame)
                logger.info(f"Saved frame: {filename} (actual size: {frame.shape})")
                
                # Log any size mismatches
                if frame.shape[1] != width or frame.shape[0] != height:
                    logger.warning(f"Size mismatch! Header: {width}x{height}, Actual: {frame.shape[1]}x{frame.shape[0]}")
                    
            else:
                logger.error("Failed to decode JPEG data")
                
                # Save raw JPEG data for analysis
                timestamp = int(time.time() * 1000)
                filename = f"bad_jpeg_{timestamp}_{frame_type}.bin"
                filepath = self.output_dir / filename
                
                with open(filepath, 'wb') as f:
                    f.write(jpeg_data)
                logger.info(f"Saved bad JPEG data: {filename}")
                
        except Exception as e:
            logger.error(f"Error decoding frame: {e}")
            
    def print_stats(self):
        """Print current statistics"""
        logger.info("=== Stream Diagnostics ===")
        for key, value in self.stats.items():
            logger.info(f"{key}: {value}")
            
        # Calculate rates
        if self.stats['bytes_received'] > 0:
            sof_rate = (self.stats['sof_markers_found'] / self.stats['bytes_received']) * 100
            logger.info(f"SOF marker rate: {sof_rate:.3f}%")
            
        if self.stats['frames_found'] > 0:
            decode_rate = (self.stats['frames_decoded'] / self.stats['frames_found']) * 100
            logger.info(f"Decode success rate: {decode_rate:.1f}%")

def main():
    # List available ports
    ports = list_ports.comports()
    if not ports:
        logger.error("No serial ports found!")
        return
        
    logger.info("Available ports:")
    for i, port in enumerate(ports):
        logger.info(f"{i}: {port.device} - {port.description}")
        
    # Use first port or prompt user
    port_name = ports[0].device
    baud_rate = 7372800  # High baud rate
    
    logger.info(f"Connecting to {port_name} at {baud_rate} baud...")
    
    try:
        ser = serial.Serial(port_name, baud_rate, timeout=0.1)
        diagnostics = StreamDiagnostics()
        
        logger.info("Starting data collection... Press Ctrl+C to stop")
        
        start_time = time.time()
        last_stats_time = start_time
        
        while True:
            # Read available data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                if data:
                    diagnostics.analyze_raw_data(data)
                    
            # Print stats every 10 seconds
            current_time = time.time()
            if current_time - last_stats_time >= 10:
                diagnostics.print_stats()
                last_stats_time = current_time
                
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        logger.info("Stopping data collection...")
        
    except Exception as e:
        logger.error(f"Error: {e}")
        
    finally:
        if 'ser' in locals():
            ser.close()
            
        if 'diagnostics' in locals():
            diagnostics.print_stats()
            logger.info(f"Diagnostic frames saved to: {diagnostics.output_dir}")

if __name__ == "__main__":
    main()
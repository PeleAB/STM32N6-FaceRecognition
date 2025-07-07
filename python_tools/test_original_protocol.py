#!/usr/bin/env python3
"""
Test the original simple text-based protocol parsing
To compare quality with current robust protocol
"""

import time
import logging
import cv2
import numpy as np
import serial
from pathlib import Path
from serial.tools import list_ports

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class OriginalProtocolParser:
    """Parse the original simple text-based protocol"""
    
    def __init__(self):
        self.buffer = bytearray()
        self.frames_decoded = 0
        self.output_dir = Path("original_test_frames")
        self.output_dir.mkdir(exist_ok=True)
        
    def add_data(self, data: bytes):
        """Add incoming data to buffer"""
        self.buffer.extend(data)
        self.process_buffer()
        
        # Keep buffer manageable
        if len(self.buffer) > 1024 * 1024:
            self.buffer = self.buffer[-512*1024:]
            
    def process_buffer(self):
        """Process buffer looking for complete frames"""
        while True:
            # Look for frame header line (e.g., "JPG 320 240 15234\n")
            newline_pos = self.buffer.find(b'\n')
            if newline_pos == -1:
                break
                
            # Extract header line
            header_line = self.buffer[:newline_pos].decode('ascii', errors='ignore').strip()
            
            # Parse header
            parts = header_line.split()
            if len(parts) >= 4 and parts[0] in ['JPG', 'ALN']:
                try:
                    frame_type = parts[0]
                    width = int(parts[1])
                    height = int(parts[2])
                    jpeg_size = int(parts[3])
                    
                    logger.info(f"Frame header: {frame_type} {width}x{height}, JPEG size: {jpeg_size}")
                    
                    # Check if we have enough data for the complete JPEG
                    header_end = newline_pos + 1
                    if len(self.buffer) >= header_end + jpeg_size:
                        # Extract JPEG data
                        jpeg_data = self.buffer[header_end:header_end + jpeg_size]
                        
                        # Try to decode
                        self.decode_frame(jpeg_data, frame_type, width, height)
                        
                        # Remove processed data from buffer
                        self.buffer = self.buffer[header_end + jpeg_size:]
                        continue
                    else:
                        # Not enough data yet
                        break
                        
                except (ValueError, IndexError) as e:
                    logger.warning(f"Invalid header: {header_line} - {e}")
                    
            # Remove invalid header line
            self.buffer = self.buffer[newline_pos + 1:]
            
    def decode_frame(self, jpeg_data: bytes, frame_type: str, expected_width: int, expected_height: int):
        """Decode JPEG frame"""
        try:
            img_array = np.frombuffer(jpeg_data, dtype=np.uint8)
            frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
            
            if frame is not None:
                self.frames_decoded += 1
                actual_height, actual_width = frame.shape[:2]
                
                # Save frame
                timestamp = int(time.time() * 1000)
                filename = f"{frame_type}_{timestamp}_{actual_width}x{actual_height}.jpg"
                filepath = self.output_dir / filename
                cv2.imwrite(str(filepath), frame)
                
                logger.info(f"Decoded frame {self.frames_decoded}: {filename}")
                
                # Check for size mismatches
                if actual_width != expected_width or actual_height != expected_height:
                    logger.warning(f"Size mismatch! Expected: {expected_width}x{expected_height}, "
                                 f"Got: {actual_width}x{actual_height}")
                                 
                # Analyze quality
                self.analyze_frame_quality(frame, frame_type)
                
            else:
                logger.error(f"Failed to decode {frame_type} JPEG data ({len(jpeg_data)} bytes)")
                
                # Save bad data for analysis
                timestamp = int(time.time() * 1000)
                filename = f"bad_{frame_type}_{timestamp}.bin"
                with open(self.output_dir / filename, 'wb') as f:
                    f.write(jpeg_data)
                    
        except Exception as e:
            logger.error(f"Error decoding frame: {e}")
            
    def analyze_frame_quality(self, frame: np.ndarray, frame_type: str):
        """Analyze frame quality metrics"""
        try:
            # Calculate basic quality metrics
            height, width = frame.shape[:2]
            
            # Check if it's grayscale or color
            if len(frame.shape) == 3:
                # Convert to grayscale for analysis
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            else:
                gray = frame
                
            # Calculate variance (blur metric)
            variance = cv2.Laplacian(gray, cv2.CV_64F).var()
            
            # Calculate brightness
            brightness = np.mean(gray)
            
            logger.info(f"{frame_type} quality - Size: {width}x{height}, "
                       f"Variance: {variance:.1f}, Brightness: {brightness:.1f}")
                       
            # Detect artifacts (very basic)
            if variance < 100:
                logger.warning(f"{frame_type} frame may be blurry (low variance: {variance:.1f})")
                
        except Exception as e:
            logger.debug(f"Error analyzing quality: {e}")

def main():
    """Main test function"""
    # List available ports
    ports = list_ports.comports()
    if not ports:
        logger.error("No serial ports found!")
        return
        
    logger.info("Available ports:")
    for i, port in enumerate(ports):
        logger.info(f"{i}: {port.device} - {port.description}")
        
    # Use first port
    port_name = ports[0].device
    baud_rate = 7372800
    
    logger.info(f"Testing original protocol on {port_name} at {baud_rate} baud...")
    logger.info("Make sure embedded side is using pc_stream.c (original protocol)!")
    
    try:
        ser = serial.Serial(port_name, baud_rate, timeout=0.1)
        parser = OriginalProtocolParser()
        
        logger.info("Collecting frames... Press Ctrl+C to stop")
        start_time = time.time()
        
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                if data:
                    parser.add_data(data)
                    
            # Print stats every 30 seconds
            if time.time() - start_time > 30:
                logger.info(f"Frames decoded so far: {parser.frames_decoded}")
                start_time = time.time()
                
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        logger.info("Stopping...")
        logger.info(f"Total frames decoded: {parser.frames_decoded}")
        logger.info(f"Frames saved to: {parser.output_dir}")
        
    except Exception as e:
        logger.error(f"Error: {e}")
        
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()
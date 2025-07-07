#!/usr/bin/env python3
"""
Test a simplified protocol that keeps quality improvements but reduces complexity
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

class SimpleProtocolParser:
    """Parse a simplified protocol similar to original but with quality improvements"""
    
    def __init__(self):
        self.buffer = bytearray()
        self.frames_decoded = 0
        self.stats = {
            'bytes_received': 0,
            'frames_found': 0,
            'frames_decoded': 0,
            'jpg_frames': 0,
            'aln_frames': 0
        }
        self.output_dir = Path("simple_test_frames")
        self.output_dir.mkdir(exist_ok=True)
        
    def add_data(self, data: bytes):
        """Add incoming data to buffer"""
        self.buffer.extend(data)
        self.stats['bytes_received'] += len(data)
        self.process_buffer()
        
        # Keep buffer manageable
        if len(self.buffer) > 2 * 1024 * 1024:  # 2MB buffer
            self.buffer = self.buffer[-1024*1024:]  # Keep last 1MB
            
    def process_buffer(self):
        """Process buffer looking for complete frames"""
        while True:
            # Look for frame header line ending with newline
            newline_pos = self.buffer.find(b'\n')
            if newline_pos == -1:
                break
                
            # Extract and parse header line
            header_line = self.buffer[:newline_pos].decode('ascii', errors='ignore').strip()
            
            if self.parse_frame_header(header_line, newline_pos + 1):
                continue
            else:
                # Invalid header, remove it and continue
                self.buffer = self.buffer[newline_pos + 1:]
                
    def parse_frame_header(self, header: str, data_start: int):
        """Parse frame header and extract frame if complete"""
        try:
            parts = header.split()
            
            # Frame format: "JPG 320 240 15234" or "ALN 96 112 8192"
            if len(parts) >= 4 and parts[0] in ['JPG', 'ALN']:
                frame_type = parts[0]
                width = int(parts[1])
                height = int(parts[2])
                jpeg_size = int(parts[3])
                
                # Sanity checks
                if not (10 <= width <= 2000 and 10 <= height <= 2000 and 100 <= jpeg_size <= 100*1024):
                    logger.warning(f"Invalid frame parameters: {header}")
                    return False
                
                # Check if we have complete JPEG data
                if len(self.buffer) >= data_start + jpeg_size:
                    jpeg_data = self.buffer[data_start:data_start + jpeg_size]
                    
                    # Process the frame
                    success = self.decode_frame(jpeg_data, frame_type, width, height)
                    
                    # Remove processed data
                    self.buffer = self.buffer[data_start + jpeg_size:]
                    
                    self.stats['frames_found'] += 1
                    if frame_type == 'JPG':
                        self.stats['jpg_frames'] += 1
                    elif frame_type == 'ALN':
                        self.stats['aln_frames'] += 1
                    
                    return True
                else:
                    # Not enough data yet, wait for more
                    return False
                    
        except (ValueError, IndexError) as e:
            logger.debug(f"Header parse error: {header} - {e}")
            
        return False
        
    def decode_frame(self, jpeg_data: bytes, frame_type: str, expected_width: int, expected_height: int):
        """Decode and analyze JPEG frame"""
        try:
            # Validate JPEG header
            if not jpeg_data.startswith(b'\\xff\\xd8'):
                logger.warning(f"Invalid JPEG header for {frame_type} frame")
                self.save_bad_data(jpeg_data, frame_type, "invalid_header")
                return False
                
            # Decode JPEG
            img_array = np.frombuffer(jpeg_data, dtype=np.uint8)
            
            # Try different decode modes for robustness
            frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
            if frame is None:
                frame = cv2.imdecode(img_array, cv2.IMREAD_UNCHANGED)
                
            if frame is not None:
                self.frames_decoded += 1
                actual_height, actual_width = frame.shape[:2]
                
                # Save frame with detailed info
                timestamp = int(time.time() * 1000)
                channels = frame.shape[2] if len(frame.shape) == 3 else 1
                filename = f"{frame_type}_{self.frames_decoded:04d}_{actual_width}x{actual_height}_{channels}ch.jpg"
                filepath = self.output_dir / filename
                cv2.imwrite(str(filepath), frame)
                
                # Detailed logging
                logger.info(f"✓ Frame {self.frames_decoded}: {frame_type} {actual_width}x{actual_height} "
                           f"({channels}ch, {len(jpeg_data)} bytes)")
                
                # Quality analysis
                self.analyze_quality(frame, frame_type, jpeg_data)
                
                # Check for issues
                if actual_width != expected_width or actual_height != expected_height:
                    logger.error(f"SIZE MISMATCH! Expected: {expected_width}x{expected_height}, "
                               f"Got: {actual_width}x{actual_height}")
                               
                self.stats['frames_decoded'] += 1
                return True
                
            else:
                logger.error(f"DECODE FAILED for {frame_type} ({len(jpeg_data)} bytes)")
                self.save_bad_data(jpeg_data, frame_type, "decode_failed")
                return False
                
        except Exception as e:
            logger.error(f"Exception decoding {frame_type} frame: {e}")
            self.save_bad_data(jpeg_data, frame_type, "exception")
            return False
            
    def save_bad_data(self, data: bytes, frame_type: str, reason: str):
        """Save problematic data for analysis"""
        timestamp = int(time.time() * 1000)
        filename = f"BAD_{frame_type}_{reason}_{timestamp}.bin"
        filepath = self.output_dir / filename
        
        with open(filepath, 'wb') as f:
            f.write(data)
            
        logger.info(f"Saved bad data: {filename} ({len(data)} bytes)")
        
    def analyze_quality(self, frame: np.ndarray, frame_type: str, jpeg_data: bytes):
        """Analyze frame quality"""
        try:
            height, width = frame.shape[:2]
            channels = frame.shape[2] if len(frame.shape) == 3 else 1
            
            # Convert to grayscale for analysis
            if channels == 3:
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            else:
                gray = frame
                
            # Quality metrics
            variance = cv2.Laplacian(gray, cv2.CV_64F).var()
            brightness = np.mean(gray)
            contrast = np.std(gray)
            
            # Compression efficiency
            pixels = width * height * channels
            compression_ratio = pixels / len(jpeg_data)
            
            logger.info(f"  Quality: var={variance:.1f}, bright={brightness:.1f}, "
                       f"contrast={contrast:.1f}, compression={compression_ratio:.1f}x")
            
            # Detect potential issues
            issues = []
            if variance < 50:
                issues.append("very_blurry")
            elif variance < 100:
                issues.append("blurry")
                
            if brightness < 30:
                issues.append("very_dark")
            elif brightness > 225:
                issues.append("very_bright")
                
            if contrast < 20:
                issues.append("low_contrast")
                
            if compression_ratio < 5:
                issues.append("poor_compression")
                
            if issues:
                logger.warning(f"  Quality issues: {', '.join(issues)}")
                
        except Exception as e:
            logger.debug(f"Quality analysis error: {e}")
            
    def print_stats(self):
        """Print statistics"""
        logger.info("=== Statistics ===")
        for key, value in self.stats.items():
            logger.info(f"{key}: {value}")
            
        if self.stats['frames_found'] > 0:
            decode_rate = (self.stats['frames_decoded'] / self.stats['frames_found']) * 100
            logger.info(f"Decode success rate: {decode_rate:.1f}%")
            
        if self.stats['bytes_received'] > 0:
            throughput = self.stats['bytes_received'] / (1024 * 1024)  # MB
            logger.info(f"Data received: {throughput:.1f} MB")

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
    
    logger.info(f"Testing simple protocol on {port_name} at {baud_rate} baud...")
    logger.info("This will work with either pc_stream.c or enhanced_pc_stream.c")
    
    try:
        ser = serial.Serial(port_name, baud_rate, timeout=0.1)
        parser = SimpleProtocolParser()
        
        logger.info("Collecting and analyzing frames... Press Ctrl+C to stop")
        start_time = time.time()
        last_stats = time.time()
        
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                if data:
                    parser.add_data(data)
                    
            # Print stats every 15 seconds
            current_time = time.time()
            if current_time - last_stats > 15:
                parser.print_stats()
                last_stats = current_time
                
            time.sleep(0.005)  # Small sleep for efficiency
            
    except KeyboardInterrupt:
        logger.info("\\n=== FINAL RESULTS ===")
        parser.print_stats()
        logger.info(f"Frames saved to: {parser.output_dir}")
        logger.info("Check the saved frames to compare quality!")
        
    except Exception as e:
        logger.error(f"Error: {e}")
        
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    main()
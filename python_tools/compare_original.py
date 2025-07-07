#!/usr/bin/env python3
"""
Compare current robust protocol with original text-based protocol
To understand quality differences
"""

import time
import struct
import logging
import cv2
import numpy as np
import serial
from pathlib import Path

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class OriginalProtocolTest:
    """Test the original text-based protocol parsing"""
    
    def __init__(self):
        self.frames_saved = 0
        self.output_dir = Path("original_frames")
        self.output_dir.mkdir(exist_ok=True)
        
    def parse_original_format(self, data: bytes):
        """Parse data using original text-based format"""
        try:
            # Look for frame markers like "FRAME_START" or similar
            data_str = data.decode('ascii', errors='ignore')
            
            # Original format typically had text headers
            lines = data_str.split('\n')
            for line in lines:
                if 'FRAME' in line or 'JPG' in line:
                    logger.info(f"Original format line: {line[:100]}...")
                    
        except Exception as e:
            # If it's not text, it might be binary JPEG data directly
            self.try_direct_jpeg_decode(data)
            
    def try_direct_jpeg_decode(self, data: bytes):
        """Try to decode data directly as JPEG"""
        try:
            # Look for JPEG markers
            if b'\xff\xd8' in data:  # JPEG SOI marker
                jpeg_start = data.find(b'\xff\xd8')
                jpeg_data = data[jpeg_start:]
                
                # Try to decode
                img_array = np.frombuffer(jpeg_data, dtype=np.uint8)
                frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                
                if frame is not None:
                    filename = f"original_frame_{self.frames_saved}.jpg"
                    cv2.imwrite(str(self.output_dir / filename), frame)
                    self.frames_saved += 1
                    logger.info(f"Decoded original frame: {filename}, size: {frame.shape}")
                    
        except Exception as e:
            logger.debug(f"Not JPEG data: {e}")

def test_current_vs_original():
    """Test current robust protocol vs trying to parse as original"""
    
    # List available ports
    from serial.tools import list_ports
    ports = list_ports.comports()
    if not ports:
        logger.error("No serial ports found!")
        return
        
    port_name = ports[0].device
    baud_rate = 7372800
    
    logger.info(f"Testing protocols on {port_name} at {baud_rate} baud...")
    
    try:
        ser = serial.Serial(port_name, baud_rate, timeout=0.1)
        original_tester = OriginalProtocolTest()
        
        logger.info("Collecting data for comparison... Press Ctrl+C to stop")
        
        data_buffer = bytearray()
        
        while True:
            if ser.in_waiting > 0:
                new_data = ser.read(ser.in_waiting)
                if new_data:
                    data_buffer.extend(new_data)
                    
                    # Test parsing as original format
                    original_tester.parse_original_format(new_data)
                    
                    # Keep buffer manageable
                    if len(data_buffer) > 512*1024:
                        data_buffer = data_buffer[-256*1024:]
                        
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        logger.info("Stopping test...")
        
    except Exception as e:
        logger.error(f"Error: {e}")
        
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    test_current_vs_original()
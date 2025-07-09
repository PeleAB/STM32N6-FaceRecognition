#!/usr/bin/env python3
"""
STM32N6 Enhanced PC Streaming Protocol with CRC32 Validation
Reference implementation for PC side message reception and validation
"""

import struct
import zlib
import serial
from typing import Optional, Tuple, Dict, Any
from dataclasses import dataclass
from enum import IntEnum

# Protocol constants
ROBUST_SOF_BYTE = 0xAA
ROBUST_HEADER_SIZE = 4  # 4 bytes for header only
ROBUST_CRC_SIZE = 4     # CRC32 at end of packet
ROBUST_MSG_HEADER_SIZE = 3
ROBUST_MAX_PAYLOAD_SIZE = 64 * 1024

class MessageType(IntEnum):
    """Message types for robust protocol"""
    FRAME_DATA = 0x01
    DETECTION_RESULTS = 0x02
    EMBEDDING_DATA = 0x03
    PERFORMANCE_METRICS = 0x04
    HEARTBEAT = 0x05
    ERROR_REPORT = 0x06
    COMMAND_REQUEST = 0x07
    COMMAND_RESPONSE = 0x08
    DEBUG_INFO = 0x09

@dataclass
class FrameHeader:
    """4-byte frame header (CRC32 follows payload)"""
    sof: int                # Start of Frame (0xAA)
    payload_size: int       # Payload size in bytes (2 bytes, not including CRC32)
    header_checksum: int    # XOR checksum of SOF + payload_size (1 byte)

@dataclass
class MessageHeader:
    """3-byte message header within payload"""
    message_type: int       # Message type (1 byte)
    sequence_id: int        # Sequence ID for message ordering (2 bytes)

@dataclass
class ProtocolStats:
    """Protocol statistics"""
    packets_received: int = 0
    packets_valid: int = 0
    crc_errors: int = 0
    header_errors: int = 0
    bytes_received: int = 0

class EnhancedProtocolReceiver:
    """Enhanced protocol receiver with CRC32 validation"""
    
    def __init__(self, port: str, baudrate: int = 921600 * 8):
        self.serial = serial.Serial(port, baudrate, timeout=1.0)
        self.stats = ProtocolStats()
        self.sequence_counters: Dict[int, int] = {}
        
    def calculate_header_checksum(self, sof: int, payload_size: int) -> int:
        """Calculate XOR checksum for header validation"""
        header_data = bytes([
            sof,
            payload_size & 0xFF,         # Low byte
            (payload_size >> 8) & 0xFF   # High byte
        ])
        checksum = 0
        for byte in header_data:
            checksum ^= byte
        return checksum
    
    def validate_crc32(self, payload: bytes, expected_crc32: int) -> bool:
        """Validate payload CRC32"""
        calculated_crc32 = zlib.crc32(payload) & 0xFFFFFFFF
        return calculated_crc32 == expected_crc32
    
    def read_frame_header(self) -> Optional[FrameHeader]:
        """Read and validate 4-byte frame header"""
        try:
            # Read 4-byte header
            header_data = self.serial.read(ROBUST_HEADER_SIZE)
            if len(header_data) != ROBUST_HEADER_SIZE:
                return None
            
            # Unpack header: sof(1) + payload_size(2) + header_checksum(1)
            sof, payload_size, header_checksum = struct.unpack('<BHB', header_data)
            
            # Validate SOF byte
            if sof != ROBUST_SOF_BYTE:
                self.stats.header_errors += 1
                return None
            
            # Validate header checksum
            expected_checksum = self.calculate_header_checksum(sof, payload_size)
            if header_checksum != expected_checksum:
                print(f"Header checksum mismatch: expected {expected_checksum:02X}, got {header_checksum:02X}")
                self.stats.header_errors += 1
                return None
            
            # Validate payload size
            if payload_size > ROBUST_MAX_PAYLOAD_SIZE:
                self.stats.header_errors += 1
                return None
            
            return FrameHeader(sof, payload_size, header_checksum)
            
        except Exception as e:
            print(f"Error reading frame header: {e}")
            return None
    
    def read_message(self) -> Optional[Tuple[MessageHeader, bytes]]:
        """Read and validate complete message with CRC32"""
        # Read frame header
        frame_header = self.read_frame_header()
        if not frame_header:
            return None
        
        try:
            # Read payload + CRC32
            total_data_size = frame_header.payload_size + ROBUST_CRC_SIZE
            data = self.serial.read(total_data_size)
            if len(data) != total_data_size:
                print(f"Data size mismatch: expected {total_data_size}, got {len(data)}")
                return None
            
            # Split payload and CRC32
            payload = data[:frame_header.payload_size]
            crc32_bytes = data[frame_header.payload_size:]
            
            # Extract CRC32 (little endian)
            received_crc32, = struct.unpack('<I', crc32_bytes)
            
            # Validate payload CRC32
            if not self.validate_crc32(payload, received_crc32):
                print(f"CRC32 mismatch: expected {received_crc32:08X}, calculated {zlib.crc32(payload) & 0xFFFFFFFF:08X}")
                self.stats.crc_errors += 1
                return None
            
            # Parse message header (first 3 bytes of payload)
            if len(payload) < ROBUST_MSG_HEADER_SIZE:
                print("Payload too small for message header")
                return None
            
            message_type, sequence_id = struct.unpack('<BH', payload[:ROBUST_MSG_HEADER_SIZE])
            message_header = MessageHeader(message_type, sequence_id)
            
            # Extract message data (remaining payload)
            message_data = payload[ROBUST_MSG_HEADER_SIZE:]
            
            # Update statistics
            self.stats.packets_received += 1
            self.stats.packets_valid += 1
            self.stats.bytes_received += ROBUST_HEADER_SIZE + frame_header.payload_size + ROBUST_CRC_SIZE
            
            # Track sequence numbers
            if message_type not in self.sequence_counters:
                self.sequence_counters[message_type] = 0
            expected_seq = self.sequence_counters[message_type] + 1
            if sequence_id != expected_seq:
                print(f"Sequence mismatch for type {message_type}: expected {expected_seq}, got {sequence_id}")
            self.sequence_counters[message_type] = sequence_id
            
            return message_header, message_data
            
        except Exception as e:
            print(f"Error reading message: {e}")
            return None
    
    def parse_frame_data(self, data: bytes) -> Optional[Dict[str, Any]]:
        """Parse frame data message"""
        if len(data) < 12:  # 4 + 4 + 4 bytes minimum
            return None
        
        frame_type = data[:4].decode('utf-8', errors='ignore').rstrip('\x00')
        width, height = struct.unpack('<II', data[4:12])
        image_data = data[12:]
        
        return {
            'frame_type': frame_type,
            'width': width,
            'height': height,
            'image_data': image_data,
            'image_size': len(image_data)
        }
    
    def parse_heartbeat(self, data: bytes) -> Optional[Dict[str, Any]]:
        """Parse heartbeat message"""
        if len(data) < 4:
            return None
        
        timestamp, = struct.unpack('<I', data[:4])
        return {'timestamp': timestamp}
    
    def get_statistics(self) -> ProtocolStats:
        """Get protocol statistics"""
        return self.stats
    
    def close(self):
        """Close serial connection"""
        if self.serial and self.serial.is_open:
            self.serial.close()

def main():
    """Example usage"""
    try:
        # Initialize receiver
        receiver = EnhancedProtocolReceiver('/dev/ttyUSB0')  # Adjust port as needed
        print("Enhanced protocol receiver started with CRC32 validation")
        
        while True:
            message = receiver.read_message()
            if message:
                header, data = message
                
                if header.message_type == MessageType.FRAME_DATA:
                    frame_info = receiver.parse_frame_data(data)
                    if frame_info:
                        print(f"Frame: {frame_info['frame_type']} {frame_info['width']}x{frame_info['height']} ({frame_info['image_size']} bytes)")
                
                elif header.message_type == MessageType.HEARTBEAT:
                    heartbeat_info = receiver.parse_heartbeat(data)
                    if heartbeat_info:
                        print(f"Heartbeat: timestamp={heartbeat_info['timestamp']}")
                
                elif header.message_type == MessageType.DETECTION_RESULTS:
                    print(f"Detection results: {len(data)} bytes")
                
                elif header.message_type == MessageType.EMBEDDING_DATA:
                    print(f"Embedding data: {len(data)} bytes")
                
                else:
                    print(f"Unknown message type: {header.message_type}")
            
            # Print statistics every 100 packets
            if receiver.stats.packets_received % 100 == 0 and receiver.stats.packets_received > 0:
                stats = receiver.get_statistics()
                print(f"Stats: RX={stats.packets_received}, Valid={stats.packets_valid}, "
                      f"CRC_Errors={stats.crc_errors}, Header_Errors={stats.header_errors}")
    
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        receiver.close()

if __name__ == "__main__":
    main()
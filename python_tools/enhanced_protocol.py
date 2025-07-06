#!/usr/bin/env python3
"""
Enhanced Communication Protocol for STM32N6 Object Detection

Features:
- Robust packet-based communication with CRC checking
- JSON metadata for extensible information transfer
- Binary frame data with compression support
- Bidirectional command/response protocol
- Auto-sync and error recovery
- Performance metrics and diagnostics
"""

import struct
import zlib
import json
import time
import logging
from typing import Optional, Dict, Any, Tuple, List
from dataclasses import dataclass
from enum import IntEnum
import numpy as np
import cv2

logger = logging.getLogger(__name__)

class PacketType(IntEnum):
    """Packet type definitions"""
    FRAME_DATA = 0x01
    DETECTION_RESULTS = 0x02
    EMBEDDING_DATA = 0x03
    PERFORMANCE_METRICS = 0x04
    COMMAND_REQUEST = 0x05
    COMMAND_RESPONSE = 0x06
    HEARTBEAT = 0x07
    ERROR_REPORT = 0x08

class CommandType(IntEnum):
    """Command type definitions"""
    GET_STATUS = 0x01
    SET_PARAMETERS = 0x02
    START_ENROLLMENT = 0x03
    STOP_ENROLLMENT = 0x04
    RESET_SYSTEM = 0x05
    GET_DIAGNOSTICS = 0x06

@dataclass
class PacketHeader:
    """Enhanced packet header structure"""
    sync_word: int = 0x12345678  # 4 bytes
    packet_type: int = 0         # 1 byte
    flags: int = 0               # 1 byte (compression, encryption, etc.)
    sequence: int = 0            # 2 bytes
    payload_length: int = 0      # 4 bytes
    metadata_length: int = 0     # 2 bytes
    crc32: int = 0              # 4 bytes
    
    # Flags
    FLAG_COMPRESSED = 0x01
    FLAG_ENCRYPTED = 0x02
    FLAG_ACKNOWLEDGMENT = 0x04
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'PacketHeader':
        """Parse header from byte data"""
        if len(data) < 18:
            raise ValueError("Insufficient data for packet header")
        
        values = struct.unpack('<IBBHIHHI', data[:18])
        return cls(*values)
    
    def to_bytes(self) -> bytes:
        """Convert header to bytes"""
        return struct.pack('<IBBHIHHI', 
                          self.sync_word, self.packet_type, self.flags,
                          self.sequence, self.payload_length, 
                          self.metadata_length, self.crc32, 0)  # padding

class EnhancedProtocol:
    """Enhanced communication protocol handler"""
    
    def __init__(self, serial_port):
        self.serial = serial_port
        self.tx_sequence = 0
        self.rx_sequence = 0
        self.stats = {
            'packets_sent': 0,
            'packets_received': 0,
            'bytes_sent': 0,
            'bytes_received': 0,
            'crc_errors': 0,
            'sync_errors': 0,
            'timeouts': 0
        }
        
    def send_packet(self, packet_type: PacketType, payload: bytes = b'', 
                   metadata: Dict = None, compress: bool = False) -> bool:
        """Send a packet with optional metadata and compression"""
        try:
            # Prepare metadata
            metadata_bytes = b''
            if metadata:
                metadata_bytes = json.dumps(metadata).encode('utf-8')
            
            # Compress payload if requested
            flags = 0
            if compress and len(payload) > 100:
                payload = zlib.compress(payload)
                flags |= PacketHeader.FLAG_COMPRESSED
            
            # Create header
            header = PacketHeader(
                packet_type=packet_type,
                flags=flags,
                sequence=self.tx_sequence,
                payload_length=len(payload),
                metadata_length=len(metadata_bytes)
            )
            
            # Calculate CRC
            data_for_crc = payload + metadata_bytes
            header.crc32 = zlib.crc32(data_for_crc) & 0xffffffff
            
            # Send packet
            packet_data = header.to_bytes() + metadata_bytes + payload
            self.serial.write(packet_data)
            
            # Update stats
            self.tx_sequence = (self.tx_sequence + 1) & 0xFFFF
            self.stats['packets_sent'] += 1
            self.stats['bytes_sent'] += len(packet_data)
            
            return True
            
        except Exception as e:
            logger.error(f"Failed to send packet: {e}")
            return False
    
    def receive_packet(self, timeout: float = 1.0) -> Optional[Dict[str, Any]]:
        """Receive and parse a packet"""
        start_time = time.time()
        
        try:
            # Find sync word
            sync_buffer = b''
            while len(sync_buffer) < 4:
                if time.time() - start_time > timeout:
                    self.stats['timeouts'] += 1
                    return None
                
                byte = self.serial.read(1)
                if not byte:
                    continue
                
                sync_buffer = (sync_buffer + byte)[-4:]
                if struct.unpack('<I', sync_buffer)[0] == PacketHeader().sync_word:
                    break
            else:
                self.stats['sync_errors'] += 1
                return None
            
            # Read header
            header_data = sync_buffer + self.serial.read(14)  # Rest of header
            if len(header_data) != 18:
                return None
            
            header = PacketHeader.from_bytes(header_data)
            
            # Read metadata and payload
            metadata_bytes = self.serial.read(header.metadata_length)
            payload_bytes = self.serial.read(header.payload_length)
            
            # Verify CRC
            data_for_crc = payload_bytes + metadata_bytes
            calculated_crc = zlib.crc32(data_for_crc) & 0xffffffff
            if calculated_crc != header.crc32:
                self.stats['crc_errors'] += 1
                logger.warning("CRC mismatch in received packet")
                return None
            
            # Decompress if needed
            if header.flags & PacketHeader.FLAG_COMPRESSED:
                payload_bytes = zlib.decompress(payload_bytes)
            
            # Parse metadata
            metadata = {}
            if metadata_bytes:
                try:
                    metadata = json.loads(metadata_bytes.decode('utf-8'))
                except json.JSONDecodeError as e:
                    logger.warning(f"Failed to parse metadata: {e}")
            
            # Update stats
            self.stats['packets_received'] += 1
            self.stats['bytes_received'] += len(header_data) + len(metadata_bytes) + len(payload_bytes)
            
            return {
                'header': header,
                'metadata': metadata,
                'payload': payload_bytes,
                'timestamp': time.time()
            }
            
        except Exception as e:
            logger.error(f"Failed to receive packet: {e}")
            return None
    
    def send_command(self, command: CommandType, parameters: Dict = None) -> bool:
        """Send a command with optional parameters"""
        metadata = {
            'command': command,
            'parameters': parameters or {},
            'timestamp': time.time()
        }
        return self.send_packet(PacketType.COMMAND_REQUEST, metadata=metadata)
    
    def send_frame(self, frame: np.ndarray, detections: List = None, 
                  performance_data: Dict = None, compress: bool = True) -> bool:
        """Send frame data with detection results and performance metrics"""
        # Encode frame as JPEG
        _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
        frame_data = buffer.tobytes()
        
        # Prepare metadata
        metadata = {
            'width': frame.shape[1],
            'height': frame.shape[0],
            'channels': frame.shape[2] if len(frame.shape) > 2 else 1,
            'encoding': 'jpeg',
            'timestamp': time.time()
        }
        
        if detections is not None:
            metadata['detections'] = detections
        
        if performance_data is not None:
            metadata['performance'] = performance_data
        
        return self.send_packet(PacketType.FRAME_DATA, frame_data, metadata, compress)
    
    def get_stats(self) -> Dict[str, Any]:
        """Get protocol statistics"""
        return self.stats.copy()

# Protocol instance for global use
_protocol_instance = None

def initialize_protocol(serial_port) -> EnhancedProtocol:
    """Initialize the global protocol instance"""
    global _protocol_instance
    _protocol_instance = EnhancedProtocol(serial_port)
    return _protocol_instance

def read_enhanced_frame(serial_port, timeout: float = 1.0) -> Optional[Dict[str, Any]]:
    """Read an enhanced frame packet"""
    if not _protocol_instance:
        initialize_protocol(serial_port)
    
    packet = _protocol_instance.receive_packet(timeout)
    if not packet or packet['header'].packet_type != PacketType.FRAME_DATA:
        return None
    
    try:
        # Decode frame data
        frame_buffer = np.frombuffer(packet['payload'], dtype=np.uint8)
        frame = cv2.imdecode(frame_buffer, cv2.IMREAD_COLOR)
        
        if frame is None:
            logger.warning("Failed to decode frame data")
            return None
        
        return {
            'image': frame,
            'metadata': packet['metadata'],
            'timestamp': packet['timestamp']
        }
        
    except Exception as e:
        logger.error(f"Failed to process frame packet: {e}")
        return None

def send_command(serial_port, command: CommandType, parameters: Dict = None) -> bool:
    """Send a command to the device"""
    if not _protocol_instance:
        initialize_protocol(serial_port)
    
    return _protocol_instance.send_command(command, parameters)

def get_protocol_stats(serial_port) -> Dict[str, Any]:
    """Get protocol statistics"""
    if not _protocol_instance:
        initialize_protocol(serial_port)
    
    return _protocol_instance.get_stats()

# Legacy compatibility functions
def read_frame(serial_port):
    """Legacy frame reading function for backward compatibility"""
    try:
        # Try enhanced protocol first
        frame_data = read_enhanced_frame(serial_port, timeout=0.1)
        if frame_data:
            return "JPG", frame_data['image'], \
                   frame_data['metadata'].get('width', 0), \
                   frame_data['metadata'].get('height', 0)
    except:
        pass
    
    # Fallback to legacy protocol
    import pc_uart_utils as legacy
    return legacy.read_frame(serial_port)

def read_detections(serial_port):
    """Legacy detection reading function for backward compatibility"""
    if not _protocol_instance:
        return None, []
    
    packet = _protocol_instance.receive_packet(timeout=0.1)
    if packet and packet['header'].packet_type == PacketType.DETECTION_RESULTS:
        detections = packet['metadata'].get('detections', [])
        frame_id = packet['metadata'].get('frame_id', 0)
        return frame_id, detections
    
    # Fallback to legacy protocol
    import pc_uart_utils as legacy
    return legacy.read_detections(serial_port)

def read_embedding(serial_port):
    """Legacy embedding reading function for backward compatibility"""
    if not _protocol_instance:
        return []
    
    packet = _protocol_instance.receive_packet(timeout=0.1)
    if packet and packet['header'].packet_type == PacketType.EMBEDDING_DATA:
        return packet['metadata'].get('embedding', [])
    
    # Fallback to legacy protocol
    import pc_uart_utils as legacy
    return legacy.read_embedding(serial_port)
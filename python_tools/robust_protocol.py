#!/usr/bin/env python3
"""
Robust Binary Protocol for STM32N6 Object Detection
Implements reliable message framing with checksums and buffering
"""

import struct
import time
import logging
import threading
import queue
from collections import deque
from enum import IntEnum
from typing import Optional, Tuple, List, Dict, Any, Callable
import cv2
import numpy as np

logger = logging.getLogger(__name__)

class MessageType(IntEnum):
    """Message types for the protocol"""
    FRAME_DATA = 0x01
    DETECTION_RESULTS = 0x02  
    EMBEDDING_DATA = 0x03
    PERFORMANCE_METRICS = 0x04
    HEARTBEAT = 0x05
    ERROR_REPORT = 0x06
    COMMAND_REQUEST = 0x07
    COMMAND_RESPONSE = 0x08
    DEBUG_INFO = 0x09

class ProtocolConstants:
    """Protocol constants and configuration"""
    SOF_BYTE = 0xAA                    # Start of Frame marker
    HEADER_SIZE = 4                    # SOF(1) + PayloadSize(2) + Checksum(1)
    MAX_PAYLOAD_SIZE = 64 * 1024       # 64KB max payload
    BUFFER_SIZE = 256 * 1024           # 256KB circular buffer
    SYNC_TIMEOUT = 5.0                 # 5 seconds to find sync
    MESSAGE_TIMEOUT = 2.0              # 2 seconds per message
    
    # Message header format within payload
    MSG_HEADER_FORMAT = '<BH'          # MessageType(1) + SequenceId(2)
    MSG_HEADER_SIZE = 3

def calculate_checksum(data: bytes) -> int:
    """Calculate simple XOR checksum"""
    checksum = 0
    for byte in data:
        checksum ^= byte
    return checksum & 0xFF

class ProtocolMessage:
    """Represents a parsed protocol message"""
    
    def __init__(self, msg_type: MessageType, sequence_id: int, payload: bytes, timestamp: float = None):
        self.msg_type = msg_type
        self.sequence_id = sequence_id
        self.payload = payload
        self.timestamp = timestamp or time.time()
        
    def __repr__(self):
        return f"ProtocolMessage(type={self.msg_type.name}, seq={self.sequence_id}, size={len(self.payload)})"

class CircularBuffer:
    """Thread-safe circular buffer for incoming bytes"""
    
    def __init__(self, size: int):
        self.buffer = bytearray(size)
        self.size = size
        self.write_pos = 0
        self.read_pos = 0
        self.data_available = 0
        self.lock = threading.Lock()
        
    def write(self, data: bytes) -> int:
        """Write data to buffer, returns bytes written"""
        if not data:
            return 0
            
        with self.lock:
            written = 0
            for byte in data:
                if self.data_available < self.size:
                    self.buffer[self.write_pos] = byte
                    self.write_pos = (self.write_pos + 1) % self.size
                    self.data_available += 1
                    written += 1
                else:
                    # Buffer full, drop oldest data
                    self.read_pos = (self.read_pos + 1) % self.size
                    self.buffer[self.write_pos] = byte
                    self.write_pos = (self.write_pos + 1) % self.size
                    written += 1
                    
            return written
    
    def peek(self, length: int) -> Optional[bytes]:
        """Peek at data without consuming it"""
        with self.lock:
            if self.data_available < length:
                return None
                
            result = bytearray()
            pos = self.read_pos
            
            for _ in range(length):
                result.append(self.buffer[pos])
                pos = (pos + 1) % self.size
                
            return bytes(result)
    
    def consume(self, length: int) -> Optional[bytes]:
        """Read and consume data from buffer"""
        with self.lock:
            if self.data_available < length:
                return None
                
            result = bytearray()
            
            for _ in range(length):
                result.append(self.buffer[self.read_pos])
                self.read_pos = (self.read_pos + 1) % self.size
                self.data_available -= 1
                
            return bytes(result)
    
    def available(self) -> int:
        """Get number of bytes available to read"""
        with self.lock:
            return self.data_available
    
    def clear(self):
        """Clear all data from buffer"""
        with self.lock:
            self.read_pos = 0
            self.write_pos = 0
            self.data_available = 0

class RobustProtocolParser:
    """Robust protocol parser with message framing and error recovery"""
    
    def __init__(self, buffer_size: int = ProtocolConstants.BUFFER_SIZE):
        self.buffer = CircularBuffer(buffer_size)
        self.message_handlers: Dict[MessageType, Callable] = {}
        self.stats = {
            'messages_received': 0,
            'bytes_received': 0,
            'sync_errors': 0,
            'checksum_errors': 0,
            'parse_errors': 0,
            'messages_dropped': 0
        }
        self.running = False
        self.last_sequence_id = {}  # Track sequence per message type
        
        # Register default message handlers
        self.register_handler(MessageType.FRAME_DATA, self._handle_frame_data)
        self.register_handler(MessageType.DETECTION_RESULTS, self._handle_detection_results)
        self.register_handler(MessageType.EMBEDDING_DATA, self._handle_embedding_data)
        self.register_handler(MessageType.PERFORMANCE_METRICS, self._handle_performance_metrics)
        self.register_handler(MessageType.HEARTBEAT, self._handle_heartbeat)
        
    def register_handler(self, msg_type: MessageType, handler: Callable[[ProtocolMessage], None]):
        """Register a handler for a specific message type"""
        self.message_handlers[msg_type] = handler
        
    def add_data(self, data: bytes) -> int:
        """Add incoming serial data to buffer"""
        if data:
            written = self.buffer.write(data)
            self.stats['bytes_received'] += len(data)
            return written
        return 0
    
    def find_sync(self) -> bool:
        """Find SOF byte in buffer and align to frame boundary"""
        max_search = min(1024, self.buffer.available())  # Search up to 1KB
        
        for i in range(max_search):
            byte_data = self.buffer.peek(1)
            if not byte_data:
                return False
                
            if byte_data[0] == ProtocolConstants.SOF_BYTE:
                return True
            else:
                # Consume invalid byte
                self.buffer.consume(1)
                self.stats['sync_errors'] += 1
                
        return False
    
    def parse_header(self) -> Optional[Tuple[int, int]]:
        """Parse frame header, returns (payload_size, checksum) or None"""
        header_data = self.buffer.peek(ProtocolConstants.HEADER_SIZE)
        if not header_data:
            return None
            
        if header_data[0] != ProtocolConstants.SOF_BYTE:
            return None
            
        # Unpack: SOF(1) + PayloadSize(2) + HeaderChecksum(1)
        sof, payload_size, header_checksum = struct.unpack('<BHB', header_data)
        
        # Verify header checksum (checksum of SOF + PayloadSize)
        calculated_checksum = calculate_checksum(header_data[:3])
        if calculated_checksum != header_checksum:
            logger.debug(f"Header checksum mismatch: {calculated_checksum:02X} != {header_checksum:02X}")
            self.stats['checksum_errors'] += 1
            return None
            
        # Validate payload size
        if payload_size > ProtocolConstants.MAX_PAYLOAD_SIZE:
            logger.warning(f"Invalid payload size: {payload_size}")
            self.stats['parse_errors'] += 1
            return None
            
        return payload_size, header_checksum
    
    def parse_message(self) -> Optional[ProtocolMessage]:
        """Parse a complete message from buffer"""
        # Find frame sync
        if not self.find_sync():
            return None
            
        # Parse header
        header_info = self.parse_header()
        if not header_info:
            # Invalid header, consume SOF and try again
            self.buffer.consume(1)
            return None
            
        payload_size, header_checksum = header_info
        
        # Check if complete message is available
        total_size = ProtocolConstants.HEADER_SIZE + payload_size
        if self.buffer.available() < total_size:
            return None  # Wait for more data
            
        # Consume header
        header_data = self.buffer.consume(ProtocolConstants.HEADER_SIZE)
        if not header_data:
            return None
            
        # Read payload
        payload_data = self.buffer.consume(payload_size)
        if not payload_data:
            logger.error("Failed to read payload after header")
            self.stats['parse_errors'] += 1
            return None
            
        # Parse message header within payload
        if len(payload_data) < ProtocolConstants.MSG_HEADER_SIZE:
            logger.warning(f"Payload too small for message header: {len(payload_data)}")
            self.stats['parse_errors'] += 1
            return None
            
        try:
            msg_type_int, sequence_id = struct.unpack(
                ProtocolConstants.MSG_HEADER_FORMAT, 
                payload_data[:ProtocolConstants.MSG_HEADER_SIZE]
            )
            
            # Validate message type
            try:
                msg_type = MessageType(msg_type_int)
            except ValueError:
                logger.warning(f"Unknown message type: {msg_type_int}")
                self.stats['parse_errors'] += 1
                return None
                
            # Extract message payload (after message header)
            message_payload = payload_data[ProtocolConstants.MSG_HEADER_SIZE:]
            
            # Create message object
            message = ProtocolMessage(msg_type, sequence_id, message_payload)
            
            # Check for dropped messages (simple sequence check)
            last_seq = self.last_sequence_id.get(msg_type, sequence_id - 1)
            if sequence_id != (last_seq + 1) % 65536:  # 16-bit sequence wraparound
                dropped = (sequence_id - last_seq - 1) % 65536
                if dropped > 0 and dropped < 1000:  # Reasonable drop count
                    self.stats['messages_dropped'] += dropped
                    logger.debug(f"Dropped {dropped} messages of type {msg_type.name}")
                    
            self.last_sequence_id[msg_type] = sequence_id
            self.stats['messages_received'] += 1
            
            return message
            
        except struct.error as e:
            logger.warning(f"Error parsing message header: {e}")
            self.stats['parse_errors'] += 1
            return None
    
    def process_messages(self, max_messages: int = 10) -> int:
        """Process available messages, returns number processed"""
        processed = 0
        
        for _ in range(max_messages):
            message = self.parse_message()
            if not message:
                break
                
            # Dispatch to handler
            handler = self.message_handlers.get(message.msg_type)
            if handler:
                try:
                    handler(message)
                    processed += 1
                except Exception as e:
                    logger.error(f"Error handling message {message.msg_type.name}: {e}")
            else:
                logger.debug(f"No handler for message type: {message.msg_type.name}")
                
        return processed
    
    def get_stats(self) -> Dict[str, Any]:
        """Get protocol statistics"""
        return self.stats.copy()
    
    def clear_stats(self):
        """Clear statistics counters"""
        for key in self.stats:
            self.stats[key] = 0
        self.last_sequence_id.clear()
    
    # Default message handlers (can be overridden)
    def _handle_frame_data(self, message: ProtocolMessage):
        """Default frame data handler"""
        logger.debug(f"Frame data received: {len(message.payload)} bytes")
        
    def _handle_detection_results(self, message: ProtocolMessage):
        """Default detection results handler"""
        logger.debug(f"Detection results received: {len(message.payload)} bytes")
        
    def _handle_embedding_data(self, message: ProtocolMessage):
        """Default embedding data handler"""
        logger.debug(f"Embedding data received: {len(message.payload)} bytes")
        
    def _handle_performance_metrics(self, message: ProtocolMessage):
        """Default performance metrics handler"""
        logger.debug(f"Performance metrics received: {len(message.payload)} bytes")
        
    def _handle_heartbeat(self, message: ProtocolMessage):
        """Default heartbeat handler"""
        logger.debug("Heartbeat received")

class FrameDataParser:
    """Parser for frame data messages"""
    
    @staticmethod
    def parse_frame(payload: bytes) -> Optional[Tuple[str, np.ndarray, int, int]]:
        """Parse frame data payload"""
        try:
            # Frame format: FrameType(4) + Width(4) + Height(4) + ImageData(...)
            if len(payload) < 12:
                return None
                
            frame_type, width, height = struct.unpack('<4sII', payload[:12])
            frame_type = frame_type.decode('ascii').rstrip('\x00')
            
            image_data = payload[12:]
            
            # Decode JPEG data
            if image_data:
                img_array = np.frombuffer(image_data, dtype=np.uint8)
                frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                
                if frame is not None:
                    return frame_type, frame, width, height
                    
        except Exception as e:
            logger.error(f"Error parsing frame data: {e}")
            
        return None

class DetectionDataParser:
    """Parser for detection results messages"""
    
    @staticmethod
    def parse_detections(payload: bytes) -> Optional[Tuple[int, List[Tuple]]]:
        """Parse detection results payload"""
        try:
            # Detection format: FrameId(4) + DetectionCount(4) + Detections(...)
            if len(payload) < 8:
                return None
                
            frame_id, detection_count = struct.unpack('<II', payload[:8])
            
            if detection_count > 100:  # Sanity check
                return None
                
            detections = []
            offset = 8
            
            # Each detection: Class(4) + X(4) + Y(4) + W(4) + H(4) + Confidence(4) + KeypointCount(4) + Keypoints(...)
            for _ in range(detection_count):
                if offset + 28 > len(payload):  # Minimum detection size
                    break
                    
                class_id, x, y, w, h, confidence, kp_count = struct.unpack('<IfffffI', payload[offset:offset+28])
                offset += 28
                
                # Read keypoints
                keypoints = []
                kp_size = kp_count * 8  # 2 floats per keypoint
                if offset + kp_size <= len(payload):
                    kp_data = struct.unpack(f'<{kp_count * 2}f', payload[offset:offset+kp_size])
                    keypoints = list(kp_data)
                    offset += kp_size
                
                detections.append((class_id, x, y, w, h, confidence, keypoints))
                
            return frame_id, detections
            
        except Exception as e:
            logger.error(f"Error parsing detection data: {e}")
            
        return None

class EmbeddingDataParser:
    """Parser for embedding data messages"""
    
    @staticmethod
    def parse_embedding(payload: bytes) -> Optional[List[float]]:
        """Parse embedding data payload"""
        try:
            # Embedding format: EmbeddingSize(4) + EmbeddingData(...)
            if len(payload) < 4:
                return None
                
            embedding_size = struct.unpack('<I', payload[:4])[0]
            
            if embedding_size > 1024:  # Sanity check
                return None
                
            expected_bytes = embedding_size * 4  # 4 bytes per float
            if len(payload) < 4 + expected_bytes:
                return None
                
            embedding_data = struct.unpack(f'<{embedding_size}f', payload[4:4+expected_bytes])
            return list(embedding_data)
            
        except Exception as e:
            logger.error(f"Error parsing embedding data: {e}")
            
        return None

# Test function
def test_protocol():
    """Test the robust protocol implementation"""
    parser = RobustProtocolParser()
    
    # Test message creation
    def create_test_message(msg_type: MessageType, payload: bytes, sequence_id: int = 0) -> bytes:
        """Create a test message with proper framing"""
        # Create message header
        msg_header = struct.pack(ProtocolConstants.MSG_HEADER_FORMAT, msg_type.value, sequence_id)
        full_payload = msg_header + payload
        
        # Create frame header
        payload_size = len(full_payload)
        header_data = struct.pack('<BH', ProtocolConstants.SOF_BYTE, payload_size)
        header_checksum = calculate_checksum(header_data)
        
        # Complete frame
        frame = struct.pack('<BHB', ProtocolConstants.SOF_BYTE, payload_size, header_checksum) + full_payload
        return frame
    
    # Test data
    test_payload = b"Hello, World!"
    test_frame = create_test_message(MessageType.DEBUG_INFO, test_payload, 123)
    
    # Add data to parser
    parser.add_data(test_frame)
    
    # Process messages
    processed = parser.process_messages()
    print(f"Processed {processed} messages")
    print(f"Stats: {parser.get_stats()}")

if __name__ == "__main__":
    # Run test
    logging.basicConfig(level=logging.DEBUG)
    test_protocol()
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
import zlib
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
    CRC_SIZE = 4                       # CRC32 at end of packet
    MAX_PAYLOAD_SIZE = 64 * 1024       # 64KB max payload
    BUFFER_SIZE = 256 * 1024           # 256KB circular buffer
    SYNC_TIMEOUT = 5.0                 # 5 seconds to find sync
    MESSAGE_TIMEOUT = 2.0              # 2 seconds per message
    
    # Message header format within payload
    MSG_HEADER_FORMAT = '<BH'          # MessageType(1) + SequenceId(2)
    MSG_HEADER_SIZE = 3

def calculate_checksum(data: bytes) -> int:
    """Calculate simple XOR checksum for header validation"""
    checksum = 0
    for byte in data:
        checksum ^= byte
    return checksum & 0xFF


class Crc32:
    crc_table = {}

    def __init__(self, _poly):
        # Generate CRC table for polynomial
        for i in range(256):
            c = i << 24
            for j in range(8):
                c = (c << 1) ^ _poly if (c & 0x80000000) else c << 1
            self.crc_table[i] = c & 0xFFFFFFFF

    # Calculate CRC from input buffer
    def calculate(self, buf):
        crc = 0xFFFFFFFF

        i = 0
        while i < len(buf):
            b = [buf[i + 3], buf[i + 2], buf[i + 1], buf[i + 0]]
            i += 4
            for byte in b:
                crc = ((crc << 8) & 0xFFFFFFFF) ^ self.crc_table[(crc >> 24) ^ byte]
        return crc

    # Create bytes array from integer input
    def crc_int_to_bytes(self, i):
        return [(i >> 24) & 0xFF, (i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF]


# Global CRC32 instance
_stm32_crc = Crc32(0x04C11DB7)

def calculate_stm32_crc32(data: bytes) -> int:
    """Calculate CRC32 matching STM32 hardware CRC peripheral
    
    STM32 CRC32 configuration:
    - Polynomial: 0x04C11DB7 (IEEE 802.3)
    - Initial value: 0xFFFFFFFF  
    - Input reflection: None
    - Output reflection: None
    - Output XOR: None
    - Processes data in 4-byte chunks with STM32 word-based ordering
    """
    return _stm32_crc.calculate(data)

def validate_crc32(payload: bytes, expected_crc32: int) -> bool:
    """Validate payload CRC32 using STM32-compatible algorithm"""
    calculated_crc32 = calculate_stm32_crc32(payload)
    return calculated_crc32 == expected_crc32

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
        """Write data to buffer, returns bytes written (optimized for bulk operations)"""
        if not data:
            return 0
            
        with self.lock:
            data_len = len(data)
            
            # Check if we can write all data without dropping
            if self.data_available + data_len <= self.size:
                # Fast path: bulk write using slicing when possible
                if self.write_pos + data_len <= self.size:
                    # Single contiguous write
                    self.buffer[self.write_pos:self.write_pos + data_len] = data
                    self.write_pos = (self.write_pos + data_len) % self.size
                else:
                    # Split write at buffer boundary
                    first_chunk = self.size - self.write_pos
                    self.buffer[self.write_pos:] = data[:first_chunk]
                    self.buffer[:data_len - first_chunk] = data[first_chunk:]
                    self.write_pos = data_len - first_chunk
                
                self.data_available += data_len
                return data_len
            else:
                # Need to drop some old data - try to drop complete messages
                space_needed = data_len
                space_available = self.size - self.data_available
                
                if space_needed > space_available:
                    # Drop old data in chunks for efficiency
                    bytes_to_drop = space_needed - space_available
                    bytes_to_drop = min(bytes_to_drop + 1024, self.data_available)  # Drop 1KB extra
                    
                    self.read_pos = (self.read_pos + bytes_to_drop) % self.size
                    self.data_available -= bytes_to_drop
                
                # Now write the new data using fast path
                if self.write_pos + data_len <= self.size:
                    self.buffer[self.write_pos:self.write_pos + data_len] = data
                    self.write_pos = (self.write_pos + data_len) % self.size
                else:
                    first_chunk = self.size - self.write_pos
                    self.buffer[self.write_pos:] = data[:first_chunk]
                    remaining = data_len - first_chunk
                    if remaining <= self.size - self.data_available:
                        self.buffer[:remaining] = data[first_chunk:]
                        self.write_pos = remaining
                    else:
                        # Truncate if still not enough space
                        remaining = self.size - self.data_available
                        self.buffer[:remaining] = data[first_chunk:first_chunk + remaining]
                        self.write_pos = remaining
                        data_len = first_chunk + remaining
                
                self.data_available = min(self.data_available + data_len, self.size)
                return data_len
    
    def peek(self, length: int) -> Optional[bytes]:
        """Peek at data without consuming it (optimized for bulk operations)"""
        with self.lock:
            if self.data_available < length:
                return None
            
            # Fast path: bulk read using slicing when possible
            if self.read_pos + length <= self.size:
                # Single contiguous read
                return bytes(self.buffer[self.read_pos:self.read_pos + length])
            else:
                # Split read at buffer boundary
                first_chunk = self.size - self.read_pos
                result = bytearray()
                result.extend(self.buffer[self.read_pos:])
                result.extend(self.buffer[:length - first_chunk])
                return bytes(result)
    
    def consume(self, length: int) -> Optional[bytes]:
        """Read and consume data from buffer (optimized for bulk operations)"""
        with self.lock:
            if self.data_available < length:
                return None
            
            # Fast path: bulk read using slicing
            if self.read_pos + length <= self.size:
                # Single contiguous read
                result = bytes(self.buffer[self.read_pos:self.read_pos + length])
                self.read_pos = (self.read_pos + length) % self.size
            else:
                # Split read at buffer boundary
                first_chunk = self.size - self.read_pos
                result = bytearray()
                result.extend(self.buffer[self.read_pos:])
                result.extend(self.buffer[:length - first_chunk])
                result = bytes(result)
                self.read_pos = length - first_chunk
            
            self.data_available -= length
            return result
    
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
            'crc_errors': 0,
            'parse_errors': 0,
            'messages_dropped': 0,
            'parse_time_ms': 0,
            'decode_time_ms': 0,
            'throughput_mbps': 0.0,
            'last_throughput_time': time.time()
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
        """Add incoming serial data to buffer with throughput monitoring"""
        if data:
            written = self.buffer.write(data)
            self.stats['bytes_received'] += len(data)
            
            # Update throughput statistics
            current_time = time.time()
            time_diff = current_time - self.stats['last_throughput_time']
            if time_diff >= 1.0:  # Update every second
                bytes_per_sec = self.stats['bytes_received'] / time_diff
                self.stats['throughput_mbps'] = (bytes_per_sec * 8) / (1024 * 1024)  # Convert to Mbps
                self.stats['last_throughput_time'] = current_time
            
            return written
        return 0
    
    def find_sync(self) -> bool:
        """Find SOF byte in buffer and align to frame boundary"""
        max_search = min(4096, self.buffer.available())  # Increased search window to 4KB
        sync_errors_before = self.stats['sync_errors']
        
        for i in range(max_search):
            byte_data = self.buffer.peek(1)
            if not byte_data:
                return False
                
            if byte_data[0] == ProtocolConstants.SOF_BYTE:
                # Found potential SOF, verify it's a valid header before declaring success
                if self.buffer.available() >= ProtocolConstants.HEADER_SIZE:
                    header_data = self.buffer.peek(ProtocolConstants.HEADER_SIZE)
                    if header_data and self._validate_header_quickly(header_data):
                        return True
                else:
                    # Not enough data for full header, but SOF found
                    return True
                    
            # Consume invalid byte, but only count as sync error if we skipped significant data
            self.buffer.consume(1)
            
        # Only count sync errors if we actually skipped a significant amount of data
        bytes_skipped = self.stats['sync_errors'] - sync_errors_before + (max_search if max_search > 0 else 0)
        if bytes_skipped > 10:  # Only count as error if we skipped more than 10 bytes
            self.stats['sync_errors'] += 1
            
        return False
    
    def _validate_header_quickly(self, header_data: bytes) -> bool:
        """Quick validation to check if header looks valid"""
        if len(header_data) < 4:
            return False
            
        try:
            sof, payload_size, header_checksum = struct.unpack('<BHB', header_data)
            
            # Basic sanity checks
            if sof != ProtocolConstants.SOF_BYTE:
                return False
            if payload_size > ProtocolConstants.MAX_PAYLOAD_SIZE or payload_size == 0:
                return False
                
            # Quick checksum validation
            calculated_checksum = calculate_checksum(header_data[:3])
            return calculated_checksum == header_checksum
            
        except struct.error:
            return False
    
    def parse_header(self) -> Optional[Tuple[int, int]]:
        """Parse frame header, returns (payload_size, checksum) or None"""
        header_data = self.buffer.peek(ProtocolConstants.HEADER_SIZE)
        if not header_data:
            return None
            
        if header_data[0] != ProtocolConstants.SOF_BYTE:
            return None
            
        try:
            # Unpack: SOF(1) + PayloadSize(2) + HeaderChecksum(1)
            sof, payload_size, header_checksum = struct.unpack('<BHB', header_data)
            
            # Validate payload size first (quick check)
            if payload_size == 0 or payload_size > ProtocolConstants.MAX_PAYLOAD_SIZE:
                logger.debug(f"Invalid payload size: {payload_size}")
                self.stats['parse_errors'] += 1
                return None
            
            # Verify header checksum (checksum of SOF + PayloadSize)
            calculated_checksum = calculate_checksum(header_data[:3])
            if calculated_checksum != header_checksum:
                # Log more details for debugging
                logger.debug(f"Header checksum mismatch: calc={calculated_checksum:02X} recv={header_checksum:02X}")
                logger.debug(f"Header bytes: {' '.join(f'{b:02X}' for b in header_data)}")
                self.stats['checksum_errors'] += 1
                return None
                
            return payload_size, header_checksum
            
        except struct.error as e:
            logger.debug(f"Struct unpack error in header: {e}")
            self.stats['parse_errors'] += 1
            return None
    
    def parse_message(self) -> Optional[ProtocolMessage]:
        """Parse a complete message from buffer"""
        max_attempts = 3  # Limit attempts to avoid infinite loops
        
        for attempt in range(max_attempts):
            # Find frame sync
            if not self.find_sync():
                return None
                
            # Parse header
            header_info = self.parse_header()
            if not header_info:
                # Invalid header, consume SOF and try again
                self.buffer.consume(1)
                if attempt == max_attempts - 1:
                    return None
                continue
                
            payload_size, header_checksum = header_info
            
            # Check if complete message is available (payload + CRC32)
            total_size = ProtocolConstants.HEADER_SIZE + payload_size + ProtocolConstants.CRC_SIZE
            if self.buffer.available() < total_size:
                return None  # Wait for more data
                
            # Consume header
            header_data = self.buffer.consume(ProtocolConstants.HEADER_SIZE)
            if not header_data:
                if attempt == max_attempts - 1:
                    return None
                continue
                
            # Read payload + CRC32
            payload_and_crc = self.buffer.consume(payload_size + ProtocolConstants.CRC_SIZE)
            if not payload_and_crc:
                logger.error("Failed to read payload and CRC32 after header")
                self.stats['parse_errors'] += 1
                if attempt == max_attempts - 1:
                    return None
                continue
                
            # Split payload and CRC32
            payload_data = payload_and_crc[:payload_size]
            crc32_bytes = payload_and_crc[payload_size:]
            
            # Extract and validate CRC32 (calculated only on payload data, not message header)
            try:
                received_crc32, = struct.unpack('<I', crc32_bytes)
                # CRC32 is calculated only on payload data (after message header)
                actual_payload = payload_data[ProtocolConstants.MSG_HEADER_SIZE:]
                if not validate_crc32(actual_payload, received_crc32):
                    calculated_crc32 = calculate_stm32_crc32(actual_payload)
                    logger.debug(f"CRC32 mismatch: expected {received_crc32:08X}, calculated {calculated_crc32:08X}")
                    self.stats['crc_errors'] += 1
                    if attempt == max_attempts - 1:
                        return None
                    continue
            except struct.error:
                logger.warning("Failed to unpack CRC32")
                self.stats['parse_errors'] += 1
                if attempt == max_attempts - 1:
                    return None
                continue
                
            # Successfully read complete message, break out of retry loop
            break
        else:
            # All attempts failed
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
    
    def process_messages(self, max_messages: int = 50) -> int:
        """Process available messages, returns number processed (increased throughput)"""
        processed = 0
        consecutive_failures = 0
        
        for _ in range(max_messages):
            message = self.parse_message()
            if not message:
                consecutive_failures += 1
                if consecutive_failures > 5:
                    break  # Avoid spinning on bad data
                continue
                
            consecutive_failures = 0  # Reset on successful parse
            
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
            
            # All frames are now raw grayscale data
            if len(image_data) == width * height:
                frame = np.frombuffer(image_data, dtype=np.uint8).reshape((height, width))
                # Convert to 3-channel for consistency
                frame = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
                return frame_type, frame, width, height
            else:
                logger.warning(f"Raw data size mismatch: expected {width * height}, got {len(image_data)}")
                return None
                    
        except Exception as e:
            logger.error(f"Error parsing frame data: {e}")
            
        return None
    
    @staticmethod
    def parse_frame_fast(payload: bytes) -> Optional[Tuple[str, np.ndarray, int, int]]:

        """Optimized frame parsing with reduced memory allocations"""
        try:
            # Frame format: FrameType(4) + Width(4) + Height(4) + ImageData(...)
            if len(payload) < 12:
                return None
                
            # Fast header parsing without intermediate objects
            frame_type = payload[:4].decode('ascii').rstrip('\x00')
            width = struct.unpack('<I', payload[4:8])[0]
            height = struct.unpack('<I', payload[8:12])[0]

            # Direct slice for image data (no copy)
            image_data = payload[12:]
            
            # All frames are now raw grayscale data
            if len(image_data) == width * height:
                frame = np.frombuffer(image_data, dtype=np.uint8).reshape((height, width))
                # Convert to 3-channel for consistency
                frame = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
                return frame_type, frame, width, height
            else:
                logger.warning(f"Raw data size mismatch: expected {width * height}, got {len(image_data)}")
                return None
                    
        except Exception as e:
            logger.error(f"Error parsing frame data (fast): {e}")
            # Fallback to regular parsing
            return FrameDataParser.parse_frame(payload)
            
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
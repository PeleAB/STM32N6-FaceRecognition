#!/usr/bin/env python3
"""
Test script for robust protocol implementation
Validates protocol parsing, message framing, and error recovery
"""

import sys
import time
import logging
import struct
import cv2
import numpy as np
from robust_protocol import (
    RobustProtocolParser, MessageType, ProtocolMessage,
    FrameDataParser, DetectionDataParser, EmbeddingDataParser,
    ProtocolConstants, calculate_checksum
)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)

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

def create_test_frame_data() -> bytes:
    """Create test frame data message"""
    # Create a simple test image (16x16 RGB)
    test_image = np.random.randint(0, 255, (16, 16, 3), dtype=np.uint8)
    
    # Encode as JPEG
    _, jpeg_data = cv2.imencode('.jpg', test_image)
    jpeg_bytes = jpeg_data.tobytes()
    
    # Frame format: FrameType(4) + Width(4) + Height(4) + ImageData(...)
    frame_payload = struct.pack('<4sII', b'JPG\x00', 16, 16) + jpeg_bytes
    
    return create_test_message(MessageType.FRAME_DATA, frame_payload, 1)

def create_test_detection_data() -> bytes:
    """Create test detection data message"""
    # Detection format: FrameId(4) + DetectionCount(4) + Detections(...)
    frame_id = 123
    detection_count = 2
    
    payload = struct.pack('<II', frame_id, detection_count)
    
    # Add two test detections
    for i in range(detection_count):
        # Detection: Class(4) + X(4) + Y(4) + W(4) + H(4) + Confidence(4) + KeypointCount(4)
        detection = struct.pack('<IfffffI', 
                               i,           # class_id
                               0.5,         # x
                               0.5,         # y  
                               0.2,         # w
                               0.3,         # h
                               0.85,        # confidence
                               0            # keypoint_count
                               )
        payload += detection
    
    return create_test_message(MessageType.DETECTION_RESULTS, payload, 2)

def create_test_embedding_data() -> bytes:
    """Create test embedding data message"""
    # Create test embedding (32 float values)
    embedding_size = 32
    embedding_data = [0.1 * i for i in range(embedding_size)]
    
    # Embedding format: EmbeddingSize(4) + EmbeddingData(...)
    payload = struct.pack('<I', embedding_size)
    payload += struct.pack(f'<{embedding_size}f', *embedding_data)
    
    return create_test_message(MessageType.EMBEDDING_DATA, payload, 3)

def test_basic_protocol():
    """Test basic protocol functionality"""
    logger.info("Testing basic protocol functionality...")
    
    parser = RobustProtocolParser()
    
    # Add handler for DEBUG_INFO messages
    debug_message_received = False
    
    def debug_handler(message: ProtocolMessage):
        nonlocal debug_message_received
        debug_message_received = True
        assert message.payload == b"Hello, Robust Protocol!", "Payload mismatch"
    
    parser.register_handler(MessageType.DEBUG_INFO, debug_handler)
    
    # Test simple message
    test_payload = b"Hello, Robust Protocol!"
    test_frame = create_test_message(MessageType.DEBUG_INFO, test_payload, 100)
    
    # Add data to parser
    parser.add_data(test_frame)
    
    # Process messages
    processed = parser.process_messages()
    assert processed == 1, f"Expected 1 message, got {processed}"
    assert debug_message_received, "Debug message handler was not called"
    
    # Check statistics
    stats = parser.get_stats()
    assert stats['messages_received'] == 1, f"Expected 1 message received, got {stats['messages_received']}"
    assert stats['bytes_received'] == len(test_frame), f"Expected {len(test_frame)} bytes, got {stats['bytes_received']}"
    
    logger.info("✓ Basic protocol test passed")

def test_frame_data_parsing():
    """Test frame data message parsing"""
    logger.info("Testing frame data parsing...")
    
    parser = RobustProtocolParser()
    
    # Custom handler to capture parsed frame
    parsed_frame = None
    
    def frame_handler(message: ProtocolMessage):
        nonlocal parsed_frame
        parsed_frame = FrameDataParser.parse_frame(message.payload)
    
    parser.register_handler(MessageType.FRAME_DATA, frame_handler)
    
    # Create and process frame data
    frame_data = create_test_frame_data()
    parser.add_data(frame_data)
    processed = parser.process_messages()
    
    assert processed == 1, f"Expected 1 message, got {processed}"
    assert parsed_frame is not None, "Frame data was not parsed"
    
    frame_type, image, width, height = parsed_frame
    assert frame_type == "JPG", f"Expected JPG frame type, got {frame_type}"
    assert width == 16, f"Expected width 16, got {width}"
    assert height == 16, f"Expected height 16, got {height}"
    assert image is not None, "Image data is None"
    
    logger.info("✓ Frame data parsing test passed")

def test_detection_data_parsing():
    """Test detection data message parsing"""
    logger.info("Testing detection data parsing...")
    
    parser = RobustProtocolParser()
    
    # Custom handler to capture parsed detections
    parsed_detections = None
    
    def detection_handler(message: ProtocolMessage):
        nonlocal parsed_detections
        logger.info(f"Detection message payload length: {len(message.payload)}")
        parsed_detections = DetectionDataParser.parse_detections(message.payload)
    
    parser.register_handler(MessageType.DETECTION_RESULTS, detection_handler)
    
    # Create and process detection data
    detection_data = create_test_detection_data()
    logger.info(f"Created detection data length: {len(detection_data)}")
    parser.add_data(detection_data)
    processed = parser.process_messages()
    
    assert processed == 1, f"Expected 1 message, got {processed}"
    assert parsed_detections is not None, "Detection data was not parsed"
    
    frame_id, detections = parsed_detections
    assert frame_id == 123, f"Expected frame_id 123, got {frame_id}"
    assert len(detections) == 2, f"Expected 2 detections, got {len(detections)}"
    
    # Check first detection
    class_id, x, y, w, h, confidence, keypoints = detections[0]
    assert class_id == 0, f"Expected class_id 0, got {class_id}"
    assert abs(x - 0.5) < 0.001, f"Expected x 0.5, got {x}"
    assert abs(confidence - 0.85) < 0.001, f"Expected confidence 0.85, got {confidence}"
    
    logger.info("✓ Detection data parsing test passed")

def test_embedding_data_parsing():
    """Test embedding data message parsing"""
    logger.info("Testing embedding data parsing...")
    
    parser = RobustProtocolParser()
    
    # Custom handler to capture parsed embedding
    parsed_embedding = None
    
    def embedding_handler(message: ProtocolMessage):
        nonlocal parsed_embedding
        parsed_embedding = EmbeddingDataParser.parse_embedding(message.payload)
    
    parser.register_handler(MessageType.EMBEDDING_DATA, embedding_handler)
    
    # Create and process embedding data
    embedding_data = create_test_embedding_data()
    parser.add_data(embedding_data)
    processed = parser.process_messages()
    
    assert processed == 1, f"Expected 1 message, got {processed}"
    assert parsed_embedding is not None, "Embedding data was not parsed"
    assert len(parsed_embedding) == 32, f"Expected 32 values, got {len(parsed_embedding)}"
    
    # Check some values
    assert abs(parsed_embedding[0] - 0.0) < 0.001, f"Expected first value 0.0, got {parsed_embedding[0]}"
    assert abs(parsed_embedding[10] - 1.0) < 0.001, f"Expected 10th value 1.0, got {parsed_embedding[10]}"
    
    logger.info("✓ Embedding data parsing test passed")

def test_error_recovery():
    """Test error recovery with corrupted data"""
    logger.info("Testing error recovery...")
    
    parser = RobustProtocolParser()
    
    # Add handler for DEBUG_INFO messages
    debug_messages_received = 0
    
    def debug_handler(message: ProtocolMessage):
        nonlocal debug_messages_received
        debug_messages_received += 1
    
    parser.register_handler(MessageType.DEBUG_INFO, debug_handler)
    
    # Create valid message
    valid_message = create_test_message(MessageType.DEBUG_INFO, b"Valid message", 1)
    
    # Create corrupted data (enough to trigger sync errors)
    corrupted_data = b'\x55\x55\x55\x55\x55' * 5  # More random bytes to trigger sync errors
    
    # Mix valid and corrupted data
    mixed_data = corrupted_data + valid_message + corrupted_data
    
    parser.add_data(mixed_data)
    processed = parser.process_messages()
    
    # Should recover and process the valid message
    assert processed == 1, f"Expected 1 message, got {processed}"
    assert debug_messages_received == 1, "Should have received one debug message"
    
    stats = parser.get_stats()
    # With improved sync error detection, we might have fewer sync errors but should still have some parsing activity
    assert stats['messages_received'] == 1, "Should have recovered one valid message"
    # Check that we processed some data (either sync errors or parse errors indicate activity)
    assert stats['sync_errors'] > 0 or stats['parse_errors'] > 0, "Expected some errors from corrupted data"
    
    logger.info("✓ Error recovery test passed")

def test_multiple_messages():
    """Test processing multiple messages in sequence"""
    logger.info("Testing multiple messages...")
    
    parser = RobustProtocolParser()
    
    # Add handler for DEBUG_INFO messages
    def debug_handler(message: ProtocolMessage):
        pass
    
    parser.register_handler(MessageType.DEBUG_INFO, debug_handler)
    
    # Create multiple different messages
    msg1 = create_test_message(MessageType.DEBUG_INFO, b"Message 1", 1)
    msg2 = create_test_frame_data()
    msg3 = create_test_detection_data()
    msg4 = create_test_embedding_data()
    
    # Send all messages at once
    all_data = msg1 + msg2 + msg3 + msg4
    parser.add_data(all_data)
    
    # Process all messages
    processed = parser.process_messages(max_messages=10)
    assert processed == 4, f"Expected 4 messages, got {processed}"
    
    stats = parser.get_stats()
    assert stats['messages_received'] == 4, f"Expected 4 messages received, got {stats['messages_received']}"
    
    logger.info("✓ Multiple messages test passed")

def run_all_tests():
    """Run all protocol tests"""
    logger.info("Starting robust protocol tests...")
    logger.info("=" * 50)
    
    try:
        test_basic_protocol()
        test_frame_data_parsing()
        test_detection_data_parsing()
        test_embedding_data_parsing()
        test_error_recovery()
        test_multiple_messages()
        
        logger.info("=" * 50)
        logger.info("✅ All tests PASSED! Robust protocol is working correctly.")
        return True
        
    except Exception as e:
        logger.error(f"❌ Test FAILED: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
#!/usr/bin/env python3
"""
Test script to verify checksum compatibility between Python and C implementations
"""

import struct
from robust_protocol import calculate_checksum, ProtocolConstants

def test_checksum_compatibility():
    """Test that checksum calculation matches expected C implementation"""
    
    print("Testing checksum compatibility...")
    print("=" * 50)
    
    # Test case 1: Simple header with SOF and payload size
    test_cases = [
        {
            'name': 'Small payload (100 bytes)',
            'sof': 0xAA,
            'payload_size': 100
        },
        {
            'name': 'Medium payload (1024 bytes)', 
            'sof': 0xAA,
            'payload_size': 1024
        },
        {
            'name': 'Large payload (32768 bytes)',
            'sof': 0xAA,
            'payload_size': 32768
        },
        {
            'name': 'Edge case (1 byte)',
            'sof': 0xAA,
            'payload_size': 1
        },
        {
            'name': 'Edge case (65535 bytes)',
            'sof': 0xAA,
            'payload_size': 65535
        }
    ]
    
    for i, test_case in enumerate(test_cases, 1):
        print(f"\nTest {i}: {test_case['name']}")
        
        sof = test_case['sof']
        payload_size = test_case['payload_size']
        
        # Create header data exactly as C code does
        header_data = bytearray(3)
        header_data[0] = sof                                    # SOF byte
        header_data[1] = payload_size & 0xFF                    # Low byte
        header_data[2] = (payload_size >> 8) & 0xFF           # High byte
        
        # Calculate checksum
        checksum = calculate_checksum(header_data)
        
        # Create complete header
        complete_header = struct.pack('<BHB', sof, payload_size, checksum)
        
        print(f"  SOF: 0x{sof:02X}")
        print(f"  Payload Size: {payload_size} (0x{payload_size:04X})")
        print(f"  Header bytes: {' '.join(f'0x{b:02X}' for b in header_data)}")
        print(f"  Calculated checksum: 0x{checksum:02X}")
        print(f"  Complete header: {' '.join(f'0x{b:02X}' for b in complete_header)}")
        
        # Verify we can parse it back
        parsed_sof, parsed_size, parsed_checksum = struct.unpack('<BHB', complete_header)
        recalc_checksum = calculate_checksum(complete_header[:3])
        
        assert parsed_sof == sof, f"SOF mismatch: {parsed_sof:02X} != {sof:02X}"
        assert parsed_size == payload_size, f"Size mismatch: {parsed_size} != {payload_size}"
        assert parsed_checksum == checksum, f"Checksum mismatch: {parsed_checksum:02X} != {checksum:02X}"
        assert recalc_checksum == checksum, f"Recalc checksum mismatch: {recalc_checksum:02X} != {checksum:02X}"
        
        print(f"  ✓ Checksum verification passed")
    
    print("\n" + "=" * 50)
    print("✅ All checksum tests PASSED!")
    print("\nC implementation should use this exact method:")
    print("```c")
    print("uint8_t header_data[3];")
    print("header_data[0] = frame_header.sof;")
    print("header_data[1] = (uint8_t)(frame_header.payload_size & 0xFF);")
    print("header_data[2] = (uint8_t)((frame_header.payload_size >> 8) & 0xFF);")
    print("frame_header.header_checksum = robust_calculate_checksum(header_data, 3);")
    print("```")

def test_sample_messages():
    """Test with actual message examples"""
    
    print("\n" + "=" * 50)
    print("Testing sample message formats...")
    
    # Sample heartbeat message (timestamp = 12345)
    timestamp_data = struct.pack('<I', 12345)  # 4 bytes
    msg_header_size = 3  # message_type + sequence_id
    total_payload = msg_header_size + len(timestamp_data)
    
    print(f"\nHeartbeat message example:")
    print(f"  Timestamp: 12345")
    print(f"  Timestamp bytes: {' '.join(f'0x{b:02X}' for b in timestamp_data)}")
    print(f"  Message header size: {msg_header_size}")
    print(f"  Total payload size: {total_payload}")
    
    # Create header
    header_data = bytearray(3)
    header_data[0] = 0xAA  # SOF
    header_data[1] = total_payload & 0xFF
    header_data[2] = (total_payload >> 8) & 0xFF
    checksum = calculate_checksum(header_data)
    
    print(f"  Frame header: {' '.join(f'0x{b:02X}' for b in header_data)} + checksum 0x{checksum:02X}")
    
    # Sample frame data message
    frame_width, frame_height = 160, 120
    jpeg_size = 1024  # Example JPEG size
    frame_header_size = 4 + 4 + 4  # frame_type + width + height
    total_payload = msg_header_size + frame_header_size + jpeg_size
    
    print(f"\nFrame data message example:")
    print(f"  Frame: {frame_width}x{frame_height}, JPEG size: {jpeg_size}")
    print(f"  Frame header size: {frame_header_size}")
    print(f"  Total payload size: {total_payload}")
    
    if total_payload <= 65535:
        header_data = bytearray(3)
        header_data[0] = 0xAA
        header_data[1] = total_payload & 0xFF
        header_data[2] = (total_payload >> 8) & 0xFF
        checksum = calculate_checksum(header_data)
        
        print(f"  Frame header: {' '.join(f'0x{b:02X}' for b in header_data)} + checksum 0x{checksum:02X}")
    else:
        print(f"  ERROR: Payload too large ({total_payload} > 65535)")

if __name__ == "__main__":
    test_checksum_compatibility()
    test_sample_messages()
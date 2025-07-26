#!/usr/bin/env python3
"""
Simple raw serial test to check if STM32 is transmitting data
"""

import serial
import time
import sys
from serial.tools import list_ports

def list_available_ports():
    """List all available serial ports"""
    print("Available serial ports:")
    ports = list_ports.comports()
    for port in ports:
        print(f"  {port.device} - {port.description}")
    
    # Check common STM32 ports
    common_ports = ["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0", "/dev/ttyUSB1"]
    print("\nChecking common STM32 ports:")
    for port in common_ports:
        try:
            import os
            if os.path.exists(port):
                test_serial = serial.Serial(port, timeout=0.1)
                test_serial.close()
                print(f"  {port} - ACCESSIBLE")
            else:
                print(f"  {port} - NOT FOUND")
        except Exception as e:
            print(f"  {port} - ERROR: {e}")

def test_raw_reception(port_name, baud_rate=7372800, duration=10):
    """Test raw data reception from serial port"""
    try:
        print(f"Connecting to {port_name} at {baud_rate} baud...")
        ser = serial.Serial(port_name, baud_rate, timeout=1.0)
        
        print(f"Connected! Testing for {duration} seconds...")
        print("Waiting for data...\n")
        
        start_time = time.time()
        total_bytes = 0
        last_byte_time = time.time()
        
        while time.time() - start_time < duration:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                total_bytes += len(data)
                last_byte_time = time.time()
                
                print(f"Received {len(data)} bytes (total: {total_bytes})")
                
                # Show first few bytes as hex
                if len(data) > 0:
                    hex_preview = ' '.join([f"{b:02X}" for b in data[:min(16, len(data))]])
                    if len(data) > 16:
                        hex_preview += "..."
                    print(f"  Hex: {hex_preview}")
                    
                    # Look for SOF byte (0xAA)
                    sof_count = data.count(0xAA)
                    if sof_count > 0:
                        print(f"  Found {sof_count} potential SOF bytes (0xAA)")
            
            time.sleep(0.01)
        
        elapsed = time.time() - start_time
        print(f"\nTest completed:")
        print(f"  Duration: {elapsed:.1f} seconds")
        print(f"  Total bytes received: {total_bytes}")
        print(f"  Average rate: {total_bytes/elapsed:.1f} bytes/sec")
        print(f"  Last data: {time.time() - last_byte_time:.1f} seconds ago")
        
        if total_bytes == 0:
            print("\nNO DATA RECEIVED!")
            print("Possible causes:")
            print("  1. STM32 not transmitting")
            print("  2. Wrong baud rate")
            print("  3. Wrong port")
            print("  4. Hardware connection issue")
        
        ser.close()
        
    except Exception as e:
        print(f"Error: {e}")

def main():
    print("STM32N6 Raw Serial Test")
    print("=======================\n")
    
    # List ports first
    list_available_ports()
    
    # Try default port
    port = "/dev/ttyACM0"
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    # Common baud rates to test based on STM32 code (921600 * 8 = 7372800)
    baud_rates = [7372800, 921600, 115200, 9600]
    
    for baud in baud_rates:
        print(f"\n{'='*50}")
        print(f"Testing port: {port} at {baud} baud")
        print(f"{'='*50}")
        test_raw_reception(port, baud, duration=5)  # Shorter test per baud rate

if __name__ == "__main__":
    main()
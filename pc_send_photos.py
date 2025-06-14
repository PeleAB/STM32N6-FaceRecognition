#!/usr/bin/env python3
import argparse
import glob
import serial
import cv2
import time

import pc_stream_receiver as recv


def send_image(ser, img_path, size):
    img = cv2.imread(img_path)
    if img is None:
        print(f"Failed to read {img_path}")
        return
    img = cv2.resize(img, (size[0], size[1]))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    ser.write(img.tobytes())
    echo, w, h = recv.read_frame(ser)
    if echo is not None:
        print(f"Echo frame {w}x{h} received for {img_path}")
    dets = recv.read_detections(ser)
    if dets:
        print(f"Detections: {dets}")


def main():
    parser = argparse.ArgumentParser(description='Send images to board over UART')
    parser.add_argument('pattern', help='Image file or glob pattern')
    parser.add_argument('--port', default='COM3', help='Serial port')
    parser.add_argument('--baud', type=int, default=921600*8, help='Baud rate')
    parser.add_argument('--width', type=int, default=224, help='Model input width')
    parser.add_argument('--height', type=int, default=224, help='Model input height')
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)
    files = sorted(glob.glob(args.pattern))
    for img_path in files:
        send_image(ser, img_path, (args.width, args.height))
        time.sleep(0.1)
    ser.close()


if __name__ == '__main__':
    main()

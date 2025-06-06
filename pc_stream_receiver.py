#!/usr/bin/env python3
import argparse
import serial
import numpy as np
import cv2


def read_frame(ser):
    line = ser.readline().decode(errors='ignore').strip()
    if not line.startswith('FRAME'):
        return None, None, None
    _, w, h, bpp = line.split()
    w, h, bpp = int(w), int(h), int(bpp)
    size = w * h * bpp
    raw = ser.read(size)
    if bpp == 2:
        frame = np.frombuffer(raw, dtype=np.uint16).reshape((h, w))
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR5652BGR)
    else:
        frame = np.frombuffer(raw, dtype=np.uint8).reshape((h, w))
        frame = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
    return frame, w, h

def read_detections(ser):
    line = ser.readline().decode(errors='ignore').strip()
    if not line.startswith('DETS'):
        return []
    parts = line.split()
    frame_id = int(parts[1])
    count = int(parts[2])
    dets = []
    for _ in range(count):
        line = ser.readline().decode(errors='ignore').strip()
        c, xc, yc, w, h, conf = line.split()
        dets.append((int(c), float(xc), float(yc), float(w), float(h), float(conf)))
    ser.readline()  # read END
    return dets

def draw_detections(img, dets):
    h, w, _ = img.shape
    for d in dets:
        _, xc, yc, ww, hh, conf = d
        x0 = int((xc - ww/2) * w)
        y0 = int((yc - hh/2) * h)
        x1 = int((xc + ww/2) * w)
        y1 = int((yc + hh/2) * h)
        cv2.rectangle(img, (x0, y0), (x1, y1), (0, 255, 0), 2)
        cv2.putText(img, f"{conf:.2f}", (x0, y0-5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0,255,0), 1)
    return img

def main():
    parser = argparse.ArgumentParser(description='Receive frames and detections over UART')
    parser.add_argument('port', help='Serial port device')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)

    try:
        while True:
            frame, w, h = read_frame(ser)
            if frame is None:
                continue
            dets = read_detections(ser)
            frame = draw_detections(frame, dets)
            cv2.imshow('stream', frame)
            if cv2.waitKey(1) == 27:
                break
    finally:
        ser.close()
        cv2.destroyAllWindows()

if __name__ == '__main__':
    main()

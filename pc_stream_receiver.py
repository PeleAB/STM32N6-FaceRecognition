#!/usr/bin/env python3
import argparse
import time

import serial
import numpy as np
import cv2
import threading
import queue

lala = 0
saved_frame = None
def read_frame(ser):
    line = ser.readline().decode(errors='ignore').strip()
    if line.startswith('FRAME'):
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
    if line.startswith('JPG'):
        _, w, h, size = line.split()
        w, h, size = int(w), int(h), int(size)
        raw = ser.read(size)
        img = np.frombuffer(raw, dtype=np.uint8)
        frame = cv2.imdecode(img, cv2.IMREAD_COLOR)
        return frame, w, h

    return None, None, None


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


def display_loop(q, stop_event):
    """Display frames from the queue with FPS counter."""
    scale = 2
    last_time = time.time()
    frame_count = 0
    fps = 0.0

    while not stop_event.is_set():
        frame = q.get()
        if frame is None:
            break

        frame_count += 1
        current_time = time.time()
        elapsed = current_time - last_time

        # Update FPS every second
        if elapsed >= 1.0:
            fps = frame_count / elapsed
            frame_count = 0
            last_time = current_time

        resized = cv2.resize(frame, (0, 0), fx=scale, fy=scale)

        # Draw FPS on the frame
        cv2.putText(resized, f"FPS: {fps:.2f}", (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 0), 2)

        cv2.imshow('stream', resized)
        if cv2.waitKey(1) == 27:
            stop_event.set()

    cv2.destroyAllWindows()

def main():
    parser = argparse.ArgumentParser(description='Receive frames and detections over UART')
    parser.add_argument('--port', default='COM3', help='Serial port device')
    parser.add_argument('--baud', type=int, default=921600*8, help='Baud rate')
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)

    frame_queue = queue.Queue(maxsize=2)
    stop_event = threading.Event()
    disp_thread = threading.Thread(target=display_loop, args=(frame_queue, stop_event))
    disp_thread.start()

    try:
        while not stop_event.is_set():
            frame, w, h = read_frame(ser)
            if frame is None:
                continue
            dets = read_detections(ser)
            frame = draw_detections(frame, dets)
            if not frame_queue.full():
                frame_queue.put(frame)
    finally:
        stop_event.set()
        frame_queue.put(None)
        disp_thread.join()
        ser.close()

if __name__ == '__main__':
    main()

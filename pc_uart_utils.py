#!/usr/bin/env python3
"""Utility functions for UART image streaming."""

import numpy as np
import cv2
import time


def read_frame(ser):
    """Read a frame from the serial port.

    Returns a tuple (image, width, height) or (None, None, None) if no frame is
    available."""
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
    """Read detection results for the current frame."""
    line = ser.readline().decode(errors='ignore').strip()
    if not line.startswith('DETS'):
        return []
    parts = line.split()
    count = int(parts[2])
    dets = []
    for _ in range(count):
        line = ser.readline().decode(errors='ignore').strip()
        c, xc, yc, w, h, conf = line.split()
        dets.append(
            (
                int(c),
                float(xc),
                float(yc),
                float(w),
                float(h),
                float(conf),
            )
        )
    ser.readline()  # END marker
    return dets


def draw_detections(img, dets):
    """Draw detection boxes on an image."""
    h, w, _ = img.shape
    for d in dets:
        _, xc, yc, ww, hh, conf = d
        x0 = int((xc - ww / 2) * w)
        y0 = int((yc - hh / 2) * h)
        x1 = int((xc + ww / 2) * w)
        y1 = int((yc + hh / 2) * h)
        cv2.rectangle(img, (x0, y0), (x1, y1), (0, 255, 0), 2)
        cv2.putText(img, f"{conf:.2f}", (x0, y0 - 5),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    return img


def display_loop(q, stop_event):
    """Display frames from a queue with FPS counter."""
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
        if elapsed >= 1.0:
            fps = frame_count / elapsed
            frame_count = 0
            last_time = current_time

        resized = cv2.resize(frame, (0, 0), fx=scale, fy=scale)
        cv2.putText(resized, f"FPS: {fps:.2f}", (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 0), 2)
        cv2.imshow('stream', resized)
        if cv2.waitKey(1) == 27:
            stop_event.set()

    cv2.destroyAllWindows()


def send_image(ser, img_path, size, display=False):
    """Send an image file to the board.

    Returns the echoed frame and detections from the MCU. If *display* is True,
    the received frame with detection boxes is shown using OpenCV."""

    img = cv2.imread(img_path)
    if img is None:
        print(f"Failed to read {img_path}")
        return None, []

    img = cv2.resize(img, size)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    ser.write(img.tobytes())
    #
    # echo, w, h = read_frame(ser)
    # dets = read_detections(ser)
    #
    # if echo is not None:
    #     echo = draw_detections(echo, dets)
    #     if display:
    #         cv2.imshow("send_result", echo)
    #         cv2.waitKey(1)
    # else:
    #     print("No echo frame received")
    #
    # return echo, dets

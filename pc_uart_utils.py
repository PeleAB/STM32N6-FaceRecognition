#!/usr/bin/env python3
"""Utility functions for UART image streaming."""

import numpy as np
import cv2
import time


def _search_header(ser, prefixes):
    """Read lines until one starting with any of *prefixes* is found."""
    if isinstance(prefixes, str):
        prefixes = (prefixes,)

    while True:
        line = ser.readline().decode(errors="ignore")
        if not line:
            return None
        for p in prefixes:
            idx = line.find(p)
            if idx != -1:
                return line[idx:].strip()


def read_frame(ser):
    """Read a frame from the serial port.

    Returns ``(image, width, height)`` or ``(None, None, None)`` if no frame is
    available. The function searches the stream for a ``FRAME`` or ``JPG``
    header rather than assuming the port is aligned on line boundaries."""

    line = _search_header(ser, ("FRAME", "JPG"))
    if line is None:
        return None, None, None

    if line.startswith("FRAME"):
        _, w, h, bpp = line.split()
        w, h, bpp = int(w), int(h), int(bpp)
        size = w * h * bpp
        raw = ser.read(size)
        if bpp == 2:
            frame = np.frombuffer(raw, dtype=np.uint16).reshape((h, w))
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR5652BGR)
        elif bpp == 3:
            frame = np.frombuffer(raw, dtype=np.uint8).reshape((h, w, 3))
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        else:
            frame = np.frombuffer(raw, dtype=np.uint8).reshape((h, w))
            frame = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
        return frame, w, h

    if line.startswith("JPG"):
        _, w, h, size = line.split()
        w, h, size = int(w), int(h), int(size)
        raw = ser.read(size)
        img = np.frombuffer(raw, dtype=np.uint8)
        frame = cv2.imdecode(img, cv2.IMREAD_COLOR)
        return frame, w, h

    return None, None, None


def read_detections(ser):
    """Read detection results for the current frame.

    Returns ``(frame_id, detections)``. The function searches for the ``DETS``
    header to avoid losing synchronization with the stream."""

    line = _search_header(ser, "DETS")
    if line is None:
        return None, []

    parts = line.split()
    if len(parts) < 3:
        return None, []

    frame_id = int(parts[1])
    count = int(parts[2])
    dets = []
    for _ in range(count):
        line = ser.readline().decode(errors="ignore").strip()
        tokens = line.split()
        if len(tokens) != 6:
            continue
        c, xc, yc, w, h, conf = tokens
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
    return frame_id, dets


def draw_detections(img, dets, color=(0, 255, 0)):
    """Draw detection boxes on an image.

    *color* selects the rectangle and text color.
    """
    h, w, _ = img.shape
    for d in dets:
        _, xc, yc, ww, hh, conf = d
        x0 = int((xc - ww / 2) * w)
        y0 = int((yc - hh / 2) * h)
        x1 = int((xc + ww / 2) * w)
        y1 = int((yc + hh / 2) * h)
        cv2.rectangle(img, (x0, y0), (x1, y1), color, 2)
        cv2.putText(
            img,
            f"{conf:.2f}",
            (x0, y0 - 5),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.5,
            color,
            1,
        )
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


def send_image(ser, img_path, size, display=False, rx=False, preview=False):
    """Send an image file to the board.

    Returns the echoed frame and detections from the MCU. If *display* is True,
    the received frame with detection boxes is shown using OpenCV."""

    img = cv2.imread(img_path)
    if img is None:
        print(f"Failed to read {img_path}")
        return None, []

    img = cv2.resize(img, size)
    if preview:
        cv2.imshow("nn_in", img)
        cv2.waitKey(1)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    ser.write(img.tobytes())

    if rx:
        time.sleep(0.5)
        echo, w, h = read_frame(ser)
        _, dets = read_detections(ser)

        if echo is not None:
            echo = draw_detections(echo, dets)
            if display:
                cv2.imshow("send_result", echo)
                cv2.waitKey(1)
        else:
            print("No echo frame received")

        return echo, dets

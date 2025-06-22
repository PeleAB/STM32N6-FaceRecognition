#!/usr/bin/env python3
"""Compare BlazeFace detections between PC and MCU."""

import argparse
from pathlib import Path
import sys

import cv2
import numpy as np
import serial

import pc_uart_utils as utils

# allow importing the BlazeFace example package
sys.path.insert(0, str(Path(__file__).resolve().parent / "BlazeFace-EXAMPLE"))
from BlazeFaceDetection.blazeFaceDetector import blazeFaceDetector


def det_box(det: tuple) -> np.ndarray:
    """Convert (class, xc, yc, w, h, conf) to [x0, y0, x1, y1]."""
    _, xc, yc, w, h, _ = det
    return np.array([xc - w / 2, yc - h / 2, xc + w / 2, yc + h / 2])


# ------------------------------------------------------------
def iou(box_a: np.ndarray, box_b: np.ndarray) -> float:
    xa0, ya0, xa1, ya1 = box_a
    xb0, yb0, xb1, yb1 = box_b
    inter_w = max(0.0, min(xa1, xb1) - max(xa0, xb0))
    inter_h = max(0.0, min(ya1, yb1) - max(ya0, yb0))
    inter = inter_w * inter_h
    area_a = (xa1 - xa0) * (ya1 - ya0)
    area_b = (xb1 - xb0) * (yb1 - yb0)
    union = area_a + area_b - inter
    return inter / union if union > 0 else 0.0


# ------------------------------------------------------------
def run_model(detector: blazeFaceDetector, img: np.ndarray) -> list[tuple]:
    """Run BlazeFace on *img* and return detections as tuples."""
    results = detector.detectFaces(img)
    dets = []
    for box, score in zip(results.boxes, results.scores):
        x0, y0, x1, y1 = box
        xc = (x0 + x1) / 2
        yc = (y0 + y1) / 2
        w = x1 - x0
        h = y1 - y0
        dets.append((0, float(xc), float(yc), float(w), float(h), float(score)))
    return dets


# ------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", default='trump2.jpeg', help="Image file to send")
    parser.add_argument("--port", default='COM3', help="Serial port, e.g. COM3 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=921600 * 8)
    parser.add_argument(
        "--model-type",
        choices=["front", "back"],
        default="front",
        help="BlazeFace model type",
    )
    parser.add_argument("--score-threshold", type=float, default=0.7)
    parser.add_argument("--iou-threshold", type=float, default=0.3)
    args = parser.parse_args()

    detector = blazeFaceDetector(
        args.model_type, args.score_threshold, args.iou_threshold
    )

    img = cv2.imread(args.image)
    if img is None:
        raise FileNotFoundError(args.image)

    pc_dets = run_model(detector, img)

    with serial.Serial(args.port, args.baud, timeout=1) as ser:
        _frame, mcu_dets = utils.send_image(
            ser,
            args.image,
            (detector.inputWidth, detector.inputHeight),
            display=False,
            rx=True,
        )
    # compute IoU sorted left to right
    pc_boxes = [det_box(d) for d in pc_dets]
    mcu_boxes = [det_box(d) for d in mcu_dets]
    pc_sorted = sorted(pc_boxes, key=lambda b: b[0])
    mcu_sorted = sorted(mcu_boxes, key=lambda b: b[0])
    count = min(len(pc_sorted), len(mcu_sorted))
    for i in range(count):
        score = iou(pc_sorted[i], mcu_sorted[i])
        print(f"IoU box {i}: {score:.2f}")

    # show both detections
    overlay = img.copy()
    utils.draw_detections(overlay, pc_dets, color=(0, 0, 255))
    utils.draw_detections(overlay, mcu_dets, color=(0, 255, 0))
    cv2.imshow("Detections", overlay)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
    print("PC detections:")
    for d in pc_dets:
        print(d)
    print("MCU detections:")
    for d in mcu_dets:
        print(d)


if __name__ == "__main__":
    main()

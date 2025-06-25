#!/usr/bin/env python3
"""Compare BlazeFace detections between PC and MCU."""

import argparse
from pathlib import Path
import sys

import cv2
import numpy as np
import serial
import onnxruntime as ort

import pc_uart_utils as utils

# add this script's directory to the path so the local package is found
sys.path.insert(0, str(Path(__file__).resolve().parent))
from BlazeFaceDetection.blazeFaceDetector import blazeFaceDetector


def crop_align(image: np.ndarray, box: np.ndarray, left_eye: np.ndarray,
               right_eye: np.ndarray, size=(112, 96)) -> np.ndarray:
    """Crop and align face using eye landmarks."""
    h, w, _ = image.shape
    x_center = (box[0] + box[2]) / 2 * w
    y_center = (box[1] + box[3]) / 2 * h
    width = (box[2] - box[0]) * w
    height = (box[3] - box[1]) * h
    lx = left_eye[0] * w
    ly = left_eye[1] * h
    rx = right_eye[0] * w
    ry = right_eye[1] * h

    angle = -np.arctan2(ry - ly, rx - lx)
    cos_a = np.cos(angle)
    sin_a = np.sin(angle)

    dst_w, dst_h = size
    out = np.zeros((dst_h, dst_w, 3), dtype=image.dtype)
    for y in range(dst_h):
        ny = (y + 0.5) / dst_h - 0.5
        for x in range(dst_w):
            nx = (x + 0.5) / dst_w - 0.5
            src_x = x_center + (nx * width) * cos_a + (ny * height) * sin_a
            src_y = y_center + (ny * height) * cos_a - (nx * width) * sin_a
            src_x = np.clip(src_x, 0, w - 1)
            src_y = np.clip(src_y, 0, h - 1)
            out[y, x] = image[int(src_y), int(src_x)]
    return out


def inflate_box(box: np.ndarray, factor: float = 1.2) -> np.ndarray:
    """Return *box* scaled by *factor* around its center."""
    cx = (box[0] + box[2]) / 2
    cy = (box[1] + box[3]) / 2
    w = (box[2] - box[0]) * factor
    h = (box[3] - box[1]) * factor
    half = np.array([w / 2, h / 2], dtype=box.dtype)
    new_box = np.array([cx - half[0], cy - half[1], cx + half[0], cy + half[1]],
                       dtype=box.dtype)
    new_box = np.clip(new_box, 0.0, 1.0)
    return new_box


def det_box(det: tuple) -> np.ndarray:
    """Return [x0, y0, x1, y1] for a detection tuple.

    Extra fields such as keypoints are ignored. Only the first six values
    are used so tuples with additional information won't raise an error.
    """
    if len(det) < 6:
        return np.zeros(4, dtype=np.float32)
    _, xc, yc, w, h, _ = det[:6]
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
    parser.add_argument(
        "--rec-model",
        default="models/mobilefacenet_fp32_PerChannel_quant_lfw_test_data_npz_1_OE_3_2_0.onnx",
        help="ONNX face recognition model path",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=5.0,
        help="Seconds to wait for MCU response",
    )
    args = parser.parse_args()

    detector = blazeFaceDetector(
        args.model_type, args.score_threshold, args.iou_threshold
    )

    session = ort.InferenceSession(str(Path(args.rec_model)))
    input_name = session.get_inputs()[0].name

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
            timeout=args.timeout,
        )
        aligned_frames = utils.read_aligned_frames(ser, len(mcu_dets))
        mcu_embs = utils.read_embeddings(ser, len(mcu_dets))
    # compute IoU sorted left to right
    pc_boxes = [det_box(d) for d in pc_dets]
    mcu_boxes = [det_box(d) for d in mcu_dets]
    pc_sorted = sorted(pc_boxes, key=lambda b: b[0])
    mcu_sorted = sorted(mcu_boxes, key=lambda b: b[0])
    count = min(len(pc_sorted), len(mcu_sorted))
    for i in range(count):
        score = iou(pc_sorted[i], mcu_sorted[i])
        print(f"IoU box {i}: {score:.2f}")

    for idx, (det, frame, emb) in enumerate(zip(mcu_dets, aligned_frames, mcu_embs)):
        _, xc, yc, w, h, _conf, kps = det
        box = np.array([xc - w / 2, yc - h / 2, xc + w / 2, yc + h / 2], dtype=np.float32)
        kps = np.array(kps, dtype=np.float32).reshape(-1, 2)
        box = inflate_box(box)
        aligned_pc = crop_align(img, box, kps[0], kps[1], size=(112, 96))

        # embedding from PC using MCU aligned frame
        face_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB).astype(np.int16)
        face_rgb -= 128
        face = np.transpose(face_rgb.astype(np.int8), (2, 0, 1))[None, ...]
        mcu_pc_out = session.run(None, {input_name: face})[0]
        mcu_pc_emb = mcu_pc_out.astype(np.float32).flatten() / 128.0

        # embedding from PC using PC aligned crop
        face_rgb2 = cv2.cvtColor(aligned_pc, cv2.COLOR_BGR2RGB).astype(np.int16)
        face_rgb2 -= 128
        face2 = np.transpose(face_rgb2.astype(np.int8), (2, 0, 1))[None, ...]
        pc_out = session.run(None, {input_name: face2})[0]
        pc_emb = pc_out.astype(np.float32).flatten() / 128.0

        mcu_emb = np.array(emb, dtype=np.float32)

        for v in (mcu_pc_emb, pc_emb, mcu_emb):
            if np.linalg.norm(v) > 0:
                v /= np.linalg.norm(v)

        cos_mcu_pc = float(np.dot(mcu_pc_emb, mcu_emb))
        cos_pc = float(np.dot(pc_emb, mcu_emb))
        print(f"Embedding {idx} MCU frame vs MCU: {cos_mcu_pc:.4f}")
        print(f"Embedding {idx} PC crop vs MCU: {cos_pc:.4f}")

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

#!/usr/bin/env python3
"""Compare YOLOv2 detections between PC and MCU."""

import argparse
import cv2
import numpy as np
import serial
import tensorflow as tf
TF_ENABLE_ONEDNN_OPTS=0
import pc_uart_utils as utils

ANCHORS = np.array([
    0.9883, 3.3606,
    2.1194, 5.3759,
    3.0520, 9.1336,
    5.5517, 9.3066,
    9.7260, 11.1422,
], dtype=np.float32)
GRID = 7
CONF_THRESHOLD = 0.6
IOU_THRESHOLD = 0.3
MAX_DET = 10


def det_box(det: tuple) -> np.ndarray:
    """Convert (class, xc, yc, w, h, conf) to [x0, y0, x1, y1]."""
    _, xc, yc, w, h, _ = det
    return np.array([xc - w / 2, yc - h / 2, xc + w / 2, yc + h / 2])


# ------------------------------------------------------------
def sigmoid(x: np.ndarray) -> np.ndarray:
    return 1.0 / (1.0 + np.exp(-x))


def softmax(x: np.ndarray) -> np.ndarray:
    e = np.exp(x - np.max(x))
    return e / np.sum(e)


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
def decode_yolov2(output: np.ndarray) -> list[tuple]:
    """Decode raw model output to bounding boxes."""
    output = output.reshape(GRID, GRID, 5, 6)
    dets = []
    for row in range(GRID):
        for col in range(GRID):
            for a in range(5):
                xc, yc, w, h, obj, cls = output[row, col, a]
                obj = sigmoid(obj)
                cls = softmax(np.array([cls]))[0]
                score = obj * cls
                if score < CONF_THRESHOLD:
                    continue
                xc = (col + sigmoid(xc)) / GRID
                yc = (row + sigmoid(yc)) / GRID
                w = ANCHORS[2 * a] * np.exp(w) / GRID
                h = ANCHORS[2 * a + 1] * np.exp(h) / GRID
                dets.append((0, xc, yc, w, h, score))
    # NMS
    dets = sorted(dets, key=lambda d: d[5], reverse=True)
    final = []
    boxes = []
    for det in dets:
        if len(final) >= MAX_DET:
            break
        _, xc, yc, w, h, score = det
        x0 = xc - w / 2
        y0 = yc - h / 2
        x1 = xc + w / 2
        y1 = yc + h / 2
        current = np.array([x0, y0, x1, y1])
        if any(iou(current, b) > IOU_THRESHOLD for b in boxes):
            continue
        final.append(det)
        boxes.append(current)
    return final


# ------------------------------------------------------------
def run_model(
    interpreter: tf.lite.Interpreter, img: np.ndarray
) -> list[tuple]:
    img = cv2.resize(img, (224, 224))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    input_details = interpreter.get_input_details()[0]
    tensor = np.expand_dims(img, 0).astype(np.uint8)
    interpreter.set_tensor(input_details["index"], tensor)
    interpreter.invoke()
    output_idx = interpreter.get_output_details()[0]["index"]
    output = interpreter.get_tensor(output_idx)
    return decode_yolov2(output[0])


# ------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", default='trump2.jpeg', help="Image file to send")
    parser.add_argument("--port", default='COM3', help="Serial port, e.g. COM3 or /dev/ttyUSB0")
    parser.add_argument(
        "--model",
        default="Model/quantized_tiny_yolo_v2_224_.tflite",
    )
    parser.add_argument("--baud", type=int, default=921600 * 8)
    args = parser.parse_args()

    interpreter = tf.lite.Interpreter(model_path=args.model)
    interpreter.allocate_tensors()

    img = cv2.imread(args.image)
    if img is None:
        raise FileNotFoundError(args.image)
    img = cv2.resize(img, (224, 224))
    pc_dets = run_model(interpreter, img)

    with serial.Serial(args.port, args.baud, timeout=1) as ser:
        _frame, mcu_dets = utils.send_image(
            ser, args.image, (224, 224), display=False, rx=True
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

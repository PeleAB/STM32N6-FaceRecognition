#!/usr/bin/env python3
"""Run face detection and recognition on an image.

This script loads a photo, detects the most confident face using
BlazeFace, aligns the face using the eye landmarks, runs a TensorFlow
Lite face recognition model and prints the resulting embedding vector.
"""

import argparse
from pathlib import Path
import sys

import cv2
import numpy as np
import tensorflow as tf

# allow importing BlazeFace modules from this directory
sys.path.insert(0, str(Path(__file__).resolve().parent))
from BlazeFaceDetection.blazeFaceDetector import blazeFaceDetector


def crop_align(image: np.ndarray, box: np.ndarray, left_eye: np.ndarray,
               right_eye: np.ndarray, size=(112, 112)) -> np.ndarray:
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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, help="Input image path")
    parser.add_argument(
        "--rec-model",
        default="face_recognition.tflite",
        help="TFLite face recognition model path",
    )
    parser.add_argument(
        "--det-model-type",
        choices=["front", "back"],
        default="front",
        help="BlazeFace model type",
    )
    parser.add_argument(
        "--visualize",
        action="store_true",
        help="Show the detected face and the aligned crop",
    )
    args = parser.parse_args()

    detector = blazeFaceDetector(args.det_model_type)

    img = cv2.imread(args.image)
    if img is None:
        raise FileNotFoundError(args.image)

    results = detector.detectFaces(img)
    if results.boxes.shape[0] == 0:
        print("No face detected")
        return

    box = results.boxes[0]
    left_eye = results.keypoints[0, 0]
    right_eye = results.keypoints[0, 1]
    aligned = crop_align(img, box, left_eye, right_eye, (112, 112))

    shown = False
    if args.visualize:
        disp = img.copy()
        h, w, _ = disp.shape
        x0 = int(box[0] * w)
        y0 = int(box[1] * h)
        x1 = int(box[2] * w)
        y1 = int(box[3] * h)
        cv2.rectangle(disp, (x0, y0), (x1, y1), (0, 255, 0), 2)
        lx = int(left_eye[0] * w)
        ly = int(left_eye[1] * h)
        rx = int(right_eye[0] * w)
        ry = int(right_eye[1] * h)
        cv2.circle(disp, (lx, ly), 2, (0, 0, 255), -1)
        cv2.circle(disp, (rx, ry), 2, (0, 0, 255), -1)
        try:
            cv2.imshow("Detected face", disp)
            cv2.imshow("Aligned face", aligned)
            shown = True
        except cv2.error:
            cv2.imwrite("detected_face.jpg", disp)
            cv2.imwrite("aligned_face.jpg", aligned)
            print("Saved detected_face.jpg and aligned_face.jpg")

    rec = tf.lite.Interpreter(model_path=str(Path(args.rec_model)))
    rec.allocate_tensors()
    input_info = rec.get_input_details()[0]
    output_info = rec.get_output_details()[0]

    face = (
        cv2.cvtColor(aligned, cv2.COLOR_BGR2RGB).astype(np.float32) / 128.0
    ) - 1.0
    face = face[None, ...]
    rec.set_tensor(input_info["index"], face)
    rec.invoke()
    embedding = rec.get_tensor(output_info["index"]).flatten()

    print("Embedding vector:")
    print(" ".join(f"{x:.6f}" for x in embedding))

    if args.visualize and shown:
        print("Close the image windows to exit")
        cv2.waitKey(0)
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

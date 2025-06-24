#!/usr/bin/env python3
"""Run face detection and recognition on two images via ONNX and compute cosine similarity."""

import argparse
from pathlib import Path
import sys

import cv2
import numpy as np
import onnxruntime as ort

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
    lx, ly = left_eye[0] * w, left_eye[1] * h
    rx, ry = right_eye[0] * w, right_eye[1] * h

    angle = -np.arctan2(ry - ly, rx - lx)
    cos_a, sin_a = np.cos(angle), np.sin(angle)
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


def get_embedding(detector, session, input_name, nchw, img_path, visualize=False, win_name=""):
    img = cv2.imread(img_path)
    if img is None:
        raise FileNotFoundError(f"Could not read '{img_path}'")

    # detect
    results = detector.detectFaces(img)
    if results.boxes.shape[0] == 0:
        print(f"No face detected in {img_path!r}")
        sys.exit(1)

    # align
    box = results.boxes[0]
    left_eye, right_eye = results.keypoints[0, 0], results.keypoints[0, 1]
    aligned = crop_align(img, box, left_eye, right_eye, (112, 112))

    # visualize
    if visualize:
        h, w, _ = img.shape
        x0, y0 = int(box[0] * w), int(box[1] * h)
        x1, y1 = int(box[2] * w), int(box[3] * h)
        disp = img.copy()
        cv2.rectangle(disp, (x0, y0), (x1, y1), (0, 255, 0), 2)
        lx, ly = int(left_eye[0] * w), int(left_eye[1] * h)
        rx, ry = int(right_eye[0] * w), int(right_eye[1] * h)
        cv2.circle(disp, (lx, ly), 2, (0, 0, 255), -1)
        cv2.circle(disp, (rx, ry), 2, (0, 0, 255), -1)
        cv2.imshow(f"{win_name} Detection", disp)
        cv2.imshow(f"{win_name} Aligned", aligned)
        cv2.waitKey()

    # preprocess for ONNX
    face =aligned.astype(np.float32)

    face = face[None, ...]


    if nchw:
        face = face.transpose(0, 3, 1, 2)

    print(f"[DEBUG] face tensor shape: {face.shape}, "
          f"min={face.min():.3f}, max={face.max():.3f}, mean={face.mean():.3f}")

    # run ONNX
    outputs = session.run(None, {input_name: face})
    emb = outputs[0].flatten()

    print(f"[DEBUG] raw emb stats: min={emb.min():.3f}, max={emb.max():.3f}, "
          f"mean={emb.mean():.3f}, L2 before norm={np.linalg.norm(emb):.3f}")

    emb = emb / np.linalg.norm(emb)
    return emb


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--image1", default="trump.jpg", help="First input image path")
    p.add_argument("--image2", default="multrump.jpg", help="Second input image path")
    p.add_argument(
        "--rec-model",
        default="models/face_recognition_sface_2021dec_int8.onnx",
        help="ONNX face recognition model path",
    )
    p.add_argument(
        "--det-model-type",
        choices=["front", "back"],
        default="front",
        help="BlazeFace model type",
    )
    p.add_argument(
        "--visualize",
        action="store_true",
        help="Show detections and aligned crops for both images"
    )
    args = p.parse_args()

    # setup detector
    detector = blazeFaceDetector(args.det_model_type)

    # setup ONNX runtime
    session = ort.InferenceSession(str(Path(args.rec_model)))
    print("=== ONNX outputs ===")
    for i, o in enumerate(session.get_outputs()):
        print(f"  [{i}] {o.name} : {o.shape}")

    inp = session.get_inputs()[0]
    input_name = inp.name
    shape = inp.shape  # e.g. [1,3,112,112] or [1,112,112,3]
    nchw = (shape[1] == 3)

    # get embeddings
    emb1 = get_embedding(detector, session, input_name, nchw, args.image1, False, "Image1")
    emb2 = get_embedding(detector, session, input_name, nchw, args.image2, False, "Image2")


    # compute cosine similarity
    cos_sim = np.dot(emb1, emb2) / (np.linalg.norm(emb1) * np.linalg.norm(emb2))
    print(f"\nCosine similarity between faces: {cos_sim:.4f}")

    if args.visualize:
        print("Press any key to close windows.")
        cv2.waitKey(0)
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

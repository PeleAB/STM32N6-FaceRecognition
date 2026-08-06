"""Create a board-compatible MobileFaceNet gallery template from photos.

Prints JSON containing a normalized signed-Q7 embedding. The detector,
five-point alignment template, and embedding model match the firmware.
"""

import argparse
import json
from pathlib import Path

import cv2
import numpy as np
import onnxruntime as ort

CANONICAL = np.array([[38.2946, 51.6963], [73.5318, 51.5014],
                      [56.0252, 71.7366], [41.5493, 92.3655],
                      [70.7299, 92.2041]], dtype=np.float32)


def letterbox_rgb(path: Path) -> np.ndarray:
    bgr = cv2.imread(str(path), cv2.IMREAD_COLOR)
    if bgr is None:
        raise ValueError(f"cannot read {path}")
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    h, w = rgb.shape[:2]
    scale = min(128.0 / w, 128.0 / h)
    resized = cv2.resize(rgb, (round(w * scale), round(h * scale)),
                         interpolation=cv2.INTER_AREA)
    canvas = np.zeros((128, 128, 3), dtype=np.uint8)
    y = (128 - resized.shape[0]) // 2
    x = (128 - resized.shape[1]) // 2
    canvas[y:y + resized.shape[0], x:x + resized.shape[1]] = resized
    return canvas


def detect_landmarks(session: ort.InferenceSession, rgb: np.ndarray) -> tuple[np.ndarray, float]:
    inp = np.transpose(rgb, (2, 0, 1))[None]
    scale, landmarks, heatmap, offset = session.run(None, {session.get_inputs()[0].name: inp})
    row, col = np.unravel_index(np.argmax(heatmap[0, :, :, 0]), (32, 32))
    confidence = float(heatmap[0, row, col, 0])
    if confidence < 0.20:
        raise ValueError(f"no confident face (best score {confidence:.2f})")
    bh = float(np.exp(np.clip(scale[0, row, col, 0], -10, 10)) * 4)
    bw = float(np.exp(np.clip(scale[0, row, col, 1], -10, 10)) * 4)
    y0 = (row + float(offset[0, row, col, 0]) + 0.5) * 4 - bh * 0.5
    x0 = (col + float(offset[0, row, col, 1]) + 0.5) * 4 - bw * 0.5
    raw = landmarks[0, row, col]
    points = np.empty((5, 2), dtype=np.float32)
    for k in range(5):
        points[k] = (x0 + float(raw[2 * k + 1]) * bw,
                     y0 + float(raw[2 * k]) * bh)
    return points, confidence


def align(rgb: np.ndarray, points: np.ndarray) -> np.ndarray:
    src_mean = points.mean(axis=0)
    dst_mean = CANONICAL.mean(axis=0)
    src = points - src_mean
    dst = CANONICAL - dst_mean
    norm = float(np.sum(src * src))
    if norm < 1.0:
        raise ValueError("could not align face landmarks")
    a = float(np.sum(src * dst) / norm)
    b = float(np.sum(src[:, 0] * dst[:, 1] - src[:, 1] * dst[:, 0]) / norm)
    tx = float(dst_mean[0] - a * src_mean[0] + b * src_mean[1])
    ty = float(dst_mean[1] - b * src_mean[0] - a * src_mean[1])
    transform = np.array([[a, -b, tx], [b, a, ty]], dtype=np.float32)
    return cv2.warpAffine(rgb, transform, (112, 112), flags=cv2.INTER_LINEAR,
                          borderMode=cv2.BORDER_CONSTANT)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model-dir", type=Path, required=True)
    parser.add_argument("photos", type=Path, nargs="+")
    args = parser.parse_args()
    fd = ort.InferenceSession(str(args.model_dir / "st_ai_output" /
                                  "centerface_OE_3_3_1.onnx"),
                              providers=["CPUExecutionProvider"])
    faceid = ort.InferenceSession(str(args.model_dir /
                                      "mobilefacenet_int8_faces.onnx"),
                                  providers=["CPUExecutionProvider"])
    vectors = []
    details = []
    for photo in args.photos:
        rgb = letterbox_rgb(photo)
        points, confidence = detect_landmarks(fd, rgb)
        crop = align(rgb, points)
        tensor = (np.transpose(crop, (2, 0, 1))[None].astype(np.float32) /
                  127.5 - 1.0)
        vector = faceid.run(None, {faceid.get_inputs()[0].name: tensor})[0][0]
        norm = float(np.linalg.norm(vector))
        if not np.isfinite(norm) or norm < 1e-9:
            raise ValueError(f"invalid embedding for {photo}")
        vectors.append(vector / norm)
        details.append({"photo": str(photo), "confidence": confidence})
    template = np.mean(vectors, axis=0)
    template /= np.linalg.norm(template)
    q7 = np.clip(np.rint(template * 127), -127, 127).astype(np.int8)
    print(json.dumps({"embeddingQ7": q7.astype(int).tolist(), "photos": details}))


if __name__ == "__main__":
    main()

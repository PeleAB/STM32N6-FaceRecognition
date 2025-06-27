import argparse
from pathlib import Path
import sys

import cv2
import numpy as np
import onnxruntime as ort

# allow importing CenterFace demo from repository root

from centerface import CenterFace


def crop_align(image: np.ndarray, box: np.ndarray, left_eye: np.ndarray,
               right_eye: np.ndarray, size=(96, 112)) -> np.ndarray:
    """Crop and align face using eye landmarks without squashing."""
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
    dst_full = max(dst_w, dst_h)
    off_x = (dst_full - dst_w) / 2.0
    off_y = (dst_full - dst_h) / 2.0

    out = np.zeros((dst_h, dst_w, 3), dtype=image.dtype)
    for y in range(dst_h):
        ny = ((y + off_y) + 0.5) / dst_full - 0.5
        for x in range(dst_w):
            nx = ((x + off_x) + 0.5) / dst_full - 0.5
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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", help="Input image path", default="trump.jpg")
    parser.add_argument(
        "--rec-model",
        default="models/mobilefacenet_integer_quant_1_OE_3_2_0.onnx",
        help="ONNX face recognition model path",
    )
    parser.add_argument(
        "--det-model",
        default="models/centerface_1x3xHxW_integer_quant.tflite",
        help="CenterFace TFLite model path",
    )
    parser.add_argument(
        "--visualize",
        action="store_true",
        default=True,
        help="Show the detected face and the aligned crop",
    )
    args = parser.parse_args()

    # load CenterFace detector
    detector = CenterFace(args.det_model)

    img = cv2.imread(args.image)
    if img is None:
        raise FileNotFoundError(f"Could not open {args.image}")

    # center‐crop to square
    h0, w0, _ = img.shape
    crop_size = min(h0, w0)
    off_x = (w0 - crop_size) // 2
    off_y = (h0 - crop_size) // 2
    img_sq = img[off_y : off_y + crop_size, off_x : off_x + crop_size]


    # detect and align using CenterFace
    print(img_sq.dtype)
    print(img_sq.shape)
    dets, lms = detector.inference(img_sq, threshold=0.5)
    if len(dets) == 0:
        print("No face detected")
        return

    det = dets[0]
    lm = lms[0]
    box = np.array(
        [
            det[0] / img_sq.shape[1],
            det[1] / img_sq.shape[0],
            det[2] / img_sq.shape[1],
            det[3] / img_sq.shape[0],
        ],
        dtype=np.float32,
    )
    box = inflate_box(box)
    left_eye = np.array([lm[0] / img_sq.shape[1], lm[1] / img_sq.shape[0]], dtype=np.float32)
    right_eye = np.array([lm[2] / img_sq.shape[1], lm[3] / img_sq.shape[0]], dtype=np.float32)
    aligned = crop_align(img_sq, box, left_eye, right_eye, size=(96, 112))

    shown = False
    if args.visualize:
        disp = img.copy()
        h, w, _ = disp.shape
        x0, y0 = int(box[0] * w), int(box[1] * h)
        x1, y1 = int(box[2] * w), int(box[3] * h)
        cv2.rectangle(disp, (x0, y0), (x1, y1), (0, 255, 0), 2)
        lx, ly = int(left_eye[0] * w), int(left_eye[1] * h)
        rx, ry = int(right_eye[0] * w), int(right_eye[1] * h)
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

    # --- ONNX Runtime inference ---
    sess = ort.InferenceSession(str(Path(args.rec_model)))
    input_name = sess.get_inputs()[0].name
    output_name = sess.get_outputs()[0].name

    # preprocess: BGR→RGB, zero‐center around 0, CHW, batch
    face_rgb = cv2.cvtColor(aligned, cv2.COLOR_BGR2RGB).astype(np.int16)
    face_rgb -= 128
    face = np.transpose(face_rgb.astype(np.int8), (2, 0, 1))[None, ...]

    # run inference

    print(face.shape)
    print(face.flatten())

    onnx_out = sess.run([output_name], {input_name: face})[0]

    print(onnx_out)
    embedding = onnx_out.astype(np.int8).flatten() / 128.0

    norm = np.linalg.norm(embedding)
    if norm > 0:
        embedding /= norm
    # print embedding
    print("Embedding vector:")
    print(" ".join(f"{x:.6f}" for x in embedding))

    # patch into your C source
    emb_line = (
        "float target_embedding[EMBEDDING_SIZE] = {"
        + ", ".join(f"{x:.6f}" for x in embedding)
        + "};\n"
    )
    c_path = Path(__file__).resolve().parents[1] / "Src" / "target_embedding.c"
    lines = c_path.read_text().splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.strip().startswith("float target_embedding"):
            lines[i] = emb_line
            break
    c_path.write_text("".join(lines))
    print("Updated", c_path)

    # also patch dummy recognition input buffer
    buf_line = (
        "int8_t dummy_fr_input[DUMMY_FR_INPUT_SIZE] = {"
        + ", ".join(str(int(x)) for x in face.flatten())
        + "};\n"
    )
    buf_path = Path(__file__).resolve().parents[1] / "Src" / "dummy_fr_input.c"
    lines = buf_path.read_text().splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.strip().startswith("int8_t dummy_fr_input"):
            lines[i] = buf_line
            break
    buf_path.write_text("".join(lines))
    print("Updated", buf_path)

    if args.visualize and shown:
        print("Close the image windows to exit")
        cv2.waitKey(0)
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

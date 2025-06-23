import numpy as np
import cv2
from pathlib import Path


def img_rgb_to_hwc_float_py(src: np.ndarray) -> np.ndarray:
    scale = 1.0 / 128.0
    height, width, _ = src.shape
    dst = np.empty((3, height, width), dtype=np.float32)
    for y in range(height):
        for x in range(width):
            for c in range(3):
                dst[c, y, x] = src[y, x, c] * scale - 1.0
    return dst


def main():
    img_path = Path(__file__).resolve().parent / 'trump.jpg'
    img = cv2.imread(str(img_path))
    if img is None:
        raise FileNotFoundError(img_path)
    img = cv2.resize(img, (112, 112))
    ref = (img.astype(np.float32) / 128.0) - 1.0
    ref = ref.transpose(2, 0, 1)
    test = img_rgb_to_hwc_float_py(img)
    diff = np.abs(ref - test).max()
    print(f'Max diff: {diff}')


if __name__ == '__main__':
    main()


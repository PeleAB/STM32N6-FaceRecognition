import numpy as np
import cv2
from pathlib import Path


def img_rgb_to_chw_s8_py(src: np.ndarray) -> np.ndarray:
    height, width, _ = src.shape
    dst = np.empty((3, height, width), dtype=np.int8)
    for y in range(height):
        for x in range(width):
            for c in range(3):
                val = int(src[y, x, c]) - 128
                dst[c, y, x] = np.int8(val)
    return dst


def main():
    img_path = Path(__file__).resolve().parent / 'trump.jpg'
    img = cv2.imread(str(img_path))
    if img is None:
        raise FileNotFoundError(img_path)
    img = cv2.resize(img, (96, 112))
    ref = img.astype(np.int16) - 128
    ref = ref.astype(np.int8).transpose(2, 0, 1)
    test = img_rgb_to_chw_s8_py(img)
    diff = np.abs(ref.astype(np.int16) - test.astype(np.int16)).max()
    print(f'Max diff: {diff}')


if __name__ == '__main__':
    main()

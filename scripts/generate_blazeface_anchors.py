import argparse
import numpy as np
# Generate anchors for BlazeFace (face/palm detection) model
# Based on input size 128x128 and default anchor settings used by MediaPipe.

# The network uses two feature maps:
#  - 16x16 grid with 2 anchors per location
#  - 8x8 grid with 6 anchors per location (duplicated anchors for keypoints)
# Anchors are normalized to [0,1] coordinates.


def generate_anchors(img_width=128, img_height=128):
    anchors = np.load("C:\\Users\\pele\\Downloads\\anchorsback.npy")
    return anchors


def write_header(anchors, path):
    with open(path, "w") as f:
        f.write("// Auto-generated BlazeFace anchors\n")
        f.write("#ifndef BLAZEFACE_ANCHORS_H\n")
        f.write("#define BLAZEFACE_ANCHORS_H\n\n")
        f.write(f"#define BLAZEFACE_NUM_ANCHORS ({len(anchors)})\n")
        f.write("#define BLAZEFACE_ANCHOR_DIM    (4)\n\n")
        f.write(f"static const float32_t BLAZEFACE_ANCHORS[{len(anchors)*4}] = {{\n")
        for cx, cy, w, h in anchors:
            f.write(f"    {cx:.7f}f,\n")
            f.write(f"    {cy:.7f}f,\n")
            f.write(f"    {w:.7f}f,\n")
            f.write(f"    {h:.7f}f,\n")
        f.write("};\n\n")
        f.write("#endif  // BLAZEFACE_ANCHORS_H\n")


def main():
    parser = argparse.ArgumentParser(description="Generate BlazeFace anchors and update header file")
    parser.add_argument("output", help="Path to output header file", default="../Inc/blazeface_anchors.h", nargs='?')
    parser.add_argument("--width", type=int, default=256, help="Input width")
    parser.add_argument("--height", type=int, default=256, help="Input height")
    args = parser.parse_args()

    anchors = generate_anchors(args.width, args.height)
    write_header(anchors, args.output)


if __name__ == "__main__":
    main()

# import numpy as np
#
# data = np.load("C:\\Users\\pele\\Downloads\\anchors.npy").flatten()
#
# print(data)
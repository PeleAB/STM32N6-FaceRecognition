import argparse

# Generate anchors for BlazeFace (face/palm detection) model
# Based on input size 128x128 and default anchor settings used by MediaPipe.

# The network uses two feature maps:
#  - 16x16 grid with 2 anchors per location
#  - 8x8 grid with 6 anchors per location (duplicated anchors for keypoints)
# Anchors are normalized to [0,1] coordinates.


def generate_anchors(img_width=128, img_height=128):
    anchors = []
    # First feature map (stride 8 -> 16x16 grid)
    stride1 = 8
    grid1 = img_width // stride1  # 16
    step = 1.0 / grid1
    offset = step / 2
    sizes = [0.25, 0.5]
    for y in range(grid1):
        for x in range(grid1):
            cx = offset + x * step
            cy = offset + y * step
            for size in sizes:
                anchors.append((cx, cy, size, size))
    # Second feature map (stride 16 -> 8x8 grid)
    stride2 = 16
    grid2 = img_width // stride2  # 8
    step = 1.0 / grid2
    offset = step / 2
    for y in range(grid2):
        for x in range(grid2):
            cx = offset + x * step
            cy = offset + y * step
            for _ in range(3):  # duplicated anchors for each keypoint
                for size in sizes:
                    anchors.append((cx, cy, size, size))
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
    parser.add_argument("--width", type=int, default=128, help="Input width")
    parser.add_argument("--height", type=int, default=128, help="Input height")
    args = parser.parse_args()

    anchors = generate_anchors(args.width, args.height)
    write_header(anchors, args.output)


if __name__ == "__main__":
    main()

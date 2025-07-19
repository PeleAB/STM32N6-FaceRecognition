#!/usr/bin/env python3
"""
Create dual dummy buffers from trump2.jpg:
1. img_buffer: 480x480 centered in 800x480 RGB565 with black padding (original camera frame)
2. nn_rgb: 128x128 RGB888 (neural network input)

Both buffers are derived from the same image to ensure consistency
between face detection and face cropping operations.
"""

import os
from PIL import Image
import numpy as np

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565"""
    r565 = (r >> 3) & 0x1F
    g565 = (g >> 2) & 0x3F  
    b565 = (b >> 3) & 0x1F
    return (r565 << 11) | (g565 << 5) | b565

def create_dual_dummy_buffers():
    """Create both dummy buffers from trump2.jpg"""
    
    # Load trump2.jpg
    img_path = "Exercises/SamplePics/trump2.jpg"
    if not os.path.exists(img_path):
        print(f"Error: {img_path} not found")
        return
    
    print(f"Loading {img_path}...")
    img = Image.open(img_path)
    print(f"Original image: {img.size} {img.mode}")
    
    # Convert to RGB if needed
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Create img_buffer: 480x480 centered in 800x480 with black padding
    print("Creating img_buffer (480x480 centered in 800x480 RGB565)...")
    img_buffer_480 = img.resize((480, 480), Image.LANCZOS)
    
    # Create 800x480 black canvas
    img_buffer_rgb = np.zeros((480, 800, 3), dtype=np.uint8)
    
    # Calculate center position: (800-480)/2 = 160 pixels from left
    left_padding = (800 - 480) // 2  # 160 pixels
    
    # Place 480x480 image in center of 800x480 canvas
    img_buffer_rgb[:, left_padding:left_padding+480, :] = np.array(img_buffer_480, dtype=np.uint8)
    
    # Convert to RGB565
    img_buffer_565 = np.zeros((480, 800), dtype=np.uint16)
    for y in range(480):
        for x in range(800):
            r, g, b = img_buffer_rgb[y, x]
            img_buffer_565[y, x] = rgb888_to_rgb565(r, g, b)
    
    # Create nn_rgb: 128x128 RGB888
    print("Creating nn_rgb (128x128 RGB888)...")
    nn_rgb = img.resize((128, 128), Image.LANCZOS)
    nn_rgb_array = np.array(nn_rgb, dtype=np.uint8)
    
    # Generate C header file
    print("Generating dummy_dual_buffer.h...")
    with open("dummy_dual_buffer.h", "w") as f:
        f.write("""#ifndef DUMMY_DUAL_BUFFER_H
#define DUMMY_DUAL_BUFFER_H

#include <stdint.h>
#include "app_config.h"

/* Dual dummy buffers derived from test image */

/* img_buffer: 480x480 centered in 800x480 RGB565 with black padding (original camera frame) */
extern const uint16_t dummy_test_img_buffer[800 * 480];

/* nn_rgb: 128x128 RGB888 (neural network input) */
extern const uint8_t dummy_test_nn_rgb[128 * 128 * 3];

/* Buffer sizes */
#define DUMMY_TEST_IMG_BUFFER_SIZE (800 * 480 * 2)
#define DUMMY_TEST_NN_RGB_SIZE (128 * 128 * 3)

#endif /* DUMMY_DUAL_BUFFER_H */
""")
    
    # Generate C source file
    print("Generating dummy_dual_buffer.c...")
    with open("dummy_dual_buffer.c", "w") as f:
        f.write("""#include "dummy_dual_buffer.h"

/* img_buffer: 480x480 centered in 800x480 RGB565 with black padding from trump2.jpg */
const uint16_t dummy_test_img_buffer[800 * 480] = {
""")
        
        # Write img_buffer data (RGB565)
        for y in range(480):
            f.write("    ")
            for x in range(800):
                if x == 799 and y == 479:
                    f.write(f"0x{img_buffer_565[y, x]:04X}")
                else:
                    f.write(f"0x{img_buffer_565[y, x]:04X}, ")
                if x % 8 == 7:  # New line every 8 values
                    f.write("\n    ")
            if y % 10 == 9:  # Progress indicator
                f.write(f"    /* Row {y+1}/480 */\n")
        
        f.write("""
};

/* nn_rgb: 128x128 RGB888 data from trump2.jpg */
const uint8_t dummy_test_nn_rgb[128 * 128 * 3] = {
""")
        
        # Write nn_rgb data (RGB888)
        for y in range(128):
            f.write("    ")
            for x in range(128):
                r, g, b = nn_rgb_array[y, x]
                if x == 127 and y == 127:
                    f.write(f"0x{r:02X}, 0x{g:02X}, 0x{b:02X}")
                else:
                    f.write(f"0x{r:02X}, 0x{g:02X}, 0x{b:02X}, ")
                if x % 4 == 3:  # New line every 4 RGB triplets
                    f.write("\n    ")
            if y % 10 == 9:  # Progress indicator
                f.write(f"    /* Row {y+1}/128 */\n")
        
        f.write("""
};
""")
    
    print(f"✅ Dual dummy buffers created:")
    print(f"   - img_buffer: {800 * 480 * 2:,} bytes (480x480 centered in 800x480 RGB565)")
    print(f"   - nn_rgb: {128 * 128 * 3:,} bytes (128x128 RGB888)")
    print(f"   - Total: {800 * 480 * 2 + 128 * 128 * 3:,} bytes")
    
    # Save preview images
    img_buffer_final = Image.fromarray(img_buffer_rgb, 'RGB')
    img_buffer_final.save("dummy_test_800x480_centered_preview.jpg", "JPEG", quality=95)
    nn_rgb.save("dummy_test_128x128_preview.jpg", "JPEG", quality=95)
    print("📸 Preview images saved: dummy_test_800x480_centered_preview.jpg, dummy_test_128x128_preview.jpg")

if __name__ == "__main__":
    create_dual_dummy_buffers()
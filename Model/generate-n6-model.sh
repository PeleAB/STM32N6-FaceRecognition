#!/bin/bash
set -eu

STEDGEAI="/c/ST/STEdgeAI/4.0/Utilities/windows/stedgeai.exe"

# Model 1: Face Detection (fd) — CenterFace
# Input: [1,3,128,128] float32 (NCHW) → uint8 for camera pipe
# Outputs: 4 heads (heatmap, scale, offset, landmarks) at 32x32 grid
"$STEDGEAI" generate --name fd \
  --no-inputs-allocation \
  --model centerface.tflite \
  --target stm32n6 \
  --st-neural-art fd@user_neuralart.json \
  --input-data-type uint8 --output-data-type float32
cp st_ai_output/fd_ecblobs.h .
cp st_ai_output/fd.c .
cp st_ai_output/stai_fd.c .
cp st_ai_output/stai_fd.h .
cp st_ai_output/fd_atonbuf.xSPI2.raw fd_data.xSPI2.bin
arm-none-eabi-objcopy -I binary fd_data.xSPI2.bin \
  --change-addresses 0x70380000 -O ihex fd_data.hex

# Model 2: Face ID embedding (faceid) — MobileFaceNet
# Input: [1,3,112,112] float32 (NCHW) → uint8 for cropped face ROI
# Output: [1,128] float32 embedding
"$STEDGEAI" generate --name faceid \
  --no-outputs-allocation \
  --model mobilefacenet_int8_faces.onnx \
  --target stm32n6 \
  --st-neural-art faceid@user_neuralart.json \
  --input-data-type uint8 --output-data-type float32
cp st_ai_output/faceid_ecblobs.h .
cp st_ai_output/faceid.c .
cp st_ai_output/stai_faceid.c .
cp st_ai_output/stai_faceid.h .
cp st_ai_output/faceid_atonbuf.xSPI2.raw faceid_data.xSPI2.bin
arm-none-eabi-objcopy -I binary faceid_data.xSPI2.bin \
  --change-addresses 0x72000000 -O ihex faceid_data.hex

# Legacy models (kept for reference, comment out to skip)
# # Model: Object Detection (od) — YOLO v2 Tiny
# "$STEDGEAI" generate --name od \
#   --no-inputs-allocation \
#   --model quantized_tiny_yolo_v2_224_.tflite \
#   --target stm32n6 \
#   --st-neural-art od@user_neuralart.json \
#   --input-data-type uint8 --output-data-type int8
#
# # Model: Re-Identification (reid) — MobileNetV2
# "$STEDGEAI" generate --name reid \
#   --no-outputs-allocation \
#   --model mobilenetv2_a100_256_128_fft_int8.tflite \
#   --target stm32n6 \
#   --st-neural-art reid@user_neuralart.json \
#   --input-data-type uint8 --output-data-type uint8

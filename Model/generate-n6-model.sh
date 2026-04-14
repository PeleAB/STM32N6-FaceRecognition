#!/bin/bash
set -eu

# Model 1: Object Detection (od)
"/c/ST/STEdgeAI/4.0/Utilities/windows/stedgeai.exe" generate --name od \
  --no-inputs-allocation \
  --model quantized_tiny_yolo_v2_224_.tflite \
  --target stm32n6 \
  --st-neural-art od@user_neuralart.json \
  --input-data-type uint8 --output-data-type int8
cp st_ai_output/od_ecblobs.h .
cp st_ai_output/od.c .
cp st_ai_output/stai_od.c .
cp st_ai_output/stai_od.h .
cp st_ai_output/od_atonbuf.xSPI2.raw od_data.xSPI2.bin
arm-none-eabi-objcopy -I binary od_data.xSPI2.bin \
  --change-addresses 0x70380000 -O ihex od_data.hex

# Model 2: Re-Identification (reid)
"/c/ST/STEdgeAI/4.0/Utilities/windows/stedgeai.exe" generate --name reid \
  --no-outputs-allocation \
  --model mobilenetv2_a100_256_128_fft_int8.tflite \
  --target stm32n6 \
  --st-neural-art reid@user_neuralart.json \
  --input-data-type uint8 --output-data-type uint8
cp st_ai_output/reid_ecblobs.h .
cp st_ai_output/reid.c .
cp st_ai_output/stai_reid.c .
cp st_ai_output/stai_reid.h .
cp st_ai_output/reid_atonbuf.xSPI2.raw reid_data.xSPI2.bin
arm-none-eabi-objcopy -I binary reid_data.xSPI2.bin \
  --change-addresses 0x72000000 -O ihex reid_data.hex
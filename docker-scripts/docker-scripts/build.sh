#!/bin/bash
# Build STM32N6 firmware
echo "Building STM32N6 firmware..."
docker-compose run --rm stm32-build

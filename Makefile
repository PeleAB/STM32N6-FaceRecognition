######################################
# helpers
######################################
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

######################################
# target
######################################
TARGET = Project
MODEL_DIR = Model
BINARY_DIR = Binary

######################################
# building variables
######################################
OPT = -Os -g3

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES += Src/main.c
C_SOURCES += Src/syscalls.c
C_SOURCES += Src/app_postprocess.c
C_SOURCES += Src/app_fuseprogramming.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/od_pp_yolov2.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/od_pp_yolov5.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/od_pp_yolov8.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/od_pp_st_yolox.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/od_pp_centernet.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/od_pp_ssd_st.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/od_pp_ssd.c
C_SOURCES += Middlewares/lib_vision_models_pp/lib_vision_models_pp/Src/vision_models_pp.c
C_SOURCES += Src/stm32_lcd_ex.c
C_SOURCES += Src/stm32n6xx_it.c
C_SOURCES += Middlewares/AI_Runtime/Npu/Devices/STM32N6XX/mcu_cache.c
C_SOURCES += Middlewares/AI_Runtime/Npu/Devices/STM32N6XX/npu_cache.c
C_SOURCES += Model/network.c
C_SOURCES += STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Source/Templates/system_stm32n6xx_fsbl.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_cortex.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_dcmipp.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_dma2d.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_gpio.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_i2c.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_i2c_ex.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_ltdc.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_ltdc_ex.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rif.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_ramcfg.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_cacheaxi.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pwr.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pwr_ex.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rcc.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rcc_ex.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_xspi.c
C_SOURCES += STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_bsec.c
C_SOURCES += STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.c
C_SOURCES += STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.c
C_SOURCES += Middlewares/Camera_Middleware/cmw_camera.c
C_SOURCES += Middlewares/Camera_Middleware/cmw_utils.c
C_SOURCES += STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_lcd.c
C_SOURCES += STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.c
C_SOURCES += STM32Cube_FW_N6/Drivers/BSP/Components/aps256xx/aps256xx.c
C_SOURCES += STM32Cube_FW_N6/Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.c
C_SOURCES += Middlewares/Camera_Middleware/sensors/imx335/imx335.c
C_SOURCES += Middlewares/Camera_Middleware/sensors/imx335/imx335_reg.c
C_SOURCES += Middlewares/Camera_Middleware/sensors/vd6g/vd6g.c
C_SOURCES += Middlewares/Camera_Middleware/sensors/vd55g1/vd55g1.c
C_SOURCES += Middlewares/Camera_Middleware/sensors/cmw_vd55g1.c
C_SOURCES += Middlewares/Camera_Middleware/sensors/cmw_vd66gy.c
C_SOURCES += Middlewares/Camera_Middleware/sensors/cmw_imx335.c
C_SOURCES += Src/crop_img.c
C_SOURCES += Src/app_cam.c
C_SOURCES += Middlewares/Camera_Middleware/ISP_Library/isp/Src/isp_algo.c
C_SOURCES += Middlewares/Camera_Middleware/ISP_Library/isp/Src/isp_cmd_parser.c
C_SOURCES += Middlewares/Camera_Middleware/ISP_Library/isp/Src/isp_core.c
C_SOURCES += Middlewares/Camera_Middleware/ISP_Library/isp/Src/isp_services.c
C_SOURCES += Middlewares/Camera_Middleware/ISP_Library/isp/Src/isp_tool_com.c
C_SOURCES += STM32Cube_FW_N6/Utilities/lcd/stm32_lcd.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_aton.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_aton_rt_main.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_aton_runtime.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_aton_util.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_sw_float.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_sw_integer.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_aton_lib.c
C_SOURCES += Middlewares/AI_Runtime/Npu/ll_aton/ll_aton_lib_sw_operators.c


# ASM sources
ASM_SOURCES += STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Source/Templates/gcc/startup_stm32n657xx_fsbl.s


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
READELF = $(GCC_PATH)/$(PREFIX)readelf
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
CP = $(PREFIX)objcopy
READELF = $(PREFIX)readelf
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary
FLASHER = STM32_Programmer_CLI
SIGNER = STM32_SigningTool_CLI
OBJCOPY = arm-none-eabi-objcopy
EL = "$(shell dirname "$(shell which $(FLASHER))")/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr"

#######################################
# CFLAGS
#######################################
CPU = -mcpu=cortex-m55 -mcmse -mthumb
FPU = -mfpu=fpv5-d16 -mfloat-abi=hard

# mcu
MCU = $(CPU) $(FPU)

# C defines
C_DEFS += -DSTM32N657xx
C_DEFS += -DUSE_FULL_ASSERT
C_DEFS += -DUSE_FULL_LL_DRIVER
C_DEFS += -DVECT_TAB_SRAM
C_DEFS += -DLL_ATON_DUMP_DEBUG_API
C_DEFS += -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6
C_DEFS += -DLL_ATON_OSAL=LL_ATON_OSAL_BARE_METAL
C_DEFS += -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC
C_DEFS += -DLL_ATON_SW_FALLBACK
C_DEFS += -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1

# C includes
# Patched files
C_INCLUDES += -IMiddlewares/Camera_Middleware
C_INCLUDES += -IMiddlewares/Camera_Middleware/sensors
C_INCLUDES += -IMiddlewares/Camera_Middleware/sensors/imx335
C_INCLUDES += -IMiddlewares/Camera_Middleware/sensors/vd55g1
C_INCLUDES += -IMiddlewares/Camera_Middleware/sensors/vd6g

C_INCLUDES += -IInc
C_INCLUDES += -IMiddlewares/lib_vision_models_pp/lib_vision_models_pp/Inc
C_INCLUDES += -IMiddlewares/AI_Runtime/Npu/ll_aton
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Inc
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Inc/Legacy
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Include
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/CMSIS/Include
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/CMSIS/DSP/Include
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/BSP/Components/Common
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK
C_INCLUDES += -IMiddlewares/Camera_Middleware/ISP_Library/evision/Inc
C_INCLUDES += -IMiddlewares/Camera_Middleware/ISP_Library/isp/Inc
C_INCLUDES += -IMiddlewares/AI_Runtime/Inc
C_INCLUDES += -IMiddlewares/AI_Runtime/Npu/Devices/STM32N6XX
C_INCLUDES += -ISTM32Cube_FW_N6/Utilities/lcd
C_INCLUDES += -ISTM32Cube_FW_N6/Drivers/BSP/Components/aps256xx

ifneq ($(REV_BOARD),C01)
C_DEFS += -DSTM32N6XX_SI_CUT1_1
AS_DEFS += -DSTM32N6XX_SI_CUT1_1
endif

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fstack-usage -fdata-sections -ffunction-sections -fcyclomatic-complexity
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fstack-usage -fdata-sections -ffunction-sections -fcyclomatic-complexity
# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

CFLAGS += -std=gnu11

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32CubeIDE/STM32N657xx.ld

# libraries
LIBS = -lc -lm -lnosys -lNetworkRuntime1000_CM55_GCC -ln6-evision-awb_gcc -ln6-evision-st-ae_gcc
LIBDIR = -LMiddlewares/AI_Runtime/Lib/GCC/ARMCortexM55
LIBDIR += -LMiddlewares/Camera_Middleware/ISP_Library/evision/Lib
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
# Uncomment to enable %f formatted output
LDFLAGS += -u _printf_float
LDFLAGS += -Wl,--print-memory-usage
# Avoid 'build/Project.elf has a LOAD segment with RWX permissions' warning
LDFLAGS += -Wl,--no-warn-rwx-segments

# default action: build all
.PHONY: all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

#######################################
# build the application
#######################################
OBJECTS = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES_S:.S=.o))


$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).list: $(OBJECTS)
	$(file > $@, $(OBJECTS))

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile $(BUILD_DIR)/$(TARGET).list
	$(CC) @$(BUILD_DIR)/$(TARGET).list $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir -p $@

#######################################
# clean up
#######################################

.PHONY: clean
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# flash
#######################################

.PHONY: flash
flash: $(BUILD_DIR)/$(TARGET)_sign.bin
	$(FLASHER) -c port=SWD mode=HOTPLUG -el $(EL) -hardRst -w $< 0x70100000
	@echo FLASH $<

.PHONY: flash_weights
flash_weights: $(MODEL_DIR)/network_data.hex
	$(FLASHER) -c port=SWD mode=HOTPLUG -el $(EL) -hardRst -w $<


$(BUILD_DIR)/$(TARGET)_sign.bin: $(BUILD_DIR)/$(TARGET).bin
	$(SIGNER) -s -bin $< -nk -t ssbl -hv 2.3 -o $(BUILD_DIR)/$(TARGET)_sign.bin

#######################################
# dependencies
#######################################
-include $(call rwildcard,$(BUILD_DIR),*.d)

AI_REL_DIR := Lib/AI_Runtime
PP_REL_DIR := Lib/lib_vision_models_pp/lib_vision_models_pp
PPW_REL_DIR := Lib/ai-postprocessing-wrapper

C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton_osal_freertos.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton_debug.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton_lib.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton_lib_sw_operators.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton_rt_main.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton_runtime.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_aton_util.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_sw_float.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ll_sw_integer.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/ll_aton/ecloader.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/Devices/STM32N6XX/npu_cache.c
C_SOURCES_AI += $(AI_REL_DIR)/Npu/Devices/STM32N6XX/mcu_cache.c

C_SOURCES_AI += $(PP_REL_DIR)/Src/vision_models_pp.c
C_SOURCES_AI += $(PP_REL_DIR)/Src/od_pp_yolov2.c
C_SOURCES_AI += $(PP_REL_DIR)/Src/od_pp_yolov5.c
C_SOURCES_AI += $(PP_REL_DIR)/Src/od_pp_yolov8.c
C_SOURCES_AI += $(PP_REL_DIR)/Src/vision_models_pp_maxi_if32.c
C_SOURCES_AI += $(PP_REL_DIR)/Src/vision_models_pp_maxi_is8.c
C_SOURCES_AI += $(PP_REL_DIR)/Src/vision_models_pp_maxi_iu8.c

C_SOURCES_AI += $(wildcard $(PPW_REL_DIR)/*.c)

C_INCLUDES_AI += -I$(AI_REL_DIR)/Inc
C_INCLUDES_AI += -I$(AI_REL_DIR)/Npu/ll_aton
C_INCLUDES_AI += -I$(AI_REL_DIR)/Npu/Devices/STM32N6XX

C_INCLUDES_AI += -I$(PP_REL_DIR)/Inc

C_INCLUDES_AI += -I$(PPW_REL_DIR)

C_DEFS_AI += -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6
C_DEFS_AI += -DLL_ATON_OSAL=LL_ATON_OSAL_FREERTOS
C_DEFS_AI += -DLL_ATON_RT_MODE=LL_ATON_RT_ASYNC
C_DEFS_AI += -DLL_ATON_SW_FALLBACK
C_DEFS_AI += -DLL_ATON_DBG_BUFFER_INFO_EXCLUDED=1
C_DEFS_AI += -DAPP_HAS_PARALLEL_NETWORKS=1

C_SOURCES += $(C_SOURCES_AI)
C_INCLUDES += $(C_INCLUDES_AI)
C_DEFS += $(C_DEFS_AI)
LIBDIR += -L$(AI_REL_DIR)/Lib/GCC/ARMCortexM55
LIBS += -l:NetworkRuntime1020_CM55_GCC.a
LIBS_IAR += $(AI_REL_DIR)/Lib/ABI2.1/ARMCortexM55/NetworkRuntime1020_CM55_IAR.a

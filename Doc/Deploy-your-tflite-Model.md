# Deploy your tflite Model

This documentation is an example to show you how to deploy your own Tiny YOLO v2 object detection tflite model. The model of this example is a Tiny YOLO v2 224x224 people detection. Please adapt it to your own model/use case.
In order to run your own model you need to follow these steps:

- [1. Generate c-model from tflite model](#1-generate-c-model-from-tflite-model)
- [2. Program your network data](#2-program-your-network-data)
- [3. Provide NN Information](#3-provide-nn-information)
  - [Post process type](#post-process-type)
  - [NN Size info](#nn-size-info)
  - [Class labels](#class-labels)
  - [Configure post process parameters](#configure-post-process-parameters)
- [4. Build Application](#4-build-application)
- [5. Run Application](#5-run-application)
  - [Dev mode](#dev-mode)
  - [Boot from flash](#boot-from-flash)

## 1. Generate c-model from tflite model

In order to generate the `Model/network.c` and the file containing the network parameters you must install STM32Cube.AI v10.0.0 or later as well as STM32CubeIDE v1.17 or later.

1. Add `/c/\<InstallFolder>/Utilities/windows/` in your path to have stedgeai known by your bash.

2. Add `/c/\<InstallFolder>/STM32CubeIDE_<X.X.X>/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.<X.X.X>/bin/` in your path to have arm-none-eabi-objcopy known by your bash.

3. Put you Tiny YOLO v2 tflite model in the `Model` folder.

```bash
cd Model
stedgeai generate --model Model_File.tflite --target stm32n6 --st-neural-art default@user_neuralart.json
cp st_ai_output/network.c .
cp st_ai_output/network_atonbuf.xSPI2.raw network_data.xSPI2.bin
arm-none-eabi-objcopy -I binary network_data.xSPI2.bin --change-addresses 0x70380000 -O ihex network_data.hex
```

You can find the following script at [Model/generate-n6-model.sh](../Model/generate-n6-model.sh)

## 2. Program your network data

Now You can program your network data in external flash.

See [Program app in external flash](../README.md#program-app-in-external-flash) for detailed info on how to program network data.

```bash
export DKEL="<STM32CubeProgrammer_N6 Install Folder>/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr"

# weights and params
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -el $DKEL -hardRst -w Model/network_data.hex
```

## 3. Provide NN Information

You need to edit the last lines of [Inc/app_config.h](../Inc/app_config.h) to make the app compatible with your network.

### Post process type

Set the postprocess type to YOLO v2:

```C
#define POSTPROCESS_TYPE POSTPROCESS_OD_YOLO_V2_UF

```

### NN Size info

Edit your NN_WIDTH, NN_HEIGHT to match your network input size:

```C
#define NN_WIDTH 224
#define NN_HEIGHT 224
```

__WARNING__: Only UINT8 format is supported.
__WARNING__: Only RGB888 format has been tested.

### Class labels

Modify the NB_CLASSES and classes_table to fit your own class labels:

```C
#define NB_CLASSES 2
static const char* classes_table[NB_CLASSES] = {
    "person",
    "not_person",
};
```

### Configure post process parameters

A documentation of the post processing library is available at [Postprocess lib](../Lib/lib_vision_models_pp/lib_objdetect_pp/README.md).

You can edit IOU thresholds, anchors values and other parameters.

```C
/* I/O configuration */
#define AI_OBJDETECT_YOLOV2_PP_NB_CLASSES        (1)
#define AI_OBJDETECT_YOLOV2_PP_NB_ANCHORS        (5)
#define AI_OBJDETECT_YOLOV2_PP_GRID_WIDTH        (7)
#define AI_OBJDETECT_YOLOV2_PP_GRID_HEIGHT       (7)
#define AI_OBJDETECT_YOLOV2_PP_NB_INPUT_BOXES    (AI_OBJDETECT_YOLOV2_PP_GRID_WIDTH * AI_OBJDETECT_YOLOV2_PP_GRID_HEIGHT)

/* Anchor boxes */
static const float32_t AI_OBJDETECT_YOLOV2_PP_ANCHORS[2*AI_OBJDETECT_YOLOV2_PP_NB_ANCHORS] = {
    0.9883000000f,     3.3606000000f,
    2.1194000000f,     5.3759000000f,
    3.0520000000f,     9.1336000000f,
    5.5517000000f,     9.3066000000f,
    9.7260000000f,     11.1422000000f,
  };

/* Postprocessing */
#define AI_OBJDETECT_YOLOV2_PP_CONF_THRESHOLD    (0.6f)
#define AI_OBJDETECT_YOLOV2_PP_IOU_THRESHOLD     (0.3f)
#define AI_OBJDETECT_YOLOV2_PP_MAX_BOXES_LIMIT   (10)
```

The list of all the defines needed by each postprocessing type is available in the [Postprocess defines documentation](Postprocess-Defines.md).

## 4. Build Application

Once your network data has been programed (step 2) and network details configured (step 3) you can build your application following this guide:

[Application build and run](../README.md#application-build-and-run---dev-mode)

## 5. Run Application

### Dev mode

See [Application build and run](../README.md#application-build-and-run---dev-mode) section for more details.

### Boot from flash

See [Program app in external flash](../README.md#application-build-and-run---boot-from-flash) to program your firmware.

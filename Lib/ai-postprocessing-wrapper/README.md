# AI Postprocessing Wrapper

The Ai Postprocessing Wrapper is made to be used with the lib_vision_models_pp.
It allows to simplify the usage of it in the use case of app using multiple types of post processing.

## Postprocessing introduction

Each app_postprocess_\<modeltype\>.c implement these functions:

```C
int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
```

```C
int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
```

To enable the post processing you need to define in a file `app_config.h` the define `POSTPROCESS_TYPE` with one of this value:

```C
#define POSTPROCESS_OD_YOLO_V2_UF       (100)  /* Yolov2 postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_OD_YOLO_V2_UI       (101)  /* Yolov2 postprocessing; Input model: uint8; output: int8            */
#define POSTPROCESS_OD_YOLO_V5_UU       (102)  /* Yolov5 postprocessing; Input model: uint8; output: uint8           */
#define POSTPROCESS_OD_YOLO_V8_UF       (103)  /* Yolov8 postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_OD_YOLO_V8_UI       (104)  /* Yolov8 postprocessing; Input model: uint8; output: int8            */
#define POSTPROCESS_OD_ST_YOLOX_UF      (105)  /* ST YoloX postprocessing; Input model: uint8; output: float32       */
#define POSTPROCESS_OD_ST_YOLOX_UI      (106)  /* ST YoloX postprocessing; Input model: uint8; output: int8          */
#define POSTPROCESS_OD_ST_SSD_UF        (107)  /* ST SSD postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_OD_FD_BAZEFACE_UF   (110)  /* blazeface postprocessing; Input model: uint8; output: float32      */
#define POSTPROCESS_OD_FD_BAZEFACE_UU   (111)  /* blazeface postprocessing; Input model: uint8; output: uint8        */
#define POSTPROCESS_OD_FD_BAZEFACE_UI   (112)  /* blazeface postprocessing; Input model: uint8; output: int8         */
#define POSTPROCESS_MPE_YOLO_V8_UF      (200)  /* Yolov8 postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_MPE_YOLO_V8_UI      (201)  /* Yolov8 postprocessing; Input model: uint8; output: int8            */
#define POSTPROCESS_MPE_PD_UF           (202)  /* Palm detector postprocessing; Input model: uint8; output: float32  */
#define POSTPROCESS_SPE_MOVENET_UF      (203)  /* Movenet postprocessing; Input model: uint8; output: float32        */
#define POSTPROCESS_SPE_MOVENET_UI      (204)  /* Movenet postprocessing; Input model: uint8; output: int8           */
#define POSTPROCESS_ISEG_YOLO_V8_UI     (300)  /* Yolov8 Seg postprocessing; Input model: uint8; output: int8        */
#define POSTPROCESS_SSEG_DEEPLAB_V3_UF  (400)  /* Deeplabv3 Seg postprocessing; Input model: uint8; output: float32  */
#define POSTPROCESS_SSEG_DEEPLAB_V3_UI  (401)  /* Deeplabv3 Seg postprocessing; Input model: uint8; output: int8     */
#define POSTPROCESS_CUSTOM              (1000) /* Custom post processing which needs to be implemented by user       */
```

Refer to the [app_postprocess.h](./app_postprocess.h) file for more details.

In the `app_config.h` file, you also need to add post-processing defines specific to the neural network model. These defines are used to extract bounding boxes, class labels, confidence scores, and other relevant information from the output of the neural network. More information about the supported models can be found in the [Postprocess library README](../lib_vision_models_pp/lib_vision_models_pp/README.md).

To simplify the manual configuration of post-processing parameters, this wrapper provides a set of defines that can be used to configure various parameters such as the number of classes, the number of anchors, the grid size, and the number of input boxes.

These defines need to be specified in the `app_config.h` file according to the specific post-processing type being used.

## Postprocess defines

### Object detection

#### Tiny YOLO v2

To use the Tiny YOLO v2 postprocessing compile one of these files:

- `app_postprocess_od_yolov2_uf.c`: input uint8 ; output float
- `app_postprocess_od_yolov2_ui.c`: input uint8 ; output int8

For more details about these parameters, see [Tiny YOLOV2 Object Detection Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#tiny-yolov2-object-detection-post-processing).

Example for Tiny YOLO V2 224x224 people detection:

```C
#define POSTPROCESS_TYPE POSTPROCESS_OD_YOLO_V2_UI

/* I/O configuration */
#define AI_OD_YOLOV2_PP_NB_CLASSES        (1)
#define AI_OD_YOLOV2_PP_NB_ANCHORS        (5)
#define AI_OD_YOLOV2_PP_GRID_WIDTH        (7)
#define AI_OD_YOLOV2_PP_GRID_HEIGHT       (7)
#define AI_OD_YOLOV2_PP_NB_INPUT_BOXES    (AI_OD_YOLOV2_PP_GRID_WIDTH * AI_OD_YOLOV2_PP_GRID_HEIGHT)

/* Anchor boxes */
static const float32_t AI_OD_YOLOV2_PP_ANCHORS[2*AI_OD_YOLOV2_PP_NB_ANCHORS] = {
    0.9883000000f,     3.3606000000f,
    2.1194000000f,     5.3759000000f,
    3.0520000000f,     9.1336000000f,
    5.5517000000f,     9.3066000000f,
    9.7260000000f,     11.1422000000f,
  };

/* Postprocessing */
#define AI_OD_YOLOV2_PP_CONF_THRESHOLD    (0.6f)
#define AI_OD_YOLOV2_PP_IOU_THRESHOLD     (0.3f)
#define AI_OD_YOLOV2_PP_MAX_BOXES_LIMIT   (10)
```

#### YOLOv5

To use the YoloV5 postprocessing compile this file:
`app_postprocess_od_yolov5_uu.c`

For more details about these parameters, see [YOLOV5 Object Detection Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#yolov5-object-detection-post-processing).

Example for YOLOV5 people detection:

```C
#define POSTPROCESS_TYPE POSTPROCESS_OD_YOLO_V5_UU

/* I/O configuration */
#define AI_OD_YOLOV5_PP_TOTAL_BOXES       (6300)
#define AI_OD_YOLOV5_PP_NB_CLASSES        (1)

/* Postprocessing */
#define AI_OD_YOLOV5_PP_CONF_THRESHOLD    (0.2000000000f)
#define AI_OD_YOLOV5_PP_IOU_THRESHOLD     (0.5000000000f)
#define AI_OD_YOLOV5_PP_MAX_BOXES_LIMIT   (10)
#define AI_OD_YOLOV5_PP_ZERO_POINT        (0)
#define AI_OD_YOLOV5_PP_SCALE             (0.0039239008910954f)
```

WARNING: Yolov5nu models has yolov8 post processing see [YOLOv8](#yolov8).

#### Object Detection YOLOv8

To use the Yolov8/yolov5nu postprocessing compile one of these files:

- `app_postprocess_od_yolov8_uf.c`: input uint8 ; output float
- `app_postprocess_od_yolov8_ui.c`: input uint8 ; output int8

For more details about these parameters, see [YOLOV8 Object Detection Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#yolov8-object-detection-post-processing).

Example for YOLOv8 256x256 people detection:

```C
#define POSTPROCESS_TYPE POSTPROCESS_OD_YOLO_V8_UF // POSTPROCESS_OD_YOLO_V8_UI if int8 as input of the post proc

/* I/O configuration */
#define AI_OD_YOLOV8_PP_TOTAL_BOXES       (1344)
#define AI_OD_YOLOV8_PP_NB_CLASSES        (1)

/* Postprocessing */
#define AI_OD_YOLOV8_PP_CONF_THRESHOLD    (0.4000000000f)
#define AI_OD_YOLOV8_PP_IOU_THRESHOLD     (0.5000000000f)
#define AI_OD_YOLOV8_PP_MAX_BOXES_LIMIT   (10)
#define AI_OD_YOLOV8_PP_ZERO_POINT        (-128) /* To be commented for float input, else to be filled */
#define AI_OD_YOLOV8_PP_SCALE             (0.005200491286814213f) /* To be commented for float input, else to be filled */
```

#### ST YOLOX

To use the yolox postprocessing compile one of these files:

- `app_postprocess_od_st_yolox_uf.c`: input uint8 ; output float
- `app_postprocess_od_st_yolox_ui.c`: input uint8 ; output int8

For more details about these parameters, see [ST YOLOX Object Detection Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#st-yolox-object-detection-post-processing).

Example for ST YOLOX 256x256 people detection:

```C
#define POSTPROCESS_TYPE POSTPROCESS_OD_ST_YOLOX_UI

/* I/O configuration */
#define AI_OD_ST_YOLOX_PP_NB_CLASSES        (1)
#define AI_OD_ST_YOLOX_PP_L_GRID_WIDTH      (32)
#define AI_OD_ST_YOLOX_PP_L_GRID_HEIGHT     (32)
#define AI_OD_ST_YOLOX_PP_L_NB_INPUT_BOXES  (AI_OD_YOLOVX_PP_L_GRID_WIDTH * AI_OD_YOLOVX_PP_L_GRID_HEIGHT)
#define AI_OD_ST_YOLOX_PP_M_GRID_WIDTH      (16)
#define AI_OD_ST_YOLOX_PP_M_GRID_HEIGHT     (16)
#define AI_OD_ST_YOLOX_PP_M_NB_INPUT_BOXES  (AI_OD_YOLOVX_PP_M_GRID_WIDTH * AI_OD_YOLOVX_PP_M_GRID_HEIGHT)
#define AI_OD_ST_YOLOX_PP_S_GRID_WIDTH      (8)
#define AI_OD_ST_YOLOX_PP_S_GRID_HEIGHT     (8)
#define AI_OD_ST_YOLOX_PP_S_NB_INPUT_BOXES  (AI_OD_YOLOVX_PP_S_GRID_WIDTH * AI_OD_YOLOVX_PP_S_GRID_HEIGHT)
#define AI_OD_ST_YOLOX_PP_NB_ANCHORS        (1)

/* Anchor boxes */
static const float32_t AI_OD_ST_YOLOX_PP_L_ANCHORS[2*AI_OD_ST_YOLOX_PP_NB_ANCHORS] = {16.000000, 16.000000};
static const float32_t AI_OD_ST_YOLOX_PP_M_ANCHORS[2*AI_OD_ST_YOLOX_PP_NB_ANCHORS] = {8.000000, 8.000000};
static const float32_t AI_OD_ST_YOLOX_PP_S_ANCHORS[2*AI_OD_ST_YOLOX_PP_NB_ANCHORS] = {4.000000, 4.000000};

/* Postprocessing */
#define AI_OD_ST_YOLOX_PP_IOU_THRESHOLD      (0.5)
#define AI_OD_ST_YOLOX_PP_CONF_THRESHOLD     (0.6)
#define AI_OD_ST_YOLOX_PP_MAX_BOXES_LIMIT    (100)
```

#### ST SSD

To use the st ssd postprocessing compile this file:
`app_postprocess_od_st_ssd_uf.c`

For more details about these parameters, see [ST SSD Object Detection Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#st-ssd-object-detection-post-processing).

Example for ST SSD:

```C
#define POSTPROCESS_TYPE POSTPROCESS_OD_ST_SSD_UF

/* I/O configuration */
#define AI_OD_SSD_ST_PP_TOTAL_DETECTIONS            (3830)
#define AI_OD_SSD_ST_PP_NB_CLASSES                  (2)

/* Postprocessing */
#define AI_OD_SSD_ST_PP_CONF_THRESHOLD              (0.6f)
#define AI_OD_SSD_ST_PP_IOU_THRESHOLD               (0.3f)
#define AI_OD_SSD_ST_PP_MAX_BOXES_LIMIT             (100)
```

#### BLAZEFACE

To use the blazeface postprocessing compile one of these files:

- `app_postprocess_od_fd_blazeface_uf.c`: input uint8 ; output float
- `app_postprocess_od_fd_blazeface_ui.c`: input uint8 ; output int8
- `app_postprocess_od_fd_blazeface_ui.c`: input uint8 ; output uint8

For more details about these parameters, see [BLAZEFACE Face Detection Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#blazeface-face-detection-post-processing).

Example for BLAZEFACE 128x128 face detection:

```C
#define POSTPROCESS_TYPE POSTPROCESS_OD_FD_BLAZEFACE_UI

/* I/O configuration */
#define AI_OD_FD_BLAZEFACE_PP_NB_KEYPOINTS              (6)
#define AI_OD_FD_BLAZEFACE_PP_NB_CLASSES                (1)
#define AI_OD_FD_BLAZEFACE_PP_IMG_SIZE                (128)

#define AI_OD_FD_BLAZEFACE_PP_OUT_0_NB_BOXES          (512)
#define AI_OD_FD_BLAZEFACE_PP_OUT_1_NB_BOXES          (384)

/* Postprocessing */
#define AI_OD_FD_BLAZEFACE_PP_CONF_THRESHOLD    (0.6000000000f)
#define AI_OD_FD_BLAZEFACE_PP_IOU_THRESHOLD     (0.3000000000f)
#define AI_OD_FD_BLAZEFACE_PP_MAX_BOXES_LIMIT   (10)
```


### Pose estimation

#### Pose estimation YOLOv8

To use the multi pose estimation Yolov8 postprocessing compile one of these files:

- `app_postprocess_mpe_yolo_v8_uf.c`: input uint8 ; output float
- `app_postprocess_mpe_yolo_v8_ui.c`: input uint8 ; output int8

For more details about these parameters, see [YOLOV8 Multi-Pose Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#yolov8-multi-pose-post-processing).

Example for YOLOv8 pose 256x256:

```C
#define POSTPROCESS_TYPE POSTPROCESS_MPE_YOLO_V8_UI

/* I/O configuration */
#define AI_MPE_YOLOV8_PP_TOTAL_BOXES       (1344)
#define AI_MPE_YOLOV8_PP_NB_CLASSES        (1)
#define AI_MPE_YOLOV8_PP_KEYPOINTS_NB      (17)

/* Postprocessing */
#define AI_MPE_YOLOV8_PP_CONF_THRESHOLD    (0.7500000000f)
#define AI_MPE_YOLOV8_PP_IOU_THRESHOLD     (0.5000000000f)
#define AI_MPE_YOLOV8_PP_MAX_BOXES_LIMIT   (10)
```

#### MoveNet

To use the single pose estimation movenet postprocessing compile one of these files:

- `app_postprocess_spe_movenet_uf.c`: input uint8 ; output float
- `app_postprocess_spe_movenet_ui.c`: input uint8 ; output int8

For more details about these parameters, see [MoveNet Single-Pose Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#movenet-single-pose-post-processing).

Example for MoveNet 192x192 13 keypoints:

```C
#define POSTPROCESS_TYPE POSTPROCESS_SPE_MOVENET_UI

/* I/O configuration */
#define AI_SPE_MOVENET_POSTPROC_HEATMAP_WIDTH        (48)		/* Model input width/4 : 192/4  */
#define AI_SPE_MOVENET_POSTPROC_HEATMAP_HEIGHT       (48)		/* Model input height/4 : 192/4 */
#define AI_SPE_MOVENET_POSTPROC_NB_KEYPOINTS         (13)		/* Only 13 and 17 keypoints are supported for the skeleton reconstruction */
```

### Instance segmentation

#### YOLOv8 seg

To use the instance segmentation Yolov8 postprocessing compile this file:
`app_postprocess_iseg_yolo_v8_ui.c`

For more details about these parameters, see [YOLOV8 Instance Segmentation Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#yolov8-instance-segmentation-post-processing).

Example for YOLOv8 seg 256x256 COCO:

```C
#define POSTPROCESS_TYPE POSTPROCESS_ISEG_YOLO_V8_UI

/* I/O configuration */
#define AI_YOLOV8_SEG_PP_TOTAL_BOXES       (1344)
#define AI_YOLOV8_SEG_PP_NB_CLASSES        (80)
#define AI_YOLOV8_SEG_PP_MASK_NB           (32)
#define AI_YOLOV8_SEG_PP_MASK_SIZE         (64)

#define AI_YOLOV8_SEG_ZERO_POINT           (25)
#define AI_YOLOV8_SEG_SCALE                (0.020020058378577232f)
#define AI_YOLOV8_SEG_MASK_ZERO_POINT      (-115)
#define AI_YOLOV8_SEG_MASK_SCALE           (0.0207486841827631f)

/* Postprocessing */
#define AI_YOLOV8_SEG_PP_CONF_THRESHOLD        (0.5900000000f)
#define AI_YOLOV8_SEG_PP_IOU_THRESHOLD         (0.3900000000f)
#define AI_YOLOV8_SEG_PP_MAX_BOXES_LIMIT       (10)
```

### Semantic segmentation

#### DeepLab V3

To use the semantic segmentation deeplab v3 postprocessing compile one of these files:

- `app_postprocess_sseg_deeplab_v3_uf.c`: input uint8 ; output float
- `app_postprocess_sseg_deeplab_v3_ui.c`: input uint8 ; output int8

For more details about these parameters, see [YOLOV8 Instance Segmentation Post Processing](../lib_vision_models_pp/lib_vision_models_pp/README.md#yolov8-instance-segmentation-post-processing).

Example for Deep Lab V3 256x256 people segmentation:

```C
#define POSTPROCESS_TYPE POSTPROCESS_SSEG_DEEPLAB_V3_UI

#define AI_SSEG_DEEPLABV3_PP_NB_CLASSES   (2)
#define AI_SSEG_DEEPLABV3_PP_WIDTH        (256)
#define AI_SSEG_DEEPLABV3_PP_HEIGHT       (256)
```

### Custom post processing

To implement your own post-processing, you can implement the call to your APIs in this file:
`app_postprocess_template.c`

```C
#define POSTPROCESS_TYPE POSTPROCESS_CUSTOM

#define YOUR_DEFINES
```


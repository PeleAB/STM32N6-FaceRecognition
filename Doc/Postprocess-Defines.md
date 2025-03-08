# Postprocess Defines

## Postprocessing introduction

The postprocessing library is used to process the output of the neural network and extract the relevant information. It is specific to the neural network model and the task it is performing as it is used to extract the bounding boxes, class labels, confidence scores... From the output of the neural network. You can find more informations about the supported models in the [Postprocess library README](../Middlewares/lib_vision_models_pp/lib_vision_models_pp/README.md).

To ease the manual configuration of the postprocessing parameters, this file provides a set of defines that can be used to configure the postprocessing parameters. These defines are used to configure the number of classes, the number of anchors, the grid size, the number of input boxes, etc. They can be found in the [Inc/app_config.h](../Inc/app_config.h) file, and need to be replaced/modified according to the model you are using.

## Postprocess defines

### Object detection

#### Tiny YOLO v2

Values set for Tiny YOLO V2 224x224 people detection:

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

#### YOLOv5

```C
/* I/O configuration */
#define AI_OD_YOLOV5_PP_TOTAL_BOXES       (6300)
#define AI_OD_YOLOV5_PP_NB_CLASSES        (1)

/* Postprocessing */
#define AI_OD_YOLOV5_PP_CONF_THRESHOLD    (0.2000000000f)
#define AI_OD_YOLOV5_PP_IOU_THRESHOLD     (0.5000000000f)
#define AI_OD_YOLOV5_PP_MAX_BOXES_LIMIT   (10)
#define AI_OD_YOLOV5_PP_ZERO_POINT        (0) /* To be commented for float input, else to be filled */
#define AI_OD_YOLOV5_PP_SCALE             (0.0039239008910954f) /* To be commented for float input, else to be filled */
```

#### YOLOv8

Values set for YOLOv8 256x256 people detection:

```C
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

Values set for ST YOLOX 256x256 people detection:

```C
/* I/O configuration */
#define AI_OBJDETECT_YOLOVX_PP_NB_CLASSES        (1)
#define AI_OBJDETECT_YOLOVX_PP_L_GRID_WIDTH      (32)
#define AI_OBJDETECT_YOLOVX_PP_L_GRID_HEIGHT     (32)
#define AI_OBJDETECT_YOLOVX_PP_L_NB_INPUT_BOXES  (AI_OBJDETECT_YOLOVX_PP_L_GRID_WIDTH * AI_OBJDETECT_YOLOVX_PP_L_GRID_HEIGHT)
#define AI_OBJDETECT_YOLOVX_PP_M_GRID_WIDTH      (16)
#define AI_OBJDETECT_YOLOVX_PP_M_GRID_HEIGHT     (16)
#define AI_OBJDETECT_YOLOVX_PP_M_NB_INPUT_BOXES  (AI_OBJDETECT_YOLOVX_PP_M_GRID_WIDTH * AI_OBJDETECT_YOLOVX_PP_M_GRID_HEIGHT)
#define AI_OBJDETECT_YOLOVX_PP_S_GRID_WIDTH      (8)
#define AI_OBJDETECT_YOLOVX_PP_S_GRID_HEIGHT     (8)
#define AI_OBJDETECT_YOLOVX_PP_S_NB_INPUT_BOXES  (AI_OBJDETECT_YOLOVX_PP_S_GRID_WIDTH * AI_OBJDETECT_YOLOVX_PP_S_GRID_HEIGHT)
#define AI_OBJDETECT_YOLOVX_PP_NB_ANCHORS        (1)

/* Anchor boxes */
static const float32_t AI_OBJDETECT_YOLOVX_PP_L_ANCHORS[2*AI_OBJDETECT_YOLOVX_PP_NB_ANCHORS] = {16.000000, 16.000000};
static const float32_t AI_OBJDETECT_YOLOVX_PP_M_ANCHORS[2*AI_OBJDETECT_YOLOVX_PP_NB_ANCHORS] = {8.000000, 8.000000};
static const float32_t AI_OBJDETECT_YOLOVX_PP_S_ANCHORS[2*AI_OBJDETECT_YOLOVX_PP_NB_ANCHORS] = {4.000000, 4.000000};

/* Postprocessing */
#define AI_OBJDETECT_YOLOVX_PP_IOU_THRESHOLD      (0.5)
#define AI_OBJDETECT_YOLOVX_PP_CONF_THRESHOLD     (0.6)
#define AI_OBJDETECT_YOLOVX_PP_MAX_BOXES_LIMIT    (100)
```

#### Standard SSD

```C
/* I/O configuration */
#define AI_OD_SSD_PP_XY_SCALE                    (10.0)
#define AI_OD_SSD_PP_WH_SCALE                    (5.0)
#define AI_OD_SSD_PP_TOTAL_DETECTIONS            (1917)
#define AI_OD_SSD_PP_NB_CLASSES                  (91)

/* Postprocessing */
#define AI_OD_SSD_PP_CONF_THRESHOLD              (0.5f)
#define AI_OD_SSD_PP_IOU_THRESHOLD               (0.4f)
#define AI_OD_SSD_PP_MAX_BOXES_LIMIT             (100)
```

#### ST SSD

```C
/* I/O configuration */
#define AI_OD_SSD_ST_PP_TOTAL_DETECTIONS            (3830)
#define AI_OD_SSD_ST_PP_NB_CLASSES                  (2)

/* Postprocessing */
#define AI_OD_SSD_ST_PP_CONF_THRESHOLD              (0.6f)
#define AI_OD_SSD_ST_PP_IOU_THRESHOLD               (0.3f)
#define AI_OD_SSD_ST_PP_MAX_BOXES_LIMIT             (100)
```

### Pose estimation

#### YOLOv8

Values set for YOLOv8 pose 256x256:

```C
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

Values set for MoveNet 192x192 13 keypoints:

```C
/* I/O configuration */
#define AI_SPE_MOVENET_POSTPROC_HEATMAP_WIDTH        (48)		/* Model input width/4 : 192/4  */
#define AI_SPE_MOVENET_POSTPROC_HEATMAP_HEIGHT       (48)		/* Model input height/4 : 192/4 */
#define AI_SPE_MOVENET_POSTPROC_NB_KEYPOINTS         (13)		/* Only 13 and 17 keypoints are supported for the skeleton reconstruction */
```

### Instance segmentation

#### YOLOv8 seg

Values set for YOLOv8 seg 256x256 COCO:

```C
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

Values set for Deep Lab V3 256x256 people segmentation:

```C
#define AI_SSEG_DEEPLABV3_PP_NB_CLASSES   (2)
#define AI_SSEG_DEEPLABV3_PP_WIDTH        (256)
#define AI_SSEG_DEEPLABV3_PP_HEIGHT       (256)
```

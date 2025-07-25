 /**
 ******************************************************************************
 * @file    main.c
 * @author  PeleAB
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#include "app_config.h"
#include "cmw_camera.h"
#include "stm32n6570_discovery_bus.h"
#include "stm32n6570_discovery_lcd.h"
#include "stm32n6570_discovery_xspi.h"
#include "stm32n6570_discovery.h"
#include "stm32_lcd.h"
#include "app_fuseprogramming.h"
#include "stm32_lcd_ex.h"
#include "app_postprocess.h"
#include "ll_aton_runtime.h"
#include "app_cam.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "stm32n6xx_hal_rif.h"
#include "app_system.h"
#include "nn_runner.h"
#include "enhanced_pc_stream.h"

#include "crop_img.h"
#include "display_utils.h"
#include "img_buffer.h"
#include "system_utils.h"
#include "face_utils.h"
#include "target_embedding.h"
#include "app_constants.h"
#include "app_config_manager.h"
#include "memory_pool.h"
#include "app_neural_network.h"
#include "app_frame_processing.h"

/* Legacy compatibility - constants moved to app_constants.h */
#define REVERIFY_INTERVAL_MS        FACE_REVERIFY_INTERVAL_MS
#define MAX_NUMBER_OUTPUT           NN_MAX_OUTPUT_BUFFERS
#define FR_WIDTH                    FACE_RECOGNITION_WIDTH
#define FR_HEIGHT                   FACE_RECOGNITION_HEIGHT
#define SIMILARITY_THRESHOLD        FACE_SIMILARITY_THRESHOLD
#define LONG_PRESS_MS               BUTTON_LONG_PRESS_DURATION_MS

/* Neural Network Context Structure */
typedef struct {
    /* Face Detection Network */
    uint8_t *detection_input_buffer;
    float32_t *detection_output_buffers[MAX_NUMBER_OUTPUT];
    int32_t detection_output_lengths[MAX_NUMBER_OUTPUT];
    uint32_t detection_input_length;
    int detection_output_count;
    
    /* Face Recognition Network */
    uint8_t *recognition_input_buffer;
    float32_t *recognition_output_buffer;
    uint32_t recognition_input_length;
    uint32_t recognition_output_length;
    
    /* Network Instance References */
    bool detection_initialized;
    bool recognition_initialized;
} nn_context_t;

/* ========================================================================= */
/* SIMPLE TARGET DETECTION APPROACH                                        */
/* ========================================================================= */

/**
 * @brief Pixel coordinates structure
 */
typedef struct {
    float cx, cy, w, h, lx, ly, rx, ry;
} pixel_coords_t;

/* Simplified Application State Machine - No Tracking */
typedef enum {
    PIPE_STATE_DETECT_AND_VERIFY = 0  /* Single state: detect faces and verify immediately */
} pipe_state_t;

/**
 * @brief Enhanced application context structure
 */
typedef struct {
    /* Centralized Neural Network Context */
    nn_context_t nn_ctx;
    
    /* Post-processing */
    pd_model_pp_static_param_t pp_params;
    pd_postprocess_out_t pp_output;
    
    /* Configuration management */
    app_config_t config;                    /**< Application configuration */
    
    /* Frame processing pipeline */
    frame_processing_context_t frame_ctx;   /**< Frame processing context */
    
    /* State Management - Simplified */
    pipe_state_t pipe_state;                /**< Current pipeline state */
    
    /* Current Frame Results */
    pd_pp_box_t best_detection;             /**< Best face detection this frame */
    float current_similarity;               /**< Current face similarity score */
    bool face_detected;                     /**< Face detected in current frame */
    bool face_verified;                     /**< Face verified in current frame */
    
    /* Simple Target Detection History */
    bool target_detection_history[5];       /**< Last 5 frames target detection status */
    uint32_t history_index;                 /**< Current index in circular buffer */
    uint32_t history_count;                 /**< Number of valid history entries */
    bool target_detected;                   /**< Target detected (3+ of last 5 frames) */
    
    /* LED Timeout Management */
    uint32_t last_stable_verification_ts;   /**< Timestamp of last stable verification */
    bool led_timeout_active;                /**< LED timeout status */
    
    /* Face Recognition */
    float current_embedding[EMBEDDING_SIZE]; /**< Current face embedding */
    int embedding_valid;                    /**< Embedding validity flag */
    
    /* User Interface */
    uint32_t button_press_ts;               /**< Button press timestamp */
    int prev_button_state;                  /**< Previous button state */
    
    /* Performance monitoring */
    performance_metrics_t performance;      /**< Performance metrics */
    uint32_t frame_count;                   /**< Frame counter */
} app_context_t;

/* Global Variables */
volatile int32_t cameraFrameReceived;

/* Global variables for cropped face display */
bool g_cropped_face_valid = false;
float g_current_similarity = 0.0f;

/* Optimized Memory Buffers - Using PSRAM for large buffers to reduce boot time */
__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
uint8_t nn_rgb[NN_WIDTH * NN_HEIGHT * NN_BPP];  /* 128x128x3 = 49KB */

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
uint8_t fr_rgb[FR_WIDTH * FR_HEIGHT * NN_BPP];  /* 112x112x3 = 37KB */

__attribute__ ((aligned (32)))
uint8_t dcmipp_out_nn[DCMIPP_OUT_NN_BUFF_LEN];  /* Camera output buffer */

#ifdef DUMMY_INPUT_BUFFER
/* ========================================================================= */
/* DUMMY INPUT BUFFER FOR TESTING                                           */
/* ========================================================================= */
/* This provides a real test image that students can use to                */
/* verify their implementations produce the expected output at each stage.   */
/* ========================================================================= */

#include "dummy_dual_buffer.h"

/**
 * @brief Load dual dummy buffers for consistent testing
 * @note This overrides both img_buffer and nn_rgb with test image data
 *       ensuring face detection and cropping coordinates are consistent
 */
static void load_dual_dummy_buffers(void)
{
    printf("Loading dual dummy buffers (test image)...\n");
    /* Load nn_rgb (128x128 RGB888) for neural network input */
    memcpy(nn_rgb, dummy_test_nn_rgb, DUMMY_TEST_NN_RGB_SIZE);
    /* Invalidate cache after loading PSRAM data */
    SCB_InvalidateDCache_by_Addr((uint32_t*)nn_rgb, DUMMY_TEST_NN_RGB_SIZE);
    printf("   nn_rgb: 128x128 RGB888 (%d bytes)\n", DUMMY_TEST_NN_RGB_SIZE);
    
    printf("Dual dummy buffers loaded: consistent test data for detection + cropping\n");
}
#endif /* DUMMY_INPUT_BUFFER */

/* Application Context */
static app_context_t g_app_ctx = {
    .pipe_state = PIPE_STATE_DETECT_AND_VERIFY,
    .face_detected = false,
    .face_verified = false,
    .current_similarity = 0.0f,
    .embedding_valid = 0,
    .button_press_ts = 0,
    .prev_button_state = 0,
    .target_detection_history = {false},
    .history_index = 0,
    .history_count = 0,
    .target_detected = false,
    .last_stable_verification_ts = 0,
    .led_timeout_active = false
};


/* Function Prototypes */
static int nn_init_detection(nn_context_t *nn_ctx);
static int nn_init_recognition_lazy(nn_context_t *nn_ctx);
static void nn_cleanup(nn_context_t *nn_ctx);
static int app_init(app_context_t *ctx);
static int app_main_loop(app_context_t *ctx);
static void app_camera_init(uint32_t *pitch_nn);
static void app_display_init(void);
static void app_input_start(void);
static int  app_get_frame(uint8_t *dest, uint32_t pitch_nn);
static void app_output(pd_postprocess_out_t *res, uint32_t total_frame_time_ms, uint32_t boot_ms, const app_context_t *ctx);
static void handle_user_button(app_context_t *ctx);
static float verify_box(app_context_t *ctx, const pd_pp_box_t *box);
static void process_frame_detections(app_context_t *ctx, pd_pp_box_t *boxes, uint32_t box_count);
static void update_led_status(app_context_t *ctx);
static void update_target_detection_history(app_context_t *ctx, bool target_found_this_frame);
static void compute_target_detection_status(app_context_t *ctx);
static float run_face_recognition_on_face(app_context_t *ctx, const pd_pp_box_t *box);
static int convert_box_coordinates(const pd_pp_box_t *box, pixel_coords_t *pixel_coords);
static int crop_face_region(const pixel_coords_t *coords, uint8_t *output_buffer);
static float calculate_face_similarity(const float32_t *embedding, const float32_t *target_embedding, uint32_t embedding_size);
static void cleanup_nn_buffers(float32_t **nn_out, int32_t *nn_out_len, int number_output);

/* Neural Network Instance Declarations */
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(face_detection);
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(face_recognition);

/**
 * @brief Initialize face detection network only (for faster boot)
 * @param nn_ctx Neural network context to initialize
 * @return 0 on success, negative on error
 */
static int nn_init_detection(nn_context_t *nn_ctx)
{
    /* Clear context */
    memset(nn_ctx, 0, sizeof(*nn_ctx));
    
    /* Initialize Face Detection Network */
    const LL_Buffer_InfoTypeDef *detection_in_info = LL_ATON_Input_Buffers_Info_face_detection();
    const LL_Buffer_InfoTypeDef *detection_out_info = LL_ATON_Output_Buffers_Info_face_detection();
    
    if (!detection_in_info || !detection_out_info) {
        return -1; /* Failed to get buffer info */
    }
    
    /* Setup detection input buffer */
    nn_ctx->detection_input_buffer = (uint8_t *) LL_Buffer_addr_start(&detection_in_info[0]);
    nn_ctx->detection_input_length = LL_Buffer_len(&detection_in_info[0]);
    
    /* Setup detection output buffers */
    nn_ctx->detection_output_count = 0;
    while (detection_out_info[nn_ctx->detection_output_count].name != NULL && 
           nn_ctx->detection_output_count < MAX_NUMBER_OUTPUT) {
        nn_ctx->detection_output_buffers[nn_ctx->detection_output_count] = 
            (float32_t *) LL_Buffer_addr_start(&detection_out_info[nn_ctx->detection_output_count]);
        nn_ctx->detection_output_lengths[nn_ctx->detection_output_count] = 
            LL_Buffer_len(&detection_out_info[nn_ctx->detection_output_count]);
        nn_ctx->detection_output_count++;
    }
    
    nn_ctx->detection_initialized = true;
    
    printf("Face Detection Network Ready: %lu bytes, %d outputs\n", 
           nn_ctx->detection_input_length, nn_ctx->detection_output_count);
    
    return 0;
}

/**
 * @brief Initialize face recognition network lazily (when first face detected)
 * @param nn_ctx Neural network context to initialize
 * @return 0 on success, negative on error
 */
static int nn_init_recognition_lazy(nn_context_t *nn_ctx)
{
    if (nn_ctx->recognition_initialized) {
        return 0; /* Already initialized */
    }
    
    /* Initialize Face Recognition Network */
    const LL_Buffer_InfoTypeDef *recognition_in_info = LL_ATON_Input_Buffers_Info_face_recognition();
    const LL_Buffer_InfoTypeDef *recognition_out_info = LL_ATON_Output_Buffers_Info_face_recognition();
    
    if (!recognition_in_info || !recognition_out_info) {
        return -2; /* Failed to get face recognition buffer info */
    }
    
    /* Setup recognition buffers */
    nn_ctx->recognition_input_buffer = (uint8_t *) LL_Buffer_addr_start(&recognition_in_info[0]);
    nn_ctx->recognition_input_length = LL_Buffer_len(&recognition_in_info[0]);
    nn_ctx->recognition_output_buffer = (float32_t *) LL_Buffer_addr_start(&recognition_out_info[0]);
    nn_ctx->recognition_output_length = LL_Buffer_len(&recognition_out_info[0]);
    
    nn_ctx->recognition_initialized = true;
    
    printf("Face Recognition Network Loaded: %lu bytes -> %lu bytes\n", 
           nn_ctx->recognition_input_length, nn_ctx->recognition_output_length);
    
    return 0;
}

/**
 * @brief Clean up neural network resources
 * @param nn_ctx Neural network context to clean up
 */
static void nn_cleanup(nn_context_t *nn_ctx)
{
    if (nn_ctx && (nn_ctx->detection_initialized || nn_ctx->recognition_initialized)) {
        /* Clean up any network-specific resources if needed */
        memset(nn_ctx, 0, sizeof(*nn_ctx));
        printf("🧹 Neural Networks cleaned up\n");
    }
}
/* ========================================================================= */
/* FUNCTION IMPLEMENTATIONS                                                  */
/* ========================================================================= */
/**
 * @brief Initialize camera system only (optimized for concurrent startup)
 * @param pitch_nn Pointer to neural network pitch value
 */
static void app_camera_init(uint32_t *pitch_nn)
{
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
    CAM_Init(&lcd_bg_area.XSize, &lcd_bg_area.YSize, pitch_nn);
#else
    lcd_bg_area.XSize = NN_WIDTH;
    lcd_bg_area.YSize = NN_HEIGHT;
    (void)pitch_nn;
#endif
}

/**
 * @brief Initialize display system only (optimized for concurrent startup)
 */
static void app_display_init(void)
{
#ifdef ENABLE_LCD_DISPLAY
    LCD_init();
#endif
}

/**
 * @brief Start camera and display pipes (after both systems are initialized)
 */
static void app_input_start(void)
{
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
    CAM_DisplayPipe_Start(img_buffer, CMW_MODE_CONTINUOUS);
#endif
}

/**
 * @brief Capture frame from camera or PC stream
 * @param dest Destination buffer for frame data
 * @param pitch_nn Neural network pitch value
 * @return 0 on success, non-zero on failure
 */
static int app_get_frame(uint8_t *dest, uint32_t pitch_nn)
{
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
    CAM_IspUpdate();
    
    uint8_t *capture_buffer = (pitch_nn != (NN_WIDTH * NN_BPP)) ? dcmipp_out_nn : dest;
    CAM_NNPipe_Start(capture_buffer, CMW_MODE_SNAPSHOT);

    /* Optimized frame capture - reduced blocking time */
    while (cameraFrameReceived == 0) {
        /* Could add a timeout here for better responsiveness */
    }
    cameraFrameReceived = 0;
    SCB_InvalidateDCache_by_Addr(dest, NN_WIDTH * NN_HEIGHT * NN_BPP);

    return 0;
#else
    return PC_STREAM_ReceiveImage(dest, NN_WIDTH * NN_HEIGHT * NN_BPP);
#endif
}


/**
 * @brief Display network output results
 * @param res Post-processing results
 * @param inf_ms Inference time in milliseconds
 * @param boot_ms Boot time in milliseconds
 * @param ctx Application context with current frame results
 */
static void app_output(pd_postprocess_out_t *res, uint32_t total_frame_time_ms, uint32_t boot_ms, const app_context_t *ctx)
{
#if defined(ENABLE_PC_STREAM) || defined(ENABLE_LCD_DISPLAY)
    Display_NetworkOutput(res, total_frame_time_ms, boot_ms, ctx);
#else
    (void)res;
    (void)total_frame_time_ms;
    (void)boot_ms;
    (void)ctx;
#endif
}

/**
 * @brief Update target detection history with result from current frame
 * @param ctx Application context
 * @param target_found_this_frame True if any face above threshold was found this frame
 */
static void update_target_detection_history(app_context_t *ctx, bool target_found_this_frame)
{
    /* Add result to circular buffer */
    ctx->target_detection_history[ctx->history_index] = target_found_this_frame;
    ctx->history_index = (ctx->history_index + 1) % 5;
    
    /* Update count (max 5) */
    if (ctx->history_count < 5) {
        ctx->history_count++;
    }
}

/**
 * @brief Compute target detection status based on voting from last 5 frames
 * @param ctx Application context
 */
static void compute_target_detection_status(app_context_t *ctx)
{
    if (ctx->history_count == 0) {
        ctx->target_detected = false;
        return;
    }
    
    /* Count positive detections in history */
    uint32_t positive_detections = 0;
    for (uint32_t i = 0; i < ctx->history_count; i++) {
        if (ctx->target_detection_history[i]) {
            positive_detections++;
        }
    }
    
    /* Target detected if 3+ of last 5 frames had target above threshold */
    ctx->target_detected = (positive_detections >= 3);
}

/**
 * @brief Convert normalized coordinates to pixel coordinates
 * @param box Bounding box with normalized coordinates
 * @param pixel_coords Output pixel coordinates structure
 * @return 0 on success, negative on error
 */
static int convert_box_coordinates(const pd_pp_box_t *box, 
                                  pixel_coords_t *pixel_coords)
{
    if (!box || !pixel_coords) {
        return -1;
    }
    
    pixel_coords->cx = box->x_center * lcd_bg_area.XSize;
    pixel_coords->cy = box->y_center * lcd_bg_area.YSize;
    pixel_coords->w  = box->width  * lcd_bg_area.XSize * FACE_BBOX_PADDING_FACTOR;
    pixel_coords->h  = box->height * lcd_bg_area.YSize * FACE_BBOX_PADDING_FACTOR;
    pixel_coords->lx = box->pKps[0].x * lcd_bg_area.XSize;
    pixel_coords->ly = box->pKps[0].y * lcd_bg_area.YSize;
    pixel_coords->rx = box->pKps[1].x * lcd_bg_area.XSize;
    pixel_coords->ry = box->pKps[1].y * lcd_bg_area.YSize;
    
    return 0;
}

/**
 * @brief Crop face region from input image
 * @param coords Pixel coordinates structure
 * @param output_buffer Output buffer for cropped face
 * @return 0 on success, negative on error
 */
static int crop_face_region(const pixel_coords_t *coords,
                           uint8_t *output_buffer)
{
    if (!coords || !output_buffer) {
        return -1;
    }
    
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
#ifdef DUMMY_INPUT_BUFFER
    img_crop_align565_to_888(dummy_test_img_buffer, lcd_bg_area.XSize, output_buffer,
                            lcd_bg_area.XSize, lcd_bg_area.YSize,
                            FACE_RECOGNITION_WIDTH, FACE_RECOGNITION_HEIGHT,
                            coords->cx, coords->cy, coords->w, coords->h,
                            coords->lx, coords->ly, coords->rx, coords->ry);
#else
    img_crop_align565_to_888(img_buffer, lcd_bg_area.XSize, output_buffer,
                            lcd_bg_area.XSize, lcd_bg_area.YSize,
                            FACE_RECOGNITION_WIDTH, FACE_RECOGNITION_HEIGHT,
                            coords->cx, coords->cy, coords->w, coords->h, 
                            coords->lx, coords->ly, coords->rx, coords->ry);
#endif //DUMMY_INPUT_BUFFER

#else
    img_crop_align(nn_rgb, output_buffer,
                   NN_WIDTH, NN_HEIGHT,
                   FACE_RECOGNITION_WIDTH, FACE_RECOGNITION_HEIGHT, NN_BPP,
                   coords->cx, coords->cy, coords->w, coords->h, 
                   coords->lx, coords->ly, coords->rx, coords->ry);
#endif
    
    return 0;
}

/**
 * @brief Calculate face similarity with target embedding
 * @param embedding Current face embedding
 * @param target_embedding Target embedding for comparison
 * @param embedding_size Size of embedding arrays
 * @return Cosine similarity score (0.0 to 1.0)
 */
static float calculate_face_similarity(const float32_t *embedding,
                                      const float32_t *target_embedding,
                                      uint32_t embedding_size)
{
    if (!embedding || !target_embedding || embedding_size == 0) {
        return 0.0f;
    }
    
    return embedding_cosine_similarity(embedding, target_embedding, embedding_size);
}

/**
 * @brief Run face recognition on a single face
 * @param ctx Application context
 * @param box Bounding box of face to recognize
 * @return Similarity score (0.0 to 1.0)
 */
static float run_face_recognition_on_face(app_context_t *ctx, const pd_pp_box_t *box)
{
    pixel_coords_t pixel_coords;
    float32_t embedding[EMBEDDING_SIZE];
    
    /* Lazy initialization of face recognition network */
    if (!ctx->nn_ctx.recognition_initialized) {
        if (nn_init_recognition_lazy(&ctx->nn_ctx) < 0) {
            printf("Face recognition network lazy initialization failed\n");
            return 0.0f;
        }
    }
    
    /* Convert coordinates */
    if (convert_box_coordinates(box, &pixel_coords) < 0) {
        return 0.0f;
    }
    
    /* Crop face region */
    if (crop_face_region(&pixel_coords, fr_rgb) < 0) {
        return 0.0f;
    }
    
    /* Prepare input for face recognition network */
    img_rgb_to_chw_float_norm(fr_rgb, (float32_t*)ctx->nn_ctx.recognition_input_buffer, 
                             FR_WIDTH * NN_BPP, FR_WIDTH, FR_HEIGHT);
    
    SCB_CleanInvalidateDCache_by_Addr(ctx->nn_ctx.recognition_input_buffer, 
                                     ctx->nn_ctx.recognition_input_length);
    
    /* Run face recognition inference */
    RunNetworkSync(&NN_Instance_face_recognition);
    SCB_InvalidateDCache_by_Addr(ctx->nn_ctx.recognition_output_buffer, 
                                ctx->nn_ctx.recognition_output_length);
    
    /* Convert output to float embedding */
    for (uint32_t i = 0; i < EMBEDDING_SIZE; i++) {
        embedding[i] = ((float32_t)ctx->nn_ctx.recognition_output_buffer[i]);
    }
    
    /* Calculate similarity */
    float similarity = calculate_face_similarity(embedding, target_embedding, EMBEDDING_SIZE);
    
    /* Store embedding in context (for button press functionality) */
    /* The last face processed will have its embedding stored - this will be overwritten */
    /* but the process_frame_detections will ensure best face embedding is preserved */
    for (uint32_t i = 0; i < EMBEDDING_SIZE; i++) {
        ctx->current_embedding[i] = embedding[i];
    }
    ctx->embedding_valid = 1;
    
    /* Send results via PC stream */
    Enhanced_PC_STREAM_SendFrame(fr_rgb, FACE_RECOGNITION_WIDTH, 
                                FACE_RECOGNITION_HEIGHT, NN_BPP, "ALN", NULL, NULL);
    Enhanced_PC_STREAM_SendEmbedding(embedding, EMBEDDING_SIZE);
    
    LL_ATON_RT_DeInit_Network(&NN_Instance_face_recognition);
    return similarity;
}

/**
 * @brief Handle user button press events
 * @param ctx Application context
 */
static void handle_user_button(app_context_t *ctx)
{
    int current_state = BSP_PB_GetState(BUTTON_USER1);
    
    /* Detect button press */
    if (current_state && !ctx->prev_button_state) {
        ctx->button_press_ts = HAL_GetTick();
    }
    /* Detect button release */
    else if (!current_state && ctx->prev_button_state) {
        uint32_t duration = HAL_GetTick() - ctx->button_press_ts;
        
        if (duration >= BUTTON_LONG_PRESS_DURATION_MS) {
            /* Long press: reset embeddings bank */
            embeddings_bank_reset();
        } else if (ctx->embedding_valid) {
            /* Short press: add current embedding */
            embeddings_bank_add(ctx->current_embedding);
        }
    }
    
    ctx->prev_button_state = current_state;
}


/**
 * @brief Legacy verify_box function - now uses run_face_recognition_on_face
 * @param ctx Application context
 * @param box Bounding box to verify
 * @return Similarity score (0.0 to 1.0)
 */
static float verify_box(app_context_t *ctx, const pd_pp_box_t *box)
{
    /* Legacy function - just call the new implementation */
    return run_face_recognition_on_face(ctx, box);
}

/**
 * @brief Initialize application context and neural networks
 * @param ctx Application context
 */
/**
 * @brief Initialize application context and subsystems
 * @param ctx Application context pointer
 * @return 0 on success, negative on error
 */
static int app_init(app_context_t *ctx)
{
    int ret = 0;
    
    /* Initialize configuration manager */
    ret = config_manager_init(&ctx->config);
    if (ret < 0) {
        return ret;
    }
    
    /* Critical path: System initialization */
    App_SystemInit();
    LL_ATON_RT_RuntimeInit();
    
    /* Parallel initialization of independent components */
    /* Initialize embeddings bank */
    embeddings_bank_init();
    
    /* Initialize hardware components concurrently */
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);
    BSP_LED_Off(LED1);
    BSP_LED_Off(LED2);
    BSP_PB_Init(BUTTON_USER1, BUTTON_MODE_GPIO);
    
    /* Initialize face detection network only (lazy load face recognition) */
    ret = nn_init_detection(&ctx->nn_ctx);
    if (ret < 0) {
        printf("Face detection network initialization failed: %d\n", ret);
        return ret;
    }
    
    /* Background initialization - can be done while other systems start */
    Enhanced_PC_STREAM_Init();
    app_postprocess_init(&ctx->pp_params);
    
    return 0;
}

/**
 * @brief Process frame detections - simplified tracker-free approach
 * @param ctx Application context
 * @param boxes Detected bounding boxes
 * @param box_count Number of detected boxes
 */
static void process_frame_detections(app_context_t *ctx, pd_pp_box_t *boxes, uint32_t box_count)
{
    /* Reset frame state */
    ctx->face_detected = false;
    ctx->face_verified = false;
    ctx->current_similarity = 0.0f;
    
    /* Reset global cropped face display variables */
    g_cropped_face_valid = false;
    g_current_similarity = 0.0f;
    
    bool target_found_this_frame = false;
    float highest_similarity = 0.0f;
    static float32_t best_embedding[EMBEDDING_SIZE];  /* Store best face embedding */
    bool best_embedding_valid = false;
    
    /* Reset embedding validity at start of frame */
    ctx->embedding_valid = 0;
    
    /* Run face recognition on ALL detected faces */
    if (box_count > 0) {
        printf("   Running face recognition on %u detected faces\n", box_count);
        
        for (uint32_t i = 0; i < box_count; i++) {
            /* Only run recognition on faces with sufficient detection confidence */
            if (boxes[i].prob >= FACE_DETECTION_CONFIDENCE_THRESHOLD) {
                printf("   Face %u: detection=%.1f%% -> ", i + 1, boxes[i].prob * 100.0f);
                
                float similarity = run_face_recognition_on_face(ctx, &boxes[i]);
                
                /* Update the box with the recognition similarity (not detection confidence) */
                boxes[i].prob = similarity;
                
                printf("recognition=%.1f%%\n", similarity * 100.0f);
                
                /* Check if this face is above threshold */
                if (similarity >= FACE_SIMILARITY_THRESHOLD) {
                    target_found_this_frame = true;
                }
                
                /* Track the face with highest similarity for display */
                if (similarity > highest_similarity) {
                    highest_similarity = similarity;
                    ctx->best_detection = boxes[i];
                    ctx->current_similarity = similarity;
                    ctx->face_detected = true;
                    
                    /* Store for LCD display */
                    g_cropped_face_valid = true;
                    g_current_similarity = similarity;
                    
                    /* Store best embedding (copy from current_embedding set by run_face_recognition_on_face) */
                    for (uint32_t j = 0; j < EMBEDDING_SIZE; j++) {
                        best_embedding[j] = ctx->current_embedding[j];
                    }
                    best_embedding_valid = true;
                }
            } else {
                /* Face detection confidence too low - skip recognition */
                printf("   Face %u: detection=%.1f%% (too low, skipping recognition)\n", 
                       i + 1, boxes[i].prob * 100.0f);
                /* Set very low similarity to indicate no recognition */
                boxes[i].prob = 0.05f;
            }
        }
    }
    
    /* Update target detection history */
    update_target_detection_history(ctx, target_found_this_frame);
    compute_target_detection_status(ctx);
    
    /* Store best embedding for button press functionality */
    if (best_embedding_valid) {
        for (uint32_t i = 0; i < EMBEDDING_SIZE; i++) {
            ctx->current_embedding[i] = best_embedding[i];
        }
        ctx->embedding_valid = 1;
    }
    
    /* Set verification status based on voting */
    ctx->face_verified = ctx->target_detected;
    
    printf("   Frame summary: faces=%u, target_this_frame=%s, target_detected=%s (%.1f%% best)\n",
           box_count,
           target_found_this_frame ? "YES" : "NO",
           ctx->target_detected ? "YES" : "NO",
           highest_similarity * 100.0f);
}

/**
 * @brief Update LED status based on simple voting mechanism
 * @param ctx Application context
 */
static void update_led_status(app_context_t *ctx)
{
    uint32_t current_time = HAL_GetTick();
    
    if (ctx->target_detected) {
        BSP_LED_On(LED2);   /* Green LED - target detected (3+ of last 5 frames) */
        BSP_LED_Off(LED1);
        /* Update timestamp for target detection */
        ctx->last_stable_verification_ts = current_time;
        ctx->led_timeout_active = false;
    } else if (ctx->face_detected) {
        BSP_LED_On(LED1);   /* Red LED - face detected but not verified */
        BSP_LED_Off(LED2);
        ctx->led_timeout_active = false;
    } else {
        /* No face detected - check if we should maintain green LED due to recent verification */
        if (ctx->last_stable_verification_ts != 0 && 
            (current_time - ctx->last_stable_verification_ts) < FACE_UNVERIFIED_LED_TIMEOUT_MS) {
            /* Keep green LED on for timeout period after last positive recognition */
            BSP_LED_On(LED2);
            BSP_LED_Off(LED1);
            ctx->led_timeout_active = true;
        } else {
            /* Timeout expired or no previous verification - turn off both LEDs */
            BSP_LED_Off(LED1);
            BSP_LED_Off(LED2);
            ctx->led_timeout_active = false;
        }
    }
}

/**
 * @brief Clean up neural network output buffers
 * @param nn_out Array of neural network output buffers
 * @param nn_out_len Array of buffer lengths
 * @param number_output Number of outputs
 */
static void cleanup_nn_buffers(float32_t **nn_out, int32_t *nn_out_len, int number_output)
{
    for (int i = 0; i < number_output; i++) {
        SCB_InvalidateDCache_by_Addr(nn_out[i], nn_out_len[i]);
    }
}

/**
 * @brief Main application loop
 * @param ctx Application context
 */
/* Removed unused helper functions to fix compilation warnings */


/* ========================================================================= */
/* EDUCATIONAL FACE RECOGNITION PIPELINE                                    */
/* ========================================================================= */
/*
 * This pipeline demonstrates a complete face detection and recognition system
 * with multi-face tracking capability, broken down into clear stages:
 *
 * PIPELINE OVERVIEW:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │  STAGE 1: Frame Capture & Preprocessing                                │
 * |  Capture frame from camera -> Convert to neural network format       |
 * └─────────────────────────┬───────────────────────────────────────────────┘
 *                           │
 * ┌─────────────────────────▼───────────────────────────────────────────────┐
 * │  STAGE 2: Face Detection Network                                       │
 * │  🧠 Run CNN to detect faces → Extract bounding boxes                   │
 * └─────────────────────────┬───────────────────────────────────────────────┘
 *                           │
 * ┌─────────────────────────▼───────────────────────────────────────────────┐
 * │  STAGE 3: Post-Processing & Face Tracking                             │
 * |  Convert network output -> Track faces -> Maintain consistency        |
 * └─────────────────────────┬───────────────────────────────────────────────┘
 *                           │
 * ┌─────────────────────────▼───────────────────────────────────────────────┐
 * │  STAGE 4: Face Recognition (Primary Face Only)                        │
 * |  Crop primary face -> Run recognition -> Smooth similarity scores     |
 * └─────────────────────────┬───────────────────────────────────────────────┘
 *                           │
 * ┌─────────────────────────▼───────────────────────────────────────────────┐
 * │  STAGE 5: System Status Update                                         │
 * |  Update LEDs -> Handle buttons -> Send communication                  |
 * └─────────────────────────┬───────────────────────────────────────────────┘
 *                           │
 * ┌─────────────────────────▼───────────────────────────────────────────────┐
 * │  STAGE 6: Output & Metrics                                             │
 * |  Display results -> Calculate performance -> Clean up                 |
 * └─────────────────────────────────────────────────────────────────────────┘
 */

/* ========================================================================= */
/* PIPELINE STAGE FUNCTIONS                                                 */
/* ========================================================================= */

/**
 * @brief Pipeline Stage 1: Frame Capture and Preprocessing
 * @param ctx Application context
 * @param pitch_nn Neural network pitch value
 * @return 0 on success, negative on error
 */
static int pipeline_stage_capture_and_preprocess(app_context_t *ctx, uint32_t pitch_nn)
{
    printf("PIPELINE STAGE 1: Frame Capture\n");
    
    /* Step 1.1: Capture frame from camera or PC stream */
    if (app_get_frame(nn_rgb, pitch_nn) != 0) {
        printf("Frame capture failed\n");
        return -1;
    }
    
#ifdef DUMMY_INPUT_BUFFER
    /* Step 1.1.5: Override both img_buffer and nn_rgb with dummy data for testing */
    load_dual_dummy_buffers();
#endif
    
    /* Step 1.2: Convert RGB to neural network input format */
    printf("   Converting RGB to CHW format for neural network...\n");
    img_rgb_to_chw_float(nn_rgb, (float32_t *)ctx->nn_ctx.detection_input_buffer, 
                        NN_WIDTH * NN_BPP, NN_WIDTH, NN_HEIGHT);
    


    /* Step 1.3: Prepare data for neural network (cache management) */
    printf("   🧠 Preparing %lu bytes for neural network input...\n", ctx->nn_ctx.detection_input_length);
    SCB_CleanInvalidateDCache_by_Addr(ctx->nn_ctx.detection_input_buffer, 
                                     ctx->nn_ctx.detection_input_length);
    
    printf("Frame captured and preprocessed (%dx%d -> %lu bytes)\n", 
           NN_WIDTH, NN_HEIGHT, ctx->nn_ctx.detection_input_length);
    return 0;
}

/**
 * @brief Pipeline Stage 2: Face Detection Neural Network
 * @param ctx Application context
 * @return 0 on success, negative on error
 */
static int pipeline_stage_face_detection(app_context_t *ctx)
{
    printf("🧠 PIPELINE STAGE 2: Face Detection Network\n");
    
    /* Step 2.1: Run face detection neural network */
    printf("   Running face detection neural network inference...\n");
    uint32_t start_time = HAL_GetTick();
    RunNetworkSync(&NN_Instance_face_detection);
    uint32_t inference_time = HAL_GetTick() - start_time;
    
    /* Step 2.2: Network cleanup */
    printf("   🧹 Cleaning up neural network resources...\n");
    LL_ATON_RT_DeInit_Network(&NN_Instance_face_detection);
    
    printf("Face detection completed in %lu ms (%d outputs ready)\n", 
           inference_time, ctx->nn_ctx.detection_output_count);
    return 0;
}

/**
 * @brief Pipeline Stage 3: Post-Processing and Face Extraction
 * @param ctx Application context
 * @return 0 on success, negative on error
 */
static int pipeline_stage_postprocessing(app_context_t *ctx)
{
    printf("PIPELINE STAGE 3: Post-Processing\n");
    
    /* Step 3.1: Run post-processing to extract bounding boxes */
    printf("   Processing %d neural network outputs...\n", ctx->nn_ctx.detection_output_count);
    int32_t ret = app_postprocess_run((void **) ctx->nn_ctx.detection_output_buffers, 
                                     ctx->nn_ctx.detection_output_count, 
                                     &ctx->pp_output, &ctx->pp_params);
    if (ret != 0) {
        printf("Post-processing failed\n");
        return -1;
    }
    
    /* Step 3.2: Extract detected faces */
    pd_pp_box_t *boxes = (pd_pp_box_t *)ctx->pp_output.pOutData;
    printf("   Extracted %d face bounding boxes\n", ctx->pp_output.box_nb);
    
    /* Step 3.3: Log detection details for educational purposes */
    for (uint32_t i = 0; i < ctx->pp_output.box_nb && i < 3; i++) {
        printf("   Face %d: confidence=%.3f, center=(%.2f,%.2f), size=%.2fx%.2f\n", 
               i + 1, boxes[i].prob, boxes[i].x_center, boxes[i].y_center, 
               boxes[i].width, boxes[i].height);
    }
    
    printf("Post-processing completed: %d faces detected\n", ctx->pp_output.box_nb);
    
    return 0;
}

/**
 * @brief Pipeline Stage 4: Face Recognition and Verification
 * @param ctx Application context
 * @return 0 on success, negative on error
 */
static int pipeline_stage_face_recognition(app_context_t *ctx)
{
    printf("PIPELINE STAGE 4: Face Recognition\n");
    
    /* Step 4.1: Process all detected faces with recognition */
    pd_pp_box_t *boxes = (pd_pp_box_t *)ctx->pp_output.pOutData;
    process_frame_detections(ctx, boxes, ctx->pp_output.box_nb);
    
    /* Step 4.2: Log recognition results */
    if (ctx->face_detected) {
        printf("Face recognition: detected=%s, verified=%s, best_similarity=%.1f%%\n",
               ctx->face_detected ? "YES" : "NO",
               ctx->face_verified ? "YES" : "NO",
               ctx->current_similarity * 100.0f);
    } else {
        printf("ℹ️ No faces above threshold detected\n");
    }
    
    return 0;
}

/**
 * @brief Pipeline Stage 5: System Status Update
 * @param ctx Application context
 * @return 0 on success, negative on error
 */
static int pipeline_stage_system_update(app_context_t *ctx)
{
    printf("PIPELINE STAGE 5: System Status Update\n");
    
    /* Step 5.1: Update LED status based on recognition results */
    update_led_status(ctx);
    
    /* Step 5.2: Handle user button interactions */
    handle_user_button(ctx);
    
    /* Step 5.3: Send heartbeat for PC communication */
    Enhanced_PC_STREAM_SendHeartbeat();
    
    printf("System status updated\n");
    return 0;
}

/**
 * @brief Pipeline Stage 6: Output and Performance Metrics
 * @param ctx Application context
 * @param frame_start_time Start time of frame processing
 * @param boot_time System boot time
 * @return 0 on success, negative on error
 */
static int pipeline_stage_output_and_metrics(app_context_t *ctx, uint32_t frame_start_time, uint32_t boot_time)
{
    printf("PIPELINE STAGE 6: Output and Metrics\n");
    
    /* Step 6.1: Calculate performance metrics */
    uint32_t frame_end_time = HAL_GetTick();
    uint32_t total_frame_time = frame_end_time - frame_start_time;
    
    ctx->frame_count++;
    ctx->performance.fps = 1000.0f / (total_frame_time + 1);
    ctx->performance.inference_time_ms = total_frame_time;
    ctx->performance.frame_count = ctx->frame_count;
    ctx->performance.detection_count = ctx->pp_output.box_nb;
    
    /* Step 6.2: Display results */
    app_output(&ctx->pp_output, total_frame_time, boot_time, ctx);
    
    /* Step 6.3: Clean up neural network buffers */
    cleanup_nn_buffers(ctx->nn_ctx.detection_output_buffers, 
                      ctx->nn_ctx.detection_output_lengths, 
                      ctx->nn_ctx.detection_output_count);
    
    printf("Frame processing completed: %.1f FPS, %lu ms total\n", 
           ctx->performance.fps, total_frame_time);
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}

/**
 * @brief Educational Pipeline Main Loop - Clear Stage-by-Stage Processing
 * @param ctx Application context
 * @return 0 on success, negative on error
 */
static int app_main_loop(app_context_t *ctx)
{
    /* Verify at least detection network is initialized */
    if (!ctx->nn_ctx.detection_initialized) {
        printf("Face detection network not initialized!\n");
        return -1;
    }
    
    uint32_t pitch_nn = 0;

    
    /* Initialize camera and display systems */
    printf("Initializing Camera and Display Systems\n");
    app_camera_init(&pitch_nn);
    app_display_init();
    app_input_start();
    printf("Systems initialized, starting pipeline\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    uint32_t boot_time = HAL_GetTick();

    /* Main processing loop with clear pipeline stages */
    while (1) {
        uint32_t frame_start_time = HAL_GetTick();
        printf("STARTING FRAME %lu PROCESSING PIPELINE\n", ctx->frame_count + 1);

        /* Stage 1: Frame Capture and Preprocessing */
        if (pipeline_stage_capture_and_preprocess(ctx, pitch_nn) != 0) {
            continue; /* Skip this frame on error */
        }
        //HINT: for dummy input the first elements of (float32_t *)ctx->nn_ctx.detection_input_buffer should look like: {206, 209, 211, 212, 213, 213, 214, 214, 214, 214, 213 <repeats 14 times>, 212, 212, 211, 208, 207, 204, 199, 193, 189, 182, 174, 163, 151, 139, 129, 119, 110, 104, 104, 106, 108, 114, 121, 126, 132, 137, 140, 141, 147, 152, 152, 152, 153, 153, 154, 154, 154, 154, 153, 151, 152, 152, 151, 150, 149, 149, 147, 146, 142, 135, 126, 114, 107, 97, 87, 73, 60, 47, 32, 19, 12, 14, 19, 26, 32, 37, 42, 52, 60, 63, 67, 70, 70, 71, 72, 72}

        /* Stage 2: Face Detection Neural Network */
        if (pipeline_stage_face_detection(ctx) != 0) {
            continue; /* Skip this frame on error */
        }
        
        //HINT: for dummy input the first elements of ctx->nn_ctx.detection_output_buffers[0] should look like: {1.89764965, 1.77754533, 1.62140954, 1.64543045, 1.68146181, 1.68146181, 1.92167056...}

        /* Stage 3: Post-Processing and Face Extraction */
        if (pipeline_stage_postprocessing(ctx) != 0) {
            continue; /* Skip this frame on error */
        }
        
        //HINT: for dummy input the cctx->pp_output->pOutData.x_center = 0.5113132 ctx->pp_output->pOutData.y_center = 0.543815017

        /* Stage 4: Face Recognition and Verification */
        if (pipeline_stage_face_recognition(ctx) != 0) {
            continue; /* Skip this frame on error */
        }
        
        /* Stage 5: System Status Update */
        if (pipeline_stage_system_update(ctx) != 0) {
            continue; /* Skip this frame on error */
        }
        
        /* Stage 6: Output and Performance Metrics */
        if (pipeline_stage_output_and_metrics(ctx, frame_start_time, boot_time) != 0) {
            continue; /* Skip this frame on error */
        }
    }
    
    return 0;
}

/**
 * @brief Main program entry point
 * @return Never returns (0 on theoretical exit)
 */
int main(void)
{

    int ret = app_init(&g_app_ctx);
    if (ret < 0) {
        /* Initialization failed - handle error */
        while (1) {
            BSP_LED_On(LED1);  /* Red LED indicates error */
            HAL_Delay(50);     /* Reduced delay for faster error indication */
            BSP_LED_Off(LED1);
            HAL_Delay(50);     /* Reduced delay for faster error indication */
        }
    }
    
    //printf("Boot completed in %lu ms\n", boot_end - boot_start);
    
    /* Start main application loop */
    ret = app_main_loop(&g_app_ctx);
    
    /* Cleanup neural networks (never reached in normal operation) */
    nn_cleanup(&g_app_ctx.nn_ctx);
    
    (void)ret; /* Suppress unused variable warning */
    return 0; /* Never reached */
}


void HAL_CACHEAXI_MspInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_ENABLE();
  __HAL_RCC_CACHEAXI_CLK_ENABLE();
  __HAL_RCC_CACHEAXI_FORCE_RESET();
  __HAL_RCC_CACHEAXI_RELEASE_RESET();
}

void HAL_CACHEAXI_MspDeInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_DISABLE();
  __HAL_RCC_CACHEAXI_CLK_DISABLE();
  __HAL_RCC_CACHEAXI_FORCE_RESET();
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  UNUSED(file);
  UNUSED(line);
  __BKPT(0);
  while (1)
  {
  }
}

#endif

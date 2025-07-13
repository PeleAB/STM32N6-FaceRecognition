 /**
 ******************************************************************************
 * @file    main.c
 * @author  GPM Application Team
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
#include "stm32n6xx_hal_rif.h"
#include "app_system.h"
#include "nn_runner.h"
#include "enhanced_pc_stream.h"
#include "app_config.h"
#include "crop_img.h"
#include "display_utils.h"
#include "img_buffer.h"
#include "system_utils.h"
#include "face_utils.h"
#include "target_embedding.h"
// #include "tracking.h"  // Removed - no longer using tracker

/* New modular includes */
#include "app_constants.h"
#include "app_config_manager.h"
#include "memory_pool.h"
#include "app_neural_network.h"
#include "enhanced_tracking.h"
#include "app_frame_processing.h"

/* Legacy compatibility - constants moved to app_constants.h */
#define REVERIFY_INTERVAL_MS        FACE_REVERIFY_INTERVAL_MS
#define MAX_NUMBER_OUTPUT           NN_MAX_OUTPUT_BUFFERS
#define FR_WIDTH                    FACE_RECOGNITION_WIDTH
#define FR_HEIGHT                   FACE_RECOGNITION_HEIGHT
#define SIMILARITY_THRESHOLD        FACE_SIMILARITY_THRESHOLD
#define LONG_PRESS_MS               BUTTON_LONG_PRESS_DURATION_MS

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
    
    /* Multi-Frame Decision Algorithm */
    float similarity_history[5];            /**< Last 5 similarity scores */
    uint32_t history_index;                 /**< Current index in circular buffer */
    uint32_t history_count;                 /**< Number of valid history entries */
    float smoothed_similarity;              /**< Smoothed similarity score */
    bool stable_verification;               /**< Stable verification status */
    
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

/* Memory Buffers */
__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
uint8_t nn_rgb[NN_WIDTH * NN_HEIGHT * NN_BPP];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
uint8_t fr_rgb[FR_WIDTH * FR_HEIGHT * NN_BPP];

__attribute__ ((aligned (32)))
uint8_t dcmipp_out_nn[DCMIPP_OUT_NN_BUFF_LEN];

/* Application Context */
static app_context_t g_app_ctx = {
    .pipe_state = PIPE_STATE_DETECT_AND_VERIFY,
    .face_detected = false,
    .face_verified = false,
    .current_similarity = 0.0f,
    .embedding_valid = 0,
    .button_press_ts = 0,
    .prev_button_state = 0,
    .history_index = 0,
    .history_count = 0,
    .smoothed_similarity = 0.0f,
    .stable_verification = false
};


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
    bool networks_initialized;
} nn_context_t;

/* Function Prototypes */
static int nn_init(nn_context_t *nn_ctx);
static void nn_cleanup(nn_context_t *nn_ctx);
static int app_init(app_context_t *ctx);
static int app_main_loop(app_context_t *ctx);
static void app_input_init(uint32_t *pitch_nn);
static int  app_get_frame(uint8_t *dest, uint32_t pitch_nn);
static void app_output(pd_postprocess_out_t *res, uint32_t inf_ms, uint32_t boot_ms, const app_context_t *ctx);
static void handle_user_button(app_context_t *ctx);
static float verify_box(app_context_t *ctx, const pd_pp_box_t *box);
static void process_frame_detections(app_context_t *ctx, pd_pp_box_t *boxes, uint32_t box_count);
static void update_led_status(const app_context_t *ctx);
static void update_similarity_history(app_context_t *ctx, float similarity);
static void compute_stable_verification(app_context_t *ctx);
static void cleanup_nn_buffers(float32_t **nn_out, int32_t *nn_out_len, int number_output);

/* Neural Network Instance Declarations */
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(face_detection);
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(face_recognition);

/**
 * @brief Initialize all neural networks and buffers in one place
 * @param nn_ctx Neural network context to initialize
 * @return 0 on success, negative on error
 */
static int nn_init(nn_context_t *nn_ctx)
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
    
    nn_ctx->networks_initialized = true;
    
    printf("✅ Neural Networks Initialized Successfully:\n");
    printf("   Face Detection: Input=%lu bytes, %d outputs\n", 
           nn_ctx->detection_input_length, nn_ctx->detection_output_count);
    printf("   Face Recognition: Input=%lu bytes, Output=%lu bytes\n", 
           nn_ctx->recognition_input_length, nn_ctx->recognition_output_length);
    
    return 0;
}

/**
 * @brief Clean up neural network resources
 * @param nn_ctx Neural network context to clean up
 */
static void nn_cleanup(nn_context_t *nn_ctx)
{
    if (nn_ctx && nn_ctx->networks_initialized) {
        /* Clean up any network-specific resources if needed */
        memset(nn_ctx, 0, sizeof(*nn_ctx));
        printf("🧹 Neural Networks cleaned up\n");
    }
}
/* ========================================================================= */
/* FUNCTION IMPLEMENTATIONS                                                  */
/* ========================================================================= */
/**
 * @brief Initialize application input systems (camera/display)
 * @param pitch_nn Pointer to neural network pitch value
 */
static void app_input_init(uint32_t *pitch_nn)
{
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
    CAM_Init(&lcd_bg_area.XSize, &lcd_bg_area.YSize, pitch_nn);
    CAM_DisplayPipe_Start(img_buffer, CMW_MODE_CONTINUOUS);
#else
    lcd_bg_area.XSize = NN_WIDTH;
    lcd_bg_area.YSize = NN_HEIGHT;
    (void)pitch_nn;
#endif

#ifdef ENABLE_LCD_DISPLAY
    LCD_init();
#else
    (void)pitch_nn;
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

    /* Wait for frame capture */
    while (cameraFrameReceived == 0) {}
    cameraFrameReceived = 0;

    /* Handle pitch conversion if necessary */
    if (pitch_nn != (NN_WIDTH * NN_BPP)) {
        SCB_InvalidateDCache_by_Addr(dcmipp_out_nn, sizeof(dcmipp_out_nn));
        img_crop(dcmipp_out_nn, dest, pitch_nn, NN_WIDTH, NN_HEIGHT, NN_BPP);
    } else {
        SCB_InvalidateDCache_by_Addr(dest, NN_WIDTH * NN_HEIGHT * NN_BPP);
    }
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
static void app_output(pd_postprocess_out_t *res, uint32_t inf_ms, uint32_t boot_ms, const app_context_t *ctx)
{
#if defined(ENABLE_PC_STREAM) || defined(ENABLE_LCD_DISPLAY)
    Display_NetworkOutput(res, inf_ms, boot_ms, ctx);
#else
    (void)res;
    (void)inf_ms;
    (void)boot_ms;
    (void)ctx;
#endif
}

/**
 * @brief Update similarity history with new measurement
 * @param ctx Application context
 * @param similarity New similarity score to add to history
 */
static void update_similarity_history(app_context_t *ctx, float similarity)
{
    /* Add similarity to circular buffer */
    ctx->similarity_history[ctx->history_index] = similarity;
    ctx->history_index = (ctx->history_index + 1) % 5;
    
    /* Update count (max 5) */
    if (ctx->history_count < 5) {
        ctx->history_count++;
    }
}

/**
 * @brief Compute stable verification decision based on similarity history
 * @param ctx Application context
 */
static void compute_stable_verification(app_context_t *ctx)
{
    if (ctx->history_count == 0) {
        ctx->smoothed_similarity = 0.0f;
        ctx->stable_verification = false;
        return;
    }
    
    /* Calculate moving average */
    float sum = 0.0f;
    for (uint32_t i = 0; i < ctx->history_count; i++) {
        sum += ctx->similarity_history[i];
    }
    ctx->smoothed_similarity = sum / ctx->history_count;
    
    /* Stable verification requires:
     * 1. At least 3 measurements
     * 2. Average similarity above threshold
     * 3. Low variance (consistent measurements)
     */
    if (ctx->history_count >= 3) {
        /* Check if average meets threshold */
        bool avg_above_threshold = ctx->smoothed_similarity >= FACE_SIMILARITY_THRESHOLD;
        
        /* Check variance for stability */
        float variance = 0.0f;
        for (uint32_t i = 0; i < ctx->history_count; i++) {
            float diff = ctx->similarity_history[i] - ctx->smoothed_similarity;
            variance += diff * diff;
        }
        variance /= ctx->history_count;
        
        /* Low variance indicates stable measurements */
        bool stable_measurements = variance < 0.01f;  /* Max variance threshold */
        
        ctx->stable_verification = avg_above_threshold && stable_measurements;
    } else {
        ctx->stable_verification = false;
    }
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
 * @brief Verify face in bounding box using face recognition
 * @param ctx Application context
 * @param box Bounding box to verify
 * @return Similarity score (0.0 to 1.0)
 */
/**
 * @brief Pixel coordinates structure
 */
typedef struct {
    float cx, cy, w, h, lx, ly, rx, ry;
} pixel_coords_t;

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
    img_crop_align565_to_888(img_buffer, lcd_bg_area.XSize, output_buffer,
                            lcd_bg_area.XSize, lcd_bg_area.YSize,
                            FACE_RECOGNITION_WIDTH, FACE_RECOGNITION_HEIGHT,
                            coords->cx, coords->cy, coords->w, coords->h, 
                            coords->lx, coords->ly, coords->rx, coords->ry);
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
 * @brief Run face recognition inference on cropped face
 * @param ctx Application context
 * @param face_region Cropped face region
 * @param embedding Output embedding array
 * @return 0 on success, negative on error
 */
static int run_face_recognition_inference(app_context_t *ctx,
                                         const uint8_t *face_region,
                                         float32_t *embedding)
{
    if (!ctx || !face_region || !embedding) {
        return -1;
    }
    
    /* Use frame processing pipeline for recognition */
    return frame_processing_recognition_stage(&ctx->frame_ctx, 0, embedding);
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
 * @brief Verify face in bounding box using face recognition (refactored)
 * @param ctx Application context
 * @param box Bounding box to verify
 * @return Similarity score (0.0 to 1.0)
 */
static float verify_box(app_context_t *ctx, const pd_pp_box_t *box)
{
    pixel_coords_t pixel_coords;
    float32_t embedding[EMBEDDING_SIZE];
    float similarity = 0.0f;
    
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
        embedding[i] = ((float32_t)ctx->nn_ctx.recognition_output_buffer[i]) ;
        ctx->current_embedding[i] = embedding[i];
    }
    
    ctx->embedding_valid = 1;
    
    /* Calculate similarity */
    similarity = calculate_face_similarity(embedding, target_embedding, EMBEDDING_SIZE);
    
    /* Send results via PC stream */
    Enhanced_PC_STREAM_SendFrame(fr_rgb, FACE_RECOGNITION_WIDTH, 
                                FACE_RECOGNITION_HEIGHT, NN_BPP, "ALN", NULL, NULL);
    Enhanced_PC_STREAM_SendEmbedding(embedding, EMBEDDING_SIZE);
    
    LL_ATON_RT_DeInit_Network(&NN_Instance_face_recognition);
    return similarity;
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
    
    /* System initialization */
    App_SystemInit();
    LL_ATON_RT_RuntimeInit();
    
    /* Initialize embeddings bank */
    embeddings_bank_init();
    
    /* Initialize enhanced PC streaming */
    Enhanced_PC_STREAM_Init();
    
    /* Hardware initialization */
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);
    BSP_LED_Off(LED1);
    BSP_LED_Off(LED2);
    BSP_PB_Init(BUTTON_USER1, BUTTON_MODE_GPIO);
    
    /* Initialize neural networks centrally */
    ret = nn_init(&ctx->nn_ctx);
    if (ret < 0) {
        printf("❌ Neural network initialization failed: %d\n", ret);
        return ret;
    }
    
    /* Post-processing initialization */
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
    
    /* Process detections if any found */
    if (box_count > 0) {
        /* Find box with highest confidence */
        uint32_t best_idx = 0;
        for (uint32_t i = 1; i < box_count; i++) {
            if (boxes[i].prob > boxes[best_idx].prob) {
                best_idx = i;
            }
        }
        
        ctx->best_detection = boxes[best_idx];
        ctx->face_detected = true;
        
        /* Run face recognition if confidence is high enough */
        if (ctx->best_detection.prob >= FACE_DETECTION_CONFIDENCE_THRESHOLD) {
            float similarity = verify_box(ctx, &ctx->best_detection);
            ctx->current_similarity = similarity;
            
            /* Update multi-frame decision algorithm */
            update_similarity_history(ctx, similarity);
            compute_stable_verification(ctx);
            
            /* Use smoothed similarity for display and decisions */
            float display_similarity = ctx->history_count >= 3 ? ctx->smoothed_similarity : similarity;
            ctx->best_detection.prob = display_similarity;
            
            /* Face verification based on stable multi-frame decision */
            ctx->face_verified = ctx->stable_verification;
            
            /* Update the box in the output with the smoothed similarity score */
            boxes[best_idx].prob = display_similarity;
        } else {
            /* No face recognition run - reset history for clean state */
            ctx->history_count = 0;
            ctx->history_index = 0;
            ctx->smoothed_similarity = 0.0f;
            ctx->stable_verification = false;
        }
    }
}

/**
 * @brief Update LED status based on stable verification state
 * @param ctx Application context
 */
static void update_led_status(const app_context_t *ctx)
{
    if (ctx->stable_verification) {
        BSP_LED_On(LED2);   /* Green LED - stable face verification */
        BSP_LED_Off(LED1);
    } else if (ctx->face_detected) {
        BSP_LED_On(LED1);   /* Red LED - face detected but not stably verified */
        BSP_LED_Off(LED2);
    } else {
        /* No face detected - turn off both LEDs */
        BSP_LED_Off(LED1);
        BSP_LED_Off(LED2);
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

/**
 * @brief Main application loop (refactored)
 * @param ctx Application context
 * @return 0 on success, negative on error
 */
static int app_main_loop(app_context_t *ctx)
{
    /* Verify NN context is initialized */
    if (!ctx->nn_ctx.networks_initialized) {
        printf("❌ Neural networks not initialized!\n");
        return -1;
    }
    uint32_t pitch_nn = 0;
    uint32_t timestamps[3] = { 0 };
    
    app_input_init(&pitch_nn);
    
    while (1) {
        /* Frame capture */
        if (app_get_frame(nn_rgb, pitch_nn) != 0) {
            continue;
        }
        
        /* Prepare input for neural network */
        img_rgb_to_chw_float(nn_rgb, (float32_t *)ctx->nn_ctx.detection_input_buffer, 
                            NN_WIDTH * NN_BPP, NN_WIDTH, NN_HEIGHT);
        SCB_CleanInvalidateDCache_by_Addr(ctx->nn_ctx.detection_input_buffer, 
                                         ctx->nn_ctx.detection_input_length);
        
        /* Run face detection */
        timestamps[0] = HAL_GetTick();
        RunNetworkSync(&NN_Instance_face_detection);
        LL_ATON_RT_DeInit_Network(&NN_Instance_face_detection);
        
        /* Post-processing */
        int32_t ret = app_postprocess_run((void **) ctx->nn_ctx.detection_output_buffers, 
                                         ctx->nn_ctx.detection_output_count, 
                                         &ctx->pp_output, &ctx->pp_params);
        if (ret != 0) {
            continue;
        }
        
        /* Process detection results - simplified approach */
        pd_pp_box_t *boxes = (pd_pp_box_t *)ctx->pp_output.pOutData;
        process_frame_detections(ctx, boxes, ctx->pp_output.box_nb);
        
        /* Update system status */
        update_led_status(ctx);
        
        /* Update performance metrics */
        timestamps[1] = HAL_GetTick();
        if (timestamps[2] == 0) {
            timestamps[2] = HAL_GetTick();
        }
        
        ctx->frame_count++;
        ctx->performance.fps = 1000.0f / (timestamps[1] - timestamps[0] + 1);
        ctx->performance.inference_time_ms = timestamps[1] - timestamps[0];
        ctx->performance.frame_count = ctx->frame_count;
        ctx->performance.detection_count = ctx->pp_output.box_nb;
        
        /* Send heartbeat periodically */
        Enhanced_PC_STREAM_SendHeartbeat();
        
        app_output(&ctx->pp_output, timestamps[1] - timestamps[0], timestamps[2], ctx);
        handle_user_button(ctx);
        
        /* Clean up buffers */
        cleanup_nn_buffers(ctx->nn_ctx.detection_output_buffers, 
                          ctx->nn_ctx.detection_output_lengths, 
                          ctx->nn_ctx.detection_output_count);
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
            HAL_Delay(100);
            BSP_LED_Off(LED1);
            HAL_Delay(100);
        }
    }
    
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

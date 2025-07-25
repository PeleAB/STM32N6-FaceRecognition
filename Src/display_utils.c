#include "display_utils.h"
#include "img_buffer.h"
#include "app_config.h"
#include "app_postprocess.h"
#include "enhanced_pc_stream.h"
#include "stm32n6570_discovery_errno.h"
#include "pd_model_pp_if.h"
#include "pd_pp_output_if.h"
#include "app_constants.h"
#include <math.h>
#ifdef ENABLE_LCD_DISPLAY
#include "stm32n6570_discovery_lcd.h"
#include "stm32_lcd_ex.h"
#endif
#include "stm32n6570_discovery_conf.h"
// #include "tracking.h"  // Removed - no longer using tracker
#include "target_embedding.h"

#ifdef ENABLE_LCD_DISPLAY
#define NUMBER_COLORS 10

static const uint32_t colors[NUMBER_COLORS] = {
    UTIL_LCD_COLOR_GREEN,
    UTIL_LCD_COLOR_RED,
    UTIL_LCD_COLOR_CYAN,
    UTIL_LCD_COLOR_MAGENTA,
    UTIL_LCD_COLOR_YELLOW,
    UTIL_LCD_COLOR_GRAY,
    UTIL_LCD_COLOR_BLACK,
    UTIL_LCD_COLOR_BROWN,
    UTIL_LCD_COLOR_BLUE,
    UTIL_LCD_COLOR_ORANGE
};
#endif

Rectangle_TypeDef lcd_bg_area = {
#if ASPECT_RATIO_MODE == ASPECT_RATIO_CROP || ASPECT_RATIO_MODE == ASPECT_RATIO_FIT
  .X0 = (LCD_FG_WIDTH - LCD_FG_HEIGHT) / 2,
#else
  .X0 = 0,
#endif
  .Y0 = 0,
  .XSize = 0,
  .YSize = 0,
};

Rectangle_TypeDef lcd_fg_area = {
  .X0 = 0,
  .Y0 = 0,
  .XSize = LCD_FG_WIDTH,
  .YSize = LCD_FG_HEIGHT,
};

#ifdef ENABLE_LCD_DISPLAY
__attribute__ ((section (".psram_bss")))
__attribute__ ((aligned (32)))
uint8_t lcd_fg_buffer[2][LCD_FG_WIDTH * LCD_FG_HEIGHT * 2];
static int lcd_fg_buffer_rd_idx;
static BSP_LCD_LayerConfig_t LayerConfig = {0};
/* Removed global tracker reference - now passed as parameter */

#define SIMILARITY_COLOR_THRESHOLD 0.7f

static void DrawPDBoundingBoxes(const pd_pp_box_t *boxes, uint32_t nb,
                                const void *ctx)
{
  UTIL_LCD_FillRect(lcd_fg_area.X0, lcd_fg_area.Y0, lcd_fg_area.XSize,
                    lcd_fg_area.YSize, 0x00000000);
  for (uint32_t i = 0; i < nb; i++) {
    uint32_t x0 = (uint32_t)((boxes[i].x_center - boxes[i].width / 2) *
                              ((float)lcd_bg_area.XSize)) + lcd_bg_area.X0;
    uint32_t y0 = (uint32_t)((boxes[i].y_center - boxes[i].height / 2) *
                              ((float)lcd_bg_area.YSize));
    uint32_t width  = (uint32_t)(boxes[i].width  * ((float)lcd_bg_area.XSize));
    uint32_t height = (uint32_t)(boxes[i].height * ((float)lcd_bg_area.YSize));
    x0 = x0 < lcd_bg_area.X0 + lcd_bg_area.XSize ? x0 : lcd_bg_area.X0 + lcd_bg_area.XSize - 1;
    y0 = y0 < lcd_bg_area.Y0 + lcd_bg_area.YSize ? y0 : lcd_bg_area.Y0 + lcd_bg_area.YSize - 1;
    width  = ((x0 + width)  < lcd_bg_area.X0 + lcd_bg_area.XSize) ? width  : (lcd_bg_area.X0 + lcd_bg_area.XSize - x0 - 1);
    height = ((y0 + height) < lcd_bg_area.Y0 + lcd_bg_area.YSize) ? height : (lcd_bg_area.Y0 + lcd_bg_area.YSize - y0 - 1);
    
    /* Choose color based on similarity threshold */
    uint32_t color_idx = boxes[i].prob >= SIMILARITY_COLOR_THRESHOLD ? 1 : 0;
    UTIL_LCD_DrawRect(x0, y0, width, height, colors[color_idx]);
    
    /* Draw alignment region visualization with rotation (shows actual crop area) */
    if (boxes[i].prob >= SIMILARITY_COLOR_THRESHOLD) {
      /* Get eye positions for rotation calculation */
      float left_eye_x = boxes[i].pKps[0].x * lcd_bg_area.XSize + lcd_bg_area.X0;
      float left_eye_y = boxes[i].pKps[0].y * lcd_bg_area.YSize;
      float right_eye_x = boxes[i].pKps[1].x * lcd_bg_area.XSize + lcd_bg_area.X0;
      float right_eye_y = boxes[i].pKps[1].y * lcd_bg_area.YSize;
      
      /* Calculate face center and crop region */
      float cx = boxes[i].x_center * lcd_bg_area.XSize + lcd_bg_area.X0;
      float cy = boxes[i].y_center * lcd_bg_area.YSize; 
      float w = boxes[i].width * lcd_bg_area.XSize * FACE_BBOX_PADDING_FACTOR;
      float h = boxes[i].height * lcd_bg_area.YSize * FACE_BBOX_PADDING_FACTOR;
      
      /* Calculate rotation angle from eye positions */
      float angle = atan2f(right_eye_y - left_eye_y, right_eye_x - left_eye_x);
      float cos_a = cosf(angle);
      float sin_a = sinf(angle);
      
      /* Draw rotated crop region as four lines forming a rectangle */
      float half_w = w * 0.5f;
      float half_h = h * 0.5f;
      
      /* Calculate rotated corner points */
      float corners_x[4], corners_y[4];
      
      /* Top-left */
      corners_x[0] = cx + (-half_w * cos_a - (-half_h) * sin_a);
      corners_y[0] = cy + (-half_w * sin_a + (-half_h) * cos_a);
      
      /* Top-right */
      corners_x[1] = cx + (half_w * cos_a - (-half_h) * sin_a);
      corners_y[1] = cy + (half_w * sin_a + (-half_h) * cos_a);
      
      /* Bottom-right */
      corners_x[2] = cx + (half_w * cos_a - half_h * sin_a);
      corners_y[2] = cy + (half_w * sin_a + half_h * cos_a);
      
      /* Bottom-left */
      corners_x[3] = cx + (-half_w * cos_a - half_h * sin_a);
      corners_y[3] = cy + (-half_w * sin_a + half_h * cos_a);
      
      /* Draw lines between corners */
      for (int j = 0; j < 4; j++) {
        int next = (j + 1) % 4;
        
        /* Clamp coordinates to display bounds */
        uint32_t x1 = (uint32_t)corners_x[j];
        uint32_t y1 = (uint32_t)corners_y[j];
        uint32_t x2 = (uint32_t)corners_x[next];
        uint32_t y2 = (uint32_t)corners_y[next];
        
        if (x1 < lcd_bg_area.X0 + lcd_bg_area.XSize && y1 < lcd_bg_area.Y0 + lcd_bg_area.YSize &&
            x2 < lcd_bg_area.X0 + lcd_bg_area.XSize && y2 < lcd_bg_area.Y0 + lcd_bg_area.YSize) {
          UTIL_LCD_DrawLine(x1, y1, x2, y2, UTIL_LCD_COLOR_CYAN);
        }
      }
    }
    
    /* Display similarity percentage above bounding box */
    UTIL_LCDEx_PrintfAt(x0, y0 - 15, LEFT_MODE, "%.1f%%", boxes[i].prob * 100.f);
  }
  /* Tracker-specific overlay removed - now using detection-based display */
  (void)ctx;  /* Context parameter unused in simplified version */
}

static void DrawPdLandmarks(const pd_pp_box_t *boxes, uint32_t nb, uint32_t nb_kp)
{
  for (uint32_t i = 0; i < nb; i++) {
    for (uint32_t j = 0; j < nb_kp; j++) {
      uint32_t x = (uint32_t)(boxes[i].pKps[j].x * ((float)lcd_bg_area.XSize)) + lcd_bg_area.X0;
      uint32_t y = (uint32_t)(boxes[i].pKps[j].y * ((float)lcd_bg_area.YSize));
      x = x < lcd_bg_area.X0 + lcd_bg_area.XSize ? x : lcd_bg_area.X0 + lcd_bg_area.XSize - 1;
      y = y < lcd_bg_area.Y0 + lcd_bg_area.YSize ? y : lcd_bg_area.Y0 + lcd_bg_area.YSize - 1;
      UTIL_LCD_SetPixel(x, y, UTIL_LCD_COLOR_RED);
    }
  }
}

#endif /* ENABLE_LCD_DISPLAY */

#ifdef ENABLE_PC_STREAM
static void StreamOutputPd(const pd_postprocess_out_t *p_postprocess)
{
  SCB_InvalidateDCache_by_Addr(img_buffer, sizeof(img_buffer));
  Enhanced_PC_STREAM_SendFrame(img_buffer, lcd_bg_area.XSize, lcd_bg_area.YSize, 2, "RAW", p_postprocess, NULL);
}
#endif /* ENABLE_PC_STREAM */

#ifdef ENABLE_LCD_DISPLAY
static void PrintInfo(uint32_t nb_rois, uint32_t total_frame_time_ms, uint32_t boottime_ms)
{
  UTIL_LCD_SetBackColor(0x40000000);
//  UTIL_LCDEx_PrintfAt(0, LINE(2), CENTER_MODE, "Objects %u", nb_rois);
  UTIL_LCDEx_PrintfAt(0, LINE(20), CENTER_MODE, "FPS: %u", 1000/total_frame_time_ms);
  UTIL_LCDEx_PrintfAt(0, LINE(21), CENTER_MODE, "Embeddings: %d/%d", embeddings_bank_count(), EMBEDDING_BANK_SIZE);
  UTIL_LCDEx_PrintfAt(0, LINE(22), CENTER_MODE, "Boot time: %ums", boottime_ms);
  UTIL_LCD_SetBackColor(0);
  Display_WelcomeScreen();
}
#endif /* ENABLE_LCD_DISPLAY */

void Display_NetworkOutput(pd_postprocess_out_t *p_postprocess, uint32_t total_frame_time_ms, uint32_t boottime_ts, const void *ctx)
{
#ifdef ENABLE_LCD_DISPLAY
  int ret = HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc,
                                         (uint32_t)lcd_fg_buffer[lcd_fg_buffer_rd_idx],
                                         LTDC_LAYER_2);
  assert(ret == HAL_OK);
  DrawPDBoundingBoxes(p_postprocess->pOutData, p_postprocess->box_nb, ctx);
  DrawPdLandmarks(p_postprocess->pOutData, p_postprocess->box_nb, AI_PD_MODEL_PP_NB_KEYPOINTS);
  
  /* Display cropped face if available - access via external global variables */
  extern uint8_t fr_rgb[];
  extern bool g_cropped_face_valid;
  extern float g_current_similarity;
  
#endif
#ifdef ENABLE_PC_STREAM
  StreamOutputPd(p_postprocess);
#endif
#ifdef ENABLE_LCD_DISPLAY
  PrintInfo(p_postprocess->box_nb, total_frame_time_ms, boottime_ts);
  ret = HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_2);
  assert(ret == HAL_OK);
  lcd_fg_buffer_rd_idx = 1 - lcd_fg_buffer_rd_idx;
#else
  (void)inference_ms;
  (void)boottime_ts;
#endif
  (void)p_postprocess; /* in case both features are disabled */
  (void)ctx; /* in case LCD display is disabled */
}

#ifdef ENABLE_LCD_DISPLAY
void LCD_init(void)
{
  BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);

  LayerConfig.X0          = lcd_bg_area.X0;
  LayerConfig.Y0          = lcd_bg_area.Y0;
  LayerConfig.X1          = lcd_bg_area.X0 + lcd_bg_area.XSize;
  LayerConfig.Y1          = lcd_bg_area.Y0 + lcd_bg_area.YSize;
  LayerConfig.PixelFormat = LCD_PIXEL_FORMAT_RGB565;
  LayerConfig.Address     = (uint32_t)img_buffer;

  BSP_LCD_ConfigLayer(0, LTDC_LAYER_1, &LayerConfig);

  LayerConfig.X0 = lcd_fg_area.X0;
  LayerConfig.Y0 = lcd_fg_area.Y0;
  LayerConfig.X1 = lcd_fg_area.X0 + lcd_fg_area.XSize;
  LayerConfig.Y1 = lcd_fg_area.Y0 + lcd_fg_area.YSize;
  LayerConfig.PixelFormat = LCD_PIXEL_FORMAT_ARGB4444;
  LayerConfig.Address = (uint32_t)lcd_fg_buffer;

  BSP_LCD_ConfigLayer(0, LTDC_LAYER_2, &LayerConfig);
  UTIL_LCD_SetFuncDriver(&LCD_Driver);
  UTIL_LCD_SetLayer(LTDC_LAYER_2);
  UTIL_LCD_Clear(0x00000000);
  UTIL_LCD_SetFont(&Font20);
  UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
}

void Display_WelcomeScreen(void)
{
  static uint32_t t0 = 0;
  if (t0 == 0)
    t0 = HAL_GetTick();

  if (HAL_GetTick() - t0 < 4000)
  {
    UTIL_LCD_SetBackColor(0x40000000);
    UTIL_LCDEx_PrintfAt(0, LINE(17), CENTER_MODE, WELCOME_MSG_1);
    UTIL_LCDEx_PrintfAt(0, LINE(18), CENTER_MODE, WELCOME_MSG_2);
    UTIL_LCD_SetBackColor(0);
  }
}


#else
void LCD_init(void)
{
}

void Display_WelcomeScreen(void)
{
}

#endif /* ENABLE_LCD_DISPLAY */

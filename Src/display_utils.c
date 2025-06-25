#include "display_utils.h"
#include "img_buffer.h"
#include "app_config.h"
#include "app_postprocess.h"
#include "pc_stream.h"
#include "stm32n6570_discovery_errno.h"
#include "pd_model_pp_if.h"
#include "pd_pp_output_if.h"
#ifdef ENABLE_LCD_DISPLAY
#include "stm32n6570_discovery_lcd.h"
#include "stm32_lcd_ex.h"
#endif
#include "stm32n6570_discovery_conf.h"
#include "tracking.h"

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
static float g_similarity_percent = 0.f;
extern tracker_t g_tracker;

#define SIMILARITY_COLOR_THRESHOLD 0.7f

static void DrawPDBoundingBoxes(const pd_pp_box_t *boxes, uint32_t nb,
                                const tracker_t *tracker)
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
    uint32_t color_idx = boxes[i].prob >= SIMILARITY_COLOR_THRESHOLD ? 1 : 0;
    UTIL_LCD_DrawRect(x0, y0, width, height, colors[color_idx]);
    UTIL_LCDEx_PrintfAt(-x0 - width, y0, RIGHT_MODE, "%.1f%%", boxes[i].prob * 100.f);
  }
  if (tracker && tracker->state == TRACK_STATE_TRACKING)
  {
    const pd_pp_box_t *b = &tracker->box;
    uint32_t x0 = (uint32_t)((b->x_center - b->width / 2) * ((float)lcd_bg_area.XSize)) + lcd_bg_area.X0;
    uint32_t y0 = (uint32_t)((b->y_center - b->height / 2) * ((float)lcd_bg_area.YSize));
    uint32_t width  = (uint32_t)(b->width  * ((float)lcd_bg_area.XSize));
    uint32_t height = (uint32_t)(b->height * ((float)lcd_bg_area.YSize));
    x0 = x0 < lcd_bg_area.X0 + lcd_bg_area.XSize ? x0 : lcd_bg_area.X0 + lcd_bg_area.XSize - 1;
    y0 = y0 < lcd_bg_area.Y0 + lcd_bg_area.YSize ? y0 : lcd_bg_area.Y0 + lcd_bg_area.YSize - 1;
    width  = ((x0 + width)  < lcd_bg_area.X0 + lcd_bg_area.XSize) ? width  : (lcd_bg_area.X0 + lcd_bg_area.XSize - x0 - 1);
    height = ((y0 + height) < lcd_bg_area.Y0 + lcd_bg_area.YSize) ? height : (lcd_bg_area.Y0 + lcd_bg_area.YSize - y0 - 1);
    UTIL_LCD_DrawRect(x0, y0, width, height, colors[8]);
  }
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
  static uint32_t stream_frame_id = 0;
  SCB_InvalidateDCache_by_Addr(img_buffer, sizeof(img_buffer));
  PC_STREAM_SendFrame(img_buffer, lcd_bg_area.XSize, lcd_bg_area.YSize, 2);
  PC_STREAM_SendDetections(p_postprocess, stream_frame_id++);
}
#endif /* ENABLE_PC_STREAM */

#ifdef ENABLE_LCD_DISPLAY
static void PrintInfo(uint32_t nb_rois, uint32_t inference_ms, uint32_t boottime_ms)
{
  UTIL_LCD_SetBackColor(0x40000000);
  UTIL_LCDEx_PrintfAt(0, LINE(2), CENTER_MODE, "Objects %u", nb_rois);
  UTIL_LCDEx_PrintfAt(0, LINE(20), CENTER_MODE, "Inference: %ums", inference_ms);
  UTIL_LCDEx_PrintfAt(0, LINE(21), CENTER_MODE, "Boot time: %ums", boottime_ms);
  UTIL_LCD_SetBackColor(0);
  Display_WelcomeScreen();
}
#endif /* ENABLE_LCD_DISPLAY */

void Display_NetworkOutput(pd_postprocess_out_t *p_postprocess, uint32_t inference_ms, uint32_t boottime_ts)
{
#ifdef ENABLE_LCD_DISPLAY
  int ret = HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc,
                                         (uint32_t)lcd_fg_buffer[lcd_fg_buffer_rd_idx],
                                         LTDC_LAYER_2);
  assert(ret == HAL_OK);
  DrawPDBoundingBoxes(p_postprocess->pOutData, p_postprocess->box_nb, &g_tracker);
  DrawPdLandmarks(p_postprocess->pOutData, p_postprocess->box_nb, AI_PD_MODEL_PP_NB_KEYPOINTS);
#endif
#ifdef ENABLE_PC_STREAM
  StreamOutputPd(p_postprocess);
#endif
#ifdef ENABLE_LCD_DISPLAY
  PrintInfo(p_postprocess->box_nb, inference_ms, boottime_ts);
  ret = HAL_LTDC_ReloadLayer(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING, LTDC_LAYER_2);
  assert(ret == HAL_OK);
  lcd_fg_buffer_rd_idx = 1 - lcd_fg_buffer_rd_idx;
#else
  (void)inference_ms;
  (void)boottime_ts;
#endif
  (void)p_postprocess; /* in case both features are disabled */
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
    UTIL_LCDEx_PrintfAt(0, LINE(16), CENTER_MODE, "Object detection");
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

void Display_Similarity(float similarity)
{
  g_similarity_percent = similarity * 100.f;
}
#endif /* ENABLE_LCD_DISPLAY */

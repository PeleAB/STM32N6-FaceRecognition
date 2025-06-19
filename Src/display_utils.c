#include "display_utils.h"
#include "img_buffer.h"
#include "app_config.h"
#include "pc_stream.h"
#include "stm32n6570_discovery_errno.h"
#ifdef ENABLE_LCD_DISPLAY
#include "stm32n6570_discovery_lcd.h"
#include "stm32_lcd_ex.h"
#endif
#include "stm32n6570_discovery_conf.h"

#ifdef ENABLE_LCD_DISPLAY
#define NUMBER_COLORS 10

CLASSES_TABLE;

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

static void DrawBoundingBoxes(const od_pp_outBuffer_t *rois, uint32_t nb_rois)
{
  UTIL_LCD_FillRect(lcd_fg_area.X0, lcd_fg_area.Y0, lcd_fg_area.XSize,
                    lcd_fg_area.YSize, 0x00000000);
  for (uint32_t i = 0; i < nb_rois; i++)
  {
    uint32_t x0 = (uint32_t)((rois[i].x_center - rois[i].width / 2) *
                              ((float)lcd_bg_area.XSize)) + lcd_bg_area.X0;
    uint32_t y0 = (uint32_t)((rois[i].y_center - rois[i].height / 2) *
                              ((float)lcd_bg_area.YSize));
    uint32_t width  = (uint32_t)(rois[i].width  * ((float)lcd_bg_area.XSize));
    uint32_t height = (uint32_t)(rois[i].height * ((float)lcd_bg_area.YSize));
    x0 = x0 < lcd_bg_area.X0 + lcd_bg_area.XSize ? x0 : lcd_bg_area.X0 + lcd_bg_area.XSize - 1;
    y0 = y0 < lcd_bg_area.Y0 + lcd_bg_area.YSize ? y0 : lcd_bg_area.Y0 + lcd_bg_area.YSize - 1;
    width  = ((x0 + width)  < lcd_bg_area.X0 + lcd_bg_area.XSize) ? width  : (lcd_bg_area.X0 + lcd_bg_area.XSize - x0 - 1);
    height = ((y0 + height) < lcd_bg_area.Y0 + lcd_bg_area.YSize) ? height : (lcd_bg_area.Y0 + lcd_bg_area.YSize - y0 - 1);
    UTIL_LCD_DrawRect(x0, y0, width, height, colors[rois[i].class_index % NUMBER_COLORS]);
    UTIL_LCDEx_PrintfAt(x0, y0, LEFT_MODE, classes_table[rois[i].class_index]);
    UTIL_LCDEx_PrintfAt(-x0-width, y0, RIGHT_MODE, "%.0f%%", rois[i].conf*100.0f);
  }
}
#endif /* ENABLE_LCD_DISPLAY */

#ifdef ENABLE_PC_STREAM
static void StreamOutput(const od_pp_out_t *p_postprocess)
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

void Display_NetworkOutput(od_pp_out_t *p_postprocess, uint32_t inference_ms, uint32_t boottime_ts)
{
#ifdef ENABLE_LCD_DISPLAY
  int ret = HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc,
                                         (uint32_t)lcd_fg_buffer[lcd_fg_buffer_rd_idx],
                                         LTDC_LAYER_2);
  assert(ret == HAL_OK);

  DrawBoundingBoxes(p_postprocess->pOutBuff, p_postprocess->nb_detect);
#endif
#ifdef ENABLE_PC_STREAM
  StreamOutput(p_postprocess);
#endif
#ifdef ENABLE_LCD_DISPLAY
  PrintInfo(p_postprocess->nb_detect, inference_ms, boottime_ts);
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
#endif /* ENABLE_LCD_DISPLAY */

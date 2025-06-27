// ====================================================================
// LVGL显示驱动 - 核心功能版本
// ====================================================================
#include "LVGL_Driver.h"
#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include <Arduino.h>

// ====================================================================
// 全局变量定义
// ====================================================================
// 显示屏参数
uint32_t screenWidth;
uint32_t screenHeight;

// LVGL显示缓冲区
static lv_disp_draw_buf_t draw_buf;

// 系统控制变量
int brightness = 255;                      // 屏幕亮度值（0-255）

// ====================================================================
// 显示驱动模块 - QSPI总线配置
// ====================================================================
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS,     // 片选信号
  LCD_SCLK,   // 串行时钟
  LCD_SDIO0,  // 数据线0
  LCD_SDIO1,  // 数据线1
  LCD_SDIO2,  // 数据线2
  LCD_SDIO3   // 数据线3
);

// 创建SH8601显示控制器对象
Arduino_GFX *gfx = new Arduino_SH8601(
  bus,                    // QSPI总线对象
  -1,                     // 复位引脚（使用-1表示不使用硬件复位）
  0,                      // 初始旋转角度
  false,                  // IPS屏幕类型标志
  LCD_WIDTH,              // 屏幕宽度
  LCD_HEIGHT              // 屏幕高度
);

// ====================================================================
// LVGL显示刷新模块
// ====================================================================
void my_disp_flush_pure(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

// ====================================================================
// 系统定时器模块
// ====================================================================
void increase_lvgl_tick_pure(void *arg) {
  lv_tick_inc(5); // 5ms时钟周期
}

// ====================================================================
// 触摸输入处理模块（简化版 - 无触摸）
// ====================================================================
void my_touchpad_read_pure(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  // 禁用触摸功能
  data->state = LV_INDEV_STATE_REL;
  data->point.x = 0;
  data->point.y = 0;
}

// ====================================================================
// LVGL核心初始化函数
// ====================================================================
void Pure_LVGL_Init(void) {
  printf("开始初始化LVGL显示系统...\n");
  
  // 添加启动延时
  delay(100);
  
  // ================================================================
  // 显示屏初始化
  // ================================================================
  printf("初始化LCD显示屏...\n");
  gfx->begin();
  gfx->Display_Brightness(brightness);
  
  // 获取屏幕尺寸
  screenWidth = gfx->width();
  screenHeight = gfx->height();
  printf("屏幕尺寸: %dx%d\n", screenWidth, screenHeight);
  
  // ================================================================
  // LVGL图形库初始化
  // ================================================================
  printf("初始化LVGL图形库...\n");
  lv_init();
  
  // 分配显示缓冲区内存（使用经过测试的稳定大小）
  uint32_t buf_size = screenWidth * screenHeight / 10 * sizeof(lv_color_t);
  printf("分配缓冲区大小: %u 字节\n", buf_size);
  
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
  
  if (buf1 == NULL || buf2 == NULL) {
    printf("内存分配失败！\n");
    return;
  }
  printf("显示缓冲区分配成功\n");
  
  // ================================================================
  // LVGL显示驱动配置
  // ================================================================
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size / sizeof(lv_color_t));
  
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush_pure;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.sw_rotate = 1;
  disp_drv.rotated = LV_DISP_ROT_90;
  lv_disp_drv_register(&disp_drv);
  
  // ================================================================
  // LVGL输入设备配置（虚拟触摸）
  // ================================================================
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read_pure;
  lv_indev_drv_register(&indev_drv);
  
  // ================================================================
  // 系统定时器配置
  // ================================================================
  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &increase_lvgl_tick_pure,
    .name = "lvgl_tick_pure"
  };
  
  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, 5000); // 5ms
  
  printf("LVGL显示系统初始化完成！\n");
}

// ====================================================================
// LVGL循环处理函数
// ====================================================================
void Pure_LVGL_Loop(void) {
  lv_timer_handler();
}

// ====================================================================
// 创建主界面UI
// ====================================================================
void Create_Test_UI(void) {
  printf("创建主界面UI...\n");
  
  // 创建主标题标签
  lv_obj_t* title_label = lv_label_create(lv_scr_act());
  lv_label_set_text(title_label, "ESP32S3 Monitor");
  lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, LV_PART_MAIN);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
  
  // 创建版本信息标签
  lv_obj_t* version_label = lv_label_create(lv_scr_act());
  lv_label_set_text(version_label, "Version: v3.5.0\nLVGL Display System");
  lv_obj_set_style_text_color(version_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_set_style_text_align(version_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_align(version_label, LV_ALIGN_CENTER, 0, 0);
  
  // 创建状态信息标签
  lv_obj_t* status_label = lv_label_create(lv_scr_act());
  lv_label_set_text(status_label, "System Status: Running\nDisplay: Active\nWiFi: Available");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
  lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
  
  // 设置背景颜色
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000080), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
  
  printf("主界面UI创建完成！\n");
}

// ====================================================================
// 全局变量定义 - C++类实例
// ====================================================================
LVGLDriver* lvglDriver = nullptr;

// ====================================================================
// LVGLDriver类实现
// ====================================================================
LVGLDriver::LVGLDriver() : brightness(255), brightnessChanged(false), buttonObj(nullptr), buttonLabel(nullptr), 
                           lvglTaskHandle(nullptr), taskRunning(false) {
}

LVGLDriver::~LVGLDriver() {
}

void LVGLDriver::init() {
  Pure_LVGL_Init();
  Create_Test_UI();
}

void LVGLDriver::loop() {
  Pure_LVGL_Loop();
}

void LVGLDriver::start() {
  // 空实现
}

void LVGLDriver::stop() {
  // 空实现
}

void LVGLDriver::createSimpleButton() {
  // 空实现
}

void LVGLDriver::updateButtonText(const char* text) {
  // 空实现
}

void LVGLDriver::setBrightness(int brightness) {
  this->brightness = brightness;
  if (gfx != nullptr) {
    gfx->Display_Brightness(brightness);
  }
  brightnessChanged = true;
}

int LVGLDriver::getBrightness() {
  return brightness;
}

bool LVGLDriver::isBrightnessChanged() {
  return brightnessChanged;
}

void LVGLDriver::clearBrightnessFlag() {
  brightnessChanged = false;
}

bool LVGLDriver::isRunning() {
  return taskRunning;
}

void LVGLDriver::buttonEventCallback(lv_event_t* e) {
  // 空实现
}

void LVGLDriver::lvglTask(void* parameter) {
  // 空实现
} 
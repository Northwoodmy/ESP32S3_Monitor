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
// 触摸输入处理模块（增强版 - 支持触摸调试）
// ====================================================================
static bool touch_pressed = false;
static int16_t touch_x = 0;
static int16_t touch_y = 0;

void my_touchpad_read_pure(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  // 简化的触摸模拟（在实际硬件中，这里需要读取触摸芯片）
  // 目前返回无触摸状态，但保留了扩展接口
  
  if (touch_pressed) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touch_x;
    data->point.y = touch_y;
    printf("触摸坐标: (%d, %d)\n", touch_x, touch_y);
  } else {
    data->state = LV_INDEV_STATE_REL;
    data->point.x = 0;
    data->point.y = 0;
  }
  
  // 重置触摸状态（防止重复触发）
  touch_pressed = false;
}

// 触摸坐标设置函数（用于触摸芯片集成）
void setTouchPoint(int16_t x, int16_t y, bool pressed) {
  touch_x = x;
  touch_y = y;
  touch_pressed = pressed;
}

// ====================================================================
// LVGL核心初始化函数
// ====================================================================
void LVGL_Init(void) {
  printf("开始初始化LVGL显示系统...\n");
  
  // 添加启动延时
  vTaskDelay(pdMS_TO_TICKS(100));
  
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
void LVGL_Loop(void) {
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
  lv_label_set_text(version_label, "Version: v3.6.0\nLVGL Display System\nC++ Modular Design");
  lv_obj_set_style_text_color(version_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_set_style_text_align(version_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_align(version_label, LV_ALIGN_CENTER, 0, -20);
  
  // 创建状态信息标签
  lv_obj_t* status_label = lv_label_create(lv_scr_act());
  lv_label_set_text(status_label, "System Status: Running\nDisplay: Active\nWiFi: Available");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
  lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
  
  // 设置背景颜色
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000080), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
  
  // 创建触摸按钮（如果LVGLDriver实例存在）
  if (lvglDriver != nullptr) {
    lvglDriver->createSimpleButton();
  }
  
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
  printf("初始化LVGLDriver类实例...\n");
  
  // 初始化变量
  brightness = 255;
  brightnessChanged = false;
  buttonObj = nullptr;
  buttonLabel = nullptr;
  lvglTaskHandle = nullptr;
  taskRunning = false;
  
  // 初始化LVGL系统
  LVGL_Init();
  
  // 设置全局实例指针
  lvglDriver = this;
  
  // 创建测试UI
  Create_Test_UI();
  
  printf("LVGLDriver初始化完成\n");
}

void LVGLDriver::loop() {
  LVGL_Loop();
}

void LVGLDriver::start() {
  if (taskRunning) {
    printf("LVGL任务已在运行中\n");
    return;
  }
  
  printf("启动LVGL任务...\n");
  
  // 先设置运行标志，避免竞争条件
  taskRunning = true;
  
  // 创建LVGL处理任务
  BaseType_t result = xTaskCreatePinnedToCore(
    lvglTask,             // 任务函数
    "LVGL_Driver_Task",   // 任务名称
    8192,                 // 堆栈大小
    this,                 // 传递this指针作为参数
    3,                    // 优先级(高于WiFi任务)
    &lvglTaskHandle,      // 任务句柄
    1                     // 运行在核心1
  );
  
  if (result == pdPASS) {
    printf("LVGL任务启动成功\n");
  } else {
    printf("LVGL任务启动失败\n");
    taskRunning = false; // 启动失败时重置标志
  }
}

void LVGLDriver::stop() {
  if (!taskRunning) {
    printf("LVGL任务未在运行\n");
    return;
  }
  
  printf("停止LVGL任务...\n");
  
  // 设置停止标志，让任务自然退出
  taskRunning = false;
  
  // 等待任务退出
  if (lvglTaskHandle != nullptr) {
    printf("等待LVGL任务退出...\n");
    vTaskDelay(pdMS_TO_TICKS(100)); // 给任务时间退出
    
    // 如果任务没有自然退出，强制删除
    if (eTaskGetState(lvglTaskHandle) != eDeleted) {
      printf("强制删除LVGL任务\n");
      vTaskDelete(lvglTaskHandle);
    }
    
    lvglTaskHandle = nullptr;
  }
  
  printf("LVGL任务已停止\n");
}

void LVGLDriver::createSimpleButton() {
  printf("创建LVGL触摸按钮...\n");
  
  // 创建按钮对象
  buttonObj = lv_btn_create(lv_scr_act());
  lv_obj_set_size(buttonObj, 200, 60);
  lv_obj_align(buttonObj, LV_ALIGN_CENTER, 0, 80);
  
  // 设置按钮样式
  lv_obj_set_style_bg_color(buttonObj, lv_color_hex(0x4CAF50), LV_PART_MAIN);
  lv_obj_set_style_bg_color(buttonObj, lv_color_hex(0x45a049), LV_STATE_PRESSED | LV_PART_MAIN);
  lv_obj_set_style_border_width(buttonObj, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(buttonObj, lv_color_hex(0x2E7D32), LV_PART_MAIN);
  lv_obj_set_style_radius(buttonObj, 8, LV_PART_MAIN);
  
  // 创建按钮标签
  buttonLabel = lv_label_create(buttonObj);
  lv_label_set_text(buttonLabel, "ESP32S3");
  lv_obj_set_style_text_color(buttonLabel, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(buttonLabel, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_center(buttonLabel);
  
  // 添加点击事件回调
  lv_obj_add_event_cb(buttonObj, buttonEventCallback, LV_EVENT_CLICKED, this);
  
  printf("LVGL触摸按钮创建完成\n");
}

void LVGLDriver::updateButtonText(const char* text) {
  if (buttonLabel != nullptr && text != nullptr) {
    lv_label_set_text(buttonLabel, text);
    printf("按钮文本已更新为: %s\n", text);
  } else {
    printf("更新按钮文本失败: 按钮未初始化或文本为空\n");
  }
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
  // 获取传递的LVGLDriver实例指针
  LVGLDriver* driver = (LVGLDriver*)lv_event_get_user_data(e);
  if (driver == nullptr) {
    printf("按钮事件回调: 驱动实例为空\n");
    return;
  }
  
  printf("按钮被点击!\n");
  
  // 切换按钮文本
  const char* currentText = lv_label_get_text(driver->buttonLabel);
  if (strcmp(currentText, "ESP32S3") == 0) {
    driver->updateButtonText("监控系统");
    
    // 演示亮度调节
    int currentBrightness = driver->getBrightness();
    int newBrightness = (currentBrightness == 255) ? 128 : 255;
    driver->setBrightness(newBrightness);
    printf("亮度调节: %d -> %d\n", currentBrightness, newBrightness);
  } else {
    driver->updateButtonText("ESP32S3");
    
    // 恢复默认亮度
    driver->setBrightness(255);
    printf("亮度恢复至默认值: 255\n");
  }
}

void LVGLDriver::lvglTask(void* parameter) {
  LVGLDriver* driver = (LVGLDriver*)parameter;
  if (driver == nullptr) {
    printf("LVGL任务: 驱动实例为空，任务退出\n");
    vTaskDelete(nullptr);
    return;
  }
  
  printf("LVGL任务开始运行 (任务运行标志: %s)\n", driver->taskRunning ? "true" : "false");
  
  // 确保任务运行标志正确设置
  if (!driver->taskRunning) {
    printf("LVGL任务: 运行标志为false，等待启动...\n");
    // 等待一小段时间让主线程设置标志
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  
  // 任务主循环
  uint32_t loopCount = 0;
  while (driver->taskRunning) {
    // 处理LVGL定时器和事件
    lv_timer_handler();
    
    // 检查亮度变化标志
    if (driver->isBrightnessChanged()) {
      printf("检测到亮度变化，当前亮度: %d\n", driver->getBrightness());
      driver->clearBrightnessFlag();
    }
    
    // 每1000次循环输出一次状态信息（约5秒）
    loopCount++;
    if (loopCount >= 1000) {
      loopCount = 0;
      printf("LVGL任务运行正常，循环计数重置\n");
    }
    
    // 5ms延时，保持LVGL流畅运行
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  
  printf("LVGL任务正常退出\n");
  driver->taskRunning = false;
  vTaskDelete(nullptr);
}

// ====================================================================
// 新增功能方法实现
// ====================================================================
void LVGLDriver::updateDisplayText(const char* title, const char* status) {
  if (title == nullptr || status == nullptr) {
    printf("更新显示文本失败: 参数为空\n");
    return;
  }
  
  // 这里可以实现动态更新屏幕上的文本显示
  printf("更新显示文本 - 标题: %s, 状态: %s\n", title, status);
  
  // 示例：如果有状态标签，可以更新它
  // 在实际应用中，需要保存对这些UI元素的引用
}

void LVGLDriver::simulateTouch(int16_t x, int16_t y) {
  printf("模拟触摸事件: (%d, %d)\n", x, y);
  setTouchPoint(x, y, true);
}

void LVGLDriver::resetUI() {
  printf("重置UI界面...\n");
  
  // 重置按钮文本
  if (buttonLabel != nullptr) {
    updateButtonText("ESP32S3");
  }
  
  // 重置亮度
  setBrightness(255);
  
  printf("UI界面已重置\n");
} 
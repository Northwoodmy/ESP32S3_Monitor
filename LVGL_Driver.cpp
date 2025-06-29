/*
 * ESP32LVGL图形库集成项目
 * 
 * 功能特性：
 * - 支持SH8601控制器的368x448分辨率LCD屏幕
 * - 使用QSPI接口提供高速数据传输
 * - 集成LVGL图形库进行UI渲染
 * - 支持电容触摸屏输入
 * - 多任务架构，确保UI响应流畅
 * 
 * 硬件配置：
 * - LCD分辨率：368x448像素，16位色深
 * - 接口类型：QSPI (4线SPI)
 * - 触摸接口：I2C
 * - 缓冲区：双缓冲机制，提高刷新效率
 */

#include <stdio.h>

// === FreeRTOS系统相关头文件 ===
#include "freertos/FreeRTOS.h"    // FreeRTOS实时操作系统
#include "freertos/task.h"        // 任务管理

// === ESP32硬件驱动头文件 ===
#include "driver/gpio.h"          // GPIO通用输入输出
#include "driver/i2c.h"           // I2C通信接口（触摸屏）
#include "driver/spi_master.h"    // SPI主机接口（LCD显示）
#include "esp_timer.h"            // ESP定时器服务
#include "esp_lcd_panel_io.h"     // LCD面板IO抽象层
#include "esp_lcd_panel_vendor.h" // LCD厂商特定驱动
#include "esp_lcd_panel_ops.h"    // LCD面板操作接口
#include "esp_err.h"              // ESP错误处理
#include "esp_log.h"              // ESP日志系统

// === LVGL图形库相关头文件 ===
#include "lvgl.h"                 // LVGL核心库
#include "demos/lv_demos.h"       // LVGL演示程序

// === 项目自定义头文件 ===
#include "LVGL_Driver.h"          // LVGL驱动头文件（必须先包含）
#include "esp_lcd_sh8601.h"       // SH8601 LCD控制器驱动
#include "touch_bsp.h"            // 触摸屏板级支持包

// === 常量定义 ===
static const char *TAG = "ESP_LCD_LVGL";  // 日志标签
static SemaphoreHandle_t lvgl_mux = NULL;  // LVGL互斥锁，保证线程安全

// === 硬件接口配置 ===
#define LCD_HOST SPI2_HOST        // LCD使用的SPI主机接口
#define TOUCH_HOST I2C_NUM_0      // 触摸屏使用的I2C接口

#define LCD_BIT_PER_PIXEL (16)    // 像素位深：16位色(RGB565)

// === 背光控制定义 ===
#define LCD_BK_LIGHT_ON_LEVEL 1                                // 背光开启电平
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL  // 背光关闭电平
#define PIN_NUM_LCD_CS (GPIO_NUM_12)      // LCD片选信号
#define PIN_NUM_LCD_PCLK (GPIO_NUM_11)    // LCD像素时钟
#define PIN_NUM_LCD_DATA0 (GPIO_NUM_4)    // LCD数据线0 (QSPI)
#define PIN_NUM_LCD_DATA1 (GPIO_NUM_5)    // LCD数据线1 (QSPI)
#define PIN_NUM_LCD_DATA2 (GPIO_NUM_6)    // LCD数据线2 (QSPI)
#define PIN_NUM_LCD_DATA3 (GPIO_NUM_7)    // LCD数据线3 (QSPI)
#define PIN_NUM_LCD_RST (-1)              // LCD复位引脚（未使用）
#define PIN_NUM_BK_LIGHT (-1)             // 背光控制引脚（未使用）

// === LCD显示屏参数 ===
#define LCD_H_RES 368     // LCD水平分辨率（宽度）
#define LCD_V_RES 448     // LCD垂直分辨率（高度）

// === 触摸屏功能开关 ===
#define USE_TOUCH 1       // 1：启用触摸功能，0：禁用触摸功能

// === LVGL配置参数 ===
#define LVGL_BUF_HEIGHT (LCD_V_RES / 4)  // LVGL缓冲区高度（屏幕高度的1/4）
#define LVGL_TICK_PERIOD_MS 2                    // LVGL时钟节拍周期（毫秒）
#define LVGL_TASK_MAX_DELAY_MS 500               // LVGL任务最大延迟时间
#define LVGL_TASK_MIN_DELAY_MS 1                 // LVGL任务最小延迟时间
#define LVGL_TASK_STACK_SIZE (12 * 1024)          // LVGL任务栈大小（4KB）
#define LVGL_TASK_PRIORITY 2                     // LVGL任务优先级

/**
 * @brief SH8601 LCD控制器初始化命令序列
 * 
 * 该数组定义了LCD初始化所需的所有命令，包括：
 * - 睡眠模式退出
 * - 显示参数配置
 * - 色彩格式设置
 * - 显示区域定义
 * - 亮度控制
 * - 显示开启
 */
static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
  { 0x11, (uint8_t[]){ 0x00 }, 0, 120 },          // 退出睡眠模式，延迟120ms
  { 0x44, (uint8_t[]){ 0x01, 0xD1 }, 2, 0 },     // 设置撕裂效果扫描线
  { 0x35, (uint8_t[]){ 0x00 }, 1, 0 },           // 启用撕裂效果输出信号
  { 0x53, (uint8_t[]){ 0x20 }, 1, 10 },          // 显示控制模式设置
  { 0x2A, (uint8_t[]){ 0x00, 0x00, 0x01, 0x6F }, 4, 0 }, // 列地址设置 (0-367)
  { 0x2B, (uint8_t[]){ 0x00, 0x00, 0x01, 0xBF }, 4, 0 }, // 行地址设置 (0-447)
  { 0x51, (uint8_t[]){ 0x00 }, 1, 10 },          // 亮度设置为0（最暗）
  { 0x29, (uint8_t[]){ 0x00 }, 0, 10 },          // 显示开启
  { 0x51, (uint8_t[]){ 0xFF }, 1, 0 },           // 亮度设置为最大值
};

/**
 * @brief LVGL显示刷新完成通知回调函数
 * 
 * 当LCD面板完成一帧数据传输后，此函数会被调用，
 * 通知LVGL库可以开始准备下一帧数据。
 * 
 * @param panel_io LCD面板IO句柄
 * @param edata 事件数据
 * @param user_ctx 用户上下文（LVGL显示驱动）
 * @return false 表示不需要调用其他回调
 */
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
  lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
  lv_disp_flush_ready(disp_driver);  // 通知LVGL刷新完成
  return false;
}

/**
 * @brief LVGL显示缓冲区刷新回调函数
 * 
 * LVGL调用此函数将渲染好的像素数据发送到LCD屏幕。
 * 该函数负责将LVGL的颜色缓冲区数据传输到指定的屏幕区域。
 * 
 * @param drv LVGL显示驱动结构
 * @param area 要刷新的屏幕区域
 * @param color_map 像素颜色数据缓冲区
 */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
  
  // 获取刷新区域的坐标边界
  const int offsetx1 = area->x1;  // 左上角X坐标
  const int offsetx2 = area->x2;  // 右下角X坐标
  const int offsety1 = area->y1;  // 左上角Y坐标
  const int offsety2 = area->y2;  // 右下角Y坐标

  // 将颜色缓冲区内容复制到LCD屏幕的指定区域
  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

/**
 * @brief LVGL显示驱动更新回调函数
 * 
 * 当LVGL显示驱动参数更新时（如屏幕旋转），此函数被调用
 * 来相应地调整LCD面板的显示方向。
 * 
 * @param drv LVGL显示驱动结构
 */
static void lvgl_update_cb(lv_disp_drv_t *drv) {
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

  // 根据LVGL的旋转设置调整LCD面板显示方向
  switch (drv->rotated) {
    case LV_DISP_ROT_NONE:      // 0度（正常方向）
      esp_lcd_panel_swap_xy(panel_handle, false);    // 不交换XY轴
      esp_lcd_panel_mirror(panel_handle, true, false); // X轴镜像
      break;
    case LV_DISP_ROT_90:        // 90度顺时针旋转
      esp_lcd_panel_swap_xy(panel_handle, true);     // 交换XY轴
      esp_lcd_panel_mirror(panel_handle, true, true); // XY轴都镜像
      break;
    case LV_DISP_ROT_180:       // 180度旋转
      esp_lcd_panel_swap_xy(panel_handle, false);    // 不交换XY轴
      esp_lcd_panel_mirror(panel_handle, false, true); // Y轴镜像
      break;
    case LV_DISP_ROT_270:       // 270度顺时针旋转
      esp_lcd_panel_swap_xy(panel_handle, true);     // 交换XY轴
      esp_lcd_panel_mirror(panel_handle, false, false); // 不镜像
      break;
  }
}

/**
 * @brief LVGL区域舍入回调函数
 * 
 * 将LVGL的刷新区域坐标调整为偶数边界，
 * 这有助于优化某些LCD控制器的数据传输效率。
 * 
 * @param disp_drv 显示驱动结构
 * @param area 要调整的区域
 */
void lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area) {
  uint16_t x1 = area->x1;
  uint16_t x2 = area->x2;
  uint16_t y1 = area->y1;
  uint16_t y2 = area->y2;

  // 将起始坐标向下舍入到最近的偶数
  area->x1 = (x1 >> 1) << 1;
  area->y1 = (y1 >> 1) << 1;
  // 将结束坐标向上舍入到最近的奇数
  area->x2 = ((x2 >> 1) << 1) + 1;
  area->y2 = ((y2 >> 1) << 1) + 1;
}

#if USE_TOUCH
/**
 * @brief LVGL触摸输入回调函数
 * 
 * LVGL调用此函数读取触摸屏状态，包括触摸位置和按压状态。
 * 注意：此处进行了坐标轴交换以匹配显示方向。
 * 
 * @param drv 输入设备驱动结构
 * @param data 触摸数据结构，用于返回触摸信息
 */
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  uint16_t tp_x, tp_y;  // 触摸坐标
  uint8_t win;          // 触摸状态
  
  // 读取触摸屏状态和坐标
  win = getTouch(&tp_x, &tp_y);
  
  if (win) {
    // 有触摸事件：进行坐标转换以匹配屏幕方向
    data->point.x = tp_y;  // 注意：这里交换了XY轴
    data->point.y = tp_x;
    data->state = LV_INDEV_STATE_PRESSED;  // 设置为按下状态
    
    // 输出调试信息到串口
    //printf("触摸坐标 X: %d, Y: %d\n", tp_x, tp_y);
  } else {
    // 无触摸事件
    data->state = LV_INDEV_STATE_RELEASED;  // 设置为释放状态
  }
}
#endif

/**
 * @brief LVGL时钟节拍定时器回调函数
 * 
 * 此函数由ESP定时器定期调用，为LVGL提供时间基准。
 * LVGL需要准确的时间信息来处理动画、超时等时间相关功能。
 * 
 * @param arg 定时器参数（未使用）
 */
static void increase_lvgl_tick(void *arg) {
  /* 告诉LVGL已经过去了多少毫秒 */
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

/**
 * @brief LVGL互斥锁获取函数
 * 
 * 由于LVGL APIs不是线程安全的，在多任务环境中访问LVGL
 * 之前必须先获取互斥锁。
 * 
 * @param timeout_ms 超时时间（毫秒），-1表示永久等待
 * @return true 成功获取锁，false 获取锁超时
 */
static bool lvgl_lock(int timeout_ms) {
  assert(lvgl_mux && "必须先调用显示初始化函数");

  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

/**
 * @brief LVGL互斥锁释放函数
 * 
 * 完成LVGL相关操作后，必须释放互斥锁以允许其他任务访问。
 */
static void lvgl_unlock(void) {
  assert(lvgl_mux && "必须先调用显示初始化函数");
  xSemaphoreGive(lvgl_mux);
}

/**
 * @brief LVGL主任务函数
 * 
 * 该任务负责持续调用LVGL的定时器处理函数，处理所有的
 * 图形渲染、动画更新、用户输入等操作。这是LVGL的核心处理循环。
 * 
 * @param arg 任务参数（未使用）
 
static void lvgl_port_task(void *arg) {
  printf("[ESP_LCD_LVGL] LVGL任务启动中...\n");
  uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
  
  while (1) {
    // 获取LVGL互斥锁确保线程安全
    if (lvgl_lock(-1)) {
      // 调用LVGL定时器处理函数，返回值表示下次调用的建议延迟时间
      task_delay_ms = lv_timer_handler();
      // 释放互斥锁
      lvgl_unlock();
    }
    
    // 限制任务延迟时间在合理范围内
    if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
    }
    
    // 任务延迟，让出CPU时间给其他任务
    vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
  }
}*/

// === LVGLDriver类实现 ===

/**
 * @brief 构造函数
 */
LVGLDriver::LVGLDriver() 
    : m_initialized(false)
    , m_running(false)
    , m_taskHandle(nullptr)
    , m_mutex(nullptr)
    , m_display(nullptr)
    , m_brightness(80)
{
    printf("[LVGLDriver] LVGL驱动实例已创建\n");
}

/**
 * @brief 析构函数
 */
LVGLDriver::~LVGLDriver() {
    stop();
    
    // 不需要删除互斥锁，因为使用的是全局的lvgl_mux
    
    printf("[LVGLDriver] LVGL驱动实例已销毁\n");
}

/**
 * @brief 初始化LVGL驱动系统
 */
bool LVGLDriver::init() {
    if (m_initialized) {
        printf("[LVGLDriver] 警告：重复初始化\n");
        return true;
    }
    
    printf("[LVGLDriver] 开始初始化LVGL驱动系统...\n");
    
    // 调用原有的LVGL初始化函数
    LVGL_Init();
    
    // 不创建自己的互斥锁，使用全局的lvgl_mux
    m_mutex = nullptr;
    
    m_initialized = true;
    printf("[LVGLDriver] LVGL驱动初始化完成\n");
    return true;
}

/**
 * @brief 启动LVGL处理任务
 */
bool LVGLDriver::start() {
    if (!m_initialized) {
        printf("[LVGLDriver] 错误：未初始化，无法启动任务\n");
        return false;
    }
    
    if (m_running) {
        printf("[LVGLDriver] 警告：任务已在运行\n");
        return true;
    }
    
    // 先设置运行标志，避免竞态条件
    m_running = true;
    
    // 创建LVGL处理任务
    BaseType_t result = xTaskCreatePinnedToCore(
        lvglTaskEntry,              // 任务函数
        "LVGL_Driver_Task",         // 任务名称
        LVGLDriver::TASK_STACK_SIZE,           // 栈大小
        this,                      // 任务参数
        LVGLDriver::TASK_PRIORITY,             // 任务优先级
        &m_taskHandle,             // 任务句柄
        LVGLDriver::TASK_CORE                  // 运行核心
    );
    
    if (result != pdPASS) {
        printf("[LVGLDriver] 错误：创建任务失败\n");
        m_running = false;  // 任务创建失败，重置运行标志
        return false;
    }
    
    printf("[LVGLDriver] LVGL任务已启动（核心%d，优先级%d）\n", LVGLDriver::TASK_CORE, LVGLDriver::TASK_PRIORITY);
    return true;
}

/**
 * @brief 停止LVGL处理任务
 */
void LVGLDriver::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // 等待任务结束
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
        printf("[LVGLDriver] LVGL任务已停止\n");
    }
}

/**
 * @brief LVGL主任务静态入口函数
 */
void LVGLDriver::lvglTaskEntry(void* arg) {
    LVGLDriver* driver = static_cast<LVGLDriver*>(arg);
    if (driver) {
        driver->lvglTask();
    }
    vTaskDelete(nullptr);
}

/**
 * @brief LVGL主任务执行函数
 */
void LVGLDriver::lvglTask() {
    printf("[LVGLDriver] LVGL任务开始运行\n");
    
    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    uint32_t loop_count = 0;
    
    while (m_running) {
        loop_count++;
        
        // 获取LVGL互斥锁确保线程安全
        if (lock(-1)) {
            // 调用LVGL定时器处理函数
            task_delay_ms = lv_timer_handler();
            // 释放互斥锁
            unlock();
            
            // 每10000次循环打印一次状态（降低输出频率）
            if (loop_count % 10000 == 0) {
                printf("[LVGLDriver] LVGL任务运行正常，循环次数: %d\n", loop_count);
            }
        } else {
            printf("[LVGLDriver] 错误：获取LVGL锁失败\n");
            // 如果获取锁失败，等待一段时间再重试
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // 限制任务延迟时间在合理范围内
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        
        // 任务延迟
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
    
    printf("[LVGLDriver] LVGL任务结束，总循环次数: %d\n", loop_count);
}

/**
 * @brief 获取LVGL互斥锁
 */
bool LVGLDriver::lock(int timeout_ms) {
    // 使用全局的lvgl_lock函数
    return lvgl_lock(timeout_ms);
}

/**
 * @brief 释放LVGL互斥锁
 */
void LVGLDriver::unlock() {
    // 使用全局的lvgl_unlock函数
    lvgl_unlock();
}

/**
 * @brief 检查LVGL系统是否已初始化
 */
bool LVGLDriver::isInitialized() const {
    return m_initialized;
}

/**
 * @brief 检查LVGL任务是否在运行
 */
bool LVGLDriver::isRunning() const {
    return m_running;
}

/**
 * @brief 获取显示器句柄
 */
lv_disp_t* LVGLDriver::getDisplay() {
    return m_display;
}

/**
 * @brief 强制刷新显示屏
 */
void LVGLDriver::forceRefresh() {
    if (lock(1000)) {
        lv_disp_trig_activity(nullptr);
        unlock();
    }
}

/**
 * @brief 设置显示屏亮度
 */
void LVGLDriver::setBrightness(uint8_t brightness) {
    if (brightness > 100) {
        brightness = 100;
    }
    
    m_brightness = brightness;
    
    // 这里可以添加硬件亮度控制代码
    // 例如：通过PWM控制背光亮度
    printf("[LVGLDriver] 设置显示亮度：%d%%\n", m_brightness);
}

/**
 * @brief 获取当前显示屏亮度
 */
uint8_t LVGLDriver::getBrightness() const {
    return m_brightness;
}

// 全局LVGL驱动实例指针
LVGLDriver* lvglDriver = nullptr;

/**
 * @brief LVGL_Init函数 - LVGL初始化
 * 
 * 该函数在系统启动时执行一次，负责完成所有硬件和软件的初始化工作：
 * 1. 串口通信初始化
 * 2. LCD显示系统初始化
 * 3. 触摸屏初始化
 * 4. LVGL图形库初始化
 * 5. 创建和启动相关任务
 */
void LVGL_Init(void) {

  printf("[ESP_LCD_LVGL] ESP32 LCD LVGL项目启动中...\n");
  
  // === 2. LVGL相关变量声明 ===
  static lv_disp_draw_buf_t disp_buf;  // LVGL显示缓冲区（包含内部绘图缓冲区）
  static lv_disp_drv_t disp_drv;       // LVGL显示驱动（包含回调函数）

  // === 3. 背光控制初始化 ===
#if PIN_NUM_BK_LIGHT >= 0
      printf("[ESP_LCD_LVGL] 初始化LCD背光控制\n");
  gpio_config_t bk_gpio_config = {
    .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT,
    .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

  // === 4. SPI总线初始化 ===
  printf("[ESP_LCD_LVGL] 初始化SPI总线（QSPI模式）\n");
  const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
    PIN_NUM_LCD_PCLK,     // 时钟线
    PIN_NUM_LCD_DATA0,    // 数据线0
    PIN_NUM_LCD_DATA1,    // 数据线1
    PIN_NUM_LCD_DATA2,    // 数据线2
    PIN_NUM_LCD_DATA3,    // 数据线3
    LCD_H_RES * LCD_V_RES * LCD_BIT_PER_PIXEL / 8  // 最大传输大小
  );
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  // === 5. LCD面板IO初始化 ===
  printf("[ESP_LCD_LVGL] 安装LCD面板IO接口\n");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(
    PIN_NUM_LCD_CS,           // 片选引脚
    notify_lvgl_flush_ready,  // 刷新完成回调
    &disp_drv                         // 回调用户数据
  );
  
  // === 6. SH8601厂商特定配置 ===
  sh8601_vendor_config_t vendor_config = {
    .init_cmds = lcd_init_cmds,                                        // 初始化命令数组
    .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]), // 命令数量
    .flags = {
      .use_qspi_interface = 1,  // 启用QSPI接口
    },
  };
  
  // 将LCD连接到SPI总线
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

  // === 7. LCD面板驱动初始化 ===
  esp_lcd_panel_handle_t panel_handle = NULL;
  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_NUM_LCD_RST,  // 复位引脚
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, // RGB颜色顺序
    .bits_per_pixel = LCD_BIT_PER_PIXEL,        // 像素位深
    .vendor_config = &vendor_config,            // 厂商特定配置
  };
  
  printf("[ESP_LCD_LVGL] 安装SH8601面板驱动\n");
  ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));       // 复位面板
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));        // 初始化面板
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true)); // 开启显示

  // === 8. 触摸屏初始化 ===
#if USE_TOUCH
      printf("[ESP_LCD_LVGL] 初始化触摸屏\n");
  Touch_Init();
#endif

  // === 9. LVGL库初始化 ===
  printf("[ESP_LCD_LVGL] 初始化LVGL图形库\n");
  lv_init();
  
  // 分配LVGL使用的绘图缓冲区（建议至少为屏幕大小的1/10）
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(
    LCD_H_RES * LVGL_BUF_HEIGHT * sizeof(lv_color_t), 
    MALLOC_CAP_DMA  // DMA兼容内存
  );
  assert(buf1);
  
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(
    LCD_H_RES * LVGL_BUF_HEIGHT * sizeof(lv_color_t), 
    MALLOC_CAP_DMA  // DMA兼容内存
  );
  assert(buf2);
  
  // 初始化LVGL双缓冲区
  lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * LVGL_BUF_HEIGHT);

  // === 10. 注册LVGL显示驱动 ===
  printf("[ESP_LCD_LVGL] 向LVGL注册显示驱动\n");
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_H_RES;                 // 水平分辨率
  disp_drv.ver_res = LCD_V_RES;                 // 垂直分辨率
  disp_drv.flush_cb = lvgl_flush_cb;            // 刷新回调函数
  disp_drv.rounder_cb = lvgl_rounder_cb;        // 区域舍入回调
  disp_drv.drv_update_cb = lvgl_update_cb;      // 驱动更新回调
  disp_drv.draw_buf = &disp_buf;                        // 绘图缓冲区
  disp_drv.user_data = panel_handle;                    // 用户数据（面板句柄）
  lv_disp_t *disp = lv_disp_drv_register(&disp_drv);   // 注册显示驱动

  // === 11. 安装LVGL时钟定时器 ===
  printf("[ESP_LCD_LVGL] 安装LVGL时钟定时器\n");
  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &increase_lvgl_tick,  // 定时器回调函数
    .name = "lvgl_tick"                       // 定时器名称
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

  // === 12. 触摸输入设备注册 ===
#if USE_TOUCH
      printf("[ESP_LCD_LVGL] 注册触摸输入设备\n");
  static lv_indev_drv_t indev_drv;  // 输入设备驱动（触摸屏）
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;     // 设备类型：指针设备
  indev_drv.disp = disp;                      // 关联的显示设备
  indev_drv.read_cb = lvgl_touch_cb;  // 触摸读取回调函数
  lv_indev_drv_register(&indev_drv);          // 注册输入设备驱动
#endif

  // === 13. 创建LVGL任务和同步机制 ===
  printf("[ESP_LCD_LVGL] 创建LVGL处理任务\n");
  lvgl_mux = xSemaphoreCreateMutex();  // 创建互斥锁
  assert(lvgl_mux);
  
  // 创建LVGL主处理任务
  //xTaskCreate(lvgl_port_task, "LVGL_Task", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
  
  // === 14. 启动演示程序 ===
  printf("[ESP_LCD_LVGL] 启动LVGL演示程序\n");
  if (lvgl_lock(-1)) {  // 获取LVGL锁
    //lv_demo_widgets();          // 启动控件演示程序
    // 其他可选的演示程序：
    // lv_demo_music();         // 现代音乐播放器演示
    // lv_demo_stress();        // LVGL压力测试
    // lv_demo_benchmark();     // 性能基准测试
    
    lvgl_unlock();      // 释放LVGL锁
  }
  
  printf("[ESP_LCD_LVGL] 系统初始化完成！\n");
}


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
#include <math.h>               // 数学函数库（fabs等）

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
  { 0x29, (uint8_t[]){ 0x00 }, 0, 10 },          // 显示开启
  { 0x51, (uint8_t[]){ 0x80 }, 1, 0 },           // 亮度设置为中等值（50%）
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
 * @brief LVGL触摸输入回调函数（支持软件旋转）
 * 
 * LVGL调用此函数读取触摸屏状态，包括触摸位置和按压状态。
 * 在软件旋转模式下，需要根据当前屏幕旋转角度来转换触摸坐标。
 * 
 * @param drv 输入设备驱动结构
 * @param data 触摸数据结构，用于返回触摸信息
 */
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  uint16_t tp_x, tp_y;  // 原始触摸坐标
  uint8_t win;          // 触摸状态
  
  // 读取触摸屏状态和坐标
  win = getTouch(&tp_x, &tp_y);
  
  if (win) {
    // 有触摸事件：根据当前屏幕旋转角度转换坐标
    lv_coord_t final_x, final_y;
    
#if USE_GYROSCOPE
    // 获取当前屏幕旋转角度
    screen_rotation_t current_rotation = SCREEN_ROTATION_0;
    if (lvglDriver && lvglDriver->isInitialized()) {
      current_rotation = lvglDriver->getScreenRotation();
    }
    
    // 根据旋转角度转换触摸坐标
    switch (current_rotation) {
      case SCREEN_ROTATION_0:   // 0度（正常方向）
        final_x = tp_y;
        final_y = tp_x;
        break;
      case SCREEN_ROTATION_90:  // 90度顺时针旋转
        final_x = tp_x;
        final_y = LCD_H_RES - tp_y - 1;
        break;
      case SCREEN_ROTATION_180: // 180度旋转
        final_x = LCD_V_RES - tp_y - 1;
        final_y = LCD_H_RES - tp_x - 1;
        break;
      case SCREEN_ROTATION_270: // 270度顺时针旋转
        final_x = LCD_V_RES - tp_x - 1;
        final_y = tp_y;
        break;
      default:
        final_x = tp_y;
        final_y = tp_x;
        break;
    }
#else
    // 未启用陀螺仪时使用默认坐标转换
    final_x = tp_y;
    final_y = tp_x;
#endif
    
    data->point.x = final_x;
    data->point.y = final_y;
    data->state = LV_INDEV_STATE_PRESSED;  // 设置为按下状态
    
    // 输出调试信息到串口
    //printf("触摸坐标 原始X: %d, Y: %d -> 转换后X: %d, Y: %d (旋转: %d)\n", 
    //       tp_x, tp_y, final_x, final_y, current_rotation);
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
    , m_tca9554_initialized(false)  // 初始化TCA9554状态标志
#if USE_GYROSCOPE
    , m_rotation_initialized(false)
    , m_current_rotation(SCREEN_ROTATION_0)
    , m_last_rotation_check_time(0)
    , m_orientation_stable_time(0)
    , m_pending_rotation(SCREEN_ROTATION_0)
#endif
{
    printf("[LVGLDriver] LVGL驱动实例已创建\n");
    
#if USE_GYROSCOPE
    // 初始化屏幕自动旋转配置（优化QMI8658参数）
    m_rotation_config.threshold = 6.0f;               // 6m/s²阈值（降低灵敏度）
    m_rotation_config.stable_time_ms = 800;           // 800ms稳定时间（提高响应速度）
    m_rotation_config.detection_interval_ms = 150;    // 150ms检测间隔（提高检测频率）
    m_rotation_config.auto_rotation_enabled = true;   // 默认启用自动旋转
    
    printf("[LVGLDriver] 屏幕自动旋转配置已初始化（软件旋转模式）\n");
#endif
}

/**
 * @brief 析构函数
 */
LVGLDriver::~LVGLDriver() {
    stop();
    
#if USE_GYROSCOPE
    // 清理屏幕旋转资源
    if (m_rotation_initialized) {
        QMI8658_Deinit();
        m_rotation_initialized = false;
        printf("[LVGLDriver] 屏幕旋转资源已清理\n");
    }
#endif
    
    // 清理TCA9554资源
    if (m_tca9554_initialized) {
        // TCA9554不需要特殊的反初始化操作，I2C驱动会自动清理
        m_tca9554_initialized = false;
        printf("[LVGLDriver] TCA9554资源已清理\n");
    }
    
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
    
    // === 1. 首先初始化I2C总线管理器（统一管理所有I2C设备） ===
    printf("[LVGLDriver] 初始化I2C总线管理器...\n");
    esp_err_t i2c_ret = I2CBus_Init();
    if (i2c_ret != ESP_OK) {
        printf("[LVGLDriver] 错误：I2C总线管理器初始化失败，错误码: 0x%x\n", i2c_ret);
        return false;
    }
    printf("[LVGLDriver] ✓ I2C总线管理器初始化成功\n");
    
    // === 2. 初始化TCA9554 IO扩展芯片（在所有I2C设备之前） ===
    if (!initTCA9554()) {
        printf("[LVGLDriver] 错误：TCA9554初始化失败\n");
        return false;
    }
    
    // === 3. 执行屏幕和触控复位序列 ===
    performDisplayReset();
    
    // 调用LVGL初始化函数并获取显示器对象
    m_display = LVGL_Init();
    
    if (m_display) {
        printf("[LVGLDriver] 显示器对象初始化成功\n");
    } else {
        printf("[LVGLDriver] 显示器对象初始化失败\n");
        return false;
    }
    
    // 不创建自己的互斥锁，使用全局的lvgl_mux
    m_mutex = nullptr;
    
    m_initialized = true;
    printf("[LVGLDriver] LVGL驱动初始化完成\n");
    
#if USE_GYROSCOPE
    // 初始化屏幕自动旋转（可选，如果失败不影响显示功能）
    if (initScreenRotation()) {
        printf("[LVGLDriver] 屏幕自动旋转功能已启用\n");
    } else {
        printf("[LVGLDriver] 警告：屏幕自动旋转初始化失败，将禁用自动旋转功能\n");
    }
#endif
    
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
        
#if USE_GYROSCOPE
        // 处理屏幕自动旋转
        processScreenRotation();
#endif
        
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
    
    // SH8601 AMOLED显示屏通过QSPI命令控制背光亮度
    // 将0-100的百分比转换为0-255的硬件值
    uint8_t hw_brightness = (uint8_t)((brightness * 255) / 100);
    
    // 使用专用的SH8601亮度控制函数
    if (m_display) {
        lv_disp_drv_t* drv = m_display->driver;
        if (drv && drv->user_data) {
            esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
            
            if (panel_handle) {
                // 调用SH8601专用的亮度控制函数
                esp_err_t ret = esp_lcd_sh8601_set_brightness(panel_handle, hw_brightness);
                
                if (ret == ESP_OK) {
                    printf("[LVGLDriver] 亮度设置成功：%d%%\n", m_brightness);
                } else {
                    printf("[LVGLDriver] 亮度设置失败，错误码：0x%x\n", ret);
                }
            } else {
                printf("[LVGLDriver] Panel句柄无效\n");
            }
        } else {
            printf("[LVGLDriver] 显示驱动无效\n");
        }
    } else {
        printf("[LVGLDriver] 显示器未初始化\n");
    }
}

/**
 * @brief 获取当前显示屏亮度
 */
uint8_t LVGLDriver::getBrightness() const {
    return m_brightness;
}

#if USE_GYROSCOPE
/**
 * @brief 初始化屏幕自动旋转功能（基于加速度计重力感应）
 */
bool LVGLDriver::initScreenRotation() {
    if (m_rotation_initialized) {
        printf("[LVGLDriver] 屏幕自动旋转已经初始化\n");
        return true;
    }
    
    printf("[LVGLDriver] 初始化QMI8658加速度计用于屏幕自动旋转...\n");
    
    // 初始化QMI8658
    if (QMI8658_Init() != 0) {
        printf("[LVGLDriver] 错误：QMI8658传感器初始化失败\n");
        return false;
    }
    
    // 配置加速度计（用于检测重力方向）
    if (QMI8658_ConfigAccelerometer(QMI8658_ACC_RANGE_4G, 
                                   QMI8658_ACC_ODR_125Hz, 
                                   QMI8658_LPF_MODE_2) != 0) {
        printf("[LVGLDriver] 错误：加速度计配置失败\n");
        return false;
    }
    
    // 启用加速度计
    if (QMI8658_EnableAccelerometer() != 0) {
        printf("[LVGLDriver] 错误：加速度计启用失败\n");
        return false;
    }
    
    m_rotation_initialized = true;
    printf("[LVGLDriver] 屏幕自动旋转初始化成功\n");
    return true;  
}

/**
 * @brief 启用/禁用自动屏幕旋转
 */
void LVGLDriver::setAutoRotationEnabled(bool enabled) {
    m_rotation_config.auto_rotation_enabled = enabled;
    printf("[LVGLDriver] 自动屏幕旋转 %s\n", enabled ? "已启用" : "已禁用");
}

/**
 * @brief 检查自动旋转是否启用
 */
bool LVGLDriver::isAutoRotationEnabled() const {
    return m_rotation_config.auto_rotation_enabled;
}

/**
 * @brief 手动设置屏幕旋转角度
 */
void LVGLDriver::setScreenRotation(screen_rotation_t rotation) {
    if (rotation != m_current_rotation) {
        performScreenRotation(rotation);
    }
}

/**
 * @brief 获取当前屏幕旋转角度
 */
screen_rotation_t LVGLDriver::getScreenRotation() const {
    return m_current_rotation;
}

/**
 * @brief 配置屏幕自动旋转检测参数
 */
void LVGLDriver::configureScreenRotation(const screen_rotation_config_t& config) {
    m_rotation_config = config;
    printf("[LVGLDriver] 屏幕自动旋转检测参数已更新\n");
    printf("  - 阈值: %.1f m/s²\n", config.threshold);
    printf("  - 稳定时间: %d ms\n", config.stable_time_ms);
    printf("  - 检测间隔: %d ms\n", config.detection_interval_ms);
    printf("  - 自动旋转: %s\n", config.auto_rotation_enabled ? "启用" : "禁用");
}

/**
 * @brief 获取当前传感器数据
 */
bool LVGLDriver::getSensorData(QMI8658_IMUData_t* accel, QMI8658_IMUData_t* gyro) {
    if (!m_rotation_initialized) {
        return false;
    }
    
    bool success = true;
    
    if (accel) {
        if (QMI8658_GetAccelerometerData(accel) == 0) {  // 修复：0表示失败
            success = false;
        }
    }
    
    if (gyro) {
        if (QMI8658_GetGyroscopeData(gyro) == 0) {  // 修复：0表示失败
            success = false;
        }
    }
    
    return success;
}

/**
 * @brief 屏幕自动旋转处理函数（基于加速度计重力感应）
 */
void LVGLDriver::processScreenRotation() {
    if (!m_rotation_initialized || !m_rotation_config.auto_rotation_enabled) {
        return;
    }
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 检查是否到了检测时间
    if (current_time - m_last_rotation_check_time < m_rotation_config.detection_interval_ms) {
        return;
    }
    
    m_last_rotation_check_time = current_time;
    
    // 读取加速度计数据
    QMI8658_IMUData_t accel;
    if (QMI8658_GetAccelerometerData(&accel) == 0) {  // 修复：0表示失败
        printf("[LVGLDriver] 错误：读取加速度计数据失败\n");
        return;  // 读取失败
    }
    
    // 打印调试数据
    static int debug_counter = 0;
    if (++debug_counter % 10 == 0) {  // 每10次打印一次，避免刷屏
        printf("[LVGLDriver] 加速度计数据: X=%.3f, Y=%.3f, Z=%.3f (m/s²)\n", 
               accel.x, accel.y, accel.z);
    }
    
    // 根据加速度计数据判断设备方向
    screen_rotation_t new_rotation = determineOrientationFromAccel(accel);
    
    // 打印方向判断结果
    if (new_rotation != m_current_rotation) {
        printf("[LVGLDriver] 检测到方向变化: %d -> %d\n", m_current_rotation, new_rotation);
    }
    
    // 检查方向是否发生变化
    if (new_rotation != m_current_rotation) {
        if (new_rotation == m_pending_rotation) {
            // 方向稳定时间累积
            uint32_t stable_duration = current_time - m_orientation_stable_time;
            if (stable_duration >= m_rotation_config.stable_time_ms) {
                // 方向已稳定足够时间，执行旋转
                printf("[LVGLDriver] 方向稳定时间达到，执行旋转: %d (稳定时间: %d ms)\n", 
                       new_rotation, stable_duration);
                performScreenRotation(new_rotation);
                m_pending_rotation = new_rotation;
            } else {
                printf("[LVGLDriver] 方向稳定中: %d (已稳定: %d/%d ms)\n", 
                       new_rotation, stable_duration, m_rotation_config.stable_time_ms);
            }
        } else {
            // 新方向，重新开始计时
            printf("[LVGLDriver] 新方向检测，开始稳定计时: %d\n", new_rotation);
            m_pending_rotation = new_rotation;
            m_orientation_stable_time = current_time;
        }
    }
}

/**
 * @brief 根据加速度计数据判断设备方向
 */
screen_rotation_t LVGLDriver::determineOrientationFromAccel(const QMI8658_IMUData_t& accel) {
    // 判断哪个轴的重力分量最大
    float abs_x = fabs(accel.x);
    float abs_y = fabs(accel.y);
    float abs_z = fabs(accel.z);
    
    // 确保重力加速度足够大，避免误判
    if (abs_x < m_rotation_config.threshold && 
        abs_y < m_rotation_config.threshold && 
        abs_z < m_rotation_config.threshold) {
        return m_current_rotation;  // 保持当前方向
    }
    
    // 根据重力方向判断屏幕方向（修正坐标系映射）
    if (abs_x > abs_y && abs_x > abs_z) {
        // X轴重力最大
        if (accel.x > 0) {
            return SCREEN_ROTATION_0;    // X轴正方向对应0度（正常方向）
        } else {
            return SCREEN_ROTATION_180;  // X轴负方向对应180度（倒置）
        }
    } else if (abs_y > abs_x && abs_y > abs_z) {
        // Y轴重力最大
        if (accel.y > 0) {
            return SCREEN_ROTATION_270;  // Y轴正方向对应270度旋转
        } else {
            return SCREEN_ROTATION_90;   // Y轴负方向对应90度旋转
        }
    }
    
    // 其他情况保持当前方向
    return m_current_rotation;
}

/**
 * @brief 执行屏幕旋转（纯软件旋转）
 * 
 * 使用LVGL的软件旋转功能，不依赖硬件旋转。
 * 这种方式适用于所有LCD控制器，包括不支持硬件旋转的QMI8658。
 */
void LVGLDriver::performScreenRotation(screen_rotation_t rotation) {
    if (!m_display || rotation == m_current_rotation) {
        return;
    }
    
    printf("[LVGLDriver] 执行软件屏幕旋转：%d -> %d\n", m_current_rotation, rotation);
    
    // 获取LVGL锁
    if (lock(1000)) {
        // 转换为LVGL旋转枚举
        lv_disp_rot_t lv_rotation;
        switch (rotation) {
            case SCREEN_ROTATION_0:
                lv_rotation = LV_DISP_ROT_NONE;
                break;
            case SCREEN_ROTATION_90:
                lv_rotation = LV_DISP_ROT_90;
                break;
            case SCREEN_ROTATION_180:
                lv_rotation = LV_DISP_ROT_180;
                break;
            case SCREEN_ROTATION_270:
                lv_rotation = LV_DISP_ROT_270;
                break;
            default:
                lv_rotation = LV_DISP_ROT_NONE;
                break;
        }
        
        // 使用LVGL纯软件旋转（不触发硬件旋转回调）
        lv_disp_set_rotation(m_display, lv_rotation);
        
        // 更新当前旋转状态
        m_current_rotation = rotation;
        
        // 立即刷新整个显示屏以确保旋转效果立即生效
        lv_obj_invalidate(lv_scr_act());
        lv_refr_now(m_display);
        
        unlock();
        
        printf("[LVGLDriver] 软件屏幕旋转完成：角度 %d度\n", rotation * 90);
        printf("[LVGLDriver] 显示分辨率：%dx%d -> %dx%d\n", 
               (rotation % 2 == 0) ? LCD_H_RES : LCD_V_RES,
               (rotation % 2 == 0) ? LCD_V_RES : LCD_H_RES,
               lv_disp_get_hor_res(m_display),
               lv_disp_get_ver_res(m_display));
    } else {
        printf("[LVGLDriver] 错误：获取LVGL锁失败，无法执行屏幕旋转\n");
    }
}
#endif

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
 * 
 * @return lv_disp_t* 返回创建的显示器对象，用于后续亮度控制等操作
 */
lv_disp_t* LVGL_Init(void) {

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
  printf("[ESP_LCD_LVGL] 向LVGL注册显示驱动（启用软件旋转）\n");
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_H_RES;                 // 水平分辨率
  disp_drv.ver_res = LCD_V_RES;                 // 垂直分辨率
  disp_drv.flush_cb = lvgl_flush_cb;            // 刷新回调函数
  disp_drv.rounder_cb = lvgl_rounder_cb;        // 区域舍入回调
  disp_drv.sw_rotate = 1;                       // 启用LVGL软件旋转功能
  disp_drv.rotated = LV_DISP_ROT_NONE;          // 设置默认旋转角度为0度
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
    lv_demo_widgets();          // 启动控件演示程序
    // 其他可选的演示程序：
    // lv_demo_music();         // 现代音乐播放器演示
    // lv_demo_stress();        // LVGL压力测试
    // lv_demo_benchmark();     // 性能基准测试
    
    lvgl_unlock();      // 释放LVGL锁
  }
  
  printf("[ESP_LCD_LVGL] 系统初始化完成！\n");
  
  // 返回创建的显示器对象
  return disp;
}

// === TCA9554 IO扩展芯片功能实现 ===

/**
 * @brief 初始化TCA9554 IO扩展芯片
 */
bool LVGLDriver::initTCA9554() {
    if (m_tca9554_initialized) {
        printf("[LVGLDriver] TCA9554已初始化，跳过重复初始化\n");
        return true;
    }
    
    printf("[LVGLDriver] 开始初始化TCA9554 IO扩展芯片...\n");
    
    // 使用传统C接口初始化TCA9554（避免I2C总线冲突）
    uint8_t result = TCA9554_Init(TCA9554_I2C_ADDRESS);
    if (result != 0) {
        printf("[LVGLDriver] 错误：TCA9554初始化失败，错误码: %d\n", result);
        return false;
    }
    
    // 配置复位引脚为输出模式（默认高电平）
    result = TCA9554_SetPinDirection(TCA9554_LCD_RESET_PIN, TCA9554_GPIO_OUTPUT);
    if (result != 0) {
        printf("[LVGLDriver] 错误：配置LCD复位引脚失败\n");
        return false;
    }
    
    result = TCA9554_SetPinDirection(TCA9554_TOUCH_RESET_PIN, TCA9554_GPIO_OUTPUT);
    if (result != 0) {
        printf("[LVGLDriver] 错误：配置触控复位引脚失败\n");
        return false;
    }
    
    result = TCA9554_SetPinDirection(TCA9554_GYRO_RESET_PIN, TCA9554_GPIO_OUTPUT);
    if (result != 0) {
        printf("[LVGLDriver] 错误：配置陀螺仪复位引脚失败\n");
        return false;
    }
    
    // 设置所有复位引脚为高电平（非复位状态）
    result = TCA9554_WritePinOutput(TCA9554_LCD_RESET_PIN, 1);
    result |= TCA9554_WritePinOutput(TCA9554_TOUCH_RESET_PIN, 1);
    result |= TCA9554_WritePinOutput(TCA9554_GYRO_RESET_PIN, 1);
    
    if (result != 0) {
        printf("[LVGLDriver] 错误：设置复位引脚初始状态失败\n");
        return false;
    }
    
    // 验证TCA9554连接状态
    if (!TCA9554_IsConnected()) {
        printf("[LVGLDriver] 错误：TCA9554设备未响应\n");
        return false;
    }
    
    m_tca9554_initialized = true;
    printf("[LVGLDriver] TCA9554初始化成功，地址: 0x%02X\n", TCA9554_I2C_ADDRESS);
    
    // 打印TCA9554状态信息
    TCA9554_PrintStatus();
    
    return true;
}

/**
 * @brief 执行屏幕和触控复位序列
 */
void LVGLDriver::performDisplayReset() {
    if (!m_tca9554_initialized) {
        printf("[LVGLDriver] 错误：TCA9554未初始化，无法执行复位\n");
        return;
    }
    
    printf("[LVGLDriver] 开始执行屏幕和触控复位序列...\n");
    
    // === 第一阶段：拉低所有复位引脚（进入复位状态） ===
    uint8_t result = 0;
    result |= TCA9554_WritePinOutput(TCA9554_LCD_RESET_PIN, 0);
    result |= TCA9554_WritePinOutput(TCA9554_TOUCH_RESET_PIN, 0);
    result |= TCA9554_WritePinOutput(TCA9554_GYRO_RESET_PIN, 0);
    
    if (result != 0) {
        printf("[LVGLDriver] 警告：拉低复位引脚时出现错误\n");
    }
    
    // 保持复位状态10ms（确保芯片完全复位）
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // === 第二阶段：先释放陀螺仪复位（陀螺仪需要最先启动） ===
    TCA9554_WritePinOutput(TCA9554_GYRO_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(5));  // 等待陀螺仪稳定
    
    // === 第三阶段：释放触控复位 ===
    TCA9554_WritePinOutput(TCA9554_TOUCH_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(20)); // 等待触控芯片稳定
    
    // === 第四阶段：最后释放LCD复位 ===
    TCA9554_WritePinOutput(TCA9554_LCD_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50)); // 等待LCD控制器稳定
    
    printf("[LVGLDriver] 复位序列完成，所有设备应已准备就绪\n");
    
    // 验证复位后的状态
    uint8_t output_status;
    if (TCA9554_ReadOutput(&output_status) == 0) {
        printf("[LVGLDriver] 复位后TCA9554输出状态: 0x%02X\n", output_status);
        printf("[LVGLDriver] LCD复位引脚: %s, 触控复位引脚: %s, 陀螺仪复位引脚: %s\n",
               (output_status & (1 << TCA9554_LCD_RESET_PIN)) ? "高" : "低",
               (output_status & (1 << TCA9554_TOUCH_RESET_PIN)) ? "高" : "低",
               (output_status & (1 << TCA9554_GYRO_RESET_PIN)) ? "高" : "低");
    }
}

/**
 * @brief 检查TCA9554连接状态
 */
bool LVGLDriver::isTCA9554Connected() {
    if (!m_tca9554_initialized) {
        return false;
    }
    
    return TCA9554_IsConnected() != 0;
}

// === 全局C接口（保持向后兼容） ===


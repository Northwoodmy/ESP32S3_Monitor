/*
 * LVGL驱动程序头文件
 * 
 * 功能特性：
 * - 支持SH8601控制器的368x448分辨率LCD屏幕
 * - 使用QSPI接口提供高速数据传输
 * - 集成LVGL图形库进行UI渲染
 * - 支持电容触摸屏输入
 * - 集成QMI8658陀螺仪实现自动屏幕旋转
 * - 多任务架构，确保UI响应流畅
 * 
 * 硬件配置：
 * - LCD分辨率：368x448像素，16位色深
 * - 接口类型：QSPI (4线SPI)
 * - 触摸接口：I2C (与陀螺仪共用)
 * - 陀螺仪接口：I2C (QMI8658)
 * - 缓冲区：双缓冲机制，提高刷新效率
 */

#ifndef LVGL_DRIVER_H
#define LVGL_DRIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl.h"
#include "I2CBusManager.h"    // 使用统一的I2C总线管理器
#include "tca9554_bsp.h"      // TCA9554 BSP接口

// 前向声明
class DisplayManager;

// 触摸活动回调函数类型
typedef void (*TouchActivityCallback)(void* userdata);

// === TCA9554控制引脚定义 ===
#define TCA9554_LCD_RESET_PIN     0    // TCA9554 GPIO0 - LCD复位引脚
#define TCA9554_POWER_RESET_PIN   1    // TCA9554 GPIO1 - 电源复位引脚
#define TCA9554_TOUCH_RESET_PIN   2    // TCA9554 GPIO2 - 触控复位引脚

// === TCA9554 I2C配置（由I2CBusManager统一管理） ===
#define TCA9554_I2C_ADDRESS       TCA9554_I2C_ADDRESS_000  // TCA9554 I2C地址（0x20）

// 陀螺仪功能配置
#define USE_GYROSCOPE 1  // 1：启用陀螺仪功能，0：禁用陀螺仪功能

#if USE_GYROSCOPE
#include "qmi8658_bsp.h"
#endif

/**
 * @brief 屏幕旋转角度枚举
 */
typedef enum {
    SCREEN_ROTATION_0 = 0,      ///< 0度（正常方向）
    SCREEN_ROTATION_90 = 1,     ///< 90度顺时针旋转
    SCREEN_ROTATION_180 = 2,    ///< 180度旋转
    SCREEN_ROTATION_270 = 3     ///< 270度顺时针旋转
} screen_rotation_t;

/**
 * @brief 屏幕自动旋转配置（基于加速度计重力感应）
 */
typedef struct {
    float threshold;            ///< 重力加速度阈值（判断旋转的临界值）
    uint32_t stable_time_ms;    ///< 稳定时间（毫秒），防止抖动
    uint32_t detection_interval_ms; ///< 检测间隔（毫秒）
    bool auto_rotation_enabled; ///< 自动旋转使能标志
} screen_rotation_config_t;

/**
 * @brief LVGL驱动管理类
 * 
 * 该类封装了LVGL显示系统的所有功能，包括：
 * - LCD显示驱动初始化
 * - 触摸屏输入处理
 * - 陀螺仪数据处理和屏幕自动旋转
 * - LVGL任务管理
 * - 显示刷新控制
 * - 触摸事件处理
 */
class LVGLDriver {
public:
    /**
     * @brief 构造函数
     */
    LVGLDriver();
    
    /**
     * @brief 析构函数
     */
    ~LVGLDriver();
    
    /**
     * @brief 初始化LVGL驱动系统
     * 
     * 执行以下初始化步骤：
     * - SPI总线配置
     * - LCD面板初始化
     * - 触摸屏初始化
     * - 陀螺仪初始化
     * - LVGL库初始化
     * - 显示缓冲区配置
     * - 输入设备注册
     * 
     * @return true 初始化成功，false 初始化失败
     */
    bool init();
    
    /**
     * @brief 启动LVGL处理任务
     * 
     * 创建并启动LVGL的主处理任务，该任务负责：
     * - 定时器处理
     * - UI渲染
     * - 事件处理
     * - 动画更新
     * - 陀螺仪数据处理
     * 
     * @return true 任务启动成功，false 任务启动失败
     */
    bool start();
    
    /**
     * @brief 停止LVGL处理任务
     * 
     * 安全停止LVGL任务并清理资源
     */
    void stop();
    
    /**
     * @brief 获取LVGL互斥锁
     * 
     * 在多任务环境中访问LVGL APIs之前必须获取锁
     * 
     * @param timeout_ms 超时时间（毫秒），-1表示永久等待
     * @return true 成功获取锁，false 获取锁超时
     */
    bool lock(int timeout_ms = -1);
    
    /**
     * @brief 释放LVGL互斥锁
     * 
     * 完成LVGL操作后必须释放锁
     */
    void unlock();
    
    /**
     * @brief 检查LVGL系统是否已初始化
     * 
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const;
    
    /**
     * @brief 检查LVGL任务是否在运行
     * 
     * @return true 任务运行中，false 任务未运行
     */
    bool isRunning() const;
    
    /**
     * @brief 获取显示器句柄
     * 
     * @return LVGL显示器句柄
     */
    lv_disp_t* getDisplay();
    
    /**
     * @brief 强制刷新显示屏
     * 
     * 立即刷新显示屏内容，用于重要更新
     */
    void forceRefresh();
    
    /**
     * @brief 设置显示屏亮度
     * 
     * @param brightness 亮度值（0-100）
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief 获取当前显示屏亮度
     * 
     * @return 当前亮度值（0-100）
     */
    uint8_t getBrightness() const;

    /**
     * @brief 初始化TCA9554 IO扩展芯片
     * 
     * 初始化TCA9554芯片，配置控制LCD、电源和触控复位的引脚
     * 注意：该函数会在LVGL初始化之前调用，以确保正确的复位时序
     * 
     * @return true 初始化成功，false 初始化失败
     */
    bool initTCA9554();
    
    /**
     * @brief 执行系统复位序列
     * 
     * 通过TCA9554控制LCD、电源和触控芯片的复位引脚，执行正确的复位时序
     * 复位时序：拉低复位引脚 -> 延时 -> 先释放电源 -> 释放触控 -> 释放LCD -> 延时稳定
     */
    void performDisplayReset();
    
    /**
     * @brief 检查TCA9554连接状态
     * 
     * @return true TCA9554已连接且正常工作，false 连接异常
     */
    bool isTCA9554Connected();
    
    /**
     * @brief 设置触摸活动回调函数
     * 
     * @param callback 触摸活动回调函数
     * @param userdata 用户数据指针
     */
    void setTouchActivityCallback(TouchActivityCallback callback, void* userdata);
    
    /**
     * @brief 清除触摸活动回调函数
     */
    void clearTouchActivityCallback();
    
    /**
     * @brief 触发触摸活动回调
     * 
     * 该函数会在触摸事件发生时被调用
     */
    void triggerTouchActivityCallback();

#if USE_GYROSCOPE
    /**
     * @brief 初始化屏幕自动旋转功能（基于加速度计）
     * 
     * @return true 初始化成功，false 初始化失败
     */
    bool initScreenRotation();
    
    /**
     * @brief 启用/禁用自动屏幕旋转
     * 
     * @param enabled true 启用，false 禁用
     */
    void setAutoRotationEnabled(bool enabled);
    
    /**
     * @brief 检查自动旋转是否启用
     * 
     * @return true 已启用，false 已禁用
     */
    bool isAutoRotationEnabled() const;
    
    /**
     * @brief 手动设置屏幕旋转角度
     * 
     * @param rotation 旋转角度
     */
    void setScreenRotation(screen_rotation_t rotation);
    
    /**
     * @brief 获取当前屏幕旋转角度
     * 
     * @return 当前旋转角度
     */
    screen_rotation_t getScreenRotation() const;
    
    /**
     * @brief 配置屏幕自动旋转检测参数
     * 
     * @param config 配置参数
     */
    void configureScreenRotation(const screen_rotation_config_t& config);
    
    /**
     * @brief 获取当前传感器数据
     * 
     * @param accel 加速度计数据
     * @param gyro 陀螺仪数据
     * @return true 读取成功，false 读取失败
     */
    bool getSensorData(QMI8658_IMUData_t* accel, QMI8658_IMUData_t* gyro);
#endif

private:
    /**
     * @brief LVGL主任务静态入口函数
     * 
     * @param arg 任务参数（LVGLDriver实例指针）
     */
    static void lvglTaskEntry(void* arg);
    
    /**
     * @brief LVGL主任务执行函数
     * 
     * 执行LVGL的主循环处理
     */
    void lvglTask();

#if USE_GYROSCOPE
    /**
     * @brief 屏幕自动旋转处理函数
     * 
     * 读取加速度计数据，判断设备方向，执行屏幕旋转
     */
    void processScreenRotation();
    
    /**
     * @brief 根据加速度计数据判断设备方向
     * 
     * @param accel 加速度计数据
     * @return 应该设置的屏幕旋转角度
     */
    screen_rotation_t determineOrientationFromAccel(const QMI8658_IMUData_t& accel);
    
    /**
     * @brief 执行屏幕旋转
     * 
     * @param rotation 目标旋转角度
     */
    void performScreenRotation(screen_rotation_t rotation);
#endif
    
    // 成员变量
    bool m_initialized;           ///< 初始化状态标志
    bool m_running;               ///< 任务运行状态标志
    TaskHandle_t m_taskHandle;    ///< LVGL任务句柄
    SemaphoreHandle_t m_mutex;    ///< LVGL互斥锁
    lv_disp_t* m_display;         ///< LVGL显示器句柄
    uint8_t m_brightness;         ///< 当前亮度值
    
    // TCA9554 IO扩展芯片相关
    bool m_tca9554_initialized;   ///< TCA9554初始化状态标志
    
    // 触摸活动回调相关
    TouchActivityCallback m_touchActivityCallback;  ///< 触摸活动回调函数
    void* m_touchActivityUserdata;                  ///< 触摸活动回调用户数据
    
#if USE_GYROSCOPE
    // 屏幕自动旋转相关成员变量
    bool m_rotation_initialized;      ///< 旋转功能初始化状态
    screen_rotation_t m_current_rotation; ///< 当前屏幕旋转角度
    screen_rotation_config_t m_rotation_config; ///< 屏幕旋转检测配置
    uint32_t m_last_rotation_check_time; ///< 上次旋转检测时间
    uint32_t m_orientation_stable_time; ///< 方向稳定时间
    screen_rotation_t m_pending_rotation; ///< 待执行的旋转角度
#endif
    
    // 任务配置常量
    static const uint32_t TASK_STACK_SIZE = 4 * 1024;  ///< 任务栈大小
    static const UBaseType_t TASK_PRIORITY = 2;         ///< 任务优先级
    static const uint32_t TASK_CORE = 1;                ///< 任务运行核心
};

// 全局LVGL驱动实例声明
extern LVGLDriver* lvglDriver;

// 对外接口函数声明
extern "C" {
    /**
     * @brief LVGL系统初始化函数
     * 
     * 提供C接口用于初始化LVGL系统
     */
    lv_disp_t* LVGL_Init(void);
}

#endif // LVGL_DRIVER_H

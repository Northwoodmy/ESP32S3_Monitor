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
 * @brief 陀螺仪旋转检测配置
 */
typedef struct {
    float threshold;            ///< 重力加速度阈值（判断旋转的临界值）
    uint32_t stable_time_ms;    ///< 稳定时间（毫秒），防止抖动
    uint32_t detection_interval_ms; ///< 检测间隔（毫秒）
    bool auto_rotation_enabled; ///< 自动旋转使能标志
} gyro_rotation_config_t;

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

#if USE_GYROSCOPE
    /**
     * @brief 初始化陀螺仪功能
     * 
     * @return true 初始化成功，false 初始化失败
     */
    bool initGyroscope();
    
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
     * @brief 配置陀螺仪旋转检测参数
     * 
     * @param config 配置参数
     */
    void configureGyroRotation(const gyro_rotation_config_t& config);
    
    /**
     * @brief 获取当前陀螺仪数据
     * 
     * @param accel 加速度计数据
     * @param gyro 陀螺仪数据
     * @return true 读取成功，false 读取失败
     */
    bool getGyroData(QMI8658_IMUData_t* accel, QMI8658_IMUData_t* gyro);
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
     * @brief 陀螺仪数据处理函数
     * 
     * 读取陀螺仪数据，判断设备方向，执行屏幕旋转
     */
    void processGyroscopeData();
    
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
    
#if USE_GYROSCOPE
    // 陀螺仪相关成员变量
    bool m_gyro_initialized;      ///< 陀螺仪初始化状态
    screen_rotation_t m_current_rotation; ///< 当前屏幕旋转角度
    gyro_rotation_config_t m_gyro_config; ///< 陀螺仪旋转检测配置
    uint32_t m_last_gyro_check_time; ///< 上次陀螺仪检测时间
    uint32_t m_orientation_stable_time; ///< 方向稳定时间
    screen_rotation_t m_pending_rotation; ///< 待执行的旋转角度
#endif
    
    // 任务配置常量
    static const uint32_t TASK_STACK_SIZE = 12 * 1024;  ///< 任务栈大小
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

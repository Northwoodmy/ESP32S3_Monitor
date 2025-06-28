/*
 * LVGL驱动程序头文件
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

#ifndef LVGL_DRIVER_H
#define LVGL_DRIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl.h"

/**
 * @brief LVGL驱动管理类
 * 
 * 该类封装了LVGL显示系统的所有功能，包括：
 * - LCD显示驱动初始化
 * - 触摸屏输入处理
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
    
    // 成员变量
    bool m_initialized;           ///< 初始化状态标志
    bool m_running;               ///< 任务运行状态标志
    TaskHandle_t m_taskHandle;    ///< LVGL任务句柄
    SemaphoreHandle_t m_mutex;    ///< LVGL互斥锁
    lv_disp_t* m_display;         ///< LVGL显示器句柄
    uint8_t m_brightness;         ///< 当前亮度值
    
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
    void LVGL_Init(void);
}

#endif // LVGL_DRIVER_H

/*
 * Monitor.h - 监控器类头文件
 * ESP32S3监控项目
 */

#ifndef MONITOR_H
#define MONITOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 前向声明
class PSRAMManager;

class Monitor {
public:
    Monitor();
    ~Monitor();
    
    // 初始化监控器
    void init();
    void init(PSRAMManager* psramManager);
    
    // 停止监控器
    void stop();

private:
    // 任务句柄
    TaskHandle_t helloTaskHandle;
    
    // PSRAM管理器指针
    PSRAMManager* m_psramManager;
    
    // 静态任务函数
    static void helloWorldTask(void* parameter);
    
    // 任务运行标志
    bool isRunning;
};

#endif // MONITOR_H 
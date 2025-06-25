/*
 * Monitor.h - 监控器类头文件
 * ESP32S3监控项目
 */

#ifndef MONITOR_H
#define MONITOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Monitor {
public:
    Monitor();
    ~Monitor();
    
    // 初始化监控器
    void init();
    
    // 停止监控器
    void stop();

private:
    // 任务句柄
    TaskHandle_t helloTaskHandle;
    
    // 静态任务函数
    static void helloWorldTask(void* parameter);
    
    // 任务运行标志
    bool isRunning;
};

#endif // MONITOR_H 
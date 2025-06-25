/*
 * Monitor.cpp - 监控器类实现文件
 * ESP32S3监控项目
 */

#include "Monitor.h"
#include "Arduino.h"

Monitor::Monitor() : helloTaskHandle(nullptr), isRunning(false) {
}

Monitor::~Monitor() {
    stop();
}

void Monitor::init() {
    if (isRunning) {
        printf("监控器已经在运行中\n");
        return;
    }
    
    printf("正在启动Hello World监控任务...\n");
    
    // 先设置运行标志，避免竞态条件
    isRunning = true;
    
    // 创建Hello World输出任务
    // 任务优先级设置为2，栈大小2048字节
    BaseType_t result = xTaskCreate(
        helloWorldTask,         // 任务函数
        "HelloWorldTask",       // 任务名称
        2048,                   // 栈大小
        this,                   // 传递给任务的参数
        2,                      // 任务优先级
        &helloTaskHandle        // 任务句柄
    );
    
    if (result == pdPASS) {
        printf("Hello World监控任务创建成功\n");
    } else {
        isRunning = false;  // 任务创建失败，重置标志
        printf("Hello World监控任务创建失败\n");
    }
}

void Monitor::stop() {
    if (!isRunning) {
        return;
    }
    
    printf("正在停止监控任务...\n");
    
    if (helloTaskHandle != nullptr) {
        vTaskDelete(helloTaskHandle);
        helloTaskHandle = nullptr;
    }
    
    isRunning = false;
    printf("监控任务已停止\n");
}

void Monitor::helloWorldTask(void* parameter) {
    Monitor* monitor = static_cast<Monitor*>(parameter);
    
    printf("Hello World任务开始运行\n");
    
    // 任务循环 - 使用无限循环，避免竞态条件
    while (true) {
        // 检查是否需要停止
        if (!monitor->isRunning) {
            break;
        }
        
        // 输出hello world
        printf("hello world\n");
        
        // 延时1秒
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    printf("Hello World任务结束\n");
    vTaskDelete(nullptr);
} 
/*
 * Monitor.cpp - 监控器类实现文件
 * ESP32S3监控项目
 */

#include "Monitor.h"
#include "PSRAMManager.h"
#include "Arduino.h"

Monitor::Monitor() : helloTaskHandle(nullptr), m_psramManager(nullptr), isRunning(false) {
}

Monitor::~Monitor() {
    stop();
}

void Monitor::init() {
    init(nullptr);
}

void Monitor::init(PSRAMManager* psramManager) {
    if (isRunning) {
        printf("监控器已经在运行中\n");
        return;
    }
    
    m_psramManager = psramManager;
    
    printf("正在启动Hello World监控任务...\n");
    
    // 先设置运行标志，避免竞态条件
    isRunning = true;
    
    if (m_psramManager && m_psramManager->isPSRAMAvailable()) {
        // 使用PSRAM栈创建任务
        printf("使用PSRAM栈创建Hello World任务\n");
        helloTaskHandle = m_psramManager->createTaskWithPSRAMStack(
            helloWorldTask,         // 任务函数
            "HelloWorldTask",       // 任务名称
            2048,                   // 栈大小
            this,                   // 传递给任务的参数
            2,                      // 任务优先级
            0                       // 运行在核心0
        );
        
        if (helloTaskHandle != nullptr) {
            printf("Hello World监控任务(PSRAM栈)创建成功\n");
        } else {
            isRunning = false;
            printf("Hello World监控任务(PSRAM栈)创建失败\n");
        }
    } else {
        // 回退到SRAM栈创建任务
        printf("使用SRAM栈创建Hello World任务\n");
        BaseType_t result = xTaskCreatePinnedToCore(
            helloWorldTask,         // 任务函数
            "HelloWorldTask",       // 任务名称
            2048,                   // 栈大小
            this,                   // 传递给任务的参数
            2,                      // 任务优先级
            &helloTaskHandle,       // 任务句柄
            0                       // 运行在核心0
        );
        
        if (result == pdPASS) {
            printf("Hello World监控任务(SRAM栈)创建成功\n");
        } else {
            isRunning = false;  // 任务创建失败，重置标志
            printf("Hello World监控任务(SRAM栈)创建失败\n");
        }
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
    
    // 只输出一次hello world
    printf("hello world\n");
    
    // 任务完成，清理状态
    monitor->isRunning = false;
    monitor->helloTaskHandle = nullptr;
    
    printf("Hello World任务结束\n");
    vTaskDelete(nullptr);
} 
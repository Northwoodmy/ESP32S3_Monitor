/*
 * Monitor.h - 监控器类头文件
 * ESP32S3监控项目
 */

#ifndef MONITOR_H
#define MONITOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
    TaskHandle_t monitorTaskHandle;
    
    // PSRAM管理器指针
    PSRAMManager* m_psramManager;
    
    // 静态任务函数
    static void monitoringTask(void* parameter);
    
    // 任务运行标志
    bool isRunning;
    
    // HTTP客户端
    HTTPClient httpClient;
    
    // 监控配置
    String metricsUrl;
    uint32_t requestInterval;  // 请求间隔(ms)
    
    // 私有方法
    bool fetchMetricsData();
    void parseAndDisplayMetrics(const String& jsonData);
    void displayPortInfo(JsonObject port);
    void displaySystemInfo(JsonObject system);
    void displayWiFiInfo(JsonObject wifi);
    bool isWiFiConnected();
};

#endif // MONITOR_H 
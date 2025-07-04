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
class ConfigStorage;

class Monitor {
public:
    Monitor();
    ~Monitor();
    
    // 初始化监控器
    void init();
    void init(PSRAMManager* psramManager);
    void init(PSRAMManager* psramManager, ConfigStorage* configStorage);
    
    // 停止监控器
    void stop();

private:
    // 任务句柄
    TaskHandle_t monitorTaskHandle;
    
    // PSRAM管理器指针
    PSRAMManager* m_psramManager;
    
    // 配置存储管理器指针
    ConfigStorage* m_configStorage;
    
    // 静态任务函数
    static void monitoringTask(void* parameter);
    
    // 任务运行标志
    bool isRunning;
    
    // HTTP客户端
    HTTPClient httpClient;
    
    // 监控配置
    String metricsUrl;
    uint32_t requestInterval;  // 请求间隔(ms)
    uint32_t connectionTimeout;  // 连接超时(ms)
    bool serverEnabled;  // 服务器监控是否启用
    
    // 私有方法
    bool fetchMetricsData();
    void parseAndDisplayMetrics(const String& jsonData);
    void displayPortInfo(JsonObject port);
    void displaySystemInfo(JsonObject system);
    void displayWiFiInfo(JsonObject wifi);
    bool isWiFiConnected();
    void loadServerConfig();  // 加载服务器配置
    void setDefaultConfig();  // 设置默认配置
};

#endif // MONITOR_H 
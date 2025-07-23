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
#include "PowerMonitorData.h"
#include "MDNSScanner.h"

// 前向声明
class PSRAMManager;
class ConfigStorage;
class DisplayManager;

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
    
    // 设置数据回调
    void setPowerDataCallback(PowerDataCallback callback, void* userData);
    
    // 获取当前功率数据
    const PowerMonitorData& getCurrentPowerData() const;

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
    bool autoScanServer;  // 自动扫描服务器开关
    
    // 自动扫描相关变量
    uint32_t m_consecutiveFailures;  // 连续失败次数
    unsigned long m_lastScanTime;    // 上次扫描时间
    static const uint32_t MAX_FAILURES_BEFORE_SCAN = 3;    // 触发扫描的失败次数阈值
    static const unsigned long SCAN_COOLDOWN_MS = 30000;   // 扫描冷却时间（30秒）
    
    // 数据回调
    PowerDataCallback m_powerDataCallback;  // 功率数据回调函数
    void* m_callbackUserData;              // 回调用户数据
    
    // 当前功率数据
    PowerMonitorData m_currentPowerData;
    
    // 私有方法
    bool fetchMetricsData();
    void parseAndDisplayMetrics(const String& jsonData);
    void displayPortInfo(JsonObject port);
    void displaySystemInfo(JsonObject system);
    void displayWiFiInfo(JsonObject wifi);
    bool isWiFiConnected();
    void loadServerConfig();  // 加载服务器配置
    void setDefaultConfig();  // 设置默认配置
    
    // 自动扫描相关方法
    bool shouldTriggerAutoScan();   // 判断是否应该触发自动扫描
    bool performAutoScan();         // 执行自动扫描
    bool updateServerUrl(const String& newUrl);  // 更新服务器URL并保存配置
    void resetFailureCounter();     // 重置失败计数器
    
    // 功率数据处理
    void processPortData(JsonObject port);
    void processSystemData(JsonObject system);
    void processWiFiData(JsonObject wifi);
    void calculateTotalPower();
    void updatePowerData();
    void triggerDataCallback();
};

#endif // MONITOR_H 
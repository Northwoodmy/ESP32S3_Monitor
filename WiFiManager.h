/*
 * WiFiManager.h - WiFi管理器类头文件
 * ESP32S3监控项目 - WiFi配置模块
 */

#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <WiFi.h>
#include <WiFiAP.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ConfigStorage.h"

// 前向声明
class PSRAMManager;

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();
    
    // 初始化WiFi管理器
    void init(ConfigStorage* storage = nullptr);
    void setPSRAMManager(PSRAMManager* psramManager);
    
    // 启动AP配置模式
    void startConfigMode();
    
    // 尝试连接到WiFi
    bool connectToWiFi(const String& ssid, const String& password);
    
    // 多WiFi配置连接功能
    bool connectToMultiWiFi();
    bool tryConnectToConfig(const WiFiConfig& config, int timeoutSeconds = 10);
    
    // 获取当前WiFi状态
    bool isConnected();
    
    // 获取当前IP地址
    String getLocalIP();
    
    // 获取AP模式IP地址
    String getAPIP();
    
    // 停止WiFi管理器
    void stop();

private:
    ConfigStorage* configStorage;
    PSRAMManager* m_psramManager;
    TaskHandle_t wifiTaskHandle;
    bool isRunning;
    bool isAPMode;
    
    // 静态任务函数
    static void wifiManagementTask(void* parameter);
    
    // WiFi连接任务
    void wifiTask();
    
    // 尝试从存储的配置连接WiFi
    void tryStoredCredentials();
    
    // 多WiFi配置相关私有方法
    void tryMultiWiFiConfigs();
    
    // 统一的WiFi重连方法
    bool attemptWiFiReconnection();
    
    // 检查是否有保存的WiFi配置
    bool hasStoredWiFiConfig();
};

#endif // WIFIMANAGER_H 
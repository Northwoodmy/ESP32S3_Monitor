/*
 * WiFiManager.cpp - WiFi管理器类实现文件
 * ESP32S3监控项目 - WiFi配置模块
 */

#include "WiFiManager.h"
#include "Arduino.h"

WiFiManager::WiFiManager() : 
    configStorage(nullptr),
    wifiTaskHandle(nullptr),
    isRunning(false),
    isAPMode(false) {
    configStorage = new ConfigStorage();
}

WiFiManager::~WiFiManager() {
    stop();
    if (configStorage) {
        delete configStorage;
        configStorage = nullptr;
    }
}

void WiFiManager::init() {
    printf("初始化WiFi管理器...\n");
    
    if (configStorage) {
        configStorage->init();
    }
    
    // 尝试从存储的配置连接WiFi
    tryStoredCredentials();
    
    // 如果没有配置或连接失败，启动AP模式
    if (!isConnected()) {
        printf("未找到有效WiFi配置或连接失败，启动AP配置模式\n");
        startConfigMode();
    }
    
    // 启动WiFi管理任务
    isRunning = true;
    BaseType_t result = xTaskCreate(
        wifiManagementTask,
        "WiFiManagement",
        4096,
        this,
        3,
        &wifiTaskHandle
    );
    
    if (result == pdPASS) {
        printf("WiFi管理任务创建成功\n");
    } else {
        isRunning = false;
        printf("WiFi管理任务创建失败\n");
    }
}

void WiFiManager::startConfigMode() {
    printf("启动AP配置模式...\n");
    
    WiFi.mode(WIFI_AP);
    
    // 设置AP参数
    String apSSID = "ESP32S3-Config";
    String apPassword = "12345678";
    
    bool success = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    if (success) {
        isAPMode = true;
        printf("AP模式启动成功\n");
        printf("AP SSID: %s\n", apSSID.c_str());
        printf("AP Password: %s\n", apPassword.c_str());
        printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        printf("AP模式启动失败\n");
    }
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
    printf("尝试连接WiFi: %s\n", ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // 等待连接，最多30秒
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        attempts++;
        printf(".");
    }
    printf("\n");
    
    if (WiFi.status() == WL_CONNECTED) {
        isAPMode = false;
        printf("WiFi连接成功\n");
        printf("IP地址: %s\n", WiFi.localIP().toString().c_str());
        
        // 保存配置到NVS
        if (configStorage) {
            configStorage->saveWiFiConfig(ssid, password);
        }
        
        return true;
    } else {
        printf("WiFi连接失败\n");
        return false;
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getLocalIP() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "";
}

String WiFiManager::getAPIP() {
    if (isAPMode) {
        return WiFi.softAPIP().toString();
    }
    return "";
}

void WiFiManager::stop() {
    if (!isRunning) {
        return;
    }
    
    printf("停止WiFi管理器...\n");
    
    if (wifiTaskHandle != nullptr) {
        vTaskDelete(wifiTaskHandle);
        wifiTaskHandle = nullptr;
    }
    
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    
    isRunning = false;
    isAPMode = false;
    
    printf("WiFi管理器已停止\n");
}

void WiFiManager::wifiManagementTask(void* parameter) {
    WiFiManager* manager = static_cast<WiFiManager*>(parameter);
    manager->wifiTask();
}

void WiFiManager::wifiTask() {
    printf("WiFi管理任务开始运行\n");
    
    while (isRunning) {
        // 检查WiFi连接状态
        if (!isAPMode && !isConnected()) {
            printf("WiFi连接丢失，尝试重新连接...\n");
            
            // 尝试重新连接存储的WiFi
            String ssid, password;
            if (configStorage && configStorage->loadWiFiConfig(ssid, password)) {
                if (!connectToWiFi(ssid, password)) {
                    printf("重新连接失败，启动AP模式\n");
                    startConfigMode();
                }
            } else {
                printf("没有存储的WiFi配置，启动AP模式\n");
                startConfigMode();
            }
        }
        
        // 每10秒检查一次
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    
    printf("WiFi管理任务结束\n");
    vTaskDelete(nullptr);
}

void WiFiManager::tryStoredCredentials() {
    if (!configStorage || !configStorage->hasWiFiConfig()) {
        printf("没有存储的WiFi配置\n");
        return;
    }
    
    String ssid, password;
    if (configStorage->loadWiFiConfig(ssid, password)) {
        printf("尝试使用存储的WiFi配置连接\n");
        connectToWiFi(ssid, password);
    }
} 
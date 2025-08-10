/*
 * WiFiManager.cpp - WiFi管理器类实现文件
 * ESP32S3监控项目 - WiFi配置模块
 */

#include "WiFiManager.h"
#include "PSRAMManager.h"
#include "Arduino.h"

WiFiManager::WiFiManager() : 
    configStorage(nullptr),
    m_psramManager(nullptr),
    wifiTaskHandle(nullptr),
    isRunning(false),
    isAPMode(false) {
    configStorage = new ConfigStorage();
}

WiFiManager::~WiFiManager() {
    stop();
}

void WiFiManager::setPSRAMManager(PSRAMManager* psramManager) {
    m_psramManager = psramManager;
}

void WiFiManager::init(ConfigStorage* storage) {
    printf("初始化WiFi管理器...\n");
    
    // 使用传入的配置存储实例，如果没有传入则创建新的
    if (storage != nullptr) {
        configStorage = storage;
        printf("使用外部配置存储实例\n");
    } else {
        printf("创建独立的配置存储实例\n");
        configStorage = new ConfigStorage();
        configStorage->init();
        
        // 启动配置存储任务
        printf("启动配置存储任务...\n");
        if (!configStorage->startTask()) {
            printf("❌ [WiFiManager] 配置存储任务启动失败\n");
            return;
        }
    }
    
    printf("WiFi模块状态检查...\n");
    
    // 确保WiFi模块正常工作
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 尝试从存储的配置连接WiFi
    tryStoredCredentials();
    
    // 如果没有配置或连接失败，启动AP模式
    if (!isConnected()) {
        printf("未找到有效WiFi配置或连接失败，启动AP配置模式\n");
        startConfigMode();
    }
    
    // 启动WiFi管理任务（运行在核心0）
    isRunning = true;
    
    if (m_psramManager && m_psramManager->isPSRAMAvailable()) {
        // 使用PSRAM栈创建任务
        printf("使用PSRAM栈创建WiFi管理任务\n");
        wifiTaskHandle = m_psramManager->createTaskWithPSRAMStack(
            wifiManagementTask,
            "WiFiManagement",
            4096,
            this,
            3,
            0                   // 运行在核心0
        );
        
        if (wifiTaskHandle != nullptr) {
            printf("WiFi管理任务(PSRAM栈)创建成功\n");
        } else {
            isRunning = false;
            printf("WiFi管理任务(PSRAM栈)创建失败\n");
        }
    } else {
        // 回退到SRAM栈创建任务
        printf("使用SRAM栈创建WiFi管理任务\n");
        BaseType_t result = xTaskCreatePinnedToCore(
            wifiManagementTask,
            "WiFiManagement",
            4096,
            this,
            3,
            &wifiTaskHandle,
            0                   // 运行在核心0
        );
        
        if (result == pdPASS) {
            printf("WiFi管理任务(SRAM栈)创建成功\n");
        } else {
            isRunning = false;
            printf("WiFi管理任务(SRAM栈)创建失败\n");
        }
    }
}

void WiFiManager::startConfigMode() {
    printf("启动AP配置模式...\n");
    
    // 确保WiFi完全关闭后再启动AP
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    printf("设置WiFi为AP模式...\n");
    WiFi.mode(WIFI_AP);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 设置AP参数
    String apSSID = "ESP32S3-Config";
    String apPassword = "12345678";
    
    printf("尝试启动AP: %s\n", apSSID.c_str());
    
    // 配置AP参数
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),  // AP IP
        IPAddress(192, 168, 4, 1),  // Gateway
        IPAddress(255, 255, 255, 0) // Subnet
    );
    
    bool success = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), 1, 0, 4);
    
    // 等待AP启动
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    if (success) {
        isAPMode = true;
        IPAddress apIP = WiFi.softAPIP();
        printf("✓ AP模式启动成功\n");
        printf("  AP SSID: %s\n", apSSID.c_str());
        printf("  AP Password: %s\n", apPassword.c_str());
        printf("  AP IP: %s\n", apIP.toString().c_str());
        printf("  配置网址: http://%s\n", apIP.toString().c_str());
        
        // 验证AP是否真的在工作
        if (apIP.toString() == "0.0.0.0") {
            printf("⚠ 警告: AP IP地址异常，尝试重新启动\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            WiFi.softAP(apSSID.c_str(), apPassword.c_str());
            vTaskDelay(pdMS_TO_TICKS(1000));
            printf("重试后AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        }
    } else {
        printf("✗ AP模式启动失败\n");
        printf("WiFi状态: %d\n", WiFi.status());
        printf("AP状态: %d\n", WiFi.getMode());
        
        // 尝试重新启动
        printf("尝试重新启动AP...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        WiFi.mode(WIFI_OFF);
        vTaskDelay(pdMS_TO_TICKS(1000));
        WiFi.mode(WIFI_AP);
        vTaskDelay(pdMS_TO_TICKS(500));
        success = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
        if (success) {
            isAPMode = true;
            printf("✓ 重试后AP启动成功\n");
            printf("  AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        } else {
            printf("✗ 重试后仍然失败\n");
        }
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
        
        // 异步保存配置到NVS (使用新的多WiFi配置系统)
        if (configStorage) {
            bool saveResult = configStorage->addWiFiConfigAsync(ssid, password, 5000);
            if (!saveResult) {
                printf("⚠ 警告: WiFi连接成功但配置保存失败\n");
            }
        } else {
            printf("⚠ 警告: 配置存储未初始化\n");
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
    
    if (configStorage) {
        delete configStorage;
        configStorage = nullptr;
    }
    
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
    
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 120000; // 2分钟尝试一次重连
    int reconnectAttempts = 0;
    const int MAX_RECONNECT_ATTEMPTS = 10; // 最多连续尝试10次
    
    while (isRunning) {
        unsigned long currentTime = millis();
        
        // 检查WiFi连接状态
        if (!isConnected()) {
            if (!isAPMode) {
                // STA模式下连接丢失，立即尝试重连
                printf("WiFi连接丢失，尝试重新连接...\n");
                
                if (attemptWiFiReconnection()) {
                    printf("WiFi重连成功\n");
                    reconnectAttempts = 0; // 重置重连计数
                } else {
                    printf("WiFi重连失败，启动AP模式\n");
                    startConfigMode();
                    lastReconnectAttempt = currentTime;
                    reconnectAttempts = 0;
                }
            } else {
                // AP模式下，只有在保存了WiFi配置的情况下才尝试重新连接
                if (hasStoredWiFiConfig()) {
                    if (currentTime - lastReconnectAttempt >= RECONNECT_INTERVAL && 
                        reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                        
                        printf("AP模式下尝试重新连接到已知WiFi网络... (尝试 %d/%d)\n", 
                               reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
                        
                        if (attemptWiFiReconnection()) {
                            printf("✓ WiFi重连成功，退出AP模式\n");
                            // 关闭AP模式，切换到STA模式
                            WiFi.softAPdisconnect(true);
                            isAPMode = false;
                            reconnectAttempts = 0;
                        } else {
                            printf("✗ WiFi重连失败 (尝试 %d/%d)\n", 
                                   reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
                            reconnectAttempts++;
                            
                            // 如果达到最大重连次数，等待更长时间再重试
                            if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
                                printf("达到最大重连次数(%d次)，将在10分钟后重新开始尝试\n", MAX_RECONNECT_ATTEMPTS);
                                lastReconnectAttempt = currentTime + 480000; // 额外等待8分钟(总共10分钟)
                                reconnectAttempts = 0;
                            }
                        }
                        
                        lastReconnectAttempt = currentTime;
                    }
                } else {
                    // 没有保存WiFi配置，不进行重连尝试
                    if (reconnectAttempts == 0) {
                        printf("AP模式：没有保存的WiFi配置，不进行重连尝试\n");
                        reconnectAttempts = -1; // 标记为已检查过，避免重复打印
                    }
                }
            }
        } else {
            // WiFi已连接，确保不在AP模式
            if (isAPMode) {
                printf("WiFi已连接，关闭AP模式\n");
                WiFi.softAPdisconnect(true);
                isAPMode = false;
            }
            reconnectAttempts = 0; // 重置重连计数
        }
        
        // 如果在AP模式下且之前标记为无配置(-1)，但现在有配置了，重置计数器
        if (isAPMode && reconnectAttempts == -1 && hasStoredWiFiConfig()) {
            printf("检测到新的WiFi配置，重新开始重连尝试\n");
            reconnectAttempts = 0;
            lastReconnectAttempt = 0; // 立即开始尝试
        }
        
        // 每10秒检查一次
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    
    printf("WiFi管理任务结束\n");
    vTaskDelete(nullptr);
}

void WiFiManager::tryStoredCredentials() {
    // 优先尝试多WiFi配置
    if (configStorage && configStorage->getWiFiConfigCountAsync(3000) > 0) {
        printf("尝试使用多WiFi配置连接\n");
        if (connectToMultiWiFi()) {
            return;
        }
    }
    
    // 回退到单WiFi配置
    if (!configStorage || !configStorage->hasWiFiConfigAsync(3000)) {
        printf("没有存储的WiFi配置\n");
        return;
    }
    
    String ssid, password;
    if (configStorage->loadWiFiConfigAsync(ssid, password, 3000)) {
        printf("尝试使用单WiFi配置连接\n");
        connectToWiFi(ssid, password);
    }
}

// 多WiFi配置连接功能实现

bool WiFiManager::connectToMultiWiFi() {
    printf("开始多WiFi配置连接尝试\n");
    
    if (!configStorage) {
        printf("配置存储未初始化\n");
        return false;
    }
    
    WiFiConfig configs[3];
    if (!configStorage->loadWiFiConfigsAsync(configs, 3000)) {
        printf("加载多WiFi配置失败\n");
        return false;
    }
    
    // 按顺序尝试连接每个配置
    for (int i = 0; i < ConfigStorage::MAX_WIFI_CONFIGS; i++) {
        if (configs[i].isValid && configs[i].ssid.length() > 0) {
            printf("尝试连接WiFi配置 %d: %s\n", i + 1, configs[i].ssid.c_str());
            
            if (tryConnectToConfig(configs[i], 15)) {
                printf("✓ WiFi配置 %d 连接成功\n", i + 1);
                isAPMode = false;
                return true;
            } else {
                printf("✗ WiFi配置 %d 连接失败\n", i + 1);
            }
        }
    }
    
    printf("所有WiFi配置连接失败\n");
    return false;
}

bool WiFiManager::tryConnectToConfig(const WiFiConfig& config, int timeoutSeconds) {
    if (!config.isValid || config.ssid.length() == 0) {
        printf("WiFi配置无效\n");
        return false;
    }
    
    printf("连接到 %s...\n", config.ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    
    // 等待连接
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < timeoutSeconds) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        attempts++;
        if (attempts % 5 == 0) {
            printf("连接中... (%d/%d)\n", attempts, timeoutSeconds);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        printf("✓ 连接成功\n");
        printf("  IP地址: %s\n", WiFi.localIP().toString().c_str());
        printf("  信号强度: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        printf("✗ 连接超时或失败\n");
        WiFi.disconnect();
        return false;
    }
}

void WiFiManager::tryMultiWiFiConfigs() {
    printf("尝试多WiFi配置连接\n");
    
    if (connectToMultiWiFi()) {
        printf("多WiFi配置连接成功\n");
    } else {
        printf("多WiFi配置连接失败，启动AP模式\n");
        startConfigMode();
    }
}

bool WiFiManager::attemptWiFiReconnection() {
    printf("开始WiFi重连尝试\n");
    
    // 优先尝试多WiFi配置
    if (configStorage && configStorage->getWiFiConfigCountAsync(3000) > 0) {
        printf("尝试多WiFi配置重连\n");
        if (connectToMultiWiFi()) {
            return true;
        }
    }
    
    // 回退到单WiFi配置
    String ssid, password;
    if (configStorage && configStorage->loadWiFiConfigAsync(ssid, password, 3000)) {
        printf("尝试单WiFi配置重连\n");
        if (connectToWiFi(ssid, password)) {
            return true;
        }
    } else {
        printf("没有存储的WiFi配置\n");
    }
    
    return false;
}

bool WiFiManager::hasStoredWiFiConfig() {
    if (!configStorage) {
        return false;
    }
    
    // 检查是否有多WiFi配置
    if (configStorage->getWiFiConfigCountAsync(1000) > 0) {
        return true;
    }
    
    // 检查是否有单WiFi配置
    if (configStorage->hasWiFiConfigAsync(1000)) {
        return true;
    }
    
    return false;
} 
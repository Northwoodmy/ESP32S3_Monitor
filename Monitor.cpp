/*
 * Monitor.cpp - 监控器类实现文件
 * ESP32S3监控项目
 */

#include "Monitor.h"
#include "PSRAMManager.h"
#include "ConfigStorage.h"
#include "DisplayManager.h"
#include "Arduino.h"

Monitor::Monitor() : monitorTaskHandle(nullptr), m_psramManager(nullptr), m_configStorage(nullptr), isRunning(false), m_powerDataCallback(nullptr), m_callbackUserData(nullptr) {
    // 设置默认配置
    setDefaultConfig();
    
    // 初始化功率数据
    memset(&m_currentPowerData, 0, sizeof(m_currentPowerData));
    m_currentPowerData.port_count = 4;
    m_currentPowerData.valid = false;
}

Monitor::~Monitor() {
    stop();
}

void Monitor::init() {
    init(nullptr);
}

void Monitor::init(PSRAMManager* psramManager) {
    init(psramManager, nullptr);
}

void Monitor::init(PSRAMManager* psramManager, ConfigStorage* configStorage) {
    if (isRunning) {
        printf("监控器已经在运行中\n");
        return;
    }
    
    m_psramManager = psramManager;
    m_configStorage = configStorage;
    
    // 如果提供了配置存储，加载服务器配置
    if (m_configStorage) {
        loadServerConfig();
    }
    
    // 如果服务器监控未启用，不启动监控任务
    if (!serverEnabled) {
        printf("服务器监控已禁用，不启动监控任务\n");
        return;
    }
    
    printf("正在启动系统监控任务...\n");
    printf("目标URL: %s\n", metricsUrl.c_str());
    printf("请求间隔: %d ms\n", requestInterval);
    printf("连接超时: %d ms\n", connectionTimeout);
    
    // 先设置运行标志，避免竞态条件
    isRunning = true;
    
    // 对于网络相关任务，必须使用内部SRAM栈，避免TCP ISN钩子断言失败
    // 不使用PSRAM栈，因为lwip要求网络操作在内部RAM中进行
    printf("使用SRAM栈创建监控任务(网络操作要求)\n");
    BaseType_t result = xTaskCreatePinnedToCore(
        monitoringTask,         // 任务函数
        "MonitoringTask",       // 任务名称
        8192,                   // 栈大小(增大以支持HTTP和JSON处理)
        this,                   // 传递给任务的参数
        3,                      // 任务优先级
        &monitorTaskHandle,     // 任务句柄
        0                       // 运行在核心0
    );
    
    if (result == pdPASS) {
        printf("系统监控任务(SRAM栈)创建成功\n");
    } else {
        isRunning = false;  // 任务创建失败，重置标志
        printf("系统监控任务(SRAM栈)创建失败\n");
    }
}

void Monitor::stop() {
    if (!isRunning) {
        return;
    }
    
    printf("正在停止监控任务...\n");
    
    if (monitorTaskHandle != nullptr) {
        vTaskDelete(monitorTaskHandle);
        monitorTaskHandle = nullptr;
    }
    
    isRunning = false;
    printf("监控任务已停止\n");
}

void Monitor::setPowerDataCallback(PowerDataCallback callback, void* userData) {
    m_powerDataCallback = callback;
    m_callbackUserData = userData;
    printf("功率数据回调已设置\n");
}

const PowerMonitorData& Monitor::getCurrentPowerData() const {
    return m_currentPowerData;
}

void Monitor::setDefaultConfig() {
    metricsUrl = "http://10.10.168.168/metrics.json";
    requestInterval = 250;  // 250毫秒请求一次
    connectionTimeout = 1000;  // 1秒连接超时
    serverEnabled = true;  // 默认启用服务器监控
}

void Monitor::loadServerConfig() {
    if (!m_configStorage) {
        printf("ConfigStorage未初始化，使用默认配置\n");
        return;
    }
    
    printf("正在加载服务器配置...\n");
    
    // 检查是否有服务器配置
    if (!m_configStorage->hasServerConfigAsync()) {
        printf("未找到服务器配置，使用默认配置\n");
        return;
    }
    
    // 加载服务器配置
    String serverUrl;
    int requestIntervalConfig;
    bool enabled;
    int connectionTimeoutConfig;
    bool autoGetData;
    bool autoScanServer;
    
    if (m_configStorage->loadServerConfigAsync(serverUrl, requestIntervalConfig, enabled, connectionTimeoutConfig, autoGetData, autoScanServer)) {
        printf("服务器配置加载成功\n");
        
        // 应用配置
        if (serverUrl.length() > 0) {
            metricsUrl = serverUrl;
            printf("服务器URL: %s\n", metricsUrl.c_str());
        }
        
        if (requestIntervalConfig > 0) {
            requestInterval = requestIntervalConfig;
            printf("请求间隔: %d ms\n", requestInterval);
        }
        
        if (connectionTimeoutConfig > 0) {
            connectionTimeout = connectionTimeoutConfig;
            printf("连接超时: %d ms\n", connectionTimeout);
        }
        
        serverEnabled = enabled;
        printf("服务器监控: %s\n", serverEnabled ? "启用" : "禁用");
    } else {
        printf("服务器配置加载失败，使用默认配置\n");
    }
}

void Monitor::monitoringTask(void* parameter) {
    Monitor* monitor = static_cast<Monitor*>(parameter);
    
    printf("系统监控任务开始运行\n");
    
    while (monitor->isRunning) {
        // 检查WiFi连接状态
        if (monitor->isWiFiConnected()) {
            // 获取并解析监控数据
            monitor->fetchMetricsData();
        } else {
            printf("WiFi未连接，跳过监控数据获取\n");
        }
        
        // 延时等待下次请求
        vTaskDelay(pdMS_TO_TICKS(monitor->requestInterval));
    }
    
    printf("系统监控任务结束\n");
    vTaskDelete(nullptr);
}

bool Monitor::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool Monitor::fetchMetricsData() {
    if (!isWiFiConnected()) {
        printf("WiFi未连接，无法获取监控数据\n");
        return false;
    }
    
    // 配置HTTP客户端，优化内存使用
    httpClient.begin(metricsUrl);
    httpClient.setTimeout(connectionTimeout);  // 使用配置的连接超时
    httpClient.setConnectTimeout(5000);  // 5秒连接超时
    
    // 设置HTTP请求头，减少不必要的数据传输
    httpClient.addHeader("Accept", "application/json");
    httpClient.addHeader("Connection", "close");  // 请求完成后立即关闭连接
    
    int httpCode = httpClient.GET();
    
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = httpClient.getString();
            
            // 解析数据（不输出调试信息）
            parseAndDisplayMetrics(payload);
            
            httpClient.end();
            return true;
        } else {
            printf("HTTP请求失败，状态码: %d\n", httpCode);
        }
    } else {
        printf("HTTP请求失败，错误: %s\n", httpClient.errorToString(httpCode).c_str());
    }
    
    httpClient.end();
    return false;
}

void Monitor::parseAndDisplayMetrics(const String& jsonData) {
    // 使用StaticJsonDocument避免动态内存分配，减少对SRAM的压力
    // 8KB应该足够解析metrics.json数据
    StaticJsonDocument<8192> doc;
    
    // 解析JSON数据
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        printf("JSON解析失败: %s\n", error.c_str());
        return;
    }
    
    // 重置当前数据
    memset(&m_currentPowerData, 0, sizeof(m_currentPowerData));
    m_currentPowerData.port_count = 4;
    m_currentPowerData.timestamp = millis();
    
    // 解析端口信息
    if (doc.containsKey("ports")) {
        JsonArray ports = doc["ports"];
        for (JsonObject port : ports) {
            processPortData(port);
            displayPortInfo(port);  // 保留原有的显示功能
        }
    }
    
    // 解析系统信息
    if (doc.containsKey("system")) {
        JsonObject system = doc["system"];
        processSystemData(system);
        displaySystemInfo(system);  // 保留原有的显示功能
    }
    
    // 解析WiFi信息
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        processWiFiData(wifi);
        displayWiFiInfo(wifi);  // 保留原有的显示功能
    }
    
    // 计算总功率
    calculateTotalPower();
    
    // 标记数据有效并触发回调
    m_currentPowerData.valid = true;
    triggerDataCallback();
}

void Monitor::displayPortInfo(JsonObject port) {
    // 数据处理逻辑，不输出调试信息
    int id = port["id"];
    bool state = port["state"];
    int fcProtocol = port["fc_protocol"];
    int current = port["current"];
    int voltage = port["voltage"];
    
    // 这里可以添加数据处理逻辑，而不是输出调试信息
    // 例如：存储到变量、更新显示等
}

void Monitor::displaySystemInfo(JsonObject system) {
    // 数据处理逻辑，不输出调试信息
    unsigned long bootTime = system["boot_time_seconds"];
    int resetReason = system["reset_reason"];
    unsigned long freeHeap = system["free_heap"];
    
    // 这里可以添加数据处理逻辑，而不是输出调试信息
    // 例如：存储到变量、更新显示等
}

void Monitor::displayWiFiInfo(JsonObject wifi) {
    // 数据处理逻辑，不输出调试信息
    const char* ssid = wifi["ssid"];
    const char* bssid = wifi["bssid"];
    int channel = wifi["channel"];
    int rssi = wifi["rssi"];
    
    // 这里可以添加数据处理逻辑，而不是输出调试信息
    // 例如：存储到变量、更新显示等
}

void Monitor::processPortData(JsonObject port) {
    int id = port["id"];
    if (id >= 1 && id <= 4) {
        int index = id - 1;
        m_currentPowerData.ports[index].id = id;
        m_currentPowerData.ports[index].state = port["state"];
        m_currentPowerData.ports[index].fc_protocol = port["fc_protocol"];
        m_currentPowerData.ports[index].current = port["current"];
        m_currentPowerData.ports[index].voltage = port["voltage"];
        
        // 计算功率 (P = V * I)
        m_currentPowerData.ports[index].power = 
            (m_currentPowerData.ports[index].voltage * m_currentPowerData.ports[index].current) / 1000;
        
        // 设置协议名称
        const char* protocol_names[] = {"None", "QC2.0", "QC3.0", "PD", "AFC", "SCP", "VOOC"};
        int protocol = m_currentPowerData.ports[index].fc_protocol;
        if (protocol >= 0 && protocol < 7) {
            strncpy(m_currentPowerData.ports[index].protocol_name, protocol_names[protocol], 15);
            m_currentPowerData.ports[index].protocol_name[15] = '\0';
        } else {
            strcpy(m_currentPowerData.ports[index].protocol_name, "Unknown");
        }
        
        m_currentPowerData.ports[index].valid = true;
    }
}

void Monitor::processSystemData(JsonObject system) {
    m_currentPowerData.system.boot_time = system["boot_time_seconds"];
    m_currentPowerData.system.reset_reason = system["reset_reason"];
    m_currentPowerData.system.free_heap = system["free_heap"];
    m_currentPowerData.system.valid = true;
}

void Monitor::processWiFiData(JsonObject wifi) {
    const char* ssid = wifi["ssid"];
    const char* bssid = wifi["bssid"];
    
    if (ssid) {
        strncpy(m_currentPowerData.wifi.ssid, ssid, 31);
        m_currentPowerData.wifi.ssid[31] = '\0';
    }
    
    if (bssid) {
        strncpy(m_currentPowerData.wifi.bssid, bssid, 17);
        m_currentPowerData.wifi.bssid[17] = '\0';
    }
    
    m_currentPowerData.wifi.channel = wifi["channel"];
    m_currentPowerData.wifi.rssi = wifi["rssi"];
    m_currentPowerData.wifi.valid = true;
}

void Monitor::calculateTotalPower() {
    m_currentPowerData.total_power = 0;
    for (int i = 0; i < 4; i++) {
        if (m_currentPowerData.ports[i].valid && m_currentPowerData.ports[i].state) {
            m_currentPowerData.total_power += m_currentPowerData.ports[i].power;
        }
    }
}

void Monitor::triggerDataCallback() {
    if (m_powerDataCallback && m_currentPowerData.valid) {
        m_powerDataCallback(m_currentPowerData, m_callbackUserData);
    }
} 
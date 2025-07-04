/*
 * Monitor.cpp - 监控器类实现文件
 * ESP32S3监控项目
 */

#include "Monitor.h"
#include "PSRAMManager.h"
#include "Arduino.h"

Monitor::Monitor() : monitorTaskHandle(nullptr), m_psramManager(nullptr), isRunning(false) {
    // 设置默认配置
    metricsUrl = "http://10.10.168.168/metrics.json";
    requestInterval = 5000;  // 5秒请求一次
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
    
    printf("正在启动系统监控任务...\n");
    printf("目标URL: %s\n", metricsUrl.c_str());
    printf("请求间隔: %d ms\n", requestInterval);
    
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

void Monitor::monitoringTask(void* parameter) {
    Monitor* monitor = static_cast<Monitor*>(parameter);
    
    printf("系统监控任务开始运行\n");
    
    while (monitor->isRunning) {
        // 检查WiFi连接状态
        if (monitor->isWiFiConnected()) {
            printf("=== 开始获取系统监控数据 ===\n");
            
            // 获取并解析监控数据
            if (monitor->fetchMetricsData()) {
                printf("监控数据获取成功\n");
            } else {
                printf("监控数据获取失败\n");
            }
            
            printf("=== 监控数据获取完成 ===\n\n");
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
    httpClient.setTimeout(10000);  // 10秒超时
    httpClient.setConnectTimeout(5000);  // 5秒连接超时
    
    // 设置HTTP请求头，减少不必要的数据传输
    httpClient.addHeader("Accept", "application/json");
    httpClient.addHeader("Connection", "close");  // 请求完成后立即关闭连接
    
    printf("发送HTTP请求到: %s\n", metricsUrl.c_str());
    
    int httpCode = httpClient.GET();
    
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = httpClient.getString();
            printf("HTTP请求成功，数据长度: %d bytes\n", payload.length());
            
            // 解析并显示数据
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
    
    printf("--- 系统监控数据 ---\n");
    
    // 解析端口信息
    if (doc.containsKey("ports")) {
        JsonArray ports = doc["ports"];
        printf("端口信息 (共%d个端口):\n", ports.size());
        
        for (JsonObject port : ports) {
            displayPortInfo(port);
        }
    }
    
    // 解析系统信息
    if (doc.containsKey("system")) {
        JsonObject system = doc["system"];
        displaySystemInfo(system);
    }
    
    // 解析WiFi信息
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        displayWiFiInfo(wifi);
    }
    
    printf("--- 监控数据解析完成 ---\n");
}

void Monitor::displayPortInfo(JsonObject port) {
    int id = port["id"];
    bool state = port["state"];
    int fcProtocol = port["fc_protocol"];
    int current = port["current"];
    int voltage = port["voltage"];
    
    printf("  端口%d: 状态=%s, 协议=%d, 电流=%dmA, 电压=%dmV\n", 
           id, 
           state ? "激活" : "关闭", 
           fcProtocol, 
           current, 
           voltage);
}

void Monitor::displaySystemInfo(JsonObject system) {
    unsigned long bootTime = system["boot_time_seconds"];
    int resetReason = system["reset_reason"];
    unsigned long freeHeap = system["free_heap"];
    
    printf("系统信息:\n");
    printf("  启动时间: %lu秒\n", bootTime);
    printf("  重置原因: %d\n", resetReason);
    printf("  可用堆内存: %lu bytes\n", freeHeap);
}

void Monitor::displayWiFiInfo(JsonObject wifi) {
    const char* ssid = wifi["ssid"];
    const char* bssid = wifi["bssid"];
    int channel = wifi["channel"];
    int rssi = wifi["rssi"];
    
    printf("WiFi信息:\n");
    printf("  SSID: %s\n", ssid);
    printf("  BSSID: %s\n", bssid);
    printf("  信道: %d\n", channel);
    printf("  信号强度: %d dBm\n", rssi);
} 
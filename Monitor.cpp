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
    
    // 初始化自动扫描相关变量
    m_consecutiveFailures = 0;
    m_lastScanTime = 0;
    
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
        4096,                   // 栈大小(增大以支持HTTP和JSON处理)
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
    autoScanServer = false;  // 默认禁用自动扫描服务器
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
        this->autoScanServer = autoScanServer;  // 使用从配置加载的autoScanServer变量
        printf("服务器监控: %s\n", serverEnabled ? "启用" : "禁用");
        printf("自动扫描服务器: %s\n", this->autoScanServer ? "启用" : "禁用");
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
            
            // 连接成功，重置失败计数器
            resetFailureCounter();
            
            httpClient.end();
            return true;
        } else {
            printf("HTTP请求失败，状态码: %d\n", httpCode);
        }
    } else {
        printf("HTTP请求失败，错误: %s\n", httpClient.errorToString(httpCode).c_str());
    }
    
    httpClient.end();
    
    // 连接失败，增加失败计数并检查是否需要自动扫描
    m_consecutiveFailures++;
    printf("连接失败次数: %d\n", m_consecutiveFailures);
    
    // 检查是否需要触发自动扫描
    if (shouldTriggerAutoScan()) {
        printf("触发自动扫描服务器...\n");
        if (performAutoScan()) {
            printf("自动扫描成功，将在下次请求中使用新的服务器地址\n");
            // 重置失败计数器，因为找到了新的服务器
            resetFailureCounter();
        } else {
            printf("自动扫描未找到可用的cp02服务器\n");
        }
    }
    
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
        
        // 处理状态字符串
        const char* stateStr = port["state"];
        if (stateStr) {
            strncpy(m_currentPowerData.ports[index].state, stateStr, 15);
            m_currentPowerData.ports[index].state[15] = '\0';
        } else {
            strcpy(m_currentPowerData.ports[index].state, "UNKNOWN");
        }
        
        m_currentPowerData.ports[index].fc_protocol = port["fc_protocol"];
        m_currentPowerData.ports[index].current = port["current"];
        m_currentPowerData.ports[index].voltage = port["voltage"];
        
        // 计算功率 (P = V * I)
        m_currentPowerData.ports[index].power = 
            (m_currentPowerData.ports[index].voltage * m_currentPowerData.ports[index].current) / 1000;
        
        // 设置协议名称
        const char* protocol_names[] = {
            "None",         // 0  FC_None
            "QC2.0",        // 1  FC_QC2
            "QC3.0",        // 2  FC_QC3
            "QC3+",         // 3  FC_QC3P
            "SFCP",         // 4  FC_SFCP
            "AFC",          // 5  FC_AFC
            "FCP",          // 6  FC_FCP
            "SCP",          // 7  FC_SCP
            "VOOC1.0",      // 8  FC_VOOC1P0
            "VOOC4.0",      // 9  FC_VOOC4P0
            "SVOOC2.0",     // 10 FC_SVOOC2P0
            "TFCP",         // 11 FC_TFCP
            "UFCS",         // 12 FC_UFCS
            "PE1.0",        // 13 FC_PE1
            "PE2.0",        // 14 FC_PE2
            "PD Fix5V",     // 15 FC_PD_Fix5V
            "PD FixHV",     // 16 FC_PD_FixHV
            "PD SPR AVS",   // 17 FC_PD_SPR_AVS
            "PD PPS",       // 18 FC_PD_PPS
            "PD EPR HV",    // 19 FC_PD_EPR_HV
            "PD AVS"        // 20 FC_PD_AVS
        };
        
        int protocol = m_currentPowerData.ports[index].fc_protocol;
        if (protocol == 0xff) {
            // 特殊值：不充电状态
            strcpy(m_currentPowerData.ports[index].protocol_name, "Not Charging");
        } else if (protocol >= 0 && protocol <= 20) {
            strncpy(m_currentPowerData.ports[index].protocol_name, protocol_names[protocol], 15);
            m_currentPowerData.ports[index].protocol_name[15] = '\0';
        } else {
            strcpy(m_currentPowerData.ports[index].protocol_name, "Unknown");
        }
        
        // 解析PD状态信息
        if (port.containsKey("pd_status")) {
            JsonObject pd_status = port["pd_status"];
            
            m_currentPowerData.ports[index].manufacturer_vid = pd_status["manufacturer_vid"] | 0;
            m_currentPowerData.ports[index].cable_vid = pd_status["cable_vid"] | 0;
            m_currentPowerData.ports[index].cable_max_vbus_voltage = pd_status["cable_max_vbus_voltage"] | 0;
            m_currentPowerData.ports[index].cable_max_vbus_current = pd_status["cable_max_vbus_current"] | 0;
            m_currentPowerData.ports[index].operating_voltage = pd_status["operating_voltage"] | 0;
            m_currentPowerData.ports[index].operating_current = pd_status["operating_current"] | 0;
            m_currentPowerData.ports[index].has_emarker = pd_status["has_emarker"] | false;
            m_currentPowerData.ports[index].pps_charging_supported = pd_status["pps_charging_supported"] | false;
        } else {
            // 没有PD状态信息时设置默认值
            m_currentPowerData.ports[index].manufacturer_vid = 0;
            m_currentPowerData.ports[index].cable_vid = 0;
            m_currentPowerData.ports[index].cable_max_vbus_voltage = 0;
            m_currentPowerData.ports[index].cable_max_vbus_current = 0;
            m_currentPowerData.ports[index].operating_voltage = 0;
            m_currentPowerData.ports[index].operating_current = 0;
            m_currentPowerData.ports[index].has_emarker = false;
            m_currentPowerData.ports[index].pps_charging_supported = false;
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

// 自动扫描相关方法实现

bool Monitor::shouldTriggerAutoScan() {
    // 检查自动扫描开关是否启用
    if (!autoScanServer) {
        if (m_consecutiveFailures >= MAX_FAILURES_BEFORE_SCAN) {
            printf("📊 连接失败次数已达 %d 次，但自动扫描功能已禁用\n", m_consecutiveFailures);
            printf("   提示: 可在Web设置页面启用'自动扫描服务器'功能\n");
        }
        return false;
    }
    
    // 检查是否达到失败次数阈值
    if (m_consecutiveFailures < MAX_FAILURES_BEFORE_SCAN) {
        printf("📊 连接失败次数: %d/%d，未达到自动扫描阈值\n", 
               m_consecutiveFailures, MAX_FAILURES_BEFORE_SCAN);
        return false;
    }
    
    // 检查冷却时间
    unsigned long currentTime = millis();
    if (currentTime - m_lastScanTime < SCAN_COOLDOWN_MS) {
        unsigned long remainingTime = (SCAN_COOLDOWN_MS - (currentTime - m_lastScanTime)) / 1000;
        printf("⏱️ 自动扫描仍在冷却中，距离下次扫描还需 %lu 秒\n", remainingTime);
        printf("   失败次数: %d/%d\n", m_consecutiveFailures, MAX_FAILURES_BEFORE_SCAN);
        return false;
    }
    
    printf("🚨 满足自动扫描触发条件:\n");
    printf("   ✅ 自动扫描已启用\n");
    printf("   ✅ 连接失败次数: %d (阈值: %d)\n", m_consecutiveFailures, MAX_FAILURES_BEFORE_SCAN);
    printf("   ✅ 冷却时间已过 (距离上次扫描: %lu 秒)\n", (currentTime - m_lastScanTime) / 1000);
    return true;
}

bool Monitor::performAutoScan() {
    printf("🔍 开始执行cp02服务器自动扫描...\n");
    
    // 更新扫描时间
    m_lastScanTime = millis();
    
    // 使用MDNSScanner扫描cp02设备
    std::vector<String> keywords;
    keywords.push_back("cp02");
    keywords.push_back("CP02");
    
    printf("正在扫描网络中的cp02设备...\n");
    std::vector<MDNSDeviceInfo> devices = UniversalMDNSScanner::findDevicesByKeywords(keywords, true);
    
    if (devices.empty()) {
        printf("❌ 自动扫描未发现任何cp02服务器\n");
        return false;
    }
    
    printf("✅ 发现 %d 个cp02服务器，开始逐个测试连接...\n", devices.size());
    
    // 测试所有发现的设备，选择第一个可用的
    for (size_t i = 0; i < devices.size(); i++) {
        MDNSDeviceInfo& device = devices[i];
        printf("📡 测试设备 %d/%d: %s (%s:%d)\n", 
               (int)(i + 1), (int)devices.size(),
               device.name.c_str(), device.ip.c_str(), device.port);
        
        // 构建测试URL
        String testServerUrl = "http://" + device.ip;
        if (device.port != 80) {
            testServerUrl += ":" + String(device.port);
        }
        testServerUrl += "/metrics.json";
        
        // 测试连接
        HTTPClient testClient;
        testClient.begin(testServerUrl);
        testClient.setTimeout(3000);  // 3秒超时，避免等待太久
        testClient.setConnectTimeout(2000);  // 2秒连接超时
        
        printf("   正在测试连接: %s\n", testServerUrl.c_str());
        int testHttpCode = testClient.GET();
        
        if (testHttpCode == HTTP_CODE_OK) {
            // 额外验证响应内容是否为有效的JSON
            String payload = testClient.getString();
            testClient.end();
            
            if (payload.length() > 10 && payload.indexOf("ports") >= 0) {
                printf("✅ 设备 %s 连接测试成功，响应数据有效\n", device.name.c_str());
                
                // 更新服务器URL并保存配置
                if (updateServerUrl(testServerUrl)) {
                    printf("🎉 自动扫描成功！新服务器已配置: %s\n", testServerUrl.c_str());
                    printf("   设备名称: %s\n", device.name.c_str());
                    printf("   设备IP: %s:%d\n", device.ip.c_str(), device.port);
                    return true;
                } else {
                    printf("⚠️ 保存新服务器URL失败，继续测试下一个设备\n");
                }
            } else {
                printf("⚠️ 设备 %s 响应数据无效，继续测试下一个设备\n", device.name.c_str());
                testClient.end();
            }
        } else {
            testClient.end();
            printf("❌ 设备 %s 连接失败，状态码: %d\n", device.name.c_str(), testHttpCode);
        }
        
        // 给每个测试之间留一点间隔
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    printf("❌ 所有发现的cp02服务器均无法连接\n");
    return false;
}

bool Monitor::updateServerUrl(const String& newUrl) {
    printf("💾 正在更新服务器URL: %s\n", newUrl.c_str());
    
    // 验证URL格式
    if (newUrl.length() == 0 || newUrl.indexOf("http") != 0) {
        printf("❌ 无效的服务器URL格式: %s\n", newUrl.c_str());
        return false;
    }
    
    // 更新内存中的URL
    String oldUrl = metricsUrl;
    metricsUrl = newUrl;
    printf("✅ 内存中的服务器URL已更新\n");
    printf("   旧URL: %s\n", oldUrl.c_str());
    printf("   新URL: %s\n", newUrl.c_str());
    
    // 如果有配置存储，保存新的URL
    if (m_configStorage) {
        printf("💾 正在保存新URL到配置存储...\n");
        
        // 保存服务器配置，保持其他配置不变
        bool success = m_configStorage->saveServerConfigAsync(
            newUrl,
            requestInterval,
            serverEnabled,
            connectionTimeout,
            true,  // autoGetData，保持启用状态
            autoScanServer,  // 保持当前自动扫描设置
            5000   // 超时时间5秒
        );
        
        if (success) {
            printf("✅ 新服务器URL已保存到配置存储\n");
            printf("   URL: %s\n", newUrl.c_str());
            printf("   请求间隔: %d ms\n", requestInterval);
            printf("   连接超时: %d ms\n", connectionTimeout);
            printf("   自动扫描: %s\n", autoScanServer ? "启用" : "禁用");
            return true;
        } else {
            printf("❌ 保存新服务器URL到配置存储失败\n");
            printf("   尝试回滚内存中的URL...\n");
            metricsUrl = oldUrl;  // 回滚
            printf("   URL已回滚到: %s\n", oldUrl.c_str());
            return false;
        }
    } else {
        printf("⚠️ 配置存储未初始化，仅更新了内存中的URL\n");
        printf("   注意: 设备重启后将丢失此URL配置\n");
        return true;  // 至少内存中的URL已更新
    }
}

void Monitor::resetFailureCounter() {
    if (m_consecutiveFailures > 0) {
        printf("🔄 重置连接失败计数器: %d → 0\n", m_consecutiveFailures);
        if (m_consecutiveFailures >= MAX_FAILURES_BEFORE_SCAN) {
            printf("   ✅ 服务器连接已恢复，自动扫描状态重置\n");
        }
        m_consecutiveFailures = 0;
    }
} 
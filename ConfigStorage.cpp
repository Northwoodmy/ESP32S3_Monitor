/*
 * ConfigStorage.cpp - NVS配置存储任务管理器实现文件
 * ESP32S3监控项目 - 配置存储模块
 * 基于FreeRTOS任务实现，确保NVS操作的线程安全性
 */

#include "ConfigStorage.h"

// 静态常量定义
const char* ConfigStorage::WIFI_NAMESPACE = "wifi_config";
const char* ConfigStorage::MULTI_WIFI_NAMESPACE = "multi_wifi";
const char* ConfigStorage::SYSTEM_NAMESPACE = "system_config";

const char* ConfigStorage::WIFI_SSID_KEY = "ssid";
const char* ConfigStorage::WIFI_PASSWORD_KEY = "password";
const char* ConfigStorage::WIFI_CONFIGURED_KEY = "configured";

const char* ConfigStorage::WIFI_COUNT_KEY = "count";
const char* ConfigStorage::WIFI_SSID_PREFIX = "ssid_";
const char* ConfigStorage::WIFI_PASSWORD_PREFIX = "pwd_";
const char* ConfigStorage::WIFI_PRIORITY_PREFIX = "prio_";

const char* ConfigStorage::DEVICE_NAME_KEY = "device_name";
const char* ConfigStorage::REFRESH_RATE_KEY = "refresh_rate";

const char* ConfigStorage::BRIGHTNESS_KEY = "brightness";

const char* ConfigStorage::TIME_PRIMARY_SERVER_KEY = "time_primary";
const char* ConfigStorage::TIME_SECONDARY_SERVER_KEY = "time_secondary";
const char* ConfigStorage::TIME_TIMEZONE_KEY = "time_timezone";
const char* ConfigStorage::TIME_SYNC_INTERVAL_KEY = "time_interval";

ConfigStorage::ConfigStorage() : configTaskHandle(nullptr), configQueue(nullptr), taskRunning(false) {
}

ConfigStorage::~ConfigStorage() {
    stopTask();
}

// 任务管理方法实现

bool ConfigStorage::init() {
    printf("🔧 [ConfigStorage] 初始化NVS配置存储任务管理器...\n");
    
    // 创建配置请求队列
    configQueue = xQueueCreate(CONFIG_QUEUE_SIZE, sizeof(ConfigRequest*));
    if (configQueue == nullptr) {
        printf("❌ [ConfigStorage] 创建配置队列失败\n");
        return false;
    }
    
    printf("✅ [ConfigStorage] 配置存储任务管理器初始化完成\n");
    return true;
}

bool ConfigStorage::startTask() {
    if (configTaskHandle != nullptr) {
        printf("⚠️ [ConfigStorage] 配置任务已经在运行\n");
        return true;
    }
    
    if (configQueue == nullptr) {
        printf("❌ [ConfigStorage] 队列未初始化，无法启动任务\n");
        return false;
    }
    
    printf("🚀 [ConfigStorage] 启动配置存储任务...\n");
    
    // 先设置运行标志
    taskRunning = true;
    
    BaseType_t result = xTaskCreate(
        configTaskFunction,           // 任务函数
        "ConfigStorageTask",          // 任务名称
        CONFIG_TASK_STACK_SIZE,       // 堆栈大小
        this,                         // 任务参数
        CONFIG_TASK_PRIORITY,         // 任务优先级
        &configTaskHandle             // 任务句柄
    );
    
    if (result != pdPASS) {
        printf("❌ [ConfigStorage] 创建配置存储任务失败\n");
        taskRunning = false;
        return false;
    }
    
    printf("✅ [ConfigStorage] 配置存储任务启动成功\n");
    
    // 等待任务实际开始运行
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return true;
}

void ConfigStorage::stopTask() {
    if (configTaskHandle == nullptr) {
        return;
    }
    
    printf("🛑 [ConfigStorage] 停止配置存储任务...\n");
    
    taskRunning = false;
    
    // 等待任务结束
    vTaskDelete(configTaskHandle);
    configTaskHandle = nullptr;
    
    // 清理队列
    if (configQueue != nullptr) {
        vQueueDelete(configQueue);
        configQueue = nullptr;
    }
    
    printf("✅ [ConfigStorage] 配置存储任务已停止\n");
}

// 静态任务函数实现
void ConfigStorage::configTaskFunction(void* parameter) {
    ConfigStorage* storage = static_cast<ConfigStorage*>(parameter);
    
    if (storage == nullptr) {
        printf("❌ [ConfigStorage] 任务参数为空，无法启动\n");
        vTaskDelete(nullptr);
        return;
    }
    
    printf("🎯 [ConfigStorage] 配置存储任务开始运行\n");
    
    if (storage->configQueue == nullptr) {
        printf("❌ [ConfigStorage] 队列未初始化，任务退出\n");
        storage->taskRunning = false;
        vTaskDelete(nullptr);
        return;
    }
    
    ConfigRequest* request;
    
    while (storage->taskRunning) {
        // 等待配置请求消息
        if (xQueueReceive(storage->configQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (request != nullptr) {
                // 处理配置请求
                storage->processConfigRequest(request);
                
                // 通知请求者操作完成
                if (request->responseSemaphore != nullptr) {
                    xSemaphoreGive(request->responseSemaphore);
                }
            }
        }
        
        // 短暂延时，避免占用过多CPU时间
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    printf("🔚 [ConfigStorage] 配置存储任务结束\n");
    vTaskDelete(nullptr);
}

// 内部任务处理方法实现
void ConfigStorage::processConfigRequest(ConfigRequest* request) {
    if (request == nullptr) {
        printf("❌ [ConfigStorage] 无效的配置请求\n");
        return;
    }
    
    request->success = false;
    
    switch (request->operation) {
        case CONFIG_OP_SAVE_WIFI: {
            WiFiConfigData* data = static_cast<WiFiConfigData*>(request->data);
            if (data != nullptr) {
                request->success = saveWiFiConfig(data->ssid, data->password);
            }
            break;
        }
        
        case CONFIG_OP_LOAD_WIFI: {
            WiFiConfigData* result = static_cast<WiFiConfigData*>(request->result);
            if (result != nullptr) {
                request->success = loadWiFiConfig(result->ssid, result->password);
            }
            break;
        }
        
        case CONFIG_OP_HAS_WIFI: {
            bool* result = static_cast<bool*>(request->result);
            if (result != nullptr) {
                *result = hasWiFiConfig();
                request->success = true;
            }
            break;
        }
        
        case CONFIG_OP_CLEAR_WIFI: {
            clearWiFiConfig();
            request->success = true;
            break;
        }
        
        case CONFIG_OP_SAVE_MULTI_WIFI: {
            MultiWiFiConfigData* data = static_cast<MultiWiFiConfigData*>(request->data);
            if (data != nullptr) {
                request->success = saveWiFiConfigs(data->configs);
            }
            break;
        }
        
        case CONFIG_OP_LOAD_MULTI_WIFI: {
            MultiWiFiConfigData* result = static_cast<MultiWiFiConfigData*>(request->result);
            if (result != nullptr) {
                request->success = loadWiFiConfigs(result->configs);
            }
            break;
        }
        
        case CONFIG_OP_ADD_WIFI: {
            WiFiConfigData* data = static_cast<WiFiConfigData*>(request->data);
            if (data != nullptr) {
                request->success = addWiFiConfig(data->ssid, data->password);
            }
            break;
        }
        
        case CONFIG_OP_GET_WIFI_COUNT: {
            int* result = static_cast<int*>(request->result);
            if (result != nullptr) {
                *result = getWiFiConfigCount();
                request->success = true;
            }
            break;
        }
        
        case CONFIG_OP_CLEAR_ALL_WIFI: {
            clearAllWiFiConfigs();
            request->success = true;
            break;
        }
        
        case CONFIG_OP_UPDATE_WIFI_PRIORITY: {
            PriorityConfigData* data = static_cast<PriorityConfigData*>(request->data);
            if (data != nullptr) {
                request->success = updateWiFiPriority(data->index, data->priority);
            }
            break;
        }
        
        case CONFIG_OP_SET_WIFI_PRIORITIES: {
            PriorityConfigData* data = static_cast<PriorityConfigData*>(request->data);
            if (data != nullptr) {
                request->success = setWiFiPriorities(data->priorities);
            }
            break;
        }
        
        case CONFIG_OP_SAVE_SYSTEM: {
            SystemConfigData* data = static_cast<SystemConfigData*>(request->data);
            if (data != nullptr) {
                request->success = saveSystemConfig(data->deviceName, data->refreshRate);
            }
            break;
        }
        
        case CONFIG_OP_LOAD_SYSTEM: {
            SystemConfigData* result = static_cast<SystemConfigData*>(request->result);
            if (result != nullptr) {
                request->success = loadSystemConfig(result->deviceName, result->refreshRate);
            }
            break;
        }
        
        case CONFIG_OP_SAVE_BRIGHTNESS: {
            BrightnessConfigData* data = static_cast<BrightnessConfigData*>(request->data);
            if (data != nullptr) {
                request->success = saveBrightness(data->brightness);
            }
            break;
        }
        
        case CONFIG_OP_LOAD_BRIGHTNESS: {
            BrightnessConfigData* result = static_cast<BrightnessConfigData*>(request->result);
            if (result != nullptr) {
                result->brightness = loadBrightness();
                request->success = true;
            }
            break;
        }
        
        case CONFIG_OP_HAS_BRIGHTNESS: {
            bool* result = static_cast<bool*>(request->result);
            if (result != nullptr) {
                *result = hasBrightnessConfig();
                request->success = true;
            }
            break;
        }
        
        case CONFIG_OP_SAVE_TIME_CONFIG: {
            TimeConfigData* data = static_cast<TimeConfigData*>(request->data);
            if (data != nullptr) {
                request->success = saveTimeConfig(data->primaryServer, data->secondaryServer, 
                                                 data->timezone, data->syncInterval);
            }
            break;
        }
        
        case CONFIG_OP_LOAD_TIME_CONFIG: {
            TimeConfigData* result = static_cast<TimeConfigData*>(request->result);
            if (result != nullptr) {
                request->success = loadTimeConfig(result->primaryServer, result->secondaryServer, 
                                                 result->timezone, result->syncInterval);
            }
            break;
        }
        
        case CONFIG_OP_RESET_ALL: {
            request->success = resetAllConfig();
            break;
        }
        
        default:
            printf("❌ [ConfigStorage] 未知的配置操作类型: %d\n", request->operation);
            break;
    }
    
    printf("🔄 [ConfigStorage] 配置操作完成，结果: %s\n", request->success ? "成功" : "失败");
}

// 辅助方法：发送请求并等待响应
bool ConfigStorage::sendRequestAndWait(ConfigRequest* request, uint32_t timeoutMs) {
    if (request == nullptr || configQueue == nullptr || !taskRunning || configTaskHandle == nullptr) {
        printf("❌ [ConfigStorage] 无效的请求参数或任务未运行\n");
        return false;
    }
    
    // 创建响应信号量
    request->responseSemaphore = xSemaphoreCreateBinary();
    if (request->responseSemaphore == nullptr) {
        printf("❌ [ConfigStorage] 创建响应信号量失败\n");
        return false;
    }
    
    // 发送请求到队列
    BaseType_t queueResult = xQueueSend(configQueue, &request, pdMS_TO_TICKS(1000));
    if (queueResult != pdTRUE) {
        printf("❌ [ConfigStorage] 发送配置请求到队列失败\n");
        vSemaphoreDelete(request->responseSemaphore);
        return false;
    }
    
    // 等待响应
    bool success = false;
    BaseType_t semResult = xSemaphoreTake(request->responseSemaphore, pdMS_TO_TICKS(timeoutMs));
    if (semResult == pdTRUE) {
        success = request->success;
    } else {
        printf("⏰ [ConfigStorage] 配置操作超时 (%d ms)\n", timeoutMs);
    }
    
    // 清理信号量
    vSemaphoreDelete(request->responseSemaphore);
    request->responseSemaphore = nullptr;
    
    return success;
}

// 异步WiFi配置操作接口实现

bool ConfigStorage::saveWiFiConfigAsync(const String& ssid, const String& password, uint32_t timeoutMs) {
    WiFiConfigData data(ssid, password);
    ConfigRequest request;
    request.operation = CONFIG_OP_SAVE_WIFI;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

bool ConfigStorage::loadWiFiConfigAsync(String& ssid, String& password, uint32_t timeoutMs) {
    WiFiConfigData result;
    ConfigRequest request;
    request.operation = CONFIG_OP_LOAD_WIFI;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    if (success) {
        ssid = result.ssid;
        password = result.password;
    }
    
    return success;
}

bool ConfigStorage::hasWiFiConfigAsync(uint32_t timeoutMs) {
    bool result = false;
    ConfigRequest request;
    request.operation = CONFIG_OP_HAS_WIFI;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    return success && result;
}

void ConfigStorage::clearWiFiConfigAsync(uint32_t timeoutMs) {
    ConfigRequest request;
    request.operation = CONFIG_OP_CLEAR_WIFI;
    
    sendRequestAndWait(&request, timeoutMs);
}

// 异步多WiFi配置操作接口实现

bool ConfigStorage::saveWiFiConfigsAsync(const WiFiConfig configs[3], uint32_t timeoutMs) {
    MultiWiFiConfigData data;
    for (int i = 0; i < 3; i++) {
        data.configs[i] = configs[i];
    }
    
    ConfigRequest request;
    request.operation = CONFIG_OP_SAVE_MULTI_WIFI;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

bool ConfigStorage::loadWiFiConfigsAsync(WiFiConfig configs[3], uint32_t timeoutMs) {
    MultiWiFiConfigData result;
    ConfigRequest request;
    request.operation = CONFIG_OP_LOAD_MULTI_WIFI;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    if (success) {
        for (int i = 0; i < 3; i++) {
            configs[i] = result.configs[i];
        }
    }
    
    return success;
}

bool ConfigStorage::addWiFiConfigAsync(const String& ssid, const String& password, uint32_t timeoutMs) {
    WiFiConfigData data(ssid, password);
    ConfigRequest request;
    request.operation = CONFIG_OP_ADD_WIFI;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

int ConfigStorage::getWiFiConfigCountAsync(uint32_t timeoutMs) {
    int result = 0;
    ConfigRequest request;
    request.operation = CONFIG_OP_GET_WIFI_COUNT;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    return success ? result : 0;
}

void ConfigStorage::clearAllWiFiConfigsAsync(uint32_t timeoutMs) {
    ConfigRequest request;
    request.operation = CONFIG_OP_CLEAR_ALL_WIFI;
    
    sendRequestAndWait(&request, timeoutMs);
}

// 异步优先级管理接口实现

bool ConfigStorage::updateWiFiPriorityAsync(int index, int priority, uint32_t timeoutMs) {
    PriorityConfigData data;
    data.index = index;
    data.priority = priority;
    
    ConfigRequest request;
    request.operation = CONFIG_OP_UPDATE_WIFI_PRIORITY;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

bool ConfigStorage::setWiFiPrioritiesAsync(const int priorities[3], uint32_t timeoutMs) {
    PriorityConfigData data;
    for (int i = 0; i < 3; i++) {
        data.priorities[i] = priorities[i];
    }
    
    ConfigRequest request;
    request.operation = CONFIG_OP_SET_WIFI_PRIORITIES;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

// 异步系统配置操作接口实现

bool ConfigStorage::saveSystemConfigAsync(const String& deviceName, int refreshRate, uint32_t timeoutMs) {
    SystemConfigData data(deviceName, refreshRate);
    ConfigRequest request;
    request.operation = CONFIG_OP_SAVE_SYSTEM;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

bool ConfigStorage::loadSystemConfigAsync(String& deviceName, int& refreshRate, uint32_t timeoutMs) {
    SystemConfigData result;
    ConfigRequest request;
    request.operation = CONFIG_OP_LOAD_SYSTEM;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    if (success) {
        deviceName = result.deviceName;
        refreshRate = result.refreshRate;
    }
    
    return success;
}

// 异步屏幕亮度配置操作接口实现

bool ConfigStorage::saveBrightnessAsync(uint8_t brightness, uint32_t timeoutMs) {
    BrightnessConfigData data(brightness);
    ConfigRequest request;
    request.operation = CONFIG_OP_SAVE_BRIGHTNESS;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

uint8_t ConfigStorage::loadBrightnessAsync(uint32_t timeoutMs) {
    BrightnessConfigData result;
    ConfigRequest request;
    request.operation = CONFIG_OP_LOAD_BRIGHTNESS;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    return success ? result.brightness : 80; // 默认值80%
}

bool ConfigStorage::hasBrightnessConfigAsync(uint32_t timeoutMs) {
    bool result = false;
    ConfigRequest request;
    request.operation = CONFIG_OP_HAS_BRIGHTNESS;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    return success && result;
}

// 异步配置重置操作接口实现

// 异步时间配置操作接口实现

bool ConfigStorage::saveTimeConfigAsync(const String& primaryServer, const String& secondaryServer, 
                                       const String& timezone, int syncInterval, uint32_t timeoutMs) {
    TimeConfigData data(primaryServer, secondaryServer, timezone, syncInterval);
    ConfigRequest request;
    request.operation = CONFIG_OP_SAVE_TIME_CONFIG;
    request.data = &data;
    
    return sendRequestAndWait(&request, timeoutMs);
}

bool ConfigStorage::loadTimeConfigAsync(String& primaryServer, String& secondaryServer, 
                                       String& timezone, int& syncInterval, uint32_t timeoutMs) {
    TimeConfigData result;
    ConfigRequest request;
    request.operation = CONFIG_OP_LOAD_TIME_CONFIG;
    request.result = &result;
    
    bool success = sendRequestAndWait(&request, timeoutMs);
    if (success) {
        primaryServer = result.primaryServer;
        secondaryServer = result.secondaryServer;
        timezone = result.timezone;
        syncInterval = result.syncInterval;
    }
    
    return success;
}

bool ConfigStorage::resetAllConfigAsync(uint32_t timeoutMs) {
    ConfigRequest request;
    request.operation = CONFIG_OP_RESET_ALL;
    
    return sendRequestAndWait(&request, timeoutMs);
}



// 内部NVS操作方法实现 (原有方法改为private)

bool ConfigStorage::saveWiFiConfig(const String& ssid, const String& password) {
    printf("💾 [ConfigStorage] 保存WiFi配置到NVS: SSID=%s\n", ssid.c_str());
    
    if (!preferences.begin(WIFI_NAMESPACE, false)) {
        printf("打开WiFi配置命名空间失败\n");
        return false;
    }
    
    bool success = true;
    
    // 保存SSID (返回写入字节数，0表示失败)
    size_t ssidResult = preferences.putString(WIFI_SSID_KEY, ssid);
    if (ssidResult == 0) {
        printf("保存SSID失败\n");
        success = false;
    }
    
    // 保存密码
    size_t passwordResult = preferences.putString(WIFI_PASSWORD_KEY, password);
    if (passwordResult == 0) {
        printf("保存密码失败\n");
        success = false;
    }
    
    // 保存配置标志
    size_t configResult = preferences.putBool(WIFI_CONFIGURED_KEY, true);
    if (configResult == 0) {
        printf("保存配置标志失败\n");
        success = false;
    }
    
    preferences.end();
    
    if (success) {
        printf("WiFi配置保存成功\n");
    } else {
        printf("WiFi配置保存失败\n");
    }
    
    return success;
}

bool ConfigStorage::loadWiFiConfig(String& ssid, String& password) {
    if (!preferences.begin(WIFI_NAMESPACE, true)) {
        printf("打开WiFi配置命名空间失败\n");
        return false;
    }
    
    if (!preferences.getBool(WIFI_CONFIGURED_KEY, false)) {
        printf("没有找到WiFi配置\n");
        preferences.end();
        return false;
    }
    
    ssid = preferences.getString(WIFI_SSID_KEY, "");
    password = preferences.getString(WIFI_PASSWORD_KEY, "");
    
    preferences.end();
    
    if (ssid.length() == 0) {
        printf("WiFi配置为空\n");
        return false;
    }
    
    printf("加载WiFi配置: SSID=%s\n", ssid.c_str());
    return true;
}

bool ConfigStorage::hasWiFiConfig() {
    if (!preferences.begin(WIFI_NAMESPACE, true)) {
        return false;
    }
    
    bool configured = preferences.getBool(WIFI_CONFIGURED_KEY, false);
    String ssid = preferences.getString(WIFI_SSID_KEY, "");
    
    preferences.end();
    
    return configured && (ssid.length() > 0);
}

void ConfigStorage::clearWiFiConfig() {
    printf("清除WiFi配置\n");
    
    if (!preferences.begin(WIFI_NAMESPACE, false)) {
        printf("打开WiFi配置命名空间失败\n");
        return;
    }
    
    preferences.clear();
    preferences.end();
    
    printf("WiFi配置已清除\n");
}

bool ConfigStorage::saveSystemConfig(const String& deviceName, int refreshRate) {
    printf("保存系统配置: 设备名=%s, 刷新率=%d\n", deviceName.c_str(), refreshRate);
    
    if (!preferences.begin(SYSTEM_NAMESPACE, false)) {
        printf("打开系统配置命名空间失败\n");
        return false;
    }
    
    bool success = true;
    
    // 保存设备名称
    size_t nameResult = preferences.putString(DEVICE_NAME_KEY, deviceName);
    if (nameResult == 0) {
        printf("保存设备名称失败\n");
        success = false;
    }
    
    // 保存刷新率
    size_t rateResult = preferences.putInt(REFRESH_RATE_KEY, refreshRate);
    if (rateResult == 0) {
        printf("保存刷新率失败\n");
        success = false;
    }
    
    preferences.end();
    
    if (success) {
        printf("系统配置保存成功\n");
    } else {
        printf("系统配置保存失败\n");
    }
    
    return success;
}

bool ConfigStorage::loadSystemConfig(String& deviceName, int& refreshRate) {
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        printf("打开系统配置命名空间失败\n");
        return false;
    }
    
    deviceName = preferences.getString(DEVICE_NAME_KEY, "ESP32S3-Monitor");
    refreshRate = preferences.getInt(REFRESH_RATE_KEY, 1000);
    
    preferences.end();
    
    printf("加载系统配置: 设备名=%s, 刷新率=%d\n", deviceName.c_str(), refreshRate);
    return true;
}

bool ConfigStorage::resetAllConfig() {
    printf("重置所有配置到默认值...\n");
    
    bool success = true;
    
    // 清除WiFi配置
    if (preferences.begin(WIFI_NAMESPACE, false)) {
        preferences.clear();
        preferences.end();
        printf("WiFi配置已清除\n");
    } else {
        printf("清除WiFi配置失败\n");
        success = false;
    }
    
    // 清除系统配置
    if (preferences.begin(SYSTEM_NAMESPACE, false)) {
        preferences.clear();
        preferences.end();
        printf("系统配置已清除\n");
    } else {
        printf("清除系统配置失败\n");
        success = false;
    }
    
    // 清除多WiFi配置
    if (preferences.begin(MULTI_WIFI_NAMESPACE, false)) {
        preferences.clear();
        preferences.end();
        printf("多WiFi配置已清除\n");
    } else {
        printf("清除多WiFi配置失败\n");
        success = false;
    }
    
    if (success) {
        printf("所有配置已重置为默认值\n");
    } else {
        printf("配置重置过程中发生错误\n");
    }
    
    return success;
}

// 多WiFi配置功能实现

bool ConfigStorage::saveWiFiConfigs(const WiFiConfig configs[3]) {
    printf("保存多WiFi配置到NVS\n");
    
    if (!preferences.begin(MULTI_WIFI_NAMESPACE, false)) {
        printf("打开多WiFi配置命名空间失败\n");
        return false;
    }
    
    // 先清除所有现有配置
    preferences.clear();
    
    bool success = true;
    int validCount = 0;
    
    // 计算有效配置数量
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (configs[i].isValid && configs[i].ssid.length() > 0) {
            validCount++;
        }
    }
    
    // 保存配置数量
    size_t countResult = preferences.putInt(WIFI_COUNT_KEY, validCount);
    if (countResult == 0) {
        printf("保存WiFi配置数量失败\n");
        success = false;
    }
    
    // 保存每个有效配置，按顺序重新排列
    int savedIndex = 0;
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (configs[i].isValid && configs[i].ssid.length() > 0) {
            String ssidKey = getWiFiSSIDKey(savedIndex);
            String passwordKey = getWiFiPasswordKey(savedIndex);
            String priorityKey = getWiFiPriorityKey(savedIndex);
            
            size_t ssidResult = preferences.putString(ssidKey.c_str(), configs[i].ssid);
            size_t passwordResult = preferences.putString(passwordKey.c_str(), configs[i].password);
            size_t priorityResult = preferences.putInt(priorityKey.c_str(), configs[i].priority);
            
            if (ssidResult == 0 || passwordResult == 0 || priorityResult == 0) {
                printf("保存WiFi配置 %d 失败\n", savedIndex);
                success = false;
            } else {
                printf("保存WiFi配置 %d: SSID=%s, 优先级=%d\n", savedIndex, configs[i].ssid.c_str(), configs[i].priority);
            }
            savedIndex++;
        }
    }
    
    preferences.end();
    
    if (success) {
        printf("多WiFi配置保存成功，共保存 %d 个配置\n", validCount);
    } else {
        printf("多WiFi配置保存失败\n");
    }
    
    return success;
}

bool ConfigStorage::loadWiFiConfigs(WiFiConfig configs[3]) {
    printf("加载多WiFi配置\n");
    
    // 初始化配置数组
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        configs[i] = WiFiConfig();
    }
    
    if (!preferences.begin(MULTI_WIFI_NAMESPACE, true)) {
        printf("打开多WiFi配置命名空间失败\n");
        return false;
    }
    
    int count = preferences.getInt(WIFI_COUNT_KEY, 0);
    printf("找到 %d 个WiFi配置\n", count);
    
    if (count <= 0) {
        preferences.end();
        return false;
    }
    
    bool success = true;
    for (int i = 0; i < count && i < MAX_WIFI_CONFIGS; i++) {
        String ssidKey = getWiFiSSIDKey(i);
        String passwordKey = getWiFiPasswordKey(i);
        String priorityKey = getWiFiPriorityKey(i);
        
        String ssid = preferences.getString(ssidKey.c_str(), "");
        String password = preferences.getString(passwordKey.c_str(), "");
        int priority = preferences.getInt(priorityKey.c_str(), 99);
        
        if (ssid.length() > 0) {
            configs[i] = WiFiConfig(ssid, password, priority);
            printf("加载WiFi配置 %d: SSID=%s, 优先级=%d\n", i, ssid.c_str(), priority);
        } else {
            printf("WiFi配置 %d 为空\n", i);
            success = false;
        }
    }
    
    preferences.end();
    
    // 按优先级排序配置
    if (success) {
        sortConfigsByPriority(configs);
    }
    
    return success;
}

int ConfigStorage::getWiFiConfigCount() {
    if (!preferences.begin(MULTI_WIFI_NAMESPACE, true)) {
        return 0;
    }
    
    int count = preferences.getInt(WIFI_COUNT_KEY, 0);
    preferences.end();
    
    return count;
}

bool ConfigStorage::addWiFiConfig(const String& ssid, const String& password) {
    printf("添加WiFi配置: SSID=%s\n", ssid.c_str());
    
    // 加载现有配置
    WiFiConfig configs[3];
    loadWiFiConfigs(configs);
    
    // 检查是否已存在相同SSID
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (configs[i].isValid && configs[i].ssid == ssid) {
            printf("WiFi配置已存在，更新密码\n");
            configs[i].password = password;
            // 保持原有优先级不变
            return saveWiFiConfigs(configs);
        }
    }
    
    // 计算新配置的默认优先级（比现有最低优先级低1）
    int maxPriority = 0;
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (configs[i].isValid && configs[i].priority > maxPriority) {
            maxPriority = configs[i].priority;
        }
    }
    int newPriority = maxPriority + 1;
    
    // 查找空位置添加新配置
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (!configs[i].isValid) {
            configs[i] = WiFiConfig(ssid, password, newPriority);
            printf("在位置 %d 添加新WiFi配置，优先级=%d\n", i, newPriority);
            return saveWiFiConfigs(configs);
        }
    }
    
    // 如果没有空位，替换最后一个配置
    printf("WiFi配置已满，替换最后一个配置\n");
    configs[MAX_WIFI_CONFIGS - 1] = WiFiConfig(ssid, password, newPriority);
    return saveWiFiConfigs(configs);
}

void ConfigStorage::clearAllWiFiConfigs() {
    printf("清除所有多WiFi配置\n");
    
    if (!preferences.begin(MULTI_WIFI_NAMESPACE, false)) {
        printf("打开多WiFi配置命名空间失败\n");
        return;
    }
    
    preferences.clear();
    preferences.end();
    
    printf("所有多WiFi配置已清除\n");
}

// 内部辅助方法实现

String ConfigStorage::getWiFiSSIDKey(int index) {
    return String(WIFI_SSID_PREFIX) + String(index);
}

String ConfigStorage::getWiFiPasswordKey(int index) {
    return String(WIFI_PASSWORD_PREFIX) + String(index);
}

String ConfigStorage::getWiFiPriorityKey(int index) {
    return String(WIFI_PRIORITY_PREFIX) + String(index);
}

// 新增：优先级管理方法实现

bool ConfigStorage::updateWiFiPriority(int index, int priority) {
    printf("更新WiFi配置 %d 的优先级为 %d\n", index, priority);
    
    if (index < 0 || index >= MAX_WIFI_CONFIGS) {
        printf("无效的配置索引: %d\n", index);
        return false;
    }
    
    // 加载现有配置
    WiFiConfig configs[3];
    if (!loadWiFiConfigs(configs)) {
        printf("加载WiFi配置失败\n");
        return false;
    }
    
    if (!configs[index].isValid) {
        printf("配置 %d 不存在或无效\n", index);
        return false;
    }
    
    // 自动解决优先级冲突
    resolveConflictingPriorities(configs, index, priority);
    
    // 更新目标配置的优先级
    configs[index].priority = priority;
    printf("更新配置 %d (%s) 的优先级为 %d\n", index, configs[index].ssid.c_str(), priority);
    
    return saveWiFiConfigs(configs);
}

bool ConfigStorage::setWiFiPriorities(const int priorities[3]) {
    printf("批量设置WiFi优先级\n");
    
    // 加载现有配置
    WiFiConfig configs[3];
    if (!loadWiFiConfigs(configs)) {
        printf("加载WiFi配置失败\n");
        return false;
    }
    
    // 逐个更新有效配置的优先级，并解决冲突
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (configs[i].isValid) {
            printf("设置配置 %d (%s) 的优先级为 %d\n", i, configs[i].ssid.c_str(), priorities[i]);
            
            // 检查并解决可能的优先级冲突
            resolveConflictingPriorities(configs, i, priorities[i]);
            
            // 设置新的优先级
            configs[i].priority = priorities[i];
        }
    }
    
    return saveWiFiConfigs(configs);
}

void ConfigStorage::sortConfigsByPriority(WiFiConfig configs[3]) {
    printf("按优先级排序WiFi配置\n");
    
    // 使用简单的冒泡排序按优先级排序（优先级数字越小越优先）
    for (int i = 0; i < MAX_WIFI_CONFIGS - 1; i++) {
        for (int j = 0; j < MAX_WIFI_CONFIGS - 1 - i; j++) {
            // 只有两个配置都有效时才比较
            if (configs[j].isValid && configs[j + 1].isValid) {
                if (configs[j].priority > configs[j + 1].priority) {
                    // 交换位置
                    WiFiConfig temp = configs[j];
                    configs[j] = configs[j + 1];
                    configs[j + 1] = temp;
                    printf("交换配置 %d (%s, 优先级%d) 和配置 %d (%s, 优先级%d)\n", 
                           j, configs[j + 1].ssid.c_str(), configs[j + 1].priority,
                           j + 1, configs[j].ssid.c_str(), configs[j].priority);
                }
            }
        }
    }
    
    printf("WiFi配置排序完成\n");
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (configs[i].isValid) {
            printf("排序后配置 %d: %s (优先级 %d)\n", i, configs[i].ssid.c_str(), configs[i].priority);
        }
    }
}

void ConfigStorage::resolveConflictingPriorities(WiFiConfig configs[3], int targetIndex, int newPriority) {
    printf("解决优先级冲突：目标索引=%d，新优先级=%d\n", targetIndex, newPriority);
    
    const int MAX_PRIORITY = 99;  // 最大优先级限制
    
    // 检查是否有其他配置使用了相同的优先级
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (i != targetIndex && configs[i].isValid && configs[i].priority == newPriority) {
            printf("发现冲突：配置 %d (%s) 也使用优先级 %d\n", i, configs[i].ssid.c_str(), newPriority);
            
            // 为冲突的配置寻找新的优先级（往后退一级）
            int adjustedPriority = newPriority + 1;
            
            // 递归检查调整后的优先级是否也有冲突
            while (adjustedPriority <= MAX_PRIORITY) {
                bool hasConflict = false;
                for (int j = 0; j < MAX_WIFI_CONFIGS; j++) {
                    if (j != i && j != targetIndex && configs[j].isValid && configs[j].priority == adjustedPriority) {
                        hasConflict = true;
                        break;
                    }
                }
                
                if (!hasConflict) {
                    // 找到可用的优先级
                    configs[i].priority = adjustedPriority;
                    printf("调整配置 %d (%s) 的优先级从 %d 改为 %d\n", i, configs[i].ssid.c_str(), newPriority, adjustedPriority);
                    break;
                } else {
                    adjustedPriority++;
                }
            }
            
            // 如果调整后的优先级超出最大值，报警但仍然设置
            if (adjustedPriority > MAX_PRIORITY) {
                configs[i].priority = MAX_PRIORITY;
                printf("警告：配置 %d (%s) 的优先级已达到最大值 %d\n", i, configs[i].ssid.c_str(), MAX_PRIORITY);
            }
        }
    }
    
    printf("优先级冲突解决完成\n");
}

// 屏幕亮度配置方法实现

bool ConfigStorage::saveBrightness(uint8_t brightness) {
    printf("💾 [ConfigStorage] 保存屏幕亮度配置: %d%%\n", brightness);
    
    if (!preferences.begin(SYSTEM_NAMESPACE, false)) {
        printf("❌ [ConfigStorage] 打开系统配置命名空间失败\n");
        return false;
    }
    
    // 保存亮度值 (0-100)
    size_t result = preferences.putUChar(BRIGHTNESS_KEY, brightness);
    preferences.end();
    
    if (result == 0) {
        printf("❌ [ConfigStorage] 亮度配置保存失败\n");
        return false;
    }
    
    printf("✅ [ConfigStorage] 亮度配置保存成功: %d%%\n", brightness);
    return true;
}

uint8_t ConfigStorage::loadBrightness() {
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        printf("⚠️ [ConfigStorage] 打开系统配置命名空间失败，使用默认亮度\n");
        return 80; // 默认亮度80%
    }
    
    uint8_t brightness = preferences.getUChar(BRIGHTNESS_KEY, 80); // 默认值80%
    preferences.end();
    
    // 验证亮度值范围
    if (brightness > 100) {
        printf("⚠️ [ConfigStorage] 加载的亮度值超出范围(%d%%)，使用默认值80%%\n", brightness);
        brightness = 80;
    }
    
    printf("📖 [ConfigStorage] 加载屏幕亮度配置: %d%%\n", brightness);
    return brightness;
}

bool ConfigStorage::hasBrightnessConfig() {
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        return false;
    }
    
    bool exists = preferences.isKey(BRIGHTNESS_KEY);
    preferences.end();
    
    printf("🔍 [ConfigStorage] 检查亮度配置存在性: %s\n", exists ? "存在" : "不存在");
    return exists;
}

// 时间配置方法实现

bool ConfigStorage::saveTimeConfig(const String& primaryServer, const String& secondaryServer, 
                                   const String& timezone, int syncInterval) {
    printf("💾 [ConfigStorage] 保存时间配置\n");
    printf("  主服务器: %s\n", primaryServer.c_str());
    printf("  备用服务器: %s\n", secondaryServer.c_str());
    printf("  时区: %s\n", timezone.c_str());
    printf("  同步间隔: %d分钟\n", syncInterval);
    
    if (!preferences.begin(SYSTEM_NAMESPACE, false)) {
        printf("❌ [ConfigStorage] 打开系统配置命名空间失败\n");
        return false;
    }
    
    // 保存时间配置
    bool success = true;
    success &= (preferences.putString(TIME_PRIMARY_SERVER_KEY, primaryServer) > 0);
    success &= (preferences.putString(TIME_SECONDARY_SERVER_KEY, secondaryServer) > 0);
    success &= (preferences.putString(TIME_TIMEZONE_KEY, timezone) > 0);
    success &= (preferences.putInt(TIME_SYNC_INTERVAL_KEY, syncInterval) > 0);
    
    preferences.end();
    
    if (success) {
        printf("✅ [ConfigStorage] 时间配置保存成功\n");
    } else {
        printf("❌ [ConfigStorage] 时间配置保存失败\n");
    }
    
    return success;
}

bool ConfigStorage::loadTimeConfig(String& primaryServer, String& secondaryServer, 
                                   String& timezone, int& syncInterval) {
    printf("📖 [ConfigStorage] 加载时间配置\n");
    
    if (!preferences.begin(SYSTEM_NAMESPACE, true)) {
        printf("⚠️ [ConfigStorage] 打开系统配置命名空间失败，使用默认时间配置\n");
        primaryServer = "pool.ntp.org";
        secondaryServer = "time.nist.gov";
        timezone = "CST-8";
        syncInterval = 60;
        return false;
    }
    
    // 加载时间配置
    primaryServer = preferences.getString(TIME_PRIMARY_SERVER_KEY, "pool.ntp.org");
    secondaryServer = preferences.getString(TIME_SECONDARY_SERVER_KEY, "time.nist.gov");
    timezone = preferences.getString(TIME_TIMEZONE_KEY, "CST-8");
    syncInterval = preferences.getInt(TIME_SYNC_INTERVAL_KEY, 60);
    
    preferences.end();
    
    // 验证同步间隔范围
    if (syncInterval < 1 || syncInterval > 1440) { // 1分钟到24小时
        printf("⚠️ [ConfigStorage] 加载的同步间隔超出范围(%d分钟)，使用默认值60分钟\n", syncInterval);
        syncInterval = 60;
    }
    
    printf("📖 [ConfigStorage] 时间配置加载完成\n");
    printf("  主服务器: %s\n", primaryServer.c_str());
    printf("  备用服务器: %s\n", secondaryServer.c_str());
    printf("  时区: %s\n", timezone.c_str());
    printf("  同步间隔: %d分钟\n", syncInterval);
    
    return true;
}
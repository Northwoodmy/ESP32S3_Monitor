/*
 * LocationManager.cpp - 定位管理器模块实现
 * 
 * 功能说明:
 * - 使用高德地图定位API获取当前位置信息
 * - 支持IP定位和GPS定位
 * - 自动解析获取城市编码和地区信息
 * - 为天气管理器提供自动地区设置功能
 * - 基于FreeRTOS任务实现
 * 
 * 作者: ESP32S3_Monitor
 * 版本: v1.0.0
 * 日期: 2024-12-26
 */

#include "LocationManager.h"

// 默认配置
static const LocationConfig DEFAULT_CONFIG = {
    .apiKey = "",                               // 需要用户配置高德API密钥
    .preferredType = LOCATION_TYPE_IP,
    .autoLocate = true,
    .retryTimes = 3,
    .timeout = 15000
};

// 构造函数
LocationManager::LocationManager() {
    _psramManager = nullptr;
    _wifiManager = nullptr;
    _configStorage = nullptr;
    _locationTaskHandle = nullptr;
    _locationMutex = nullptr;
    _state = LOCATION_STATE_INIT;
    _isInitialized = false;
    _isRunning = false;
    _debugMode = false;
    _memoryStatsEnabled = true;
    
    // 初始化内存监控
    _currentMemoryUsage = 0;
    _peakMemoryUsage = 0;
    
    // 初始化统计信息
    _statistics = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    // 初始化定位数据
    _currentLocation = {};
    _currentLocation.isValid = false;
    
    // 初始化配置
    _config = DEFAULT_CONFIG;
}

// 析构函数
LocationManager::~LocationManager() {
    stop();
    
    // 删除互斥锁
    if (_locationMutex) {
        vSemaphoreDelete(_locationMutex);
        _locationMutex = nullptr;
    }
}

// 初始化定位管理器
bool LocationManager::init(PSRAMManager* psramManager, WiFiManager* wifiManager, ConfigStorage* configStorage) {
    if (_isInitialized) {
        printDebugInfo("定位管理器已初始化");
        return true;
    }
    
    printf("🌍 初始化定位管理器...\n");
    
    // 检查依赖
    if (!psramManager || !wifiManager || !configStorage) {
        printf("❌ 定位管理器依赖参数无效\n");
        return false;
    }
    
    _psramManager = psramManager;
    _wifiManager = wifiManager;
    _configStorage = configStorage;
    
    // 创建互斥锁
    _locationMutex = xSemaphoreCreateMutex();
    if (!_locationMutex) {
        printf("❌ 创建定位数据互斥锁失败\n");
        return false;
    }
    
    // 加载配置
    if (!loadConfig()) {
        printf("⚠️ 加载定位配置失败，使用默认配置\n");
        _config = DEFAULT_CONFIG;
    }
    
    // 设置状态
    _state = LOCATION_STATE_READY;
    _isInitialized = true;
    
    printf("✅ 定位管理器初始化成功\n");
    printf("   首选定位类型: %s\n", _config.preferredType == LOCATION_TYPE_IP ? "IP定位" : "GPS定位");
    printf("   自动定位: %s\n", _config.autoLocate ? "启用" : "禁用");
    printf("   重试次数: %d\n", _config.retryTimes);
    
    return true;
}

// 启动定位管理器
bool LocationManager::start() {
    if (!_isInitialized) {
        printf("❌ 定位管理器未初始化\n");
        return false;
    }
    
    if (_isRunning) {
        printDebugInfo("定位管理器已在运行");
        return true;
    }
    
    printf("🚀 启动定位管理器...\n");
    
    // 创建定位任务
    if (!createLocationTask()) {
        printf("❌ 创建定位任务失败\n");
        return false;
    }
    
    _isRunning = true;
    
    printf("✅ 定位管理器启动成功\n");
    return true;
}

// 停止定位管理器
void LocationManager::stop() {
    if (!_isRunning) {
        return;
    }
    
    printf("🛑 停止定位管理器...\n");
    
    _isRunning = false;
    
    // 删除任务
    deleteLocationTask();
    
    printf("✅ 定位管理器已停止\n");
}

// 重启定位管理器
void LocationManager::restart() {
    printf("🔄 重启定位管理器...\n");
    stop();
    vTaskDelay(pdMS_TO_TICKS(1000));
    start();
}

// 设置配置
bool LocationManager::setConfig(const LocationConfig& config) {
    if (!lockLocationData()) {
        return false;
    }
    
    // 验证配置的有效性
    if (!validateConfig(config)) {
        printf("❌ 定位配置验证失败\n");
        unlockLocationData();
        return false;
    }
    
    _config = config;
    bool success = saveConfig();
    
    unlockLocationData();
    
    if (success) {
        printf("✅ 定位配置更新成功\n");
    } else {
        printf("❌ 定位配置保存失败\n");
    }
    
    return success;
}

// 获取配置
LocationConfig LocationManager::getConfig() const {
    return _config;
}

// 设置API密钥
bool LocationManager::setApiKey(const String& apiKey) {
    if (!lockLocationData()) {
        return false;
    }
    
    _config.apiKey = apiKey;
    bool success = saveConfig();
    
    unlockLocationData();
    
    if (success) {
        printf("✅ 高德定位API密钥设置成功\n");
    } else {
        printf("❌ 高德定位API密钥保存失败\n");
    }
    
    return success;
}

// 设置自动定位
bool LocationManager::setAutoLocate(bool enable) {
    if (!lockLocationData()) {
        return false;
    }
    
    _config.autoLocate = enable;
    bool success = saveConfig();
    
    unlockLocationData();
    
    printf("✅ 自动定位设置为: %s\n", enable ? "启用" : "禁用");
    return success;
}

// 设置首选定位类型
bool LocationManager::setPreferredType(LocationType type) {
    if (!lockLocationData()) {
        return false;
    }
    
    _config.preferredType = type;
    bool success = saveConfig();
    
    unlockLocationData();
    
    const char* typeStr = (type == LOCATION_TYPE_IP) ? "IP定位" : "GPS定位";
    printf("✅ 首选定位类型设置为: %s\n", typeStr);
    return success;
}

// 定位当前位置
bool LocationManager::locateCurrentPosition() {
    if (!_isInitialized) {
        printf("❌ 定位管理器未初始化\n");
        return false;
    }
    
    if (!isWiFiConnected()) {
        printf("❌ WiFi未连接，无法定位\n");
        _state = LOCATION_STATE_NO_WIFI;
        return false;
    }
    
    if (_config.apiKey.isEmpty()) {
        printf("❌ 高德API密钥未设置\n");
        _state = LOCATION_STATE_ERROR;
        return false;
    }
    
    _state = LOCATION_STATE_LOCATING;
    
    printf("🌍 开始定位当前位置...\n");
    
    bool success = false;
    unsigned long startTime = millis();
    
    // 根据首选类型进行定位
    if (_config.preferredType == LOCATION_TYPE_IP) {
        success = performIPLocation();
    } else {
        // GPS定位预留，目前使用IP定位
        success = performIPLocation();
    }
    
    unsigned long duration = millis() - startTime;
    _statistics.locationDuration = duration;
    
    if (success) {
        _state = LOCATION_STATE_SUCCESS;
        _statistics.lastLocationTime = millis();
        printf("✅ 位置定位成功，耗时: %lu毫秒\n", duration);
    } else {
        _state = LOCATION_STATE_FAILED;
        printf("❌ 位置定位失败，耗时: %lu毫秒\n", duration);
    }
    
    updateStatistics(success);
    
    return success;
}

// IP定位
bool LocationManager::locateByIP() {
    return performIPLocation();
}

// 获取当前位置
LocationData LocationManager::getCurrentLocation() const {
    return _currentLocation;
}

// 检查定位数据有效性
bool LocationManager::isLocationDataValid() const {
    return _currentLocation.isValid;
}

// 获取状态
LocationState LocationManager::getState() const {
    return _state;
}

// 获取统计信息
LocationStatistics LocationManager::getStatistics() const {
    return _statistics;
}

// 获取状态字符串
String LocationManager::getStateString() const {
    switch (_state) {
        case LOCATION_STATE_INIT: return "初始化";
        case LOCATION_STATE_READY: return "就绪";
        case LOCATION_STATE_LOCATING: return "定位中";
        case LOCATION_STATE_SUCCESS: return "成功";
        case LOCATION_STATE_FAILED: return "失败";
        case LOCATION_STATE_NO_WIFI: return "无WiFi";
        case LOCATION_STATE_ERROR: return "错误";
        default: return "未知";
    }
}

// 获取上次定位时间
unsigned long LocationManager::getLastLocationTime() const {
    return _statistics.lastLocationTime;
}

// 获取位置字符串
String LocationManager::getLocationString() const {
    if (!_currentLocation.isValid) {
        return "未知位置";
    }
    
    String location = _currentLocation.province;
    if (!_currentLocation.city.isEmpty() && _currentLocation.city != _currentLocation.province) {
        location += " " + _currentLocation.city;
    }
    if (!_currentLocation.district.isEmpty() && _currentLocation.district != _currentLocation.city) {
        location += " " + _currentLocation.district;
    }
    
    return location;
}

// 获取天气查询用的城市代码
String LocationManager::getCityCodeForWeather() const {
    if (!_currentLocation.isValid) {
        return "";
    }
    
    // 优先使用区域编码（adcode），如果没有则使用城市编码
    if (!_currentLocation.adcode.isEmpty()) {
        return _currentLocation.adcode;
    } else if (!_currentLocation.citycode.isEmpty()) {
        return _currentLocation.citycode;
    }
    
    return "";
}

// 检查是否有可用的定位数据
bool LocationManager::isLocationAvailable() const {
    return _currentLocation.isValid && !getCityCodeForWeather().isEmpty();
}

// 设置调试模式
void LocationManager::setDebugMode(bool enabled) {
    _debugMode = enabled;
    printf("🔧 定位管理器调试模式: %s\n", enabled ? "开启" : "关闭");
}

// 检查调试模式
bool LocationManager::isDebugMode() const {
    return _debugMode;
}

// 打印当前位置
void LocationManager::printCurrentLocation() const {
    printf("\n=== 当前位置信息 ===\n");
    if (_currentLocation.isValid) {
        printf("省份: %s\n", _currentLocation.province.c_str());
        printf("城市: %s\n", _currentLocation.city.c_str());
        printf("区县: %s\n", _currentLocation.district.c_str());
        printf("区域编码: %s\n", _currentLocation.adcode.c_str());
        printf("城市编码: %s\n", _currentLocation.citycode.c_str());
        printf("定位类型: %s\n", _currentLocation.type == LOCATION_TYPE_IP ? "IP定位" : "GPS定位");
        printf("定位时间: %lu\n", _currentLocation.timestamp);
    } else {
        printf("无有效位置数据\n");
    }
    printf("==================\n");
}

// 打印统计信息
void LocationManager::printStatistics() const {
    printf("\n=== 定位统计信息 ===\n");
    printf("总请求: %lu次\n", _statistics.totalRequests);
    printf("成功请求: %lu次\n", _statistics.successRequests);
    printf("失败请求: %lu次\n", _statistics.failedRequests);
    printf("成功率: %.1f%%\n", _statistics.totalRequests > 0 ? 
           (float)_statistics.successRequests * 100.0 / _statistics.totalRequests : 0.0);
    printf("上次定位: %lu\n", _statistics.lastLocationTime);
    printf("定位耗时: %lu毫秒\n", _statistics.locationDuration);
    printf("--- 内存使用统计 ---\n");
    printf("总内存使用: %u字节\n", _statistics.totalMemoryUsed);
    printf("PSRAM使用: %u字节\n", _statistics.psramMemoryUsed);
    printf("内部RAM使用: %u字节\n", _statistics.internalMemoryUsed);
    printf("内存分配次数: %lu\n", _statistics.memoryAllocations);
    printf("内存释放次数: %lu\n", _statistics.memoryDeallocations);
    printf("==================\n");
}

// 打印配置信息
void LocationManager::printConfig() const {
    printf("\n=== 定位配置信息 ===\n");
    printf("API密钥: %s\n", _config.apiKey.isEmpty() ? "未设置" : "已设置");
    printf("首选定位类型: %s\n", _config.preferredType == LOCATION_TYPE_IP ? "IP定位" : "GPS定位");
    printf("自动定位: %s\n", _config.autoLocate ? "启用" : "禁用");
    printf("重试次数: %d\n", _config.retryTimes);
    printf("超时时间: %d毫秒\n", _config.timeout);
    printf("==================\n");
}

// 定位任务
void LocationManager::locationTask(void* parameter) {
    LocationManager* manager = static_cast<LocationManager*>(parameter);
    if (!manager) {
        printf("❌ 定位管理器实例无效\n");
        vTaskDelete(NULL);
        return;
    }
    
    printf("🌍 定位任务开始运行\n");
    
    manager->locationLoop();
    
    printf("🌍 定位任务结束\n");
    vTaskDelete(NULL);
}

// 定位循环
void LocationManager::locationLoop() {
    const TickType_t normalDelay = pdMS_TO_TICKS(30000);  // 30秒检查一次
    
    while (_isRunning) {
        if (_config.autoLocate && isWiFiConnected() && !_config.apiKey.isEmpty()) {
            // 如果没有有效位置数据，或者距离上次定位超过1小时，则重新定位
            unsigned long currentTime = millis();
            bool needLocation = !_currentLocation.isValid || 
                              (currentTime - _statistics.lastLocationTime > 3600000); // 1小时
            
            if (needLocation) {
                locateCurrentPosition();
            }
        }
        
        vTaskDelay(normalDelay);
    }
}

// 执行IP定位
bool LocationManager::performIPLocation() {
    printf("[LocationManager] 开始执行IP定位...\n");
    
    String url = buildLocationUrl();
    if (url.isEmpty()) {
        printf("❌ [LocationManager] 构建定位URL失败\n");
        return false;
    }
    
    String response;
    
    printDebugInfo("请求URL: " + url);
    
    if (!makeHttpRequest(url, response)) {
        printf("❌ [LocationManager] HTTP请求失败\n");
        return false;
    }
    
    if (!parseLocationResponse(response)) {
        printf("❌ [LocationManager] 解析定位响应失败\n");
        return false;
    }
    
    printf("✅ [LocationManager] IP定位执行成功\n");
    return true;
}

// 执行GPS定位（预留）
bool LocationManager::performGPSLocation() {
    // GPS定位功能预留
    printf("[LocationManager] GPS定位功能预留中...\n");
    return false;
}

// 进行HTTP请求
bool LocationManager::makeHttpRequest(const String& url, String& response) {
    // 参数验证
    if (url.isEmpty() || url.length() > 512) {
        printf("❌ [LocationManager] 无效的URL参数\n");
        return false;
    }
    
    // 检查WiFi连接状态
    if (!isWiFiConnected()) {
        printf("❌ [LocationManager] WiFi未连接\n");
        return false;
    }
    
    printf("[LocationManager] 发起HTTP请求: %s\n", url.c_str());
    
    HTTPClient http;
    
    // 设置超时时间
    http.setTimeout(_config.timeout);
    
    // 初始化HTTP客户端
    if (!http.begin(url)) {
        printf("❌ [LocationManager] HTTP客户端初始化失败\n");
        return false;
    }
    
    // 添加HTTP头
    http.addHeader("User-Agent", "ESP32S3-Monitor/1.0");
    http.addHeader("Accept", "application/json");
    
    // 发起GET请求
    int httpCode = http.GET();
    
    printf("[LocationManager] HTTP响应码: %d\n", httpCode);
    
    if (httpCode <= 0) {
        printf("❌ [LocationManager] HTTP请求失败，错误码: %d\n", httpCode);
        http.end();
        return false;
    }
    
    if (httpCode != HTTP_CODE_OK) {
        printf("❌ [LocationManager] HTTP请求失败，状态码: %d\n", httpCode);
        http.end();
        return false;
    }
    
    // 获取响应长度
    int contentLength = http.getSize();
    printf("[LocationManager] 响应内容长度: %d字节\n", contentLength);
    
    // 预估响应大小，为大响应使用PSRAM缓冲区
    const size_t LARGE_RESPONSE_THRESHOLD = 1024;  // 1KB阈值
    bool usePSRAM = (contentLength > LARGE_RESPONSE_THRESHOLD || contentLength == -1);
    
    if (usePSRAM && _psramManager && _psramManager->isPSRAMAvailable()) {
        // 使用PSRAM分配响应缓冲区
        size_t bufferSize = (contentLength > 0) ? contentLength + 256 : 4096;
        char* responseBuffer = (char*)_psramManager->allocateDataBuffer(bufferSize, "HTTP响应缓冲区");
        
        if (responseBuffer) {
            trackMemoryAllocation(bufferSize, true);
            _statistics.httpBufferSize = bufferSize;
            printf("[LocationManager] 使用PSRAM分配HTTP响应缓冲区: %u字节\n", bufferSize);
            
            // 使用WiFiClient流式读取响应
            WiFiClient* client = http.getStreamPtr();
            if (client) {
                size_t bytesRead = 0;
                unsigned long startTime = millis();
                
                while (client->connected() && bytesRead < bufferSize - 1) {
                    if (client->available()) {
                        int c = client->read();
                        if (c >= 0) {
                            responseBuffer[bytesRead++] = (char)c;
                        }
                    }
                    
                    // 超时检查
                    if (millis() - startTime > _config.timeout) {
                        printf("❌ [LocationManager] 响应读取超时\n");
                        break;
                    }
                    
                    // 让出CPU时间
                    if (bytesRead % 100 == 0) {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }
                }
                
                responseBuffer[bytesRead] = '\0';
                response = String(responseBuffer);
                printf("[LocationManager] 使用PSRAM缓冲区读取响应: %u字节\n", bytesRead);
            } else {
                printf("❌ [LocationManager] 无法获取HTTP流\n");
                response = "";
            }
            
            // 释放PSRAM缓冲区
            _psramManager->deallocate(responseBuffer);
            trackMemoryDeallocation(bufferSize, true);
        } else {
            printf("[LocationManager] PSRAM分配失败，使用标准方法\n");
            response = http.getString();
            _statistics.httpBufferSize = response.length();
        }
    } else {
        // 小响应或无PSRAM时使用标准方法
        response = http.getString();
        _statistics.httpBufferSize = response.length();
        printf("[LocationManager] 使用标准方法获取响应: %d字节\n", response.length());
    }
    
    http.end();
    
    if (response.isEmpty()) {
        printf("❌ [LocationManager] HTTP响应为空\n");
        return false;
    }
    
    printf("[LocationManager] HTTP响应长度: %d字节\n", response.length());
    printDebugInfo("HTTP响应长度: " + String(response.length()));
    
    return true;
}

// 解析定位响应
bool LocationManager::parseLocationResponse(const String& response) {
    if (response.isEmpty()) {
        printf("❌ [LocationManager] 响应数据为空\n");
        return false;
    }
    
    printf("[LocationManager] 开始解析定位响应，长度: %d\n", response.length());
    
    // 使用PSRAM分配JSON解析缓冲区
    const size_t capacity = JSON_OBJECT_SIZE(20) + JSON_ARRAY_SIZE(5) + 1024;
    DynamicJsonDocument* doc = nullptr;
    void* docBuffer = nullptr;
    bool usedPSRAMForJSON = false;
    
    if (_psramManager && _psramManager->isPSRAMAvailable()) {
        // 使用PSRAM分配JSON文档
        docBuffer = _psramManager->allocateDataBuffer(capacity, "JSON解析缓冲区");
        if (docBuffer) {
            doc = new(docBuffer) DynamicJsonDocument(capacity);
            usedPSRAMForJSON = true;
            trackMemoryAllocation(capacity, true);
            _statistics.jsonBufferSize = capacity;
            printf("[LocationManager] 使用PSRAM分配JSON解析缓冲区: %u字节\n", capacity);
        }
    }
    
    // 如果PSRAM分配失败，使用内部RAM
    if (!doc) {
        doc = new DynamicJsonDocument(capacity);
        usedPSRAMForJSON = false;
        trackMemoryAllocation(capacity, false);
        _statistics.jsonBufferSize = capacity;
        printf("[LocationManager] 使用内部RAM分配JSON解析缓冲区: %u字节\n", capacity);
    }
    
    if (!doc) {
        printf("❌ [LocationManager] JSON文档分配失败\n");
        return false;
    }
    
    DeserializationError error = deserializeJson(*doc, response);
    if (error) {
        printf("❌ [LocationManager] JSON解析失败: %s\n", error.c_str());
        // 清理JSON文档
        doc->~DynamicJsonDocument();
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    // 检查响应状态
    const char* status = (*doc)["status"];
    if (!status || strcmp(status, "1") != 0) {
        const char* info = (*doc)["info"];
        printf("❌ [LocationManager] API响应错误: %s\n", info ? info : "未知错误");
        // 清理JSON文档
        doc->~DynamicJsonDocument();
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    // 解析定位数据
    if (!lockLocationData()) {
        printf("❌ [LocationManager] 获取定位数据锁失败\n");
        // 清理JSON文档
        doc->~DynamicJsonDocument();
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    // 解析响应数据
    const char* province = (*doc)["province"];
    const char* city = (*doc)["city"];
    const char* adcode = (*doc)["adcode"];
    const char* rectangle = (*doc)["rectangle"];
    
    _currentLocation.province = province ? String(province) : "";
    _currentLocation.city = city ? String(city) : "";
    _currentLocation.district = ""; // IP定位通常不返回区县信息
    _currentLocation.adcode = adcode ? String(adcode) : "";
    _currentLocation.citycode = ""; // IP定位返回的通常是adcode
    _currentLocation.rectangle = rectangle ? String(rectangle) : "";
    _currentLocation.type = LOCATION_TYPE_IP;
    _currentLocation.timestamp = millis();
    _currentLocation.isValid = true;
    
    // 如果adcode有效，尝试转换为6位城市代码
    if (!_currentLocation.adcode.isEmpty()) {
        String weatherCode = convertAdcodeToWeatherCode(_currentLocation.adcode);
        if (!weatherCode.isEmpty()) {
            _currentLocation.citycode = weatherCode;
        }
    }
    
    printf("[LocationManager] 定位数据解析成功: %s %s\n", 
           _currentLocation.province.c_str(), _currentLocation.city.c_str());
    
    unlockLocationData();
    
    // 清理JSON文档
    doc->~DynamicJsonDocument();
    if (usedPSRAMForJSON) {
        _psramManager->deallocate(docBuffer);
        trackMemoryDeallocation(capacity, true);
    } else {
        delete doc;
        trackMemoryDeallocation(capacity, false);
    }
    
    printDebugInfo("定位数据解析成功");
    
    return true;
}

// 解析地理编码响应（预留）
bool LocationManager::parseGeocodeResponse(const String& response) {
    // 地理编码功能预留
    return true;
}

// 加载配置
bool LocationManager::loadConfig() {
    if (!_configStorage) {
        printf("[LocationManager] 配置存储未初始化\n");
        return false;
    }
    
    printf("[LocationManager] 开始加载定位配置...\n");
    
    LocationConfig config;
    
    // 加载API密钥
    config.apiKey = _configStorage->getStringAsync(getConfigKey("apiKey"), "");
    printf("[LocationManager] 加载API密钥: %s\n", config.apiKey.isEmpty() ? "未设置" : "已设置");
    
    // 加载其他配置
    config.preferredType = (LocationType)_configStorage->getIntAsync(getConfigKey("preferredType"), LOCATION_TYPE_IP);
    config.autoLocate = _configStorage->getBoolAsync(getConfigKey("autoLocate"), true);
    config.retryTimes = _configStorage->getIntAsync(getConfigKey("retryTimes"), 3);
    config.timeout = _configStorage->getIntAsync(getConfigKey("timeout"), 15000);
    
    printf("[LocationManager] 加载首选定位类型: %s\n", 
           config.preferredType == LOCATION_TYPE_IP ? "IP定位" : "GPS定位");
    printf("[LocationManager] 加载自动定位: %s\n", config.autoLocate ? "启用" : "禁用");
    printf("[LocationManager] 加载重试次数: %d\n", config.retryTimes);
    printf("[LocationManager] 加载超时时间: %d毫秒\n", config.timeout);
    
    _config = config;
    
    // 检查是否有有效的配置
    bool hasValidConfig = !config.apiKey.isEmpty();
    
    if (hasValidConfig) {
        printf("[LocationManager] 配置加载成功\n");
        printDebugInfo("配置加载成功");
        return true;
    } else {
        printf("[LocationManager] 未找到有效的配置，需要用户配置\n");
        printDebugInfo("未找到有效配置");
        return false;
    }
}

// 保存配置
bool LocationManager::saveConfig() {
    if (!_configStorage) {
        printf("[LocationManager] 配置存储未初始化\n");
        return false;
    }
    
    printf("[LocationManager] 开始保存定位配置...\n");
    
    bool success = true;
    
    // 分批保存配置，避免队列竞争
    success &= _configStorage->putStringAsync(getConfigKey("apiKey"), _config.apiKey, 10000);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    success &= _configStorage->putIntAsync(getConfigKey("preferredType"), (int)_config.preferredType, 10000);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    success &= _configStorage->putBoolAsync(getConfigKey("autoLocate"), _config.autoLocate, 10000);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    success &= _configStorage->putIntAsync(getConfigKey("retryTimes"), _config.retryTimes, 10000);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    success &= _configStorage->putIntAsync(getConfigKey("timeout"), _config.timeout, 10000);
    
    if (success) {
        printf("[LocationManager] 配置保存成功\n");
        printDebugInfo("配置保存成功");
    } else {
        printf("[LocationManager] 配置保存失败\n");
        printDebugInfo("配置保存失败");
    }
    
    return success;
}

// 获取配置键
String LocationManager::getConfigKey(const String& key) const {
    // 使用更短的键名以避免NVS限制
    if (key == "apiKey") return "loc_api";
    if (key == "preferredType") return "loc_type";
    if (key == "autoLocate") return "loc_auto";
    if (key == "retryTimes") return "loc_retry";
    if (key == "timeout") return "loc_timeout";
    return "loc_" + key;
}

// 构建定位URL
String LocationManager::buildLocationUrl() const {
    // 验证配置有效性
    if (_config.apiKey.isEmpty()) {
        printf("❌ [LocationManager] API密钥为空\n");
        return "";
    }
    
    // 检查API密钥长度
    if (_config.apiKey.length() < 10 || _config.apiKey.length() > 64) {
        printf("❌ [LocationManager] API密钥长度异常: %d\n", _config.apiKey.length());
        return "";
    }
    
    // 安全构建URL
    String url;
    url.reserve(256);  // 预分配内存避免多次重分配
    
    url = AMAP_LOCATION_BASE_URL;
    url += "?key=";
    url += _config.apiKey;
    url += "&output=json";
    
    printf("[LocationManager] 构建定位URL成功，长度: %d\n", url.length());
    return url;
}

// 构建地理编码URL（预留）
String LocationManager::buildGeocodeUrl(const String& address) const {
    // 地理编码URL构建预留
    return "";
}

// 检查WiFi连接
bool LocationManager::isWiFiConnected() const {
    return _wifiManager && _wifiManager->isConnected();
}

// 更新统计信息
void LocationManager::updateStatistics(bool success) {
    _statistics.totalRequests++;
    if (success) {
        _statistics.successRequests++;
    } else {
        _statistics.failedRequests++;
    }
}

// 打印调试信息
void LocationManager::printDebugInfo(const String& message) const {
    if (_debugMode) {
        printf("[LocationManager] %s\n", message.c_str());
    }
}

// 跟踪内存分配
void LocationManager::trackMemoryAllocation(size_t size, bool isPSRAM) {
    if (!_memoryStatsEnabled) return;
    
    _statistics.memoryAllocations++;
    updateMemoryStatistics(size, isPSRAM);
    
    if (_debugMode) {
        printf("[LocationManager] 内存分配: %u字节 (%s), 当前使用: %u字节\n", 
               size, isPSRAM ? "PSRAM" : "内部RAM", _currentMemoryUsage);
    }
}

// 跟踪内存释放
void LocationManager::trackMemoryDeallocation(size_t size, bool isPSRAM) {
    if (!_memoryStatsEnabled) return;
    
    _statistics.memoryDeallocations++;
    
    if (isPSRAM) {
        _statistics.psramMemoryUsed = (_statistics.psramMemoryUsed > size) ? 
                                     _statistics.psramMemoryUsed - size : 0;
    } else {
        _statistics.internalMemoryUsed = (_statistics.internalMemoryUsed > size) ? 
                                        _statistics.internalMemoryUsed - size : 0;
    }
    
    _statistics.totalMemoryUsed = (_statistics.totalMemoryUsed > size) ? 
                                 _statistics.totalMemoryUsed - size : 0;
    _currentMemoryUsage = (_currentMemoryUsage > size) ? 
                         _currentMemoryUsage - size : 0;
    
    if (_debugMode) {
        printf("[LocationManager] 内存释放: %u字节 (%s), 当前使用: %u字节\n", 
               size, isPSRAM ? "PSRAM" : "内部RAM", _currentMemoryUsage);
    }
}

// 更新内存统计
void LocationManager::updateMemoryStatistics(size_t allocated, bool isPSRAM) {
    if (!_memoryStatsEnabled) return;
    
    if (isPSRAM) {
        _statistics.psramMemoryUsed += allocated;
    } else {
        _statistics.internalMemoryUsed += allocated;
    }
    
    _statistics.totalMemoryUsed += allocated;
    _currentMemoryUsage += allocated;
    
    if (_currentMemoryUsage > _peakMemoryUsage) {
        _peakMemoryUsage = _currentMemoryUsage;
    }
}

// 创建定位任务
bool LocationManager::createLocationTask() {
    size_t stackSize = 4096;  // 4KB栈大小
    
    printf("[LocationManager] 创建定位任务(使用内部RAM栈，固定到核心0)...\n");
    
    BaseType_t result = xTaskCreatePinnedToCore(
        locationTask,
        "LocationTask",
        stackSize / sizeof(StackType_t),
        this,
        2,  // 优先级
        &_locationTaskHandle,
        0   // 核心0
    );
    
    if (result != pdPASS) {
        printf("❌ [LocationManager] 创建定位任务失败\n");
        return false;
    }
    
    printf("✅ [LocationManager] 定位任务创建成功(内部RAM栈，固定到核心0)\n");
    return true;
}

// 删除定位任务
void LocationManager::deleteLocationTask() {
    if (_locationTaskHandle) {
        vTaskDelete(_locationTaskHandle);
        _locationTaskHandle = nullptr;
        printDebugInfo("定位任务已删除");
    }
}

// 锁定定位数据
bool LocationManager::lockLocationData(unsigned long timeout) {
    if (!_locationMutex) {
        return false;
    }
    
    return xSemaphoreTake(_locationMutex, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

// 解锁定位数据
void LocationManager::unlockLocationData() {
    if (_locationMutex) {
        xSemaphoreGive(_locationMutex);
    }
}

// 验证配置有效性
bool LocationManager::validateConfig(const LocationConfig& config) {
    // 检查重试次数
    if (config.retryTimes < 1 || config.retryTimes > 10) {
        printf("[LocationManager] 错误：重试次数应在1-10之间\n");
        return false;
    }
    
    // 检查超时时间
    if (config.timeout < 5000 || config.timeout > 60000) {
        printf("[LocationManager] 错误：超时时间应在5000-60000毫秒之间\n");
        return false;
    }
    
    printf("[LocationManager] 配置验证通过\n");
    return true;
}

// 地区代码转换
String LocationManager::convertAdcodeToWeatherCode(const String& adcode) const {
    if (adcode.isEmpty() || adcode.length() != 6) {
        return "";
    }
    
    // 高德地图的adcode就是标准的6位区域代码，可以直接用于天气查询
    return adcode;
}

// 检查adcode有效性
bool LocationManager::isValidAdcode(const String& adcode) const {
    if (adcode.isEmpty() || adcode.length() != 6) {
        return false;
    }
    
    // 检查是否为数字
    for (int i = 0; i < adcode.length(); i++) {
        if (!isdigit(adcode[i])) {
            return false;
        }
    }
    
    return true;
}

// 打印内存使用情况
void LocationManager::printMemoryUsage() const {
    printf("\n=== 定位管理器内存使用情况 ===\n");
    printf("当前内存使用: %u字节\n", _currentMemoryUsage);
    printf("峰值内存使用: %u字节\n", _peakMemoryUsage);
    printf("总内存使用: %u字节\n", _statistics.totalMemoryUsed);
    printf("PSRAM使用: %u字节\n", _statistics.psramMemoryUsed);
    printf("内部RAM使用: %u字节\n", _statistics.internalMemoryUsed);
    printf("JSON缓冲区大小: %u字节\n", _statistics.jsonBufferSize);
    printf("HTTP缓冲区大小: %u字节\n", _statistics.httpBufferSize);
    printf("内存分配次数: %lu\n", _statistics.memoryAllocations);
    printf("内存释放次数: %lu\n", _statistics.memoryDeallocations);
    printf("内存统计: %s\n", _memoryStatsEnabled ? "启用" : "禁用");
    printf("===============================\n");
}

// 获取内存使用JSON
String LocationManager::getMemoryUsageJSON() const {
    String json = "{";
    json += "\"currentMemoryUsage\":" + String(_currentMemoryUsage) + ",";
    json += "\"peakMemoryUsage\":" + String(_peakMemoryUsage) + ",";
    json += "\"totalMemoryUsed\":" + String(_statistics.totalMemoryUsed) + ",";
    json += "\"psramMemoryUsed\":" + String(_statistics.psramMemoryUsed) + ",";
    json += "\"internalMemoryUsed\":" + String(_statistics.internalMemoryUsed) + ",";
    json += "\"jsonBufferSize\":" + String(_statistics.jsonBufferSize) + ",";
    json += "\"httpBufferSize\":" + String(_statistics.httpBufferSize) + ",";
    json += "\"memoryAllocations\":" + String(_statistics.memoryAllocations) + ",";
    json += "\"memoryDeallocations\":" + String(_statistics.memoryDeallocations) + ",";
    json += "\"memoryStatsEnabled\":" + String(_memoryStatsEnabled ? "true" : "false");
    json += "}";
    return json;
}

// 重置内存统计
void LocationManager::resetMemoryStatistics() {
    _statistics.totalMemoryUsed = 0;
    _statistics.psramMemoryUsed = 0;
    _statistics.internalMemoryUsed = 0;
    _statistics.jsonBufferSize = 0;
    _statistics.httpBufferSize = 0;
    _statistics.memoryAllocations = 0;
    _statistics.memoryDeallocations = 0;
    _currentMemoryUsage = 0;
    _peakMemoryUsage = 0;
    
    printf("[LocationManager] 内存统计已重置\n");
}
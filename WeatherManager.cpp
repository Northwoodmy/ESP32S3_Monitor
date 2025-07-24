/*
 * WeatherManager.cpp - 天气管理器模块实现
 * 
 * 功能说明:
 * - 使用HTTP客户端获取高德天气API数据
 * - 支持实时天气和未来天气预报
 * - 定时更新天气信息
 * - 提供天气数据给其他模块使用
 * - 基于FreeRTOS任务实现
 * 
 * 作者: ESP32S3_Monitor
 * 版本: v1.0.0
 * 日期: 2024-12-19
 */

#include "WeatherManager.h"

// 默认配置
static const WeatherConfig DEFAULT_CONFIG = {
    .apiKey = "",                               // 需要用户配置高德API密钥
    .cityCode = "",                             // 空值，需要用户配置
    .cityName = "",                             // 空值，需要用户配置
    .autoUpdate = true,
    .updateInterval = 30,                       // 30分钟
    .enableForecast = false
};

// 构造函数
WeatherManager::WeatherManager() {
    _psramManager = nullptr;
    _wifiManager = nullptr;
    _configStorage = nullptr;
    _weatherTaskHandle = nullptr;
    _weatherMutex = nullptr;
    _forecastData = nullptr;
    _forecastCount = 0;
    _state = WEATHER_STATE_INIT;
    _isInitialized = false;
    _isRunning = false;
    _debugMode = false;
    
    // 初始化内存监控
    _currentMemoryUsage = 0;
    _peakMemoryUsage = 0;
    _memoryStatsEnabled = true;
    
    // 初始化统计信息
    _statistics = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    // 初始化天气数据
    _currentWeather = {};
    _currentWeather.isValid = false;
    
    // 初始化配置
    _config = DEFAULT_CONFIG;
}

// 析构函数
WeatherManager::~WeatherManager() {
    stop();
    
    // 清理预报数据
    cleanupForecastData();
    
    // 删除互斥锁
    if (_weatherMutex) {
        vSemaphoreDelete(_weatherMutex);
        _weatherMutex = nullptr;
    }
}

// 初始化天气管理器
bool WeatherManager::init(PSRAMManager* psramManager, WiFiManager* wifiManager, ConfigStorage* configStorage) {
    if (_isInitialized) {
        printDebugInfo("天气管理器已初始化");
        return true;
    }
    
    printf("🌤️ 初始化天气管理器...\n");
    
    // 检查依赖
    if (!psramManager || !wifiManager || !configStorage) {
        printf("❌ 天气管理器依赖参数无效\n");
        return false;
    }
    
    _psramManager = psramManager;
    _wifiManager = wifiManager;
    _configStorage = configStorage;
    
    // 创建互斥锁
    _weatherMutex = xSemaphoreCreateMutex();
    if (!_weatherMutex) {
        printf("❌ 创建天气数据互斥锁失败\n");
        return false;
    }
    
    // 加载配置
    if (!loadConfig()) {
        printf("⚠️ 加载天气配置失败，使用默认配置\n");
        _config = DEFAULT_CONFIG;
    }
    
    // 设置状态
    _state = WEATHER_STATE_READY;
    _isInitialized = true;
    
    printf("✅ 天气管理器初始化成功\n");
    printf("   城市: %s (%s)\n", _config.cityName.c_str(), _config.cityCode.c_str());
    printf("   自动更新: %s\n", _config.autoUpdate ? "启用" : "禁用");
    printf("   更新间隔: %d分钟\n", _config.updateInterval);
    printf("   预报功能: %s\n", _config.enableForecast ? "启用" : "禁用");
    
    return true;
}

// 启动天气管理器
bool WeatherManager::start() {
    if (!_isInitialized) {
        printf("❌ 天气管理器未初始化\n");
        return false;
    }
    
    if (_isRunning) {
        printDebugInfo("天气管理器已在运行");
        return true;
    }
    
    printf("🚀 启动天气管理器...\n");
    
    // 创建天气更新任务
    if (!createWeatherTask()) {
        printf("❌ 创建天气更新任务失败\n");
        return false;
    }
    
    _isRunning = true;
    
    printf("✅ 天气管理器启动成功\n");
    return true;
}

// 停止天气管理器
void WeatherManager::stop() {
    if (!_isRunning) {
        return;
    }
    
    printf("🛑 停止天气管理器...\n");
    
    _isRunning = false;
    
    // 删除任务
    deleteWeatherTask();
    
    printf("✅ 天气管理器已停止\n");
}

// 重启天气管理器
void WeatherManager::restart() {
    printf("🔄 重启天气管理器...\n");
    stop();
    vTaskDelay(pdMS_TO_TICKS(1000));
    start();
}

// 设置配置
bool WeatherManager::setConfig(const WeatherConfig& config) {
    if (!lockWeatherData()) {
        return false;
    }
    
    // 验证配置的有效性
    if (!validateConfig(config)) {
        printf("❌ 天气配置验证失败\n");
        unlockWeatherData();
        return false;
    }
    
    _config = config;
    bool success = saveConfig();
    
    unlockWeatherData();
    
    if (success) {
        printf("✅ 天气配置更新成功\n");
        printf("   城市: %s (%s)\n", _config.cityName.c_str(), _config.cityCode.c_str());
    } else {
        printf("❌ 天气配置保存失败\n");
    }
    
    return success;
}

// 获取配置
WeatherConfig WeatherManager::getConfig() const {
    return _config;
}

// 设置API密钥
bool WeatherManager::setApiKey(const String& apiKey) {
    if (!lockWeatherData()) {
        return false;
    }
    
    _config.apiKey = apiKey;
    bool success = saveConfig();
    
    unlockWeatherData();
    
    if (success) {
        printf("✅ 高德API密钥设置成功\n");
    } else {
        printf("❌ 高德API密钥保存失败\n");
    }
    
    return success;
}

// 设置城市代码
bool WeatherManager::setCityCode(const String& cityCode) {
    if (!lockWeatherData()) {
        return false;
    }
    
    _config.cityCode = cityCode;
    bool success = saveConfig();
    
    unlockWeatherData();
    
    if (success) {
        printf("✅ 城市代码设置成功: %s\n", cityCode.c_str());
    } else {
        printf("❌ 城市代码保存失败\n");
    }
    
    return success;
}

// 设置自动更新
bool WeatherManager::setAutoUpdate(bool enable) {
    if (!lockWeatherData()) {
        return false;
    }
    
    _config.autoUpdate = enable;
    bool success = saveConfig();
    
    unlockWeatherData();
    
    printf("✅ 自动更新设置为: %s\n", enable ? "启用" : "禁用");
    return success;
}

// 设置更新间隔
bool WeatherManager::setUpdateInterval(int minutes) {
    if (minutes < 5 || minutes > 1440) {  // 5分钟到24小时
        printf("❌ 更新间隔无效，有效范围: 5-1440分钟\n");
        return false;
    }
    
    if (!lockWeatherData()) {
        return false;
    }
    
    _config.updateInterval = minutes;
    bool success = saveConfig();
    
    unlockWeatherData();
    
    printf("✅ 更新间隔设置为: %d分钟\n", minutes);
    return success;
}

// 更新天气数据
bool WeatherManager::updateWeatherData() {
    if (!_isInitialized) {
        printf("❌ 天气管理器未初始化\n");
        return false;
    }
    
    if (!isWiFiConnected()) {
        printf("❌ WiFi未连接，无法更新天气\n");
        _state = WEATHER_STATE_NO_WIFI;
        return false;
    }
    
    if (_config.apiKey.isEmpty()) {
        printf("❌ 高德API密钥未设置\n");
        _state = WEATHER_STATE_ERROR;
        return false;
    }
    
    _state = WEATHER_STATE_UPDATING;
    
    printf("🌤️ 开始更新天气数据...\n");
    
    bool success = performWeatherUpdate();
    
    if (success) {
        _state = WEATHER_STATE_SUCCESS;
        _statistics.lastUpdateTime = millis();
        _statistics.nextUpdateTime = _statistics.lastUpdateTime + (_config.updateInterval * 60 * 1000);
        printf("✅ 天气数据更新成功\n");
    } else {
        _state = WEATHER_STATE_FAILED;
        printf("❌ 天气数据更新失败\n");
    }
    
    updateStatistics(success);
    
    return success;
}

// 更新预报数据
bool WeatherManager::updateForecastData() {
    if (!_isInitialized || !_config.enableForecast) {
        return false;
    }
    
    if (!isWiFiConnected()) {
        printf("❌ WiFi未连接，无法更新预报\n");
        return false;
    }
    
    if (_config.apiKey.isEmpty()) {
        printf("❌ 高德API密钥未设置\n");
        return false;
    }
    
    printf("📅 开始更新天气预报...\n");
    
    bool success = performForecastUpdate();
    
    if (success) {
        printf("✅ 天气预报更新成功\n");
    } else {
        printf("❌ 天气预报更新失败\n");
    }
    
    return success;
}

// 获取当前天气
WeatherData WeatherManager::getCurrentWeather() const {
    return _currentWeather;
}

// 获取预报数据
WeatherForecast* WeatherManager::getForecastData(int& count) const {
    count = _forecastCount;
    return _forecastData;
}

// 检查天气数据有效性
bool WeatherManager::isWeatherDataValid() const {
    return _currentWeather.isValid;
}

// 获取状态
WeatherState WeatherManager::getState() const {
    return _state;
}

// 获取统计信息
WeatherStatistics WeatherManager::getStatistics() const {
    return _statistics;
}

// 获取状态字符串
String WeatherManager::getStateString() const {
    switch (_state) {
        case WEATHER_STATE_INIT: return "初始化";
        case WEATHER_STATE_READY: return "就绪";
        case WEATHER_STATE_UPDATING: return "更新中";
        case WEATHER_STATE_SUCCESS: return "成功";
        case WEATHER_STATE_FAILED: return "失败";
        case WEATHER_STATE_NO_WIFI: return "无WiFi";
        case WEATHER_STATE_ERROR: return "错误";
        default: return "未知";
    }
}

// 获取上次更新时间
unsigned long WeatherManager::getLastUpdateTime() const {
    return _statistics.lastUpdateTime;
}

// 获取下次更新时间
unsigned long WeatherManager::getNextUpdateTime() const {
    return _statistics.nextUpdateTime;
}

// 设置调试模式
void WeatherManager::setDebugMode(bool enabled) {
    _debugMode = enabled;
    printf("🔧 天气管理器调试模式: %s\n", enabled ? "开启" : "关闭");
}

// 检查调试模式
bool WeatherManager::isDebugMode() const {
    return _debugMode;
}

// 打印当前天气
void WeatherManager::printCurrentWeather() const {
    printf("\n=== 当前天气信息 ===\n");
    if (_currentWeather.isValid) {
        printf("城市: %s (%s)\n", _currentWeather.city.c_str(), _currentWeather.adcode.c_str());
        printf("天气: %s\n", _currentWeather.weather.c_str());
        printf("温度: %s°C\n", _currentWeather.temperature.c_str());
        printf("湿度: %s%%\n", _currentWeather.humidity.c_str());
        printf("风向: %s\n", _currentWeather.winddirection.c_str());
        printf("风力: %s级\n", _currentWeather.windpower.c_str());
        printf("发布时间: %s\n", _currentWeather.reporttime.c_str());
    } else {
        printf("无有效天气数据\n");
    }
    printf("==================\n");
}

// 打印预报数据
void WeatherManager::printForecastData() const {
    printf("\n=== 天气预报信息 ===\n");
    if (_forecastData && _forecastCount > 0) {
        for (int i = 0; i < _forecastCount; i++) {
            printf("日期: %s (%s)\n", _forecastData[i].date.c_str(), _forecastData[i].week.c_str());
            printf("  白天: %s %s°C %s %s级\n", 
                   _forecastData[i].dayweather.c_str(), 
                   _forecastData[i].daytemp.c_str(),
                   _forecastData[i].daywind.c_str(),
                   _forecastData[i].daypower.c_str());
            printf("  夜间: %s %s°C %s %s级\n", 
                   _forecastData[i].nightweather.c_str(), 
                   _forecastData[i].nighttemp.c_str(),
                   _forecastData[i].nightwind.c_str(),
                   _forecastData[i].nightpower.c_str());
            printf("---\n");
        }
    } else {
        printf("无预报数据\n");
    }
    printf("==================\n");
}

// 打印统计信息
void WeatherManager::printStatistics() const {
    printf("\n=== 天气统计信息 ===\n");
    printf("总请求: %lu次\n", _statistics.totalRequests);
    printf("成功请求: %lu次\n", _statistics.successRequests);
    printf("失败请求: %lu次\n", _statistics.failedRequests);
    printf("成功率: %.1f%%\n", _statistics.totalRequests > 0 ? 
           (float)_statistics.successRequests * 100.0 / _statistics.totalRequests : 0.0);
    printf("上次更新: %lu\n", _statistics.lastUpdateTime);
    printf("下次更新: %lu\n", _statistics.nextUpdateTime);
    printf("--- 内存使用统计 ---\n");
    printf("总内存使用: %u字节\n", _statistics.totalMemoryUsed);
    printf("PSRAM使用: %u字节\n", _statistics.psramMemoryUsed);
    printf("内部RAM使用: %u字节\n", _statistics.internalMemoryUsed);
    printf("内存分配次数: %lu\n", _statistics.memoryAllocations);
    printf("内存释放次数: %lu\n", _statistics.memoryDeallocations);
    printf("==================\n");
}

// 打印配置信息
void WeatherManager::printConfig() const {
    printf("\n=== 天气配置信息 ===\n");
    printf("API密钥: %s\n", _config.apiKey.isEmpty() ? "未设置" : "已设置");
    printf("城市代码: %s\n", _config.cityCode.c_str());
    printf("城市名称: %s\n", _config.cityName.c_str());
    printf("自动更新: %s\n", _config.autoUpdate ? "启用" : "禁用");
    printf("更新间隔: %d分钟\n", _config.updateInterval);
    printf("预报功能: %s\n", _config.enableForecast ? "启用" : "禁用");
    printf("==================\n");
}

// 天气更新任务
void WeatherManager::weatherUpdateTask(void* parameter) {
    WeatherManager* manager = static_cast<WeatherManager*>(parameter);
    if (!manager) {
        printf("❌ 天气管理器实例无效\n");
        vTaskDelete(NULL);
        return;
    }
    
    printf("🌤️ 天气更新任务开始运行\n");
    
    manager->weatherUpdateLoop();
    
    printf("🌤️ 天气更新任务结束\n");
    vTaskDelete(NULL);
}

// 天气更新循环
void WeatherManager::weatherUpdateLoop() {
    const TickType_t normalDelay = pdMS_TO_TICKS(60000);  // 1分钟检查一次
    
    // 启动后立即更新一次
    if (_config.autoUpdate) {
        vTaskDelay(pdMS_TO_TICKS(5000));  // 等待5秒后开始
        updateWeatherData();
        if (_config.enableForecast) {
            vTaskDelay(pdMS_TO_TICKS(2000));  // 间隔2秒
            updateForecastData();
        }
    }
    
    while (_isRunning) {
        if (_config.autoUpdate && isWiFiConnected()) {
            unsigned long currentTime = millis();
            
            // 检查是否需要更新
            if (currentTime >= _statistics.nextUpdateTime) {
                updateWeatherData();
                
                // 更新预报数据
                if (_config.enableForecast) {
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    updateForecastData();
                }
            }
        }
        
        vTaskDelay(normalDelay);
    }
}

// 执行天气更新
bool WeatherManager::performWeatherUpdate() {
    printf("[WeatherManager] 开始执行天气更新...\n");
    
    String url = buildWeatherUrl();
    if (url.isEmpty()) {
        printf("❌ [WeatherManager] 构建天气URL失败\n");
        return false;
    }
    
    String response;
    
    printDebugInfo("请求URL: " + url);
    
    if (!makeHttpRequest(url, response)) {
        printf("❌ [WeatherManager] HTTP请求失败\n");
        return false;
    }
    
    if (!parseWeatherResponse(response)) {
        printf("❌ [WeatherManager] 解析天气响应失败\n");
        return false;
    }
    
    printf("✅ [WeatherManager] 天气更新执行成功\n");
    return true;
}

// 执行预报更新
bool WeatherManager::performForecastUpdate() {
    printf("[WeatherManager] 开始执行预报更新...\n");
    
    String url = buildForecastUrl();
    if (url.isEmpty()) {
        printf("❌ [WeatherManager] 构建预报URL失败\n");
        return false;
    }
    
    String response;
    
    printDebugInfo("请求预报URL: " + url);
    
    if (!makeHttpRequest(url, response)) {
        printf("❌ [WeatherManager] HTTP预报请求失败\n");
        return false;
    }
    
    if (!parseForecastResponse(response)) {
        printf("❌ [WeatherManager] 解析预报响应失败\n");
        return false;
    }
    
    printf("✅ [WeatherManager] 预报更新执行成功\n");
    return true;
}

// 进行HTTP请求
bool WeatherManager::makeHttpRequest(const String& url, String& response) {
    // 参数验证
    if (url.isEmpty() || url.length() > 512) {
        printf("❌ [WeatherManager] 无效的URL参数\n");
        return false;
    }
    
    // 检查WiFi连接状态
    if (!isWiFiConnected()) {
        printf("❌ [WeatherManager] WiFi未连接\n");
        return false;
    }
    
    printf("[WeatherManager] 发起HTTP请求: %s\n", url.c_str());
    
    HTTPClient http;
    
    // 设置超时时间
    http.setTimeout(WEATHER_TIMEOUT);
    
    // 初始化HTTP客户端
    if (!http.begin(url)) {
        printf("❌ [WeatherManager] HTTP客户端初始化失败\n");
        return false;
    }
    
    // 添加HTTP头
    http.addHeader("User-Agent", "ESP32S3-Monitor/1.0");
    http.addHeader("Accept", "application/json");
    
    // 发起GET请求
    int httpCode = http.GET();
    
    printf("[WeatherManager] HTTP响应码: %d\n", httpCode);
    
    if (httpCode <= 0) {
        printf("❌ [WeatherManager] HTTP请求失败，错误码: %d\n", httpCode);
        http.end();
        return false;
    }
    
    if (httpCode != HTTP_CODE_OK) {
        printf("❌ [WeatherManager] HTTP请求失败，状态码: %d\n", httpCode);
        http.end();
        return false;
    }
    
    // 获取响应长度
    int contentLength = http.getSize();
    printf("[WeatherManager] 响应内容长度: %d字节\n", contentLength);
    
    // 预估响应大小，为大响应使用PSRAM缓冲区
    const size_t LARGE_RESPONSE_THRESHOLD = 2048;  // 2KB阈值
    bool usePSRAM = (contentLength > LARGE_RESPONSE_THRESHOLD || contentLength == -1);
    
    if (usePSRAM && _psramManager && _psramManager->isPSRAMAvailable()) {
        // 使用PSRAM分配响应缓冲区
        size_t bufferSize = (contentLength > 0) ? contentLength + 256 : 8192;  // 添加一些缓冲
        char* responseBuffer = (char*)_psramManager->allocateDataBuffer(bufferSize, "HTTP响应缓冲区");
        
        if (responseBuffer) {
            trackMemoryAllocation(bufferSize, true);
            _statistics.httpBufferSize = bufferSize;
            printf("[WeatherManager] 使用PSRAM分配HTTP响应缓冲区: %u字节\n", bufferSize);
            
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
                    if (millis() - startTime > WEATHER_TIMEOUT) {
                        printf("❌ [WeatherManager] 响应读取超时\n");
                        break;
                    }
                    
                    // 让出CPU时间
                    if (bytesRead % 100 == 0) {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }
                }
                
                responseBuffer[bytesRead] = '\0';
                response = String(responseBuffer);
                printf("[WeatherManager] 使用PSRAM缓冲区读取响应: %u字节\n", bytesRead);
            } else {
                printf("❌ [WeatherManager] 无法获取HTTP流\n");
                response = "";
            }
            
            // 释放PSRAM缓冲区
            _psramManager->deallocate(responseBuffer);
            trackMemoryDeallocation(bufferSize, true);
        } else {
            printf("[WeatherManager] PSRAM分配失败，使用标准方法\n");
            response = http.getString();
            _statistics.httpBufferSize = response.length();
        }
    } else {
        // 小响应或无PSRAM时使用标准方法
        response = http.getString();
        _statistics.httpBufferSize = response.length();
        printf("[WeatherManager] 使用标准方法获取响应: %d字节\n", response.length());
    }
    
    http.end();
    
    if (response.isEmpty()) {
        printf("❌ [WeatherManager] HTTP响应为空\n");
        return false;
    }
    
    printf("[WeatherManager] HTTP响应长度: %d字节\n", response.length());
    printDebugInfo("HTTP响应长度: " + String(response.length()));
    
    return true;
}

// 解析天气响应
bool WeatherManager::parseWeatherResponse(const String& response) {
    if (response.isEmpty()) {
        printf("❌ [WeatherManager] 响应数据为空\n");
        return false;
    }
    
    printf("[WeatherManager] 开始解析天气响应，长度: %d\n", response.length());
    
    // 使用PSRAM分配JSON解析缓冲区以减少内部RAM使用
    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(5) + 1024;
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
            printf("[WeatherManager] 使用PSRAM分配JSON解析缓冲区: %u字节\n", capacity);
        }
    }
    
    // 如果PSRAM分配失败，使用内部RAM
    if (!doc) {
        doc = new DynamicJsonDocument(capacity);
        usedPSRAMForJSON = false;
        trackMemoryAllocation(capacity, false);
        _statistics.jsonBufferSize = capacity;
        printf("[WeatherManager] 使用内部RAM分配JSON解析缓冲区: %u字节\n", capacity);
    }
    
    if (!doc) {
        printf("❌ [WeatherManager] JSON文档分配失败\n");
        return false;
    }
    
    DeserializationError error = deserializeJson(*doc, response);
    if (error) {
        printf("❌ [WeatherManager] JSON解析失败: %s\n", error.c_str());
        // 清理JSON文档
        doc->~DynamicJsonDocument();  // 显式调用析构函数
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
        printf("❌ [WeatherManager] API响应错误: %s\n", info ? info : "未知错误");
        // 清理JSON文档
        doc->~DynamicJsonDocument();  // 显式调用析构函数
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    // 解析天气数据
    if (!lockWeatherData()) {
        printf("❌ [WeatherManager] 获取天气数据锁失败\n");
        // 清理JSON文档
        doc->~DynamicJsonDocument();  // 显式调用析构函数
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    JsonArray lives = (*doc)["lives"];
    if (lives.size() > 0) {
        JsonObject weather = lives[0];
        
        // 安全的字符串赋值
        const char* city = weather["city"];
        const char* adcode = weather["adcode"];
        const char* weatherStr = weather["weather"];
        const char* temperature = weather["temperature"];
        const char* humidity = weather["humidity"];
        const char* winddirection = weather["winddirection"];
        const char* windpower = weather["windpower"];
        const char* reporttime = weather["reporttime"];
        
        _currentWeather.city = city ? String(city) : "";
        _currentWeather.adcode = adcode ? String(adcode) : "";
        _currentWeather.weather = weatherStr ? String(weatherStr) : "";
        _currentWeather.temperature = temperature ? String(temperature) : "";
        _currentWeather.humidity = humidity ? String(humidity) : "";
        _currentWeather.winddirection = winddirection ? String(winddirection) : "";
        _currentWeather.windpower = windpower ? String(windpower) : "";
        _currentWeather.reporttime = reporttime ? String(reporttime) : "";
        _currentWeather.isValid = true;
        
        printf("[WeatherManager] 天气数据解析成功: %s, %s°C\n", 
               _currentWeather.weather.c_str(), _currentWeather.temperature.c_str());
    } else {
        printf("❌ [WeatherManager] 响应中无天气数据\n");
        unlockWeatherData();
        // 清理JSON文档
        doc->~DynamicJsonDocument();  // 显式调用析构函数
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    unlockWeatherData();
    
    // 清理JSON文档
    doc->~DynamicJsonDocument();  // 显式调用析构函数
    if (usedPSRAMForJSON) {
        _psramManager->deallocate(docBuffer);
        trackMemoryDeallocation(capacity, true);
    } else {
        delete doc;
        trackMemoryDeallocation(capacity, false);
    }
    
    printDebugInfo("天气数据解析成功");
    
    return true;
}

// 解析预报响应
bool WeatherManager::parseForecastResponse(const String& response) {
    if (response.isEmpty()) {
        printf("❌ [WeatherManager] 预报响应数据为空\n");
        return false;
    }
    
    printf("[WeatherManager] 开始解析预报响应，长度: %d\n", response.length());
    
    // 使用PSRAM分配JSON解析缓冲区以减少内部RAM使用
    const size_t capacity = JSON_OBJECT_SIZE(20) + JSON_ARRAY_SIZE(10) + 2048;
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
            printf("[WeatherManager] 使用PSRAM分配JSON解析缓冲区: %u字节\n", capacity);
        }
    }
    
    // 如果PSRAM分配失败，使用内部RAM
    if (!doc) {
        doc = new DynamicJsonDocument(capacity);
        usedPSRAMForJSON = false;
        trackMemoryAllocation(capacity, false);
        _statistics.jsonBufferSize = capacity;
        printf("[WeatherManager] 使用内部RAM分配JSON解析缓冲区: %u字节\n", capacity);
    }
    
    if (!doc) {
        printf("❌ [WeatherManager] JSON文档分配失败\n");
        return false;
    }
    
    DeserializationError error = deserializeJson(*doc, response);
    if (error) {
        printf("❌ [WeatherManager] 预报JSON解析失败: %s\n", error.c_str());
        // 清理JSON文档
        doc->~DynamicJsonDocument();  // 显式调用析构函数
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
        printf("❌ [WeatherManager] 预报API响应错误: %s\n", info ? info : "未知错误");
        // 清理JSON文档
        doc->~DynamicJsonDocument();  // 显式调用析构函数
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    // 解析预报数据
    if (!lockWeatherData()) {
        printf("❌ [WeatherManager] 获取预报数据锁失败\n");
        // 清理JSON文档
        doc->~DynamicJsonDocument();  // 显式调用析构函数
        if (usedPSRAMForJSON) {
            _psramManager->deallocate(docBuffer);
            trackMemoryDeallocation(capacity, true);
        } else {
            delete doc;
            trackMemoryDeallocation(capacity, false);
        }
        return false;
    }
    
    // 清理旧数据
    cleanupForecastData();
    
    JsonArray forecasts = (*doc)["forecasts"];
    if (forecasts.size() > 0) {
        JsonObject forecast = forecasts[0];
        JsonArray casts = forecast["casts"];
        
        _forecastCount = casts.size();
        printf("[WeatherManager] 预报数据条数: %d\n", _forecastCount);
        
        if (_forecastCount > 0 && _forecastCount <= 7) {  // 限制最大7天预报
            size_t totalSize = sizeof(WeatherForecast) * _forecastCount;
            
            // 优先使用PSRAM分配预报数据
            if (_psramManager && _psramManager->isPSRAMAvailable()) {
                _forecastData = (WeatherForecast*)_psramManager->allocateDataBuffer(totalSize, "天气预报数据");
                if (_forecastData) {
                    trackMemoryAllocation(totalSize, true);
                    _statistics.forecastDataSize = totalSize;
                    printf("[WeatherManager] 使用PSRAM分配预报数据: %u字节\n", totalSize);
                } else {
                    printf("[WeatherManager] PSRAM分配失败，使用内部RAM\n");
                    _forecastData = (WeatherForecast*)malloc(totalSize);
                    if (_forecastData) {
                        trackMemoryAllocation(totalSize, false);
                        _statistics.forecastDataSize = totalSize;
                    }
                }
            } else {
                _forecastData = (WeatherForecast*)malloc(totalSize);
                if (_forecastData) {
                    trackMemoryAllocation(totalSize, false);
                    _statistics.forecastDataSize = totalSize;
                }
                printf("[WeatherManager] 使用内部RAM分配预报数据: %u字节\n", totalSize);
            }
            
            if (_forecastData) {
                printf("[WeatherManager] 预报数据内存分配成功\n");
                
                // 初始化内存
                memset(_forecastData, 0, totalSize);
                
                for (int i = 0; i < _forecastCount; i++) {
                    JsonObject cast = casts[i];
                    
                    // 更安全的字符串赋值方式
                    String date = cast["date"] | "";
                    String week = cast["week"] | "";
                    String dayweather = cast["dayweather"] | "";
                    String nightweather = cast["nightweather"] | "";
                    String daytemp = cast["daytemp"] | "";
                    String nighttemp = cast["nighttemp"] | "";
                    String daywind = cast["daywind"] | "";
                    String nightwind = cast["nightwind"] | "";
                    String daypower = cast["daypower"] | "";
                    String nightpower = cast["nightpower"] | "";
                    
                    // 赋值到结构体
                    _forecastData[i].date = date;
                    _forecastData[i].week = week;
                    _forecastData[i].dayweather = dayweather;
                    _forecastData[i].nightweather = nightweather;
                    _forecastData[i].daytemp = daytemp;
                    _forecastData[i].nighttemp = nighttemp;
                    _forecastData[i].daywind = daywind;
                    _forecastData[i].nightwind = nightwind;
                    _forecastData[i].daypower = daypower;
                    _forecastData[i].nightpower = nightpower;
                    
                    printf("[WeatherManager] 解析预报 %d: %s %s\n", 
                           i, _forecastData[i].date.c_str(), _forecastData[i].dayweather.c_str());
                }
            } else {
                printf("❌ [WeatherManager] 预报数据内存分配失败\n");
                _forecastCount = 0;
            }
        } else {
            printf("❌ [WeatherManager] 预报数据条数异常: %d\n", _forecastCount);
            _forecastCount = 0;
        }
    } else {
        printf("❌ [WeatherManager] 响应中无预报数据\n");
    }
    
    unlockWeatherData();
    
    // 清理JSON文档
    doc->~DynamicJsonDocument();  // 显式调用析构函数
    if (usedPSRAMForJSON) {
        _psramManager->deallocate(docBuffer);
        trackMemoryDeallocation(capacity, true);
    } else {
        delete doc;
        trackMemoryDeallocation(capacity, false);
    }
    
    printf("✅ [WeatherManager] 预报数据解析成功，条数: %d\n", _forecastCount);
    printDebugInfo("预报数据解析成功，条数: " + String(_forecastCount));
    
    return true;
}

// 加载配置
bool WeatherManager::loadConfig() {
    if (!_configStorage) {
        printf("[WeatherManager] 配置存储未初始化\n");
        return false;
    }
    
    printf("[WeatherManager] 开始加载天气配置...\n");
    
    WeatherConfig config;
    
    // 加载API密钥
    config.apiKey = _configStorage->getStringAsync(getConfigKey("apiKey"), "");
    printf("[WeatherManager] 加载API密钥: %s\n", config.apiKey.isEmpty() ? "未设置" : "已设置");
    
    // 加载城市代码 - 重要：使用空字符串作为默认值来检测是否真的有保存的配置
    String savedCityCode = _configStorage->getStringAsync(getConfigKey("cityCode"), "");
    if (savedCityCode.isEmpty()) {
        printf("[WeatherManager] 未找到保存的城市代码，需要用户配置\n");
        config.cityCode = "";
    } else {
        config.cityCode = savedCityCode;
        printf("[WeatherManager] 加载城市代码: %s\n", config.cityCode.c_str());
    }
    
    // 加载城市名称
    String savedCityName = _configStorage->getStringAsync(getConfigKey("cityName"), "");
    if (savedCityName.isEmpty()) {
        printf("[WeatherManager] 未找到保存的城市名称，需要用户配置\n");
        config.cityName = "";
    } else {
        config.cityName = savedCityName;
        printf("[WeatherManager] 加载城市名称: %s\n", config.cityName.c_str());
    }
    
    // 加载其他配置
    config.autoUpdate = _configStorage->getBoolAsync(getConfigKey("autoUpdate"), true);
    config.updateInterval = _configStorage->getIntAsync(getConfigKey("updateInterval"), 30);
    config.enableForecast = _configStorage->getBoolAsync(getConfigKey("enableForecast"), false);
    
    printf("[WeatherManager] 加载自动更新: %s\n", config.autoUpdate ? "启用" : "禁用");
    printf("[WeatherManager] 加载更新间隔: %d分钟\n", config.updateInterval);
    printf("[WeatherManager] 加载预报功能: %s\n", config.enableForecast ? "启用" : "禁用");
    
    _config = config;
    
    // 检查是否有有效的配置，通过检查关键配置是否为空来判断
    bool hasValidConfig = !config.apiKey.isEmpty() || !savedCityCode.isEmpty() || !savedCityName.isEmpty();
    
    // 额外验证：检查是否真的有保存的配置文件
    if (hasValidConfig) {
        printf("[WeatherManager] 配置加载成功\n");
        printf("[WeatherManager] 检测到有效配置:\n");
        printf("  - API密钥: %s\n", config.apiKey.isEmpty() ? "未设置" : "已设置");
        printf("  - 城市代码: %s\n", config.cityCode.c_str());
        printf("  - 城市名称: %s\n", config.cityName.c_str());
        printDebugInfo("配置加载成功");
        return true;
    } else {
        printf("[WeatherManager] 未找到有效的配置，需要用户配置\n");
        printf("[WeatherManager] 当前配置状态:\n");
        printf("  - API密钥: 空\n");
        printf("  - 城市代码: 空（需要用户配置）\n");
        printf("  - 城市名称: 空（需要用户配置）\n");
        printDebugInfo("未找到有效配置");
        return false;
    }
}

// 保存配置
bool WeatherManager::saveConfig() {
    if (!_configStorage) {
        printf("[WeatherManager] 配置存储未初始化\n");
        return false;
    }
    
    printf("[WeatherManager] 开始保存天气配置...\n");
    
    bool success = true;
    
    // 分批保存配置，避免队列竞争
    printf("[WeatherManager] 保存API密钥...\n");
    bool result1 = _configStorage->putStringAsync(getConfigKey("apiKey"), _config.apiKey, 10000);
    printf("[WeatherManager] API密钥保存结果: %s\n", result1 ? "成功" : "失败");
    success &= result1;
    
    // 添加延时避免竞争
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[WeatherManager] 保存城市代码...\n");
    bool result2 = _configStorage->putStringAsync(getConfigKey("cityCode"), _config.cityCode, 10000);
    printf("[WeatherManager] 城市代码保存结果: %s\n", result2 ? "成功" : "失败");
    success &= result2;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[WeatherManager] 保存城市名称...\n");
    bool result3 = _configStorage->putStringAsync(getConfigKey("cityName"), _config.cityName, 10000);
    printf("[WeatherManager] 城市名称保存结果: %s\n", result3 ? "成功" : "失败");
    success &= result3;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[WeatherManager] 保存自动更新设置...\n");
    bool result4 = _configStorage->putBoolAsync(getConfigKey("autoUpdate"), _config.autoUpdate, 10000);
    printf("[WeatherManager] 自动更新设置保存结果: %s\n", result4 ? "成功" : "失败");
    success &= result4;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[WeatherManager] 保存更新间隔...\n");
    bool result5 = _configStorage->putIntAsync(getConfigKey("updateInterval"), _config.updateInterval, 10000);
    printf("[WeatherManager] 更新间隔保存结果: %s\n", result5 ? "成功" : "失败");
    success &= result5;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[WeatherManager] 保存预报功能设置...\n");
    bool result6 = _configStorage->putBoolAsync(getConfigKey("enableForecast"), _config.enableForecast, 10000);
    printf("[WeatherManager] 预报功能设置保存结果: %s\n", result6 ? "成功" : "失败");
    success &= result6;
    
    if (success) {
        printf("[WeatherManager] 配置保存成功\n");
        printf("[WeatherManager] 保存的配置内容:\n");
        printf("  - API密钥: %s\n", _config.apiKey.isEmpty() ? "未设置" : "已设置");
        printf("  - 城市代码: %s\n", _config.cityCode.c_str());
        printf("  - 城市名称: %s\n", _config.cityName.c_str());
        printf("  - 自动更新: %s\n", _config.autoUpdate ? "启用" : "禁用");
        printf("  - 更新间隔: %d分钟\n", _config.updateInterval);
        printf("  - 预报功能: %s\n", _config.enableForecast ? "启用" : "禁用");
        printDebugInfo("配置保存成功");
    } else {
        printf("[WeatherManager] 配置保存失败\n");
        printf("[WeatherManager] 保存失败详情:\n");
        printf("  - API密钥保存: %s\n", result1 ? "成功" : "失败");
        printf("  - 城市代码保存: %s\n", result2 ? "成功" : "失败");
        printf("  - 城市名称保存: %s\n", result3 ? "成功" : "失败");
        printf("  - 自动更新保存: %s\n", result4 ? "成功" : "失败");
        printf("  - 更新间隔保存: %s\n", result5 ? "成功" : "失败");
        printf("  - 预报功能保存: %s\n", result6 ? "成功" : "失败");
        printDebugInfo("配置保存失败");
    }
    
    return success;
}

// 获取配置键
String WeatherManager::getConfigKey(const String& key) const {
    // 使用更短的键名以避免NVS限制
    if (key == "apiKey") return "wt_api";
    if (key == "cityCode") return "wt_code";
    if (key == "cityName") return "wt_name";
    if (key == "autoUpdate") return "wt_auto";
    if (key == "updateInterval") return "wt_intv";
    if (key == "enableForecast") return "wt_fcst";
    return "wt_" + key;
}

// 构建天气URL
String WeatherManager::buildWeatherUrl() const {
    // 验证配置有效性
    if (_config.apiKey.isEmpty()) {
        printf("❌ [WeatherManager] API密钥为空\n");
        return "";
    }
    
    if (_config.cityCode.isEmpty()) {
        printf("❌ [WeatherManager] 城市代码为空\n");
        return "";
    }
    
    // 检查API密钥长度（高德API密钥通常是32位）
    if (_config.apiKey.length() < 10 || _config.apiKey.length() > 64) {
        printf("❌ [WeatherManager] API密钥长度异常: %d\n", _config.apiKey.length());
        return "";
    }
    
    // 安全构建URL
    String url;
    url.reserve(256);  // 预分配内存避免多次重分配
    
    url = AMAP_BASE_URL;
    url += "?key=";
    url += _config.apiKey;
    url += "&city=";
    url += _config.cityCode;
    url += "&extensions=base";
    url += "&output=json";
    
    printf("[WeatherManager] 构建天气URL成功，长度: %d\n", url.length());
    return url;
}

// 构建预报URL
String WeatherManager::buildForecastUrl() const {
    // 验证配置有效性
    if (_config.apiKey.isEmpty()) {
        printf("❌ [WeatherManager] API密钥为空\n");
        return "";
    }
    
    if (_config.cityCode.isEmpty()) {
        printf("❌ [WeatherManager] 城市代码为空\n");
        return "";
    }
    
    // 检查API密钥长度
    if (_config.apiKey.length() < 10 || _config.apiKey.length() > 64) {
        printf("❌ [WeatherManager] API密钥长度异常: %d\n", _config.apiKey.length());
        return "";
    }
    
    // 安全构建URL
    String url;
    url.reserve(256);  // 预分配内存避免多次重分配
    
    url = AMAP_BASE_URL;
    url += "?key=";
    url += _config.apiKey;
    url += "&city=";
    url += _config.cityCode;
    url += "&extensions=all";
    url += "&output=json";
    
    printf("[WeatherManager] 构建预报URL成功，长度: %d\n", url.length());
    return url;
}

// 检查WiFi连接
bool WeatherManager::isWiFiConnected() const {
    return _wifiManager && _wifiManager->isConnected();
}

// 更新统计信息
void WeatherManager::updateStatistics(bool success) {
    _statistics.totalRequests++;
    if (success) {
        _statistics.successRequests++;
    } else {
        _statistics.failedRequests++;
    }
}

// 打印调试信息
void WeatherManager::printDebugInfo(const String& message) const {
    if (_debugMode) {
        printf("[WeatherManager] %s\n", message.c_str());
    }
}

// 分配内存
void* WeatherManager::allocateMemory(size_t size) {
    if (_psramManager && _psramManager->isPSRAMAvailable()) {
        return _psramManager->allocate(size);
    }
    return malloc(size);
}

// 释放内存
void WeatherManager::freeMemory(void* ptr) {
    if (!ptr) return;
    
    if (_psramManager && _psramManager->isPSRAMAvailable()) {
        _psramManager->deallocate(ptr);
    } else {
        free(ptr);
    }
}

// 清理预报数据
void WeatherManager::cleanupForecastData() {
    if (_forecastData) {
        size_t dataSize = _statistics.forecastDataSize;
        
        // 检查数据的完整性
        if (dataSize > 0) {
            // 注意：无法确定内存是在PSRAM还是内部RAM中分配的
            // 统一使用free释放，因为预报数据可能在两种类型的内存中
            // TODO: 需要改进内存分配策略，记录分配类型
            if (_psramManager && _psramManager->isPSRAMAvailable()) {
                // 尝试PSRAM释放
                _psramManager->deallocate(_forecastData);
                trackMemoryDeallocation(dataSize, true);
            } else {
                // 使用标准free释放
                free(_forecastData);
                trackMemoryDeallocation(dataSize, false);
            }
        }
        
        _forecastData = nullptr;
        _statistics.forecastDataSize = 0;
    }
    _forecastCount = 0;
}

// 创建天气任务
bool WeatherManager::createWeatherTask() {
    size_t stackSize = 4096;  // 8KB栈大小
    
    // 对于网络相关任务，必须使用内部RAM栈，避免TCP ISN钩子断言失败
    // 不使用PSRAM栈，因为lwip要求网络操作在内部RAM中进行
    printf("[WeatherManager] 创建天气任务(使用内部RAM栈，固定到核心0)...\n");
    
    BaseType_t result = xTaskCreatePinnedToCore(
        weatherUpdateTask,
        "WeatherTask",
        stackSize / sizeof(StackType_t),
        this,
        2,  // 优先级
        &_weatherTaskHandle,
        0   // 核心0
    );
    
    if (result != pdPASS) {
        printf("❌ [WeatherManager] 创建天气任务失败\n");
        return false;
    }
    
    printf("✅ [WeatherManager] 天气任务创建成功(内部RAM栈，固定到核心0)\n");
    return true;
}

// 删除天气任务
void WeatherManager::deleteWeatherTask() {
    if (_weatherTaskHandle) {
        vTaskDelete(_weatherTaskHandle);
        _weatherTaskHandle = nullptr;
        printDebugInfo("天气任务已删除");
    }
}

// 锁定天气数据
bool WeatherManager::lockWeatherData(unsigned long timeout) {
    if (!_weatherMutex) {
        return false;
    }
    
    return xSemaphoreTake(_weatherMutex, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

// 解锁天气数据
void WeatherManager::unlockWeatherData() {
    if (_weatherMutex) {
        xSemaphoreGive(_weatherMutex);
    }
}

// 验证配置有效性
bool WeatherManager::validateConfig(const WeatherConfig& config) {
    // 检查城市代码和城市名称是否都提供
    if (config.cityCode.isEmpty() && config.cityName.isEmpty()) {
        printf("[WeatherManager] 城市代码和城市名称都为空，跳过验证\n");
        return true;  // 允许空配置，用户可以后续设置
    }
    
    // 如果只有一个为空，则需要验证
    if (config.cityCode.isEmpty() && !config.cityName.isEmpty()) {
        printf("[WeatherManager] 警告：城市名称已设置但城市代码为空\n");
        return false;
    }
    
    if (!config.cityCode.isEmpty() && config.cityName.isEmpty()) {
        printf("[WeatherManager] 警告：城市代码已设置但城市名称为空\n");
        return false;
    }
    
    // 检查城市代码格式（应该是6位数字）
    if (!config.cityCode.isEmpty()) {
        if (config.cityCode.length() != 6) {
            printf("[WeatherManager] 错误：城市代码长度应为6位，当前为%d位\n", config.cityCode.length());
            return false;
        }
        
        for (int i = 0; i < config.cityCode.length(); i++) {
            if (!isdigit(config.cityCode[i])) {
                printf("[WeatherManager] 错误：城市代码应为数字，发现非数字字符\n");
                return false;
            }
        }
    }
    
    // 检查更新间隔
    if (config.updateInterval < 5 || config.updateInterval > 1440) {
        printf("[WeatherManager] 错误：更新间隔应在5-1440分钟之间\n");
        return false;
    }
    
    printf("[WeatherManager] 配置验证通过\n");
    return true;
}

// 更新内存统计
void WeatherManager::updateMemoryStatistics(size_t allocated, bool isPSRAM) {
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

// 跟踪内存分配
void WeatherManager::trackMemoryAllocation(size_t size, bool isPSRAM) {
    if (!_memoryStatsEnabled) return;
    
    _statistics.memoryAllocations++;
    updateMemoryStatistics(size, isPSRAM);
    
    if (_debugMode) {
        printf("[WeatherManager] 内存分配: %u字节 (%s), 当前使用: %u字节\n", 
               size, isPSRAM ? "PSRAM" : "内部RAM", _currentMemoryUsage);
    }
}

// 跟踪内存释放
void WeatherManager::trackMemoryDeallocation(size_t size, bool isPSRAM) {
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
        printf("[WeatherManager] 内存释放: %u字节 (%s), 当前使用: %u字节\n", 
               size, isPSRAM ? "PSRAM" : "内部RAM", _currentMemoryUsage);
    }
}

// 打印内存使用情况
void WeatherManager::printMemoryUsage() const {
    printf("\n=== 天气管理器内存使用情况 ===\n");
    printf("当前内存使用: %u字节\n", _currentMemoryUsage);
    printf("峰值内存使用: %u字节\n", _peakMemoryUsage);
    printf("总内存使用: %u字节\n", _statistics.totalMemoryUsed);
    printf("PSRAM使用: %u字节\n", _statistics.psramMemoryUsed);
    printf("内部RAM使用: %u字节\n", _statistics.internalMemoryUsed);
    printf("预报数据大小: %u字节\n", _statistics.forecastDataSize);
    printf("JSON缓冲区大小: %u字节\n", _statistics.jsonBufferSize);
    printf("HTTP缓冲区大小: %u字节\n", _statistics.httpBufferSize);
    printf("内存分配次数: %lu\n", _statistics.memoryAllocations);
    printf("内存释放次数: %lu\n", _statistics.memoryDeallocations);
    printf("内存统计: %s\n", _memoryStatsEnabled ? "启用" : "禁用");
    printf("===============================\n");
}

// 获取内存使用JSON
String WeatherManager::getMemoryUsageJSON() const {
    String json = "{";
    json += "\"currentMemoryUsage\":" + String(_currentMemoryUsage) + ",";
    json += "\"peakMemoryUsage\":" + String(_peakMemoryUsage) + ",";
    json += "\"totalMemoryUsed\":" + String(_statistics.totalMemoryUsed) + ",";
    json += "\"psramMemoryUsed\":" + String(_statistics.psramMemoryUsed) + ",";
    json += "\"internalMemoryUsed\":" + String(_statistics.internalMemoryUsed) + ",";
    json += "\"forecastDataSize\":" + String(_statistics.forecastDataSize) + ",";
    json += "\"jsonBufferSize\":" + String(_statistics.jsonBufferSize) + ",";
    json += "\"httpBufferSize\":" + String(_statistics.httpBufferSize) + ",";
    json += "\"memoryAllocations\":" + String(_statistics.memoryAllocations) + ",";
    json += "\"memoryDeallocations\":" + String(_statistics.memoryDeallocations) + ",";
    json += "\"memoryStatsEnabled\":" + String(_memoryStatsEnabled ? "true" : "false");
    json += "}";
    return json;
}

// 重置内存统计
void WeatherManager::resetMemoryStatistics() {
    _statistics.totalMemoryUsed = 0;
    _statistics.psramMemoryUsed = 0;
    _statistics.internalMemoryUsed = 0;
    _statistics.forecastDataSize = 0;
    _statistics.jsonBufferSize = 0;
    _statistics.httpBufferSize = 0;
    _statistics.memoryAllocations = 0;
    _statistics.memoryDeallocations = 0;
    _currentMemoryUsage = 0;
    _peakMemoryUsage = 0;
    
    printf("[WeatherManager] 内存统计已重置\n");
} 
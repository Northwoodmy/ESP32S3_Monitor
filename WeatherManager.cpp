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
    .cityCode = "110000",                       // 默认北京
    .cityName = "北京",
    .autoUpdate = true,
    .updateInterval = 30,                       // 30分钟
    .enableForecast = true
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
    
    // 初始化统计信息
    _statistics = {0, 0, 0, 0, 0};
    
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
    
    _config = config;
    bool success = saveConfig();
    
    unlockWeatherData();
    
    if (success) {
        printf("✅ 天气配置更新成功\n");
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
    
    // 获取响应内容
    response = http.getString();
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
    
    // 使用固定大小的JSON文档，避免栈溢出
    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(5) + 1024;
    StaticJsonDocument<capacity> doc;
    
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        printf("❌ [WeatherManager] JSON解析失败: %s\n", error.c_str());
        return false;
    }
    
    // 检查响应状态
    const char* status = doc["status"];
    if (!status || strcmp(status, "1") != 0) {
        const char* info = doc["info"];
        printf("❌ [WeatherManager] API响应错误: %s\n", info ? info : "未知错误");
        return false;
    }
    
    // 解析天气数据
    if (!lockWeatherData()) {
        printf("❌ [WeatherManager] 获取天气数据锁失败\n");
        return false;
    }
    
    JsonArray lives = doc["lives"];
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
        return false;
    }
    
    unlockWeatherData();
    
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
    
    // 使用固定大小的JSON文档，避免栈溢出
    const size_t capacity = JSON_OBJECT_SIZE(20) + JSON_ARRAY_SIZE(10) + 2048;
    StaticJsonDocument<capacity> doc;
    
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        printf("❌ [WeatherManager] 预报JSON解析失败: %s\n", error.c_str());
        return false;
    }
    
    // 检查响应状态
    const char* status = doc["status"];
    if (!status || strcmp(status, "1") != 0) {
        const char* info = doc["info"];
        printf("❌ [WeatherManager] 预报API响应错误: %s\n", info ? info : "未知错误");
        return false;
    }
    
    // 解析预报数据
    if (!lockWeatherData()) {
        printf("❌ [WeatherManager] 获取预报数据锁失败\n");
        return false;
    }
    
    // 清理旧数据
    cleanupForecastData();
    
    JsonArray forecasts = doc["forecasts"];
    if (forecasts.size() > 0) {
        JsonObject forecast = forecasts[0];
        JsonArray casts = forecast["casts"];
        
        _forecastCount = casts.size();
        printf("[WeatherManager] 预报数据条数: %d\n", _forecastCount);
        
        if (_forecastCount > 0 && _forecastCount <= 7) {  // 限制最大7天预报
            // 使用标准内存分配器而不是PSRAM
            _forecastData = (WeatherForecast*)malloc(sizeof(WeatherForecast) * _forecastCount);
            if (_forecastData) {
                printf("[WeatherManager] 预报数据内存分配成功\n");
                
                // 初始化内存
                memset(_forecastData, 0, sizeof(WeatherForecast) * _forecastCount);
                
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
                unlockWeatherData();
                return false;
            }
        } else {
            printf("❌ [WeatherManager] 预报数据条数异常: %d\n", _forecastCount);
            _forecastCount = 0;
        }
    } else {
        printf("❌ [WeatherManager] 响应中无预报数据\n");
    }
    
    unlockWeatherData();
    
    printf("✅ [WeatherManager] 预报数据解析成功，条数: %d\n", _forecastCount);
    printDebugInfo("预报数据解析成功，条数: " + String(_forecastCount));
    
    return true;
}

// 加载配置
bool WeatherManager::loadConfig() {
    if (!_configStorage) {
        return false;
    }
    
    WeatherConfig config;
    
    config.apiKey = _configStorage->getStringAsync(getConfigKey("apiKey"), "");
    config.cityCode = _configStorage->getStringAsync(getConfigKey("cityCode"), "110000");
    config.cityName = _configStorage->getStringAsync(getConfigKey("cityName"), "北京");
    config.autoUpdate = _configStorage->getBoolAsync(getConfigKey("autoUpdate"), true);
    config.updateInterval = _configStorage->getIntAsync(getConfigKey("updateInterval"), 30);
    config.enableForecast = _configStorage->getBoolAsync(getConfigKey("enableForecast"), true);
    
    _config = config;
    
    printDebugInfo("配置加载成功");
    return true;
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
        printDebugInfo("配置保存成功");
    } else {
        printf("[WeatherManager] 配置保存失败\n");
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
        free(_forecastData);  // 使用标准free释放malloc分配的内存
        _forecastData = nullptr;
    }
    _forecastCount = 0;
}

// 创建天气任务
bool WeatherManager::createWeatherTask() {
    size_t stackSize = 8192;  // 8KB栈大小
    
    // 对于网络相关任务，必须使用内部RAM栈，避免TCP ISN钩子断言失败
    // 不使用PSRAM栈，因为lwip要求网络操作在内部RAM中进行
    printf("[WeatherManager] 创建天气任务(使用内部RAM栈)...\n");
    
    BaseType_t result = xTaskCreate(
        weatherUpdateTask,
        "WeatherTask",
        stackSize / sizeof(StackType_t),
        this,
        2,  // 优先级
        &_weatherTaskHandle
    );
    
    if (result != pdPASS) {
        printf("❌ [WeatherManager] 创建天气任务失败\n");
        return false;
    }
    
    printf("✅ [WeatherManager] 天气任务创建成功(内部RAM栈)\n");
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
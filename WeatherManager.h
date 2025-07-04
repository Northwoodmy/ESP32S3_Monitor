/*
 * WeatherManager.h - 天气管理器模块
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

#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "PSRAMManager.h"
#include "WiFiManager.h"
#include "ConfigStorage.h"

// 高德地图天气API配置
#define AMAP_BASE_URL "http://restapi.amap.com/v3/weather/weatherInfo"
#define WEATHER_UPDATE_INTERVAL 30 * 60 * 1000  // 30分钟更新一次
#define WEATHER_RETRY_INTERVAL 5 * 60 * 1000    // 失败后5分钟重试
#define WEATHER_TIMEOUT 10000                    // HTTP请求超时时间（10秒）

// 天气数据结构
typedef struct {
    String city;                    // 城市名称
    String adcode;                  // 城市编码
    String weather;                 // 天气现象
    String temperature;             // 实时温度
    String temperature_float;       // 实时温度浮点值
    String humidity;                // 相对湿度
    String winddirection;           // 风向
    String windpower;               // 风力等级
    String reporttime;              // 数据发布时间
    bool isValid;                   // 数据有效性
} WeatherData;

// 天气预报数据结构
typedef struct {
    String date;                    // 日期
    String week;                    // 星期
    String dayweather;              // 白天天气现象
    String nightweather;            // 夜间天气现象
    String daytemp;                 // 白天温度
    String nighttemp;               // 夜间温度
    String daywind;                 // 白天风向
    String nightwind;               // 夜间风向
    String daypower;                // 白天风力
    String nightpower;              // 夜间风力
} WeatherForecast;

// 天气管理器状态
typedef enum {
    WEATHER_STATE_INIT = 0,         // 初始化状态
    WEATHER_STATE_READY,            // 准备就绪
    WEATHER_STATE_UPDATING,         // 正在更新
    WEATHER_STATE_SUCCESS,          // 更新成功
    WEATHER_STATE_FAILED,           // 更新失败
    WEATHER_STATE_NO_WIFI,          // 无WiFi连接
    WEATHER_STATE_ERROR             // 错误状态
} WeatherState;

// 天气统计信息
typedef struct {
    unsigned long totalRequests;    // 总请求次数
    unsigned long successRequests;  // 成功请求次数
    unsigned long failedRequests;   // 失败请求次数
    unsigned long lastUpdateTime;   // 上次更新时间
    unsigned long nextUpdateTime;   // 下次更新时间
} WeatherStatistics;

// 天气配置信息
typedef struct {
    String apiKey;                  // 高德API密钥
    String cityCode;                // 城市代码
    String cityName;                // 城市名称
    bool autoUpdate;                // 自动更新开关
    int updateInterval;             // 更新间隔（分钟）
    bool enableForecast;            // 启用预报功能
} WeatherConfig;

class WeatherManager {
public:
    WeatherManager();
    ~WeatherManager();
    
    // 初始化和控制
    bool init(PSRAMManager* psramManager, WiFiManager* wifiManager, ConfigStorage* configStorage);
    bool start();
    void stop();
    void restart();
    
    // 配置管理
    bool setConfig(const WeatherConfig& config);
    WeatherConfig getConfig() const;
    bool setApiKey(const String& apiKey);
    bool setCityCode(const String& cityCode);
    bool setAutoUpdate(bool enable);
    bool setUpdateInterval(int minutes);
    
    // 天气数据操作
    bool updateWeatherData();
    bool updateForecastData();
    WeatherData getCurrentWeather() const;
    WeatherForecast* getForecastData(int& count) const;
    bool isWeatherDataValid() const;
    
    // 状态和统计
    WeatherState getState() const;
    WeatherStatistics getStatistics() const;
    String getStateString() const;
    unsigned long getLastUpdateTime() const;
    unsigned long getNextUpdateTime() const;
    
    // 调试和监控
    void setDebugMode(bool enabled);
    bool isDebugMode() const;
    void printCurrentWeather() const;
    void printForecastData() const;
    void printStatistics() const;
    void printConfig() const;
    
    // 静态任务函数
    static void weatherUpdateTask(void* parameter);
    
private:
    // 私有成员变量
    PSRAMManager* _psramManager;
    WiFiManager* _wifiManager;
    ConfigStorage* _configStorage;
    
    TaskHandle_t _weatherTaskHandle;
    StaticTask_t _weatherTaskBuffer;
    SemaphoreHandle_t _weatherMutex;
    
    WeatherData _currentWeather;
    WeatherForecast* _forecastData;
    int _forecastCount;
    WeatherState _state;
    WeatherStatistics _statistics;
    WeatherConfig _config;
    
    bool _isInitialized;
    bool _isRunning;
    bool _debugMode;
    
    // 私有方法
    void weatherUpdateLoop();
    bool performWeatherUpdate();
    bool performForecastUpdate();
    
    // HTTP请求相关
    bool makeHttpRequest(const String& url, String& response);
    bool parseWeatherResponse(const String& response);
    bool parseForecastResponse(const String& response);
    
    // 配置管理
    bool loadConfig();
    bool saveConfig();
    String getConfigKey(const String& key) const;
    
    // 工具方法
    String buildWeatherUrl() const;
    String buildForecastUrl() const;
    bool isWiFiConnected() const;
    void updateStatistics(bool success);
    void printDebugInfo(const String& message) const;
    
    // 内存管理
    void* allocateMemory(size_t size);
    void freeMemory(void* ptr);
    void cleanupForecastData();
    
    // 任务控制
    bool createWeatherTask();
    void deleteWeatherTask();
    
    // 互斥锁操作
    bool lockWeatherData(unsigned long timeout = 1000);
    void unlockWeatherData();
    
    // 配置验证
    bool validateConfig(const WeatherConfig& config);
};

#endif // WEATHER_MANAGER_H 
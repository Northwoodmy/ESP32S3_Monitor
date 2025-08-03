/*
 * LocationManager.h - 定位管理器模块
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

#ifndef LOCATION_MANAGER_H
#define LOCATION_MANAGER_H

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

// 高德地图定位API配置
#define AMAP_LOCATION_BASE_URL "http://restapi.amap.com/v3/ip"
#define AMAP_GEOCODE_BASE_URL "http://restapi.amap.com/v3/geocode/geo"
#define LOCATION_TIMEOUT 15000                    // HTTP请求超时时间（15秒）
#define LOCATION_RETRY_TIMES 3                    // 重试次数

// 定位类型
typedef enum {
    LOCATION_TYPE_IP = 0,           // IP定位
    LOCATION_TYPE_GPS,              // GPS定位（预留）
    LOCATION_TYPE_MANUAL            // 手动设置
} LocationType;

// 定位状态
typedef enum {
    LOCATION_STATE_INIT = 0,        // 初始化状态
    LOCATION_STATE_READY,           // 准备就绪
    LOCATION_STATE_LOCATING,        // 正在定位
    LOCATION_STATE_SUCCESS,         // 定位成功
    LOCATION_STATE_FAILED,          // 定位失败
    LOCATION_STATE_NO_WIFI,         // 无WiFi连接
    LOCATION_STATE_ERROR            // 错误状态
} LocationState;

// 定位数据结构
typedef struct {
    String province;                // 省份
    String city;                    // 城市
    String district;                // 区县
    String adcode;                  // 区域编码
    String citycode;                // 城市编码
    String rectangle;               // 所在矩形区域范围
    LocationType type;              // 定位类型
    unsigned long timestamp;        // 定位时间戳
    bool isValid;                   // 数据有效性
} LocationData;

// 定位配置
typedef struct {
    String apiKey;                  // 高德API密钥
    LocationType preferredType;     // 首选定位类型
    bool autoLocate;                // 自动定位开关
    int retryTimes;                 // 重试次数
    int timeout;                    // 超时时间
} LocationConfig;

// 定位统计信息
typedef struct {
    unsigned long totalRequests;        // 总请求次数
    unsigned long successRequests;      // 成功请求次数
    unsigned long failedRequests;       // 失败请求次数
    unsigned long lastLocationTime;     // 上次定位时间
    unsigned long locationDuration;     // 定位持续时间
    // 内存使用统计
    size_t totalMemoryUsed;             // 总内存使用量
    size_t psramMemoryUsed;             // PSRAM内存使用量
    size_t internalMemoryUsed;          // 内部RAM使用量
    size_t jsonBufferSize;              // JSON缓冲区大小
    size_t httpBufferSize;              // HTTP缓冲区大小
    unsigned long memoryAllocations;    // 内存分配次数
    unsigned long memoryDeallocations;  // 内存释放次数
} LocationStatistics;

class LocationManager {
public:
    LocationManager();
    ~LocationManager();
    
    // 初始化和控制
    bool init(PSRAMManager* psramManager, WiFiManager* wifiManager, ConfigStorage* configStorage);
    bool start();
    void stop();
    void restart();
    
    // 配置管理
    bool setConfig(const LocationConfig& config);
    LocationConfig getConfig() const;
    bool setApiKey(const String& apiKey);
    bool setAutoLocate(bool enable);
    bool setPreferredType(LocationType type);
    
    // 定位操作
    bool locateCurrentPosition();
    bool locateByIP();
    LocationData getCurrentLocation() const;
    bool isLocationDataValid() const;
    
    // 状态和统计
    LocationState getState() const;
    LocationStatistics getStatistics() const;
    String getStateString() const;
    unsigned long getLastLocationTime() const;
    
    // 辅助功能
    String getLocationString() const;           // 获取位置描述字符串
    String getCityCodeForWeather() const;       // 获取适用于天气查询的城市代码
    bool isLocationAvailable() const;           // 检查是否有可用的定位数据
    
    // 调试和监控
    void setDebugMode(bool enabled);
    bool isDebugMode() const;
    void printCurrentLocation() const;
    void printStatistics() const;
    void printConfig() const;
    void printMemoryUsage() const;
    String getMemoryUsageJSON() const;
    void resetMemoryStatistics();
    
    // 静态任务函数
    static void locationTask(void* parameter);
    
private:
    // 私有成员变量
    PSRAMManager* _psramManager;
    WiFiManager* _wifiManager;
    ConfigStorage* _configStorage;
    
    TaskHandle_t _locationTaskHandle;
    SemaphoreHandle_t _locationMutex;
    
    LocationData _currentLocation;
    LocationState _state;
    LocationStatistics _statistics;
    LocationConfig _config;
    
    bool _isInitialized;
    bool _isRunning;
    bool _debugMode;
    bool _memoryStatsEnabled;
    
    // 内存使用监控
    size_t _currentMemoryUsage;
    size_t _peakMemoryUsage;
    
    // 私有方法
    void locationLoop();
    bool performIPLocation();
    bool performGPSLocation();  // 预留GPS定位功能
    
    // HTTP请求相关
    bool makeHttpRequest(const String& url, String& response);
    bool parseLocationResponse(const String& response);
    bool parseGeocodeResponse(const String& response);
    
    // 配置管理
    bool loadConfig();
    bool saveConfig();
    String getConfigKey(const String& key) const;
    
    // 工具方法
    String buildLocationUrl() const;
    String buildGeocodeUrl(const String& address) const;
    bool isWiFiConnected() const;
    void updateStatistics(bool success);
    void printDebugInfo(const String& message) const;
    
    // 内存管理
    void trackMemoryAllocation(size_t size, bool isPSRAM);
    void trackMemoryDeallocation(size_t size, bool isPSRAM);
    void updateMemoryStatistics(size_t allocated, bool isPSRAM);
    
    // 任务控制
    bool createLocationTask();
    void deleteLocationTask();
    
    // 互斥锁操作
    bool lockLocationData(unsigned long timeout = 1000);
    void unlockLocationData();
    
    // 配置验证
    bool validateConfig(const LocationConfig& config);
    
    // 地区代码转换
    String convertAdcodeToWeatherCode(const String& adcode) const;
    bool isValidAdcode(const String& adcode) const;
};

#endif // LOCATION_MANAGER_H
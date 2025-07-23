/*
 * ConfigStorage.h - NVS配置存储任务管理器头文件
 * ESP32S3监控项目 - 配置存储模块
 * 基于FreeRTOS任务实现，确保NVS操作的线程安全性
 */

#ifndef CONFIGSTORAGE_H
#define CONFIGSTORAGE_H

#include <Preferences.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// WiFi配置结构体
struct WiFiConfig {
    String ssid;
    String password;
    int priority;  // 优先级，1最高，数字越小优先级越高
    bool isValid;
    
    WiFiConfig() : ssid(""), password(""), priority(99), isValid(false) {}
    WiFiConfig(const String& s, const String& p, int prio = 99) : ssid(s), password(p), priority(prio), isValid(true) {}
};

// 屏幕模式枚举
enum ScreenMode {
    SCREEN_MODE_ALWAYS_ON,    // 亮屏：始终点亮
    SCREEN_MODE_SCHEDULED,    // 定时：指定时间段点亮
    SCREEN_MODE_TIMEOUT,      // 延时：无操作后延时熄屏
    SCREEN_MODE_ALWAYS_OFF    // 熄屏：始终关闭
};

// 配置操作类型枚举
enum ConfigOperation {
    CONFIG_OP_SAVE_WIFI,
    CONFIG_OP_LOAD_WIFI,
    CONFIG_OP_HAS_WIFI,
    CONFIG_OP_CLEAR_WIFI,
    CONFIG_OP_SAVE_MULTI_WIFI,
    CONFIG_OP_LOAD_MULTI_WIFI,
    CONFIG_OP_ADD_WIFI,
    CONFIG_OP_GET_WIFI_COUNT,
    CONFIG_OP_CLEAR_ALL_WIFI,
    CONFIG_OP_UPDATE_WIFI_PRIORITY,
    CONFIG_OP_SET_WIFI_PRIORITIES,
    CONFIG_OP_SAVE_SYSTEM,
    CONFIG_OP_LOAD_SYSTEM,
    CONFIG_OP_SAVE_BRIGHTNESS,
    CONFIG_OP_LOAD_BRIGHTNESS,
    CONFIG_OP_HAS_BRIGHTNESS,
    CONFIG_OP_SAVE_THEME_CONFIG,
    CONFIG_OP_LOAD_THEME_CONFIG,
    CONFIG_OP_HAS_THEME_CONFIG,
    CONFIG_OP_SAVE_TIME_CONFIG,
    CONFIG_OP_LOAD_TIME_CONFIG,
    CONFIG_OP_SAVE_SCREEN_CONFIG,
    CONFIG_OP_LOAD_SCREEN_CONFIG,
    CONFIG_OP_HAS_SCREEN_CONFIG,
    CONFIG_OP_SAVE_SERVER_CONFIG,
    CONFIG_OP_LOAD_SERVER_CONFIG,
    CONFIG_OP_HAS_SERVER_CONFIG,
    CONFIG_OP_RESET_ALL,
    CONFIG_OP_PUT_STRING,
    CONFIG_OP_GET_STRING,
    CONFIG_OP_PUT_BOOL,
    CONFIG_OP_GET_BOOL,
    CONFIG_OP_PUT_INT,
    CONFIG_OP_GET_INT
};

// 配置请求消息结构体
struct ConfigRequest {
    ConfigOperation operation;
    void* data;           // 指向请求数据的指针
    void* result;         // 指向结果数据的指针
    SemaphoreHandle_t responseSemaphore;  // 用于同步等待响应
    bool success;         // 操作结果标志
    
    ConfigRequest() : operation(CONFIG_OP_SAVE_WIFI), data(nullptr), result(nullptr), 
                     responseSemaphore(nullptr), success(false) {}
};

// WiFi配置请求数据结构
struct WiFiConfigData {
    String ssid;
    String password;
    
    WiFiConfigData() : ssid(""), password("") {}
    WiFiConfigData(const String& s, const String& p) : ssid(s), password(p) {}
};

// 多WiFi配置请求数据结构
struct MultiWiFiConfigData {
    WiFiConfig configs[3];
    
    MultiWiFiConfigData() {
        for (int i = 0; i < 3; i++) {
            configs[i] = WiFiConfig();
        }
    }
};

// 系统配置请求数据结构
struct SystemConfigData {
    String deviceName;
    int refreshRate;
    
    SystemConfigData() : deviceName("ESP32S3-Monitor"), refreshRate(1000) {}
    SystemConfigData(const String& name, int rate) : deviceName(name), refreshRate(rate) {}
};

// 优先级配置请求数据结构
struct PriorityConfigData {
    int index;
    int priority;
    int priorities[3];
    
    PriorityConfigData() : index(0), priority(0) {
        for (int i = 0; i < 3; i++) {
            priorities[i] = 0;
        }
    }
};

// 亮度配置请求数据结构
struct BrightnessConfigData {
    uint8_t brightness;
    
    BrightnessConfigData() : brightness(80) {}
    BrightnessConfigData(uint8_t b) : brightness(b) {}
};

// 主题配置请求数据结构
struct ThemeConfigData {
    int theme;  // 使用int类型避免头文件循环依赖
    
    ThemeConfigData() : theme(0) {}  // 默认为UI1主题
    ThemeConfigData(int t) : theme(t) {}
};

// 时间配置请求数据结构
struct TimeConfigData {
    String primaryServer;
    String secondaryServer;
    String timezone;
    int syncInterval;
    
    TimeConfigData() : primaryServer("pool.ntp.org"), secondaryServer("time.nist.gov"), 
                       timezone("CST-8"), syncInterval(60) {}
    TimeConfigData(const String& primary, const String& secondary, const String& tz, int interval) 
        : primaryServer(primary), secondaryServer(secondary), timezone(tz), syncInterval(interval) {}
};

// 屏幕设置配置请求数据结构
struct ScreenConfigData {
    ScreenMode mode;           // 屏幕模式
    int startHour;            // 定时模式开始时间（小时）
    int startMinute;          // 定时模式开始时间（分钟）
    int endHour;              // 定时模式结束时间（小时）
    int endMinute;            // 定时模式结束时间（分钟）
    int timeoutMinutes;       // 延时模式延时时间（分钟）
    
    ScreenConfigData() : mode(SCREEN_MODE_ALWAYS_ON), startHour(8), startMinute(0), 
                        endHour(22), endMinute(0), timeoutMinutes(10) {}
    ScreenConfigData(ScreenMode m, int sh, int sm, int eh, int em, int timeout) 
        : mode(m), startHour(sh), startMinute(sm), endHour(eh), endMinute(em), 
          timeoutMinutes(timeout) {}
};

// 服务器配置请求数据结构
struct ServerConfigData {
    String serverUrl;               // 监控服务器地址
    int requestInterval;            // 请求间隔（毫秒）
    bool enabled;                   // 是否启用监控
    int connectionTimeout;          // 连接超时时间（毫秒）
    bool autoGetData;               // 自动获取数据开关
    bool autoScanServer;            // 自动扫描服务器开关
    
        ServerConfigData() : serverUrl("http://10.10.168.168/metrics.json"), requestInterval(250),
                         enabled(true), connectionTimeout(1000), autoGetData(true), autoScanServer(false) {}
    ServerConfigData(const String& url, int interval, bool en, int timeout, bool autoGet, bool autoScan) 
        : serverUrl(url), requestInterval(interval), enabled(en), connectionTimeout(timeout), 
          autoGetData(autoGet), autoScanServer(autoScan) {}
};

// 通用配置请求数据结构
struct GenericConfigData {
    String key;
    String stringValue;
    bool boolValue;
    int intValue;
    String defaultStringValue;
    bool defaultBoolValue;
    int defaultIntValue;
    
    GenericConfigData() : key(""), stringValue(""), boolValue(false), intValue(0), 
                         defaultStringValue(""), defaultBoolValue(false), defaultIntValue(0) {}
    GenericConfigData(const String& k, const String& v, const String& def = "") 
        : key(k), stringValue(v), boolValue(false), intValue(0), 
          defaultStringValue(def), defaultBoolValue(false), defaultIntValue(0) {}
    GenericConfigData(const String& k, bool v, bool def = false) 
        : key(k), stringValue(""), boolValue(v), intValue(0), 
          defaultStringValue(""), defaultBoolValue(def), defaultIntValue(0) {}
    GenericConfigData(const String& k, int v, int def = 0) 
        : key(k), stringValue(""), boolValue(false), intValue(v), 
          defaultStringValue(""), defaultBoolValue(false), defaultIntValue(def) {}
};

class ConfigStorage {
public:
    ConfigStorage();
    ~ConfigStorage();
    
    // 任务管理方法
    bool init();
    bool startTask();
    void stopTask();
    
    // 异步WiFi配置操作接口
    bool saveWiFiConfigAsync(const String& ssid, const String& password, uint32_t timeoutMs = 5000);
    bool loadWiFiConfigAsync(String& ssid, String& password, uint32_t timeoutMs = 5000);
    bool hasWiFiConfigAsync(uint32_t timeoutMs = 5000);
    void clearWiFiConfigAsync(uint32_t timeoutMs = 5000);
    
    // 异步多WiFi配置操作接口
    bool saveWiFiConfigsAsync(const WiFiConfig configs[3], uint32_t timeoutMs = 5000);
    bool loadWiFiConfigsAsync(WiFiConfig configs[3], uint32_t timeoutMs = 5000);
    bool addWiFiConfigAsync(const String& ssid, const String& password, uint32_t timeoutMs = 5000);
    int getWiFiConfigCountAsync(uint32_t timeoutMs = 5000);
    void clearAllWiFiConfigsAsync(uint32_t timeoutMs = 5000);
    
    // 异步优先级管理接口
    bool updateWiFiPriorityAsync(int index, int priority, uint32_t timeoutMs = 5000);
    bool setWiFiPrioritiesAsync(const int priorities[3], uint32_t timeoutMs = 5000);
    
    // 异步系统配置操作接口
    bool saveSystemConfigAsync(const String& deviceName, int refreshRate, uint32_t timeoutMs = 5000);
    bool loadSystemConfigAsync(String& deviceName, int& refreshRate, uint32_t timeoutMs = 5000);
    
    // 异步屏幕亮度配置操作接口
    bool saveBrightnessAsync(uint8_t brightness, uint32_t timeoutMs = 5000);
    uint8_t loadBrightnessAsync(uint32_t timeoutMs = 5000);
    bool hasBrightnessConfigAsync(uint32_t timeoutMs = 5000);
    
    // 异步主题配置操作接口
    bool saveThemeConfigAsync(int theme, uint32_t timeoutMs = 5000);
    int loadThemeConfigAsync(uint32_t timeoutMs = 5000);
    bool hasThemeConfigAsync(uint32_t timeoutMs = 5000);
    
    // 异步时间配置操作接口
    bool saveTimeConfigAsync(const String& primaryServer, const String& secondaryServer, 
                            const String& timezone, int syncInterval, uint32_t timeoutMs = 5000);
    bool loadTimeConfigAsync(String& primaryServer, String& secondaryServer, 
                            String& timezone, int& syncInterval, uint32_t timeoutMs = 5000);
    
    // 异步屏幕设置配置操作接口
    bool saveScreenConfigAsync(ScreenMode mode, int startHour, int startMinute, 
                              int endHour, int endMinute, int timeoutMinutes, uint32_t timeoutMs = 5000);
    bool loadScreenConfigAsync(ScreenMode& mode, int& startHour, int& startMinute, 
                              int& endHour, int& endMinute, int& timeoutMinutes, uint32_t timeoutMs = 5000);
    bool hasScreenConfigAsync(uint32_t timeoutMs = 5000);
    
    // 异步服务器配置操作接口
    bool saveServerConfigAsync(const String& serverUrl, int requestInterval, 
                              bool enabled, int connectionTimeout, bool autoGetData, bool autoScanServer, uint32_t timeoutMs = 5000);
    bool loadServerConfigAsync(String& serverUrl, int& requestInterval, 
                              bool& enabled, int& connectionTimeout, bool& autoGetData, bool& autoScanServer, uint32_t timeoutMs = 5000);
    bool hasServerConfigAsync(uint32_t timeoutMs = 5000);
    
    // 异步配置重置操作接口
    bool resetAllConfigAsync(uint32_t timeoutMs = 5000);
    
    // 异步通用配置操作接口
    bool putStringAsync(const String& key, const String& value, uint32_t timeoutMs = 5000);
    String getStringAsync(const String& key, const String& defaultValue = "", uint32_t timeoutMs = 5000);
    bool putBoolAsync(const String& key, bool value, uint32_t timeoutMs = 5000);
    bool getBoolAsync(const String& key, bool defaultValue = false, uint32_t timeoutMs = 5000);
    bool putIntAsync(const String& key, int value, uint32_t timeoutMs = 5000);
    int getIntAsync(const String& key, int defaultValue = 0, uint32_t timeoutMs = 5000);
    
    // 静态任务函数
    static void configTaskFunction(void* parameter);
    
    // 常量定义
    static const int MAX_WIFI_CONFIGS = 3;
    static const int CONFIG_TASK_STACK_SIZE = 3072;
    static const int CONFIG_TASK_PRIORITY = 2;
    static const int CONFIG_QUEUE_SIZE = 10;
    
private:
    // 任务相关成员
    TaskHandle_t configTaskHandle;
    QueueHandle_t configQueue;
    bool taskRunning;
    
    // NVS操作对象
    Preferences preferences;
    
    // NVS命名空间
    static const char* WIFI_NAMESPACE;
    static const char* MULTI_WIFI_NAMESPACE;
    static const char* SYSTEM_NAMESPACE;
    
    // WiFi配置键名
    static const char* WIFI_SSID_KEY;
    static const char* WIFI_PASSWORD_KEY;
    static const char* WIFI_CONFIGURED_KEY;
    
    // 多WiFi配置键名
    static const char* WIFI_COUNT_KEY;
    static const char* WIFI_SSID_PREFIX;
    static const char* WIFI_PASSWORD_PREFIX;
    static const char* WIFI_PRIORITY_PREFIX;
    
    // 系统配置键名
    static const char* DEVICE_NAME_KEY;
    static const char* REFRESH_RATE_KEY;
    
    // 屏幕亮度配置键名
    static const char* BRIGHTNESS_KEY;
    
    // 主题配置键名
    static const char* THEME_KEY;
    
    // 时间配置键名
    static const char* TIME_PRIMARY_SERVER_KEY;
    static const char* TIME_SECONDARY_SERVER_KEY;
    static const char* TIME_TIMEZONE_KEY;
    static const char* TIME_SYNC_INTERVAL_KEY;
    
    // 屏幕设置配置键名
    static const char* SCREEN_MODE_KEY;
    static const char* SCREEN_START_HOUR_KEY;
    static const char* SCREEN_START_MINUTE_KEY;
    static const char* SCREEN_END_HOUR_KEY;
    static const char* SCREEN_END_MINUTE_KEY;
    static const char* SCREEN_TIMEOUT_MINUTES_KEY;
    
    // 服务器配置键名
    static const char* SERVER_URL_KEY;
    static const char* REQUEST_INTERVAL_KEY;
    static const char* ENABLED_KEY;
    static const char* CONNECTION_TIMEOUT_KEY;
    static const char* AUTO_GET_DATA_KEY;
    static const char* AUTO_SCAN_SERVER_KEY;
    
    // 内部任务处理方法
    void processConfigRequest(ConfigRequest* request);
    
    // 内部NVS操作方法 (这些方法只在任务内部调用)
    bool saveWiFiConfig(const String& ssid, const String& password);
    bool loadWiFiConfig(String& ssid, String& password);
    bool hasWiFiConfig();
    void clearWiFiConfig();
    
    bool saveWiFiConfigs(const WiFiConfig configs[3]);
    bool loadWiFiConfigs(WiFiConfig configs[3]);
    int getWiFiConfigCount();
    bool addWiFiConfig(const String& ssid, const String& password);
    void clearAllWiFiConfigs();
    
    bool updateWiFiPriority(int index, int priority);
    bool setWiFiPriorities(const int priorities[3]);
    void sortConfigsByPriority(WiFiConfig configs[3]);
    void resolveConflictingPriorities(WiFiConfig configs[3], int targetIndex, int newPriority);
    
    bool saveSystemConfig(const String& deviceName, int refreshRate);
    bool loadSystemConfig(String& deviceName, int& refreshRate);
    
    bool saveBrightness(uint8_t brightness);
    uint8_t loadBrightness();
    bool hasBrightnessConfig();
    
    bool saveThemeConfig(int theme);
    int loadThemeConfig();
    bool hasThemeConfig();
    
    bool saveTimeConfig(const String& primaryServer, const String& secondaryServer, 
                       const String& timezone, int syncInterval);
    bool loadTimeConfig(String& primaryServer, String& secondaryServer, 
                       String& timezone, int& syncInterval);
    
    bool saveScreenConfig(ScreenMode mode, int startHour, int startMinute, 
                         int endHour, int endMinute, int timeoutMinutes);
    bool loadScreenConfig(ScreenMode& mode, int& startHour, int& startMinute, 
                         int& endHour, int& endMinute, int& timeoutMinutes);
    bool hasScreenConfig();
    
    bool saveServerConfig(const String& serverUrl, int requestInterval, 
                         bool enabled, int connectionTimeout, bool autoGetData, bool autoScanServer);
    bool loadServerConfig(String& serverUrl, int& requestInterval, 
                         bool& enabled, int& connectionTimeout, bool& autoGetData, bool& autoScanServer);
    bool hasServerConfig();
    
    bool resetAllConfig();
    
    // 内部通用配置方法
    bool putString(const String& key, const String& value);
    String getString(const String& key, const String& defaultValue = "");
    bool putBool(const String& key, bool value);
    bool getBool(const String& key, bool defaultValue = false);
    bool putInt(const String& key, int value);
    int getInt(const String& key, int defaultValue = 0);
    
    // 内部辅助方法
    String getWiFiSSIDKey(int index);
    String getWiFiPasswordKey(int index);
    String getWiFiPriorityKey(int index);
    
    // 辅助方法：发送请求并等待响应
    bool sendRequestAndWait(ConfigRequest* request, uint32_t timeoutMs);
};

#endif // CONFIGSTORAGE_H 
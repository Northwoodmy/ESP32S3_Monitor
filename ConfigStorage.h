/*
 * ConfigStorage.h - NVS配置存储类头文件
 * ESP32S3监控项目 - 配置存储模块
 */

#ifndef CONFIGSTORAGE_H
#define CONFIGSTORAGE_H

#include <Preferences.h>
#include <Arduino.h>

// WiFi配置结构体
struct WiFiConfig {
    String ssid;
    String password;
    int priority;  // 优先级，1最高，数字越小优先级越高
    bool isValid;
    
    WiFiConfig() : ssid(""), password(""), priority(99), isValid(false) {}
    WiFiConfig(const String& s, const String& p, int prio = 99) : ssid(s), password(p), priority(prio), isValid(true) {}
};

class ConfigStorage {
public:
    ConfigStorage();
    ~ConfigStorage();
    
    // 初始化存储
    bool init();
    
    // 多WiFi配置操作 (新功能)
    bool saveWiFiConfigs(const WiFiConfig configs[3]);
    bool loadWiFiConfigs(WiFiConfig configs[3]);
    int getWiFiConfigCount();
    bool addWiFiConfig(const String& ssid, const String& password);
    void clearAllWiFiConfigs();
    
    // 优先级管理新功能
    bool updateWiFiPriority(int index, int priority);
    bool setWiFiPriorities(const int priorities[3]);
    void sortConfigsByPriority(WiFiConfig configs[3]);
    void resolveConflictingPriorities(WiFiConfig configs[3], int targetIndex, int newPriority);  // 新增：解决优先级冲突
    
    // 兼容性方法 (保持向后兼容)
    bool saveWiFiConfig(const String& ssid, const String& password);
    bool loadWiFiConfig(String& ssid, String& password);
    bool hasWiFiConfig();
    void clearWiFiConfig();
    
    // 系统配置操作
    bool saveSystemConfig(const String& deviceName, int refreshRate);
    bool loadSystemConfig(String& deviceName, int& refreshRate);
    
    // 配置重置操作
    bool resetAllConfig();
    
    // 常量定义
    static const int MAX_WIFI_CONFIGS = 3;
    
private:
    Preferences preferences;
    
    // NVS命名空间
    static const char* WIFI_NAMESPACE;
    static const char* MULTI_WIFI_NAMESPACE;
    static const char* SYSTEM_NAMESPACE;
    
    // WiFi配置键名 (旧版本兼容)
    static const char* WIFI_SSID_KEY;
    static const char* WIFI_PASSWORD_KEY;
    static const char* WIFI_CONFIGURED_KEY;
    
    // 多WiFi配置键名 (新功能)
    static const char* WIFI_COUNT_KEY;
    static const char* WIFI_SSID_PREFIX;
    static const char* WIFI_PASSWORD_PREFIX;
    static const char* WIFI_PRIORITY_PREFIX;  // 新增：优先级前缀
    
    // 系统配置键名
    static const char* DEVICE_NAME_KEY;
    static const char* REFRESH_RATE_KEY;
    
    // 内部辅助方法
    String getWiFiSSIDKey(int index);
    String getWiFiPasswordKey(int index);
    String getWiFiPriorityKey(int index);  // 新增：获取优先级键名
};

#endif // CONFIGSTORAGE_H 
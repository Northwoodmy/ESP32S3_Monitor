/*
 * ConfigStorage.h - NVS配置存储类头文件
 * ESP32S3监控项目 - 配置存储模块
 */

#ifndef CONFIGSTORAGE_H
#define CONFIGSTORAGE_H

#include <Preferences.h>
#include <Arduino.h>

class ConfigStorage {
public:
    ConfigStorage();
    ~ConfigStorage();
    
    // 初始化存储
    bool init();
    
    // WiFi配置操作
    bool saveWiFiConfig(const String& ssid, const String& password);
    bool loadWiFiConfig(String& ssid, String& password);
    bool hasWiFiConfig();
    void clearWiFiConfig();
    
    // 系统配置操作
    bool saveSystemConfig(const String& deviceName, int refreshRate);
    bool loadSystemConfig(String& deviceName, int& refreshRate);
    
private:
    Preferences preferences;
    
    // NVS命名空间
    static const char* WIFI_NAMESPACE;
    static const char* SYSTEM_NAMESPACE;
    
    // WiFi配置键名
    static const char* WIFI_SSID_KEY;
    static const char* WIFI_PASSWORD_KEY;
    static const char* WIFI_CONFIGURED_KEY;
    
    // 系统配置键名
    static const char* DEVICE_NAME_KEY;
    static const char* REFRESH_RATE_KEY;
};

#endif // CONFIGSTORAGE_H 
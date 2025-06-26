/*
 * ConfigStorage.cpp - NVS配置存储类实现文件
 * ESP32S3监控项目 - 配置存储模块
 */

#include "ConfigStorage.h"

// 静态常量定义
const char* ConfigStorage::WIFI_NAMESPACE = "wifi_config";
const char* ConfigStorage::SYSTEM_NAMESPACE = "system_config";

const char* ConfigStorage::WIFI_SSID_KEY = "ssid";
const char* ConfigStorage::WIFI_PASSWORD_KEY = "password";
const char* ConfigStorage::WIFI_CONFIGURED_KEY = "configured";

const char* ConfigStorage::DEVICE_NAME_KEY = "device_name";
const char* ConfigStorage::REFRESH_RATE_KEY = "refresh_rate";

ConfigStorage::ConfigStorage() {
}

ConfigStorage::~ConfigStorage() {
}

bool ConfigStorage::init() {
    printf("初始化NVS配置存储...\n");
    return true;
}

bool ConfigStorage::saveWiFiConfig(const String& ssid, const String& password) {
    printf("保存WiFi配置到NVS: SSID=%s\n", ssid.c_str());
    
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
    
    if (success) {
        printf("所有配置已重置为默认值\n");
    } else {
        printf("配置重置过程中发生错误\n");
    }
    
    return success;
} 
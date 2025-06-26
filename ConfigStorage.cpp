/*
 * ConfigStorage.cpp - NVS配置存储类实现文件
 * ESP32S3监控项目 - 配置存储模块
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
    
    // 保存每个配置
    int savedIndex = 0;
    for (int i = 0; i < MAX_WIFI_CONFIGS && savedIndex < validCount; i++) {
        if (configs[i].isValid && configs[i].ssid.length() > 0) {
            String ssidKey = getWiFiSSIDKey(savedIndex);
            String passwordKey = getWiFiPasswordKey(savedIndex);
            
            size_t ssidResult = preferences.putString(ssidKey.c_str(), configs[i].ssid);
            size_t passwordResult = preferences.putString(passwordKey.c_str(), configs[i].password);
            
            if (ssidResult == 0 || passwordResult == 0) {
                printf("保存WiFi配置 %d 失败\n", savedIndex);
                success = false;
            } else {
                printf("保存WiFi配置 %d: SSID=%s\n", savedIndex, configs[i].ssid.c_str());
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
        
        String ssid = preferences.getString(ssidKey.c_str(), "");
        String password = preferences.getString(passwordKey.c_str(), "");
        
        if (ssid.length() > 0) {
            configs[i] = WiFiConfig(ssid, password);
            printf("加载WiFi配置 %d: SSID=%s\n", i, ssid.c_str());
        } else {
            printf("WiFi配置 %d 为空\n", i);
            success = false;
        }
    }
    
    preferences.end();
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
            return saveWiFiConfigs(configs);
        }
    }
    
    // 查找空位置添加新配置
    for (int i = 0; i < MAX_WIFI_CONFIGS; i++) {
        if (!configs[i].isValid) {
            configs[i] = WiFiConfig(ssid, password);
            return saveWiFiConfigs(configs);
        }
    }
    
    // 如果没有空位，替换最后一个配置
    printf("WiFi配置已满，替换最后一个配置\n");
    configs[MAX_WIFI_CONFIGS - 1] = WiFiConfig(ssid, password);
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
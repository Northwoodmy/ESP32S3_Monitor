/*
 * WebServerManager.h - Web服务器管理器类头文件
 * ESP32S3监控项目 - Web服务器模块
 */

#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ConfigStorage.h"
#include "WiFiManager.h"
#include "OTAManager.h"
#include "FileManager.h"

// 前向声明
class PSRAMManager;
class DisplayManager;
class WeatherManager;
class LocationManager;

class WebServerManager {
public:
    WebServerManager(WiFiManager* wifiMgr, ConfigStorage* configStore, OTAManager* otaMgr, FileManager* fileMgr);
    ~WebServerManager();
    
    // 初始化Web服务器
    void init();
    
    // 设置PSRAM管理器
    void setPSRAMManager(PSRAMManager* psramManager);
    
    // 设置显示管理器
    void setDisplayManager(DisplayManager* displayManager);
    
    // 设置天气管理器
    void setWeatherManager(WeatherManager* weatherManager);
    
    // 设置定位管理器
    void setLocationManager(LocationManager* locationManager);
    
    // 启动服务器
    void start();
    
    // 停止服务器
    void stop();
    
    // 处理客户端请求
    void handleClient();

private:
    WebServer* server;
    WiFiManager* wifiManager;
    ConfigStorage* configStorage;
    OTAManager* otaManager;
    FileManager* fileManager;
    PSRAMManager* m_psramManager;
    DisplayManager* m_displayManager;
    WeatherManager* m_weatherManager;
    LocationManager* m_locationManager;
    TaskHandle_t serverTaskHandle;
    bool isRunning;
    
    // 静态任务函数
    static void serverTask(void* parameter);
    
    // HTTP处理函数
    void handleRoot();
    void handleWiFiConfig();
    void handleOTAPage();
    void handleWiFiScan();
    void handleSystemInfo();
    void handleRestart();
    void handleResetConfig();
    void handleNotFound();
    
    // API处理函数
    void handleAPI();
    void handleSaveWiFi();
    void handleGetStatus();
    
    // 多WiFi配置API
    void handleGetWiFiConfigs();
    void handleDeleteWiFiConfig();
    void handleConnectWiFiConfig();
    void handleUpdateWiFiPriority();
    void handleSetWiFiPriorities();
    
    // OTA升级相关API
    void handleOTAUpload();
    void handleOTAStatus();
    void handleOTAReboot();
    
    // 服务器OTA升级相关API
    void handleServerOTAStart();
    void handleServerOTAStatus();
    void handleServerFirmwareList();
    void handleServerFirmwareVersion();
    
    // 文件管理相关API
    void handleFileManager();
    void handleFileList();
    void handleFileUpload();
    void handleFileDownload();
    void handleFileDelete();
    void handleFileRename();
    void handleFileCreate();
    void handleFileSystemStatus();
    void handleFileSystemFormat();
    void handleFileSystemFormatStatus();
    
    // 屏幕配置相关API
    void handleScreenConfig();
    void handleSetBrightness();
    void handleScreenTest();
    
    // 系统设置相关API
    void handleSystemSettings();
    void handleGetTimeConfig();
    void handleSetTimeConfig();
    void handleSyncTime();
    
    // 天气设置相关API
    void handleWeatherSettings();
    void handleGetWeatherConfig();
    void handleSetWeatherApiKey();
    void handleSetWeatherCity();
    void handleSetWeatherUpdateConfig();
    void handleGetCurrentWeather();
    void handleGetWeatherStats();
    void handleTestWeatherApi();
    void handleUpdateWeatherNow();
    
    // 定位相关API
    void handleGetLocationData();
    void handleSetLocationApiKey();
    void handleLocationNow();
    
    // 服务器设置相关API
    void handleServerSettingsPage();
    void handleGetServerConfig();
    void handleSetServerConfig();
    void handleTestServerConnection();
    void handleGetServerData();
    void handleMDNSScanServers();  // mDNS扫描服务器
    
    // 屏幕设置相关API
    void handleScreenSettings();
    void handleGetScreenSettings();
    void handleSetScreenSettings();
    void handleGetCurrentRotation();
    
    // 主题设置相关API
    void handleGetThemeSettings();
    void handleSetThemeSettings();
    
    // 获取主页HTML
    String getIndexHTML();
    
    // 获取文件管理器HTML
    String getFileManagerHTML();
    
    // 获取OTA升级页面HTML
    String getOTAPageHTML();
    
    // 获取系统设置页面HTML
    String getSystemSettingsHTML();
    
    // 获取天气设置页面HTML
    String getWeatherSettingsHTML();
    
    // 获取服务器设置页面HTML
    String getServerSettingsHTML();
    
    // 获取CSS样式
    String getCSS();
    
    // 获取基础CSS样式
    String getBaseCSS();
    
    // 获取JavaScript代码
    String getJavaScript();
    
    // 获取OTA JavaScript代码
    String getOTAJavaScript();
    
    // 获取系统设置JavaScript代码
    String getSystemSettingsJavaScript();
    
    // 获取天气设置JavaScript代码
    String getWeatherSettingsJavaScript();
    
    // 获取系统设置CSS样式
    String getSystemSettingsCSS();
    
    // 获取天气设置CSS样式
    String getWeatherSettingsCSS();
    
    // 获取服务器设置CSS样式
    String getServerSettingsCSS();
    
    // 获取服务器设置JavaScript代码
    String getServerSettingsJavaScript();
    
    // 获取OTA页面CSS样式
    String getOTAPageCSS();
    
    // 获取屏幕设置页面HTML
    String getScreenSettingsHTML();
    
    // 获取屏幕设置页面CSS样式
    String getScreenSettingsCSS();
    
    // 获取屏幕设置页面JavaScript代码
    String getScreenSettingsJavaScript();
};

#endif // WEBSERVERMANAGER_H 
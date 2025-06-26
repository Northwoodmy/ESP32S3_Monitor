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

class WebServerManager {
public:
    WebServerManager(WiFiManager* wifiMgr, ConfigStorage* configStore, OTAManager* otaMgr, FileManager* fileMgr);
    ~WebServerManager();
    
    // 初始化Web服务器
    void init();
    
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
    
    // OTA升级相关API
    void handleOTAUpload();
    void handleOTAStatus();
    void handleOTAReboot();
    
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
    
    // 获取主页HTML
    String getIndexHTML();
    
    // 获取文件管理器HTML
    String getFileManagerHTML();
    
    // 获取OTA升级页面HTML
    String getOTAPageHTML();
    
    // 获取CSS样式
    String getCSS();
    
    // 获取JavaScript代码
    String getJavaScript();
    
    // 获取OTA JavaScript代码
    String getOTAJavaScript();
};

#endif // WEBSERVERMANAGER_H 
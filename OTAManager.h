/*
 * OTAManager.h - OTA升级管理器类头文件
 * ESP32S3监控项目 - OTA升级模块
 */

#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <Update.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 前向声明
class TimeManager;
class WeatherManager;
class Monitor;
class DisplayManager;
class WebServerManager;
class PSRAMManager;
class ConfigStorage;

// OTA升级状态枚举
enum class OTAStatus {
    IDLE,           // 空闲状态
    UPLOADING,      // 文件上传中
    DOWNLOADING,    // 从服务器下载固件中
    WRITING,        // 固件写入中
    SUCCESS,        // 升级成功
    FAILED          // 升级失败
};

// OTA升级类型枚举
enum class OTAType {
    LOCAL,          // 本地上传升级
    SERVER          // 服务器下载升级
};

class OTAManager {
public:
    OTAManager();
    ~OTAManager();
    
    // 初始化OTA管理器
    void init();
    
    // 设置任务管理器引用（用于OTA前停止任务释放资源）
    void setTaskManagers(TimeManager* timeManager, 
                        WeatherManager* weatherManager, 
                        Monitor* monitor,
                        DisplayManager* displayManager = nullptr,
                        WebServerManager* webServerManager = nullptr,
                        PSRAMManager* psramManager = nullptr,
                        ConfigStorage* configStorage = nullptr);
    
    // 本地OTA升级相关方法
    // 开始OTA升级
    bool beginOTA(size_t fileSize);
    
    // 写入OTA数据块
    bool writeOTAData(uint8_t* data, size_t len);
    
    // 设置实际文件大小（用于动态大小处理）
    bool setActualSize(size_t actualSize);
    
    // 结束OTA升级
    bool endOTA();
    
    // 服务器OTA升级相关方法
    // 从服务器下载并升级固件
    bool downloadAndUpdateFromServer(const String& serverUrl, const String& firmwareFile = "");
    
    // 检查服务器上的固件版本
    String checkServerFirmwareVersion(const String& serverUrl);
    
    // 获取服务器上的固件列表
    String getServerFirmwareList(const String& serverUrl);
    
    // 取消OTA升级
    void abortOTA();
    
    // 获取当前状态
    OTAStatus getStatus() const;
    
    // 获取当前OTA类型
    OTAType getOTAType() const;
    
    // 获取升级进度百分比
    float getProgress() const;
    
    // 获取错误信息
    String getError() const;
    
    // 获取状态信息JSON
    String getStatusJSON() const;
    
    // 重启设备（升级完成后调用）
    void rebootDevice();
    
    // 检查是否有足够的空间进行OTA升级
    bool hasEnoughSpace(size_t fileSize) const;

private:
    OTAStatus status;
    OTAType otaType;
    String errorMessage;
    size_t totalSize;
    size_t writtenSize;
    unsigned long lastProgressTime;
    
    // HTTP客户端
    HTTPClient httpClient;
    WiFiClientSecure wifiClientSecure;
    
    // 任务管理器引用（用于OTA前停止任务释放资源）
    TimeManager* m_timeManager;
    WeatherManager* m_weatherManager;
    Monitor* m_monitor;
    DisplayManager* m_displayManager;
    WebServerManager* m_webServerManager;
    PSRAMManager* m_psramManager;
    ConfigStorage* m_configStorage;
    
    // 任务状态记录（用于OTA后恢复）
    bool m_timeManagerWasRunning;
    bool m_weatherManagerWasRunning;
    bool m_monitorWasRunning;
    bool m_displayManagerWasRunning;
    bool m_webServerManagerWasRunning;
    bool m_psramManagerWasRunning;
    bool m_configStorageWasRunning;
    
    // 重置状态
    void resetStatus();
    
    // 更新状态
    void updateStatus(OTAStatus newStatus, const String& error = "");
    
    // 任务管理方法
    bool stopTasksForOTA();      // OTA前停止任务
    void restoreTasksAfterOTA(); // OTA后恢复任务（如果需要）
    
    // 屏幕显示更新方法
    void updateScreenDisplay();  // 更新屏幕OTA进度显示
    
    // 服务器OTA相关私有方法
    // 下载固件数据并写入
    bool downloadAndWriteFirmware(const String& downloadUrl);
    
    // 解析服务器响应
    bool parseServerResponse(const String& response, String& firmwareUrl);
    
    // 验证下载的固件
    bool validateFirmware(const String& firmwareData);
    
    // 静态任务函数
    static void rebootTask(void* parameter);
    static void serverOTATask(void* parameter);
    
    // 服务器OTA任务参数结构
    struct ServerOTAParams {
        OTAManager* manager;
        String serverUrl;
        String firmwareFile;
    };
};

#endif // OTAMANAGER_H 
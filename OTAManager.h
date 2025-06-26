/*
 * OTAManager.h - OTA升级管理器类头文件
 * ESP32S3监控项目 - OTA升级模块
 */

#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <Update.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// OTA升级状态枚举
enum class OTAStatus {
    IDLE,           // 空闲状态
    UPLOADING,      // 文件上传中
    WRITING,        // 固件写入中
    SUCCESS,        // 升级成功
    FAILED          // 升级失败
};

class OTAManager {
public:
    OTAManager();
    ~OTAManager();
    
    // 初始化OTA管理器
    void init();
    
    // 开始OTA升级
    bool beginOTA(size_t fileSize);
    
    // 写入OTA数据块
    bool writeOTAData(uint8_t* data, size_t len);
    
    // 设置实际文件大小（用于动态大小处理）
    bool setActualSize(size_t actualSize);
    
    // 结束OTA升级
    bool endOTA();
    
    // 取消OTA升级
    void abortOTA();
    
    // 获取当前状态
    OTAStatus getStatus() const;
    
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
    String errorMessage;
    size_t totalSize;
    size_t writtenSize;
    unsigned long lastProgressTime;
    
    // 重置状态
    void resetStatus();
    
    // 更新状态
    void updateStatus(OTAStatus newStatus, const String& error = "");
    
    // 静态任务函数
    static void rebootTask(void* parameter);
};

#endif // OTAMANAGER_H 
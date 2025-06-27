/*
 * OTAManager.cpp - OTA升级管理器类实现文件
 * ESP32S3监控项目 - OTA升级模块
 */

#include "OTAManager.h"
#include "Arduino.h"

OTAManager::OTAManager() :
    status(OTAStatus::IDLE),
    errorMessage(""),
    totalSize(0),
    writtenSize(0),
    lastProgressTime(0) {
}

OTAManager::~OTAManager() {
    if (status == OTAStatus::UPLOADING || status == OTAStatus::WRITING) {
        abortOTA();
    }
}

void OTAManager::init() {
    printf("初始化OTA管理器...\n");
    resetStatus();
    printf("OTA管理器初始化完成\n");
}

bool OTAManager::beginOTA(size_t fileSize) {
    if (fileSize == 0) {
        printf("开始动态大小OTA升级\n");
    } else {
        printf("开始OTA升级，文件大小: %u 字节\n", fileSize);
    }
    
    if (status != OTAStatus::IDLE) {
        updateStatus(OTAStatus::FAILED, "OTA正在进行中，无法开始新的升级");
        return false;
    }
    
    // 对于动态大小（fileSize=0），跳过大小检查
    if (fileSize > 0) {
        if (!hasEnoughSpace(fileSize)) {
            updateStatus(OTAStatus::FAILED, "存储空间不足");
            return false;
        }
        
        // 开始更新，指定文件大小
        if (!Update.begin(fileSize)) {
            String error = "OTA开始失败: ";
            error += Update.errorString();
            updateStatus(OTAStatus::FAILED, error);
            printf("OTA开始失败: %s\n", Update.errorString());
            return false;
        }
    } else {
        // 动态大小模式，使用UPDATE_SIZE_UNKNOWN
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            String error = "OTA开始失败: ";
            error += Update.errorString();
            updateStatus(OTAStatus::FAILED, error);
            printf("OTA开始失败: %s\n", Update.errorString());
            return false;
        }
    }
    
    totalSize = fileSize;
    writtenSize = 0;
    lastProgressTime = millis();
    updateStatus(OTAStatus::UPLOADING, "");
    
    printf("OTA升级开始成功\n");
    return true;
}

bool OTAManager::writeOTAData(uint8_t* data, size_t len) {
    if (status != OTAStatus::UPLOADING && status != OTAStatus::WRITING) {
        updateStatus(OTAStatus::FAILED, "OTA未处于上传状态");
        return false;
    }
    
    updateStatus(OTAStatus::WRITING, "");
    
    size_t written = Update.write(data, len);
    if (written != len) {
        String error = "写入数据失败: ";
        error += Update.errorString();
        updateStatus(OTAStatus::FAILED, error);
        printf("写入OTA数据失败: %s\n", Update.errorString());
        return false;
    }
    
    writtenSize += written;
    
    // 每1000ms打印一次进度（减少日志输出）
    unsigned long currentTime = millis();
    if (currentTime - lastProgressTime >= 1000) {
        printf("OTA进度: %.1f%% (%u 字节已写入)\n", getProgress(), writtenSize);
        lastProgressTime = currentTime;
    }
    
    return true;
}

bool OTAManager::setActualSize(size_t actualSize) {
    printf("设置实际文件大小: %u 字节\n", actualSize);
    
    if (status != OTAStatus::UPLOADING && status != OTAStatus::WRITING) {
        updateStatus(OTAStatus::FAILED, "OTA未处于正确状态，无法设置文件大小");
        return false;
    }
    
    if (actualSize == 0) {
        updateStatus(OTAStatus::FAILED, "实际文件大小无效");
        return false;
    }
    
    // 检查空间是否足够
    if (!hasEnoughSpace(actualSize)) {
        updateStatus(OTAStatus::FAILED, "存储空间不足");
        return false;
    }
    
    // 检查写入的数据大小
    if (writtenSize != actualSize) {
        String error = "数据大小不匹配，预期: ";
        error += actualSize;
        error += "，实际写入: ";
        error += writtenSize;
        updateStatus(OTAStatus::FAILED, error);
        return false;
    }
    
    totalSize = actualSize;
    printf("实际文件大小设置成功: %u 字节\n", actualSize);
    printf("最终进度: %.1f%% (%u/%u 字节)\n", getProgress(), writtenSize, totalSize);
    return true;
}

bool OTAManager::endOTA() {
    printf("结束OTA升级...\n");
    printf("当前状态: %d, 已写入: %u, 总大小: %u\n", (int)status, writtenSize, totalSize);
    
    // 检查状态：允许UPLOADING、WRITING状态
    if (status != OTAStatus::WRITING && status != OTAStatus::UPLOADING) {
        if (status == OTAStatus::FAILED) {
            printf("OTA已经处于失败状态，错误信息: %s\n", errorMessage.c_str());
            return false;
        } else {
            updateStatus(OTAStatus::FAILED, "OTA未处于正确的状态");
            return false;
        }
    }
    
    // 检查是否有数据写入
    if (writtenSize == 0) {
        updateStatus(OTAStatus::FAILED, "没有写入任何数据");
        return false;
    }
    
    // 对于动态大小模式，totalSize可能在setActualSize中设置
    // 如果totalSize仍为0，则使用writtenSize作为实际大小
    if (totalSize == 0) {
        totalSize = writtenSize;
        printf("使用实际写入大小作为总大小: %u 字节\n", totalSize);
    }
    
    // 检查写入的数据大小是否与预期一致（仅在totalSize > 0时检查）
    if (totalSize > 0 && writtenSize != totalSize) {
        String error = "数据大小不匹配，预期: ";
        error += totalSize;
        error += "，实际: ";
        error += writtenSize;
        updateStatus(OTAStatus::FAILED, error);
        return false;
    }
    
    if (!Update.end(true)) {
        String error = "OTA结束失败: ";
        error += Update.errorString();
        updateStatus(OTAStatus::FAILED, error);
        printf("OTA结束失败: %s\n", Update.errorString());
        return false;
    }
    
    updateStatus(OTAStatus::SUCCESS, "");
    printf("OTA升级成功完成！\n");
    printf("最终进度: %.1f%% (%u/%u 字节)\n", getProgress(), writtenSize, totalSize);
    
    return true;
}

void OTAManager::abortOTA() {
    printf("取消OTA升级...\n");
    
    if (status == OTAStatus::UPLOADING || status == OTAStatus::WRITING) {
        Update.abort();
        updateStatus(OTAStatus::FAILED, "OTA升级被取消");
        printf("OTA升级已取消\n");
    }
}

OTAStatus OTAManager::getStatus() const {
    return status;
}

float OTAManager::getProgress() const {
    if (totalSize == 0) {
        // 动态模式下，根据已写入的数据量估算进度
        // 对于1MB文件，我们可以基于经验值估算进度
        if (writtenSize == 0) {
            return 0.0f;
        } else if (writtenSize < 500000) {  // 小于500KB
            return (float)writtenSize / 500000.0f * 50.0f;  // 0-50%
        } else if (writtenSize < 1000000) { // 500KB-1MB
            return 50.0f + ((float)(writtenSize - 500000) / 500000.0f * 40.0f);  // 50-90%
        } else {
            return 90.0f + ((float)(writtenSize - 1000000) / 100000.0f * 10.0f);  // 90-100%
        }
    }
    return (float)writtenSize / (float)totalSize * 100.0f;
}

String OTAManager::getError() const {
    return errorMessage;
}

String OTAManager::getStatusJSON() const {
    DynamicJsonDocument doc(512);
    
    switch (status) {
        case OTAStatus::IDLE:
            doc["status"] = "idle";
            doc["message"] = "等待升级";
            break;
        case OTAStatus::UPLOADING:
            doc["status"] = "uploading";
            doc["message"] = "文件上传中";
            break;
        case OTAStatus::WRITING:
            doc["status"] = "writing";
            doc["message"] = "固件写入中";
            break;
        case OTAStatus::SUCCESS:
            doc["status"] = "success";
            doc["message"] = "升级成功";
            break;
        case OTAStatus::FAILED:
            doc["status"] = "failed";
            doc["message"] = errorMessage.length() > 0 ? errorMessage : "升级失败";
            break;
    }
    
    float progress = getProgress();
    doc["progress"] = (int)(progress * 10) / 10.0f;  // 确保是数字类型，保留1位小数
    doc["progressText"] = String(progress, 1) + "%";
    doc["totalSize"] = (int)totalSize;
    doc["writtenSize"] = (int)writtenSize;
    
    if (errorMessage.length() > 0) {
        doc["error"] = errorMessage;
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

void OTAManager::rebootDevice() {
    printf("设备将在3秒后重启...\n");
    
    // 创建重启任务（运行在核心0）
    xTaskCreatePinnedToCore(
        rebootTask,
        "RebootTask",
        2048,
        nullptr,
        1,
        nullptr,
        0           // 运行在核心0
    );
}

bool OTAManager::hasEnoughSpace(size_t fileSize) const {
    size_t freeSpace = ESP.getFreeSketchSpace();
    printf("可用固件空间: %u 字节，需要空间: %u 字节\n", freeSpace, fileSize);
    
    // 预留10%的缓冲空间
    return freeSpace > (fileSize + fileSize / 10);
}

void OTAManager::resetStatus() {
    status = OTAStatus::IDLE;
    errorMessage = "";
    totalSize = 0;
    writtenSize = 0;
    lastProgressTime = 0;
}

void OTAManager::updateStatus(OTAStatus newStatus, const String& error) {
    // 记录状态变化
    if (status != newStatus) {
        printf("OTA状态变化: %d -> %d\n", (int)status, (int)newStatus);
    }
    
    status = newStatus;
    errorMessage = error;
    
    if (newStatus == OTAStatus::FAILED && error.length() > 0) {
        printf("OTA错误: %s\n", error.c_str());
    }
}

void OTAManager::rebootTask(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    printf("正在重启设备...\n");
    ESP.restart();
} 
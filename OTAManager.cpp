/*
 * OTAManager.cpp - OTA升级管理器类实现文件
 * ESP32S3监控项目 - OTA升级模块
 */

#include "OTAManager.h"
#include "Arduino.h"

OTAManager::OTAManager() :
    status(OTAStatus::IDLE),
    otaType(OTAType::LOCAL),
    errorMessage(""),
    totalSize(0),
    writtenSize(0),
    lastProgressTime(0) {
}

OTAManager::~OTAManager() {
    if (status == OTAStatus::UPLOADING || status == OTAStatus::WRITING || status == OTAStatus::DOWNLOADING) {
        abortOTA();
    }
}

void OTAManager::init() {
    printf("初始化OTA管理器...\n");
    resetStatus();
    
    // 初始化HTTP客户端
    httpClient.setTimeout(30000);  // 30秒超时
    httpClient.setReuse(false);
    
    // 配置HTTPS客户端（如果需要）
    wifiClientSecure.setInsecure();  // 跳过SSL证书验证，如果服务器支持HTTPS
    
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
    
    // 设置为本地OTA类型
    otaType = OTAType::LOCAL;
    
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

// 服务器OTA升级相关方法实现
bool OTAManager::downloadAndUpdateFromServer(const String& serverUrl, const String& firmwareFile) {
    printf("开始从服务器下载固件升级...\n");
    printf("服务器地址: %s\n", serverUrl.c_str());
    printf("固件文件: %s\n", firmwareFile.c_str());
    
    if (status != OTAStatus::IDLE) {
        updateStatus(OTAStatus::FAILED, "OTA正在进行中，无法开始新的升级");
        return false;
    }
    
    // 设置为服务器OTA类型
    otaType = OTAType::SERVER;
    
    // 创建服务器OTA任务
    ServerOTAParams* params = new ServerOTAParams();
    params->manager = this;
    params->serverUrl = serverUrl;
    params->firmwareFile = firmwareFile;
    
    // 创建任务来处理服务器OTA（避免阻塞主任务）
    xTaskCreatePinnedToCore(
        serverOTATask,
        "ServerOTATask",
        8192,  // 增加栈大小以支持HTTP操作
        params,
        5,     // 较高优先级
        nullptr,
        0      // 运行在核心0
    );
    
    return true;
}

String OTAManager::checkServerFirmwareVersion(const String& serverUrl) {
    printf("检查服务器固件版本...\n");
    
    String versionUrl = serverUrl + "/version.json";
    httpClient.begin(versionUrl);
    
    int httpCode = httpClient.GET();
    String response = "";
    
    if (httpCode == HTTP_CODE_OK) {
        response = httpClient.getString();
        printf("服务器响应: %s\n", response.c_str());
    } else {
        printf("HTTP请求失败，错误码: %d\n", httpCode);
    }
    
    httpClient.end();
    return response;
}

String OTAManager::getServerFirmwareList(const String& serverUrl) {
    printf("获取服务器固件列表...\n");
    
    String listUrl = serverUrl + "/firmware-list.json";
    httpClient.begin(listUrl);
    
    int httpCode = httpClient.GET();
    String response = "";
    
    if (httpCode == HTTP_CODE_OK) {
        response = httpClient.getString();
        printf("固件列表响应: %s\n", response.c_str());
    } else {
        printf("HTTP请求失败，错误码: %d\n", httpCode);
    }
    
    httpClient.end();
    return response;
}

void OTAManager::abortOTA() {
    printf("取消OTA升级...\n");
    
    if (status == OTAStatus::UPLOADING || status == OTAStatus::WRITING || status == OTAStatus::DOWNLOADING) {
        Update.abort();
        updateStatus(OTAStatus::FAILED, "OTA升级被取消");
        printf("OTA升级已取消\n");
    }
}

OTAStatus OTAManager::getStatus() const {
    return status;
}

OTAType OTAManager::getOTAType() const {
    return otaType;
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
        case OTAStatus::DOWNLOADING:
            doc["status"] = "downloading";
            doc["message"] = "从服务器下载固件中";
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
    doc["otaType"] = (otaType == OTAType::LOCAL) ? "local" : "server";
    
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
    otaType = OTAType::LOCAL;
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

// 服务器OTA相关私有方法实现
bool OTAManager::downloadAndWriteFirmware(const String& downloadUrl) {
    printf("开始下载固件: %s\n", downloadUrl.c_str());
    
    updateStatus(OTAStatus::DOWNLOADING, "");
    
    httpClient.begin(downloadUrl);
    
    int httpCode = httpClient.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        String error = "HTTP请求失败，错误码: ";
        error += httpCode;
        updateStatus(OTAStatus::FAILED, error);
        httpClient.end();
        return false;
    }
    
    int contentLength = httpClient.getSize();
    printf("固件大小: %d 字节\n", contentLength);
    
    if (contentLength <= 0) {
        updateStatus(OTAStatus::FAILED, "无法获取固件大小");
        httpClient.end();
        return false;
    }
    
    // 检查空间是否足够
    if (!hasEnoughSpace(contentLength)) {
        updateStatus(OTAStatus::FAILED, "存储空间不足");
        httpClient.end();
        return false;
    }
    
    // 开始OTA升级
    if (!Update.begin(contentLength)) {
        String error = "OTA开始失败: ";
        error += Update.errorString();
        updateStatus(OTAStatus::FAILED, error);
        httpClient.end();
        return false;
    }
    
    totalSize = contentLength;
    writtenSize = 0;
    lastProgressTime = millis();
    updateStatus(OTAStatus::WRITING, "");
    
    // 获取数据流
    WiFiClient* stream = httpClient.getStreamPtr();
    uint8_t buffer[1024];
    
    while (httpClient.connected() && (contentLength > 0 || contentLength == -1)) {
        size_t availableSize = stream->available();
        if (availableSize) {
            int readSize = stream->readBytes(buffer, ((availableSize > sizeof(buffer)) ? sizeof(buffer) : availableSize));
            
            size_t written = Update.write(buffer, readSize);
            if (written != readSize) {
                String error = "写入数据失败: ";
                error += Update.errorString();
                updateStatus(OTAStatus::FAILED, error);
                httpClient.end();
                return false;
            }
            
            writtenSize += written;
            
            if (contentLength > 0) {
                contentLength -= readSize;
            }
            
            // 每1000ms打印一次进度
            unsigned long currentTime = millis();
            if (currentTime - lastProgressTime >= 1000) {
                printf("下载进度: %.1f%% (%u 字节已写入)\n", getProgress(), writtenSize);
                lastProgressTime = currentTime;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // 避免watchdog触发
    }
    
    httpClient.end();
    
    // 结束OTA升级
    if (!Update.end(true)) {
        String error = "OTA结束失败: ";
        error += Update.errorString();
        updateStatus(OTAStatus::FAILED, error);
        return false;
    }
    
    updateStatus(OTAStatus::SUCCESS, "");
    printf("服务器OTA升级成功完成！\n");
    printf("最终进度: %.1f%% (%u/%u 字节)\n", getProgress(), writtenSize, totalSize);
    
    return true;
}

bool OTAManager::parseServerResponse(const String& response, String& firmwareUrl) {
    // 解析服务器响应，提取固件下载URL
    DynamicJsonDocument doc(1024);
    
    if (deserializeJson(doc, response)) {
        printf("解析服务器响应失败\n");
        return false;
    }
    
    if (doc.containsKey("firmware_url")) {
        firmwareUrl = doc["firmware_url"].as<String>();
        return true;
    }
    
    return false;
}

bool OTAManager::validateFirmware(const String& firmwareData) {
    // 这里可以添加固件验证逻辑，比如检查固件头部、校验和等
    // 简单起见，这里只检查数据是否非空
    return firmwareData.length() > 0;
}

void OTAManager::rebootTask(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    printf("正在重启设备...\n");
    ESP.restart();
}

void OTAManager::serverOTATask(void* parameter) {
    ServerOTAParams* params = (ServerOTAParams*)parameter;
    OTAManager* manager = params->manager;
    
    printf("服务器OTA任务开始...\n");
    
    // 构建固件下载URL
    String downloadUrl = params->serverUrl;
    if (!downloadUrl.endsWith("/")) {
        downloadUrl += "/";
    }
    
    if (params->firmwareFile.length() > 0) {
        downloadUrl += params->firmwareFile;
    } else {
        downloadUrl += "firmware.bin";  // 默认固件文件名
    }
    
    printf("固件下载URL: %s\n", downloadUrl.c_str());
    
    // 执行下载和升级
    if (manager->downloadAndWriteFirmware(downloadUrl)) {
        printf("服务器OTA升级成功！\n");
    } else {
        printf("服务器OTA升级失败！\n");
    }
    
    // 清理参数
    delete params;
    
    // 删除任务
    vTaskDelete(nullptr);
} 
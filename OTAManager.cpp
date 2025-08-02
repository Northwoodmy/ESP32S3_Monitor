/*
 * OTAManager.cpp - OTA升级管理器类实现文件
 * ESP32S3监控项目 - OTA升级模块
 */

#include "OTAManager.h"
#include "Arduino.h"
#include "Version.h"
#include "TimeManager.h"
#include "WeatherManager.h"
#include "Monitor.h"
#include "DisplayManager.h"
#include "WebServerManager.h"
#include "PSRAMManager.h"
#include "ConfigStorage.h"

OTAManager::OTAManager() :
    status(OTAStatus::IDLE),
    otaType(OTAType::LOCAL),
    errorMessage(""),
    totalSize(0),
    writtenSize(0),
    lastProgressTime(0),
    m_timeManager(nullptr),
    m_weatherManager(nullptr),
    m_monitor(nullptr),
    m_displayManager(nullptr),
    m_webServerManager(nullptr),
    m_psramManager(nullptr),
    m_configStorage(nullptr),
    m_timeManagerWasRunning(false),
    m_weatherManagerWasRunning(false),
    m_monitorWasRunning(false),
    m_displayManagerWasRunning(false),
    m_webServerManagerWasRunning(false),
    m_psramManagerWasRunning(false),
    m_configStorageWasRunning(false) {
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

void OTAManager::setTaskManagers(TimeManager* timeManager, 
                                WeatherManager* weatherManager, 
                                Monitor* monitor,
                                DisplayManager* displayManager,
                                WebServerManager* webServerManager,
                                PSRAMManager* psramManager,
                                ConfigStorage* configStorage) {
    m_timeManager = timeManager;
    m_weatherManager = weatherManager;
    m_monitor = monitor;
    m_displayManager = displayManager;
    m_webServerManager = webServerManager;
    m_psramManager = psramManager;
    m_configStorage = configStorage;
    
    printf("OTA管理器：已设置任务管理器引用\n");
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
    
    // OTA升级前停止其他任务以释放系统资源
    printf("OTA升级前停止其他任务以释放系统资源...\n");
    if (!stopTasksForOTA()) {
        printf("警告：停止任务时出现问题，但继续进行OTA升级\n");
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
    
    // 启动屏幕OTA进度显示
    if (m_displayManager) {
        m_displayManager->startOTADisplay(otaType == OTAType::SERVER);
    }
    
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
        
        // OTA失败，通知屏幕显示失败状态
        if (m_displayManager) {
            m_displayManager->completeOTADisplay(false, "Update failed");
        }
        
        // OTA失败，恢复之前停止的任务
        printf("OTA升级失败，恢复之前停止的任务...\n");
        restoreTasksAfterOTA();
        
        return false;
    }
    
    updateStatus(OTAStatus::SUCCESS, "");
    printf("OTA升级成功完成！\n");
    printf("最终进度: %.1f%% (%u/%u 字节)\n", getProgress(), writtenSize, totalSize);
    
    // OTA成功，通知屏幕显示完成状态
    if (m_displayManager) {
        m_displayManager->completeOTADisplay(true, "Update successful, rebooting device");
    }
    
    // OTA成功，通常会立即重启设备，所以不需要恢复任务
    printf("OTA升级成功，准备重启设备（不恢复任务）\n");
    
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
    
    // 服务器OTA升级前停止其他任务以释放系统资源
    printf("服务器OTA升级前停止其他任务以释放系统资源...\n");
    if (!stopTasksForOTA()) {
        printf("警告：停止任务时出现问题，但继续进行OTA升级\n");
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

// 版本比较相关方法实现
bool OTAManager::parseVersion(const String& version, int& major, int& minor, int& patch) {
    printf("解析版本号: %s\n", version.c_str());
    
    // 初始化版本号组件
    major = 0;
    minor = 0;
    patch = 0;
    
    String cleanVersion = version;
    
    // 移除 'v' 前缀（如果存在）
    if (cleanVersion.startsWith("v") || cleanVersion.startsWith("V")) {
        cleanVersion = cleanVersion.substring(1);
    }
    
    // 按点分割版本号
    int firstDot = cleanVersion.indexOf('.');
    if (firstDot == -1) {
        // 没有找到点，尝试解析为单个主版本号
        major = cleanVersion.toInt();
        printf("解析结果: %d.0.0\n", major);
        return true;
    }
    
    int secondDot = cleanVersion.indexOf('.', firstDot + 1);
    
    // 解析主版本号
    major = cleanVersion.substring(0, firstDot).toInt();
    
    if (secondDot == -1) {
        // 只有主版本号和次版本号
        minor = cleanVersion.substring(firstDot + 1).toInt();
        patch = 0;
    } else {
        // 有完整的三段版本号
        minor = cleanVersion.substring(firstDot + 1, secondDot).toInt();
        patch = cleanVersion.substring(secondDot + 1).toInt();
    }
    
    printf("解析结果: %d.%d.%d\n", major, minor, patch);
    return true;
}

int OTAManager::compareVersions(const String& version1, const String& version2) {
    printf("比较版本: %s vs %s\n", version1.c_str(), version2.c_str());
    
    int major1, minor1, patch1;
    int major2, minor2, patch2;
    
    if (!parseVersion(version1, major1, minor1, patch1)) {
        printf("版本1解析失败\n");
        return 0;  // 解析失败，视为相等
    }
    
    if (!parseVersion(version2, major2, minor2, patch2)) {
        printf("版本2解析失败\n");
        return 0;  // 解析失败，视为相等
    }
    
    // 比较主版本号
    if (major1 < major2) {
        printf("比较结果: %s < %s (主版本号)\n", version1.c_str(), version2.c_str());
        return -1;
    } else if (major1 > major2) {
        printf("比较结果: %s > %s (主版本号)\n", version1.c_str(), version2.c_str());
        return 1;
    }
    
    // 主版本号相同，比较次版本号
    if (minor1 < minor2) {
        printf("比较结果: %s < %s (次版本号)\n", version1.c_str(), version2.c_str());
        return -1;
    } else if (minor1 > minor2) {
        printf("比较结果: %s > %s (次版本号)\n", version1.c_str(), version2.c_str());
        return 1;
    }
    
    // 次版本号相同，比较补丁版本号
    if (patch1 < patch2) {
        printf("比较结果: %s < %s (补丁版本号)\n", version1.c_str(), version2.c_str());
        return -1;
    } else if (patch1 > patch2) {
        printf("比较结果: %s > %s (补丁版本号)\n", version1.c_str(), version2.c_str());
        return 1;
    }
    
    // 版本号完全相同
    printf("比较结果: %s == %s\n", version1.c_str(), version2.c_str());
    return 0;
}

bool OTAManager::needsUpdate(const String& serverVersion) {
    String currentVersion = VERSION_STRING;
    printf("检查是否需要升级: 当前版本=%s, 服务器版本=%s\n", 
           currentVersion.c_str(), serverVersion.c_str());
    
    int result = compareVersions(currentVersion, serverVersion);
    bool needs = (result < 0);  // 当前版本小于服务器版本时需要升级
    
    printf("升级检查结果: %s\n", needs ? "需要升级" : "无需升级");
    return needs;
}

void OTAManager::abortOTA() {
    printf("取消OTA升级...\n");
    
    if (status == OTAStatus::UPLOADING || status == OTAStatus::WRITING || status == OTAStatus::DOWNLOADING) {
        Update.abort();
        updateStatus(OTAStatus::FAILED, "OTA升级被取消");
        printf("OTA升级已取消\n");
        
        // OTA取消后恢复之前停止的任务
        printf("OTA取消，恢复之前停止的任务...\n");
        restoreTasksAfterOTA();
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
        // 动态模式下，无法准确计算进度百分比
        // 返回一个基于写入量的简单指示，但不超过99%
        if (writtenSize == 0) {
            return 0.0f;
        } else {
            // 基于文件大小的经验值估算，但限制在99%以下
            // 对于ESP32固件，通常在1-6MB范围内
            float estimatedProgress;
            if (writtenSize < 1048576) {  // 小于1MB
                estimatedProgress = (float)writtenSize / 1048576.0f * 30.0f;  // 0-30%
            } else if (writtenSize < 3145728) {  // 1MB-3MB
                estimatedProgress = 30.0f + ((float)(writtenSize - 1048576) / 2097152.0f * 40.0f);  // 30-70%
            } else if (writtenSize < 6291456) {  // 3MB-6MB
                estimatedProgress = 70.0f + ((float)(writtenSize - 3145728) / 3145728.0f * 25.0f);  // 70-95%
            } else {
                estimatedProgress = 95.0f;  // 超过6MB，固定在95%
            }
            
            // 确保进度不超过99%，为设置实际大小后的100%留出空间
            return (estimatedProgress > 99.0f) ? 99.0f : estimatedProgress;
        }
    }
    
    // 已知总大小，计算实际进度百分比
    float actualProgress = (float)writtenSize / (float)totalSize * 100.0f;
    // 确保进度不超过100%
    return (actualProgress > 100.0f) ? 100.0f : actualProgress;
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
    
    // 更新屏幕显示
    updateScreenDisplay();
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
        
        // HTTP请求失败，恢复之前停止的任务
        printf("HTTP请求失败，恢复之前停止的任务...\n");
        restoreTasksAfterOTA();
        
        return false;
    }
    
    int contentLength = httpClient.getSize();
    printf("固件大小: %d 字节\n", contentLength);
    
    if (contentLength <= 0) {
        updateStatus(OTAStatus::FAILED, "无法获取固件大小");
        httpClient.end();
        
        // 无法获取固件大小，恢复之前停止的任务
        printf("无法获取固件大小，恢复之前停止的任务...\n");
        restoreTasksAfterOTA();
        
        return false;
    }
    
    // 检查空间是否足够
    if (!hasEnoughSpace(contentLength)) {
        updateStatus(OTAStatus::FAILED, "存储空间不足");
        httpClient.end();
        
        // 存储空间不足，恢复之前停止的任务
        printf("存储空间不足，恢复之前停止的任务...\n");
        restoreTasksAfterOTA();
        
        return false;
    }
    
    // 开始OTA升级
    if (!Update.begin(contentLength)) {
        String error = "OTA开始失败: ";
        error += Update.errorString();
        updateStatus(OTAStatus::FAILED, error);
        httpClient.end();
        
        // OTA开始失败，恢复之前停止的任务
        printf("OTA开始失败，恢复之前停止的任务...\n");
        restoreTasksAfterOTA();
        
        return false;
    }
    
    totalSize = contentLength;
    writtenSize = 0;
    lastProgressTime = millis();
    updateStatus(OTAStatus::WRITING, "");
    
    // 启动屏幕OTA进度显示（服务器OTA）
    if (m_displayManager) {
        printf("[OTAManager] 启动服务器OTA页面显示\n");
        m_displayManager->startOTADisplay(true);  // true表示服务器OTA
    }
    
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
                
                // 写入数据失败，恢复之前停止的任务
                printf("写入数据失败，恢复之前停止的任务...\n");
                restoreTasksAfterOTA();
                
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
        
        // 服务器OTA失败，恢复之前停止的任务
        printf("服务器OTA升级失败，恢复之前停止的任务...\n");
        restoreTasksAfterOTA();
        
        return false;
    }
    
    updateStatus(OTAStatus::SUCCESS, "");
    printf("服务器OTA升级成功完成！\n");
    printf("最终进度: %.1f%% (%u/%u 字节)\n", getProgress(), writtenSize, totalSize);
    
    // 服务器OTA成功，通常会立即重启设备，所以不需要恢复任务
    printf("服务器OTA升级成功，准备重启设备（不恢复任务）\n");
    
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

bool OTAManager::stopTasksForOTA() {
    printf("🛑 [OTA] 开始停止系统任务以释放资源...\n");
    
    bool allStopped = true;
    
    // 记录当前任务运行状态，用于后续恢复（如果需要）
    m_timeManagerWasRunning = false;
    m_weatherManagerWasRunning = false;
    m_monitorWasRunning = false;
    m_displayManagerWasRunning = false;
    m_webServerManagerWasRunning = false;
    m_psramManagerWasRunning = false;
    m_configStorageWasRunning = false;
    
    // 停止时间管理任务
    if (m_timeManager) {
        printf("🕒 [OTA] 停止时间管理任务...\n");
        // 这里需要检查任务是否在运行，但TimeManager可能没有提供isRunning()方法
        // 直接调用stop()，它内部会检查状态
        m_timeManager->stop();
        m_timeManagerWasRunning = true; // 记录曾经运行过
        printf("✅ [OTA] 时间管理任务已停止\n");
    }
    
    // 停止天气管理任务
    if (m_weatherManager) {
        printf("🌤️ [OTA] 停止天气管理任务...\n");
        m_weatherManager->stop();
        m_weatherManagerWasRunning = true; // 记录曾经运行过
        printf("✅ [OTA] 天气管理任务已停止\n");
    }
    
    // 停止监控任务
    if (m_monitor) {
        printf("📊 [OTA] 停止监控任务...\n");
        m_monitor->stop();
        m_monitorWasRunning = true; // 记录曾经运行过
        printf("✅ [OTA] 监控任务已停止\n");
    }
    
    // 保留显示管理任务运行，用于显示OTA进度条
    if (m_displayManager) {
        printf("🖥️ [OTA] 保持显示管理任务运行以显示OTA进度\n");
        // 不停止显示管理任务，用户需要看到OTA升级进度条
        // m_displayManager->stop();
        // m_displayManagerWasRunning = true;
    }
    
    // Web服务器保持运行以支持OTA状态查询，但可以选择停止
    if (m_webServerManager) {
        printf("🌐 [OTA] 保持Web服务器运行以支持OTA状态查询\n");
        // 可选：m_webServerManager->stop();
        // m_webServerManagerWasRunning = true;
    }
    
    // PSRAM管理器保持运行，因为OTA可能需要内存管理
    if (m_psramManager) {
        printf("💾 [OTA] 保持PSRAM管理器运行\n");
        // 可选：m_psramManager->stop();
        // m_psramManagerWasRunning = true;
    }
    
    // 配置存储保持运行，可能需要保存OTA相关配置
    if (m_configStorage) {
        printf("⚙️ [OTA] 保持配置存储运行\n");
        // 可选：m_configStorage->stopTask();
        // m_configStorageWasRunning = true;
    }
    
    // 等待任务完全停止
    printf("⏳ [OTA] 等待任务完全停止...\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    printf("✅ [OTA] 系统任务停止完成，已释放资源用于OTA升级\n");
    
    // 显示内存状态
    if (m_psramManager) {
        printf("📊 [OTA] 当前内存状态:\n");
        printf("   - 可用堆内存: %u 字节\n", ESP.getFreeHeap());
        printf("   - 最小堆内存: %u 字节\n", ESP.getMinFreeHeap());
        if (m_psramManager->isPSRAMAvailable()) {
            printf("   - 可用PSRAM: %u 字节\n", m_psramManager->getFreeSize());
        }
    }
    
    return allStopped;
}

void OTAManager::restoreTasksAfterOTA() {
    printf("🔄 [OTA] OTA升级完成，准备恢复系统任务...\n");
    
    // 注意：通常OTA成功后会重启设备，所以这个方法可能不会被调用
    // 这个方法主要用于OTA失败后的恢复
    
    // 恢复时间管理任务
    if (m_timeManager && m_timeManagerWasRunning) {
        printf("🕒 [OTA] 恢复时间管理任务...\n");
        if (m_timeManager->start()) {
            printf("✅ [OTA] 时间管理任务恢复成功\n");
        } else {
            printf("❌ [OTA] 时间管理任务恢复失败\n");
        }
    }
    
    // 恢复天气管理任务
    if (m_weatherManager && m_weatherManagerWasRunning) {
        printf("🌤️ [OTA] 恢复天气管理任务...\n");
        if (m_weatherManager->start()) {
            printf("✅ [OTA] 天气管理任务恢复成功\n");
        } else {
            printf("❌ [OTA] 天气管理任务恢复失败\n");
        }
    }
    
    // 恢复监控任务
    if (m_monitor && m_monitorWasRunning) {
        printf("📊 [OTA] 恢复监控任务...\n");
        // Monitor类没有start方法，需要调用init来重新启动
        printf("⚠️ [OTA] 监控任务需要手动重启\n");
    }
    
    // 显示管理任务保持运行，无需恢复
    if (m_displayManager) {
        printf("🖥️ [OTA] 显示管理任务一直保持运行，无需恢复\n");
        // 显示管理任务没有被停止，所以不需要恢复
    }
    
    printf("✅ [OTA] 系统任务恢复完成\n");
}

void OTAManager::updateScreenDisplay() {
    if (!m_displayManager) {
        return;  // 没有DisplayManager引用
    }
    
    // 转换OTA状态为数字形式
    int statusNum = 0;
    const char* statusText = "";
    
    switch (status) {
        case OTAStatus::IDLE:
            statusNum = 0;
            statusText = "Waiting for update";
            break;
        case OTAStatus::UPLOADING:
            statusNum = 1;
            statusText = "Uploading firmware";
            break;
        case OTAStatus::DOWNLOADING:
            statusNum = 2;
            statusText = "Downloading from server";
            break;
        case OTAStatus::WRITING:
            statusNum = 3;
            statusText = "Writing firmware";
            break;
        case OTAStatus::SUCCESS:
            statusNum = 4;
            statusText = "Update successful";
            break;
        case OTAStatus::FAILED:
            statusNum = 5;
            statusText = "Update failed";
            break;
    }
    
    // 获取当前进度
    float currentProgress = getProgress();
    
    // 更新屏幕显示
    m_displayManager->updateOTAStatus(
        statusNum,
        currentProgress,
        totalSize,
        writtenSize,
        statusText,
        (status == OTAStatus::FAILED && errorMessage.length() > 0) ? errorMessage.c_str() : nullptr
    );
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
/*
 * FileManager.cpp - SPIFFS文件管理器类实现文件
 * ESP32S3监控项目 - 文件管理模块
 */

#include "FileManager.h"
#include <time.h>

FileManager::FileManager() : initialized(false), formatting(false), formatResult(false), formatTaskHandle(nullptr) {
}

FileManager::~FileManager() {
    // 等待格式化任务完成
    if (formatTaskHandle != nullptr) {
        vTaskDelete(formatTaskHandle);
        formatTaskHandle = nullptr;
    }
    
    if (initialized) {
        SPIFFS.end();
    }
}

bool FileManager::init() {
    printf("初始化SPIFFS文件系统...\n");
    
    if (!SPIFFS.begin(true)) {
        printf("SPIFFS初始化失败\n");
        return false;
    }
    
    initialized = true;
    
    // 显示文件系统信息
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    
    printf("SPIFFS文件系统初始化成功\n");
    printf("总容量: %s\n", formatBytes(totalBytes).c_str());
    printf("已使用: %s\n", formatBytes(usedBytes).c_str());
    printf("可用空间: %s\n", formatBytes(totalBytes - usedBytes).c_str());
    printf("使用率: %.1f%%\n", getUsagePercent());
    
    return true;
}

bool FileManager::uploadFile(const String& path, const uint8_t* data, size_t size) {
    if (!initialized) {
        printf("文件系统未初始化\n");
        return false;
    }
    
    String sanitizedPath = sanitizePath(path);
    printf("上传文件: %s (%s)\n", sanitizedPath.c_str(), formatBytes(size).c_str());
    
    // 检查可用空间
    if (size > getFreeBytes()) {
        printf("可用空间不足\n");
        return false;
    }
    
    File file = SPIFFS.open(sanitizedPath, FILE_WRITE);
    if (!file) {
        printf("无法创建文件: %s\n", sanitizedPath.c_str());
        return false;
    }
    
    size_t written = file.write(data, size);
    file.close();
    
    if (written != size) {
        printf("写入文件失败，期望: %zu，实际: %zu\n", size, written);
        return false;
    }
    
    printf("文件上传成功: %s\n", sanitizedPath.c_str());
    return true;
}

bool FileManager::downloadFile(const String& path, uint8_t*& data, size_t& size) {
    if (!initialized) {
        printf("文件系统未初始化\n");
        return false;
    }
    
    String sanitizedPath = sanitizePath(path);
    
    if (!SPIFFS.exists(sanitizedPath)) {
        printf("文件不存在: %s\n", sanitizedPath.c_str());
        return false;
    }
    
    File file = SPIFFS.open(sanitizedPath, FILE_READ);
    if (!file) {
        printf("无法打开文件: %s\n", sanitizedPath.c_str());
        return false;
    }
    
    size = file.size();
    data = new uint8_t[size];
    
    size_t bytesRead = file.read(data, size);
    file.close();
    
    if (bytesRead != size) {
        delete[] data;
        data = nullptr;
        printf("读取文件失败\n");
        return false;
    }
    
    printf("文件下载成功: %s (%s)\n", sanitizedPath.c_str(), formatBytes(size).c_str());
    return true;
}

bool FileManager::deleteFile(const String& path) {
    if (!initialized) {
        printf("文件系统未初始化\n");
        return false;
    }
    
    String sanitizedPath = sanitizePath(path);
    
    if (!SPIFFS.exists(sanitizedPath)) {
        printf("文件不存在: %s\n", sanitizedPath.c_str());
        return false;
    }
    
    bool success = SPIFFS.remove(sanitizedPath);
    if (success) {
        printf("文件删除成功: %s\n", sanitizedPath.c_str());
    } else {
        printf("文件删除失败: %s\n", sanitizedPath.c_str());
    }
    
    return success;
}

bool FileManager::renameFile(const String& oldPath, const String& newPath) {
    if (!initialized) {
        printf("文件系统未初始化\n");
        return false;
    }
    
    String sanitizedOldPath = sanitizePath(oldPath);
    String sanitizedNewPath = sanitizePath(newPath);
    
    if (!SPIFFS.exists(sanitizedOldPath)) {
        printf("源文件不存在: %s\n", sanitizedOldPath.c_str());
        return false;
    }
    
    if (SPIFFS.exists(sanitizedNewPath)) {
        printf("目标文件已存在: %s\n", sanitizedNewPath.c_str());
        return false;
    }
    
    bool success = SPIFFS.rename(sanitizedOldPath, sanitizedNewPath);
    if (success) {
        printf("文件重命名成功: %s -> %s\n", sanitizedOldPath.c_str(), sanitizedNewPath.c_str());
    } else {
        printf("文件重命名失败: %s -> %s\n", sanitizedOldPath.c_str(), sanitizedNewPath.c_str());
    }
    
    return success;
}

std::vector<FileInfo> FileManager::listFiles(const String& path) {
    std::vector<FileInfo> files;
    
    if (!initialized) {
        printf("文件系统未初始化\n");
        return files;
    }
    
    String sanitizedPath = sanitizePath(path);
    
    File root = SPIFFS.open(sanitizedPath);
    if (!root) {
        printf("无法打开目录: %s\n", sanitizedPath.c_str());
        return files;
    }
    
    if (!root.isDirectory()) {
        printf("路径不是目录: %s\n", sanitizedPath.c_str());
        root.close();
        return files;
    }
    
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        size_t fileSize = file.size();
        bool isDir = file.isDirectory();
        time_t lastModified = file.getLastWrite();
        
        files.push_back(FileInfo(fileName, fileSize, isDir, lastModified));
        file = root.openNextFile();
    }
    
    root.close();
    
    printf("列出目录文件: %s (%zu个文件)\n", sanitizedPath.c_str(), files.size());
    return files;
}

bool FileManager::exists(const String& path) {
    if (!initialized) {
        return false;
    }
    
    String sanitizedPath = sanitizePath(path);
    return SPIFFS.exists(sanitizedPath);
}

size_t FileManager::getFileSize(const String& path) {
    if (!initialized) {
        return 0;
    }
    
    String sanitizedPath = sanitizePath(path);
    
    File file = SPIFFS.open(sanitizedPath, FILE_READ);
    if (!file) {
        return 0;
    }
    
    size_t size = file.size();
    file.close();
    return size;
}

FileInfo FileManager::getFileInfo(const String& path) {
    if (!initialized) {
        return FileInfo("", 0);
    }
    
    String sanitizedPath = sanitizePath(path);
    
    File file = SPIFFS.open(sanitizedPath, FILE_READ);
    if (!file) {
        return FileInfo("", 0);
    }
    
    String fileName = file.name();
    size_t fileSize = file.size();
    bool isDir = file.isDirectory();
    time_t lastModified = file.getLastWrite();
    
    file.close();
    
    return FileInfo(fileName, fileSize, isDir, lastModified);
}

size_t FileManager::getTotalBytes() {
    if (!initialized) {
        return 0;
    }
    return SPIFFS.totalBytes();
}

size_t FileManager::getUsedBytes() {
    if (!initialized) {
        return 0;
    }
    return SPIFFS.usedBytes();
}

size_t FileManager::getFreeBytes() {
    if (!initialized) {
        return 0;
    }
    return SPIFFS.totalBytes() - SPIFFS.usedBytes();
}

float FileManager::getUsagePercent() {
    if (!initialized) {
        return 0.0f;
    }
    
    size_t total = SPIFFS.totalBytes();
    if (total == 0) {
        return 0.0f;
    }
    
    return (float)SPIFFS.usedBytes() / (float)total * 100.0f;
}

String FileManager::readFileAsString(const String& path) {
    if (!initialized) {
        return "";
    }
    
    String sanitizedPath = sanitizePath(path);
    
    File file = SPIFFS.open(sanitizedPath, FILE_READ);
    if (!file) {
        printf("无法打开文件读取: %s\n", sanitizedPath.c_str());
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    return content;
}

bool FileManager::writeFileFromString(const String& path, const String& content) {
    if (!initialized) {
        return false;
    }
    
    String sanitizedPath = sanitizePath(path);
    
    // 检查可用空间
    if (content.length() > getFreeBytes()) {
        printf("可用空间不足\n");
        return false;
    }
    
    File file = SPIFFS.open(sanitizedPath, FILE_WRITE);
    if (!file) {
        printf("无法创建文件: %s\n", sanitizedPath.c_str());
        return false;
    }
    
    size_t written = file.print(content);
    file.close();
    
    if (written != content.length()) {
        printf("写入文件失败\n");
        return false;
    }
    
    printf("文件写入成功: %s (%s)\n", sanitizedPath.c_str(), formatBytes(content.length()).c_str());
    return true;
}

String FileManager::getFileSystemStatusJSON() {
    DynamicJsonDocument doc(512);
    
    doc["initialized"] = initialized;
    
    if (initialized) {
        doc["totalBytes"] = getTotalBytes();
        doc["usedBytes"] = getUsedBytes();
        doc["freeBytes"] = getFreeBytes();
        doc["usagePercent"] = getUsagePercent();
        doc["totalFormatted"] = formatBytes(getTotalBytes());
        doc["usedFormatted"] = formatBytes(getUsedBytes());
        doc["freeFormatted"] = formatBytes(getFreeBytes());
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

String FileManager::getFileListJSON(const String& path) {
    DynamicJsonDocument doc(2048);
    
    doc["path"] = path;
    doc["success"] = true;
    
    JsonArray files = doc.createNestedArray("files");
    
    std::vector<FileInfo> fileList = listFiles(path);
    for (const FileInfo& file : fileList) {
        JsonObject fileObj = files.createNestedObject();
        fileObj["name"] = file.name;
        fileObj["size"] = file.size;
        fileObj["sizeFormatted"] = formatBytes(file.size);
        fileObj["isDirectory"] = file.isDirectory;
        fileObj["lastModified"] = file.lastModified;
        fileObj["lastModifiedFormatted"] = formatTime(file.lastModified);
    }
    
    doc["fileCount"] = fileList.size();
    
    String response;
    serializeJson(doc, response);
    return response;
}

bool FileManager::startFormatFileSystem() {
    if (!initialized) {
        printf("文件系统未初始化\n");
        return false;
    }
    
    if (formatting) {
        printf("格式化操作正在进行中\n");
        return false;
    }
    
    printf("启动异步格式化任务...\n");
    formatting = true;
    formatResult = false;
    
    // 创建格式化任务
    BaseType_t result = xTaskCreate(
        formatTask,
        "FormatTask",
        4096,  // 栈大小
        this,  // 参数
        1,     // 低优先级，避免影响其他任务
        &formatTaskHandle
    );
    
    if (result == pdPASS) {
        printf("格式化任务创建成功\n");
        return true;
    } else {
        printf("格式化任务创建失败\n");
        formatting = false;
        return false;
    }
}

void FileManager::formatTask(void* parameter) {
    FileManager* manager = static_cast<FileManager*>(parameter);
    
    printf("格式化任务开始执行...\n");
    
    // 关闭SPIFFS以准备格式化
    SPIFFS.end();
    manager->initialized = false;
    
    // 添加延时，让出CPU时间，避免看门狗错误
    vTaskDelay(pdMS_TO_TICKS(200));
    
    printf("开始格式化SPIFFS文件系统（这将需要一些时间）...\n");
    
    // 执行格式化操作（耗时操作）
    bool success = SPIFFS.format();
    
    // 格式化完成后延时
    vTaskDelay(pdMS_TO_TICKS(200));
    
    if (success) {
        printf("文件系统格式化成功，重新初始化...\n");
        // 重新初始化SPIFFS
        manager->init();
        manager->formatResult = true;
    } else {
        printf("文件系统格式化失败，尝试重新初始化...\n");
        // 格式化失败，仍然尝试重新初始化
        manager->init();
        manager->formatResult = false;
    }
    
    // 标记格式化完成
    manager->formatting = false;
    manager->formatTaskHandle = nullptr;
    
    printf("格式化任务执行完成，结果: %s\n", success ? "成功" : "失败");
    
    // 任务自我删除
    vTaskDelete(nullptr);
}

String FileManager::formatBytes(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024.0, 1) + " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        return String(bytes / (1024.0 * 1024.0), 1) + " MB";
    } else {
        return String(bytes / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
    }
}

String FileManager::formatTime(time_t timestamp) {
    if (timestamp == 0) {
        return "未知";
    }
    
    struct tm* timeinfo = localtime(&timestamp);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
    return String(buffer);
}

bool FileManager::isValidPath(const String& path) {
    // 检查路径是否有效
    if (path.length() == 0) {
        return false;
    }
    
    // 检查危险字符
    if (path.indexOf("..") != -1) {
        return false;
    }
    
    return true;
}

String FileManager::sanitizePath(const String& path) {
    String cleanPath = path;
    
    // 确保路径以 / 开头
    if (!cleanPath.startsWith("/")) {
        cleanPath = "/" + cleanPath;
    }
    
    // 移除重复的 /
    while (cleanPath.indexOf("//") != -1) {
        cleanPath.replace("//", "/");
    }
    
    return cleanPath;
} 
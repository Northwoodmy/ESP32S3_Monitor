/*
 * FileManager.h - SPIFFS文件管理器类头文件
 * ESP32S3监控项目 - 文件管理模块
 */

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>

struct FileInfo {
    String name;
    size_t size;
    bool isDirectory;
    time_t lastModified;
    
    FileInfo(const String& n, size_t s, bool isDir = false, time_t modified = 0) 
        : name(n), size(s), isDirectory(isDir), lastModified(modified) {}
};

class FileManager {
public:
    FileManager();
    ~FileManager();
    
    // 初始化SPIFFS文件系统
    bool init();
    
    // 获取文件系统状态
    bool isReady() const { return initialized; }
    
    // 文件操作
    bool uploadFile(const String& path, const uint8_t* data, size_t size);
    bool downloadFile(const String& path, uint8_t*& data, size_t& size);
    bool deleteFile(const String& path);
    bool renameFile(const String& oldPath, const String& newPath);
    
    // 目录操作
    bool createDirectory(const String& path);
    bool deleteDirectory(const String& path);
    std::vector<FileInfo> listFiles(const String& path = "/");
    
    // 文件信息
    bool exists(const String& path);
    size_t getFileSize(const String& path);
    FileInfo getFileInfo(const String& path);
    
    // 文件系统信息
    size_t getTotalBytes();
    size_t getUsedBytes();
    size_t getFreeBytes();
    float getUsagePercent();
    
    // 读取文件内容为字符串（适用于文本文件）
    String readFileAsString(const String& path);
    bool writeFileFromString(const String& path, const String& content);
    
    // 获取文件系统状态JSON
    String getFileSystemStatusJSON();
    String getFileListJSON(const String& path = "/");
    
    // 格式化文件系统（异步）
    bool startFormatFileSystem();
    bool isFormatting() const { return formatting; }
    bool getFormatResult() const { return formatResult; }
    
    // 路径处理工具（公有方法）
    String sanitizePath(const String& path);

private:
    bool initialized;
    bool formatting;
    bool formatResult;
    TaskHandle_t formatTaskHandle;
    
    // 工具函数
    String formatBytes(size_t bytes);
    String formatTime(time_t timestamp);
    bool isValidPath(const String& path);
    
    // 格式化任务
    static void formatTask(void* parameter);
};

#endif // FILEMANAGER_H 
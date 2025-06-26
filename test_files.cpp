/*
 * test_files.cpp - 测试文件，用于创建SPIFFS示例文件
 * 可以在setup()中调用这些函数来创建示例文件
 */

#include "FileManager.h"

void createTestFiles(FileManager* fileManager) {
    if (!fileManager->isReady()) {
        printf("文件系统未初始化，无法创建测试文件\n");
        return;
    }
    
    printf("创建测试文件...\n");
    
    // 创建一个配置文件
    String configContent = R"({
  "device_name": "ESP32S3_Monitor",
  "version": "v3.3.0",
  "features": [
    "WiFi Management",
    "OTA Updates", 
    "File Management",
    "System Monitoring"
  ],
  "settings": {
    "max_wifi_configs": 3,
    "auto_reconnect": true,
    "debug_mode": false
  }
})";
    
    if (fileManager->writeFileFromString("/config.json", configContent)) {
        printf("✅ 创建配置文件: /config.json\n");
    }
    
    // 创建一个说明文件
    String readmeContent = R"(# ESP32S3 监控项目

这是一个基于ESP32S3开发的WiFi配置管理器项目。

## 主要功能

- WiFi自动配置和管理
- 现代化Web配置界面  
- NVS存储WiFi信息
- 系统状态监控
- OTA固件升级
- SPIFFS文件管理

## 版本信息

当前版本: v3.3.0

## 文件管理

你现在正在使用新增的文件管理功能！
可以通过Web界面管理SPIFFS分区中的文件。

支持的操作：
- 上传文件
- 下载文件  
- 删除文件
- 重命名文件
- 创建新文件
- 查看文件系统状态

## 注意事项

- 文件大小不能超过可用空间
- 建议定期备份重要文件
- 格式化操作将删除所有文件，请谨慎使用
)";
    
    if (fileManager->writeFileFromString("/README.md", readmeContent)) {
        printf("✅ 创建说明文件: /README.md\n");
    }
    
    // 创建一个简单的HTML文件
    String htmlContent = R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>欢迎页面</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 40px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            text-align: center;
        }
        .container {
            background: rgba(255,255,255,0.1);
            padding: 30px;
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎉 欢迎使用文件管理器！</h1>
        <p>这是一个存储在SPIFFS中的HTML文件。</p>
        <p>你可以通过文件管理器上传、编辑和管理各种文件。</p>
        <h2>✨ 功能特色</h2>
        <ul style="text-align: left; display: inline-block;">
            <li>支持多种文件类型</li>
            <li>拖拽上传</li>
            <li>实时状态显示</li>
            <li>批量文件操作</li>
        </ul>
    </div>
</body>
</html>)";
    
    if (fileManager->writeFileFromString("/welcome.html", htmlContent)) {
        printf("✅ 创建HTML文件: /welcome.html\n");
    }
    
    // 创建一个日志文件
    String logContent = R"(ESP32S3 Monitor - 系统日志
========================================

2024-01-01 00:00:00 - 系统启动
2024-01-01 00:00:01 - WiFi管理器初始化完成
2024-01-01 00:00:02 - Web服务器启动
2024-01-01 00:00:03 - OTA管理器就绪
2024-01-01 00:00:04 - 文件管理器初始化完成
2024-01-01 00:00:05 - 所有模块启动完成

文件管理器功能测试:
- 创建测试文件: ✅
- 文件读写操作: ✅  
- Web界面访问: ✅
- 文件上传下载: ✅

系统运行正常。
)";
    
    if (fileManager->writeFileFromString("/system.log", logContent)) {
        printf("✅ 创建日志文件: /system.log\n");
    }
    
    printf("测试文件创建完成！\n");
    printf("你现在可以通过Web界面访问文件管理器查看这些文件。\n");
} 
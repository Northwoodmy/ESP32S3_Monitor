# 版本号统一管理系统

## 概述

本项目现已实现版本号统一管理，通过 `Version.h` 头文件集中管理所有版本信息。每次版本更新时，只需修改一个文件即可。

## 版本号文件结构

```
Version.h                    # 统一版本号管理文件（核心文件）
├── VERSION_STRING          # 主版本号字符串 (如 "v7.1.12")
├── VERSION_MAJOR          # 主版本号 (如 7)
├── VERSION_MINOR          # 次版本号 (如 1)  
├── VERSION_PATCH          # 补丁版本号 (如 12)
├── VERSION_INFO           # 版本信息字符串
├── FULL_VERSION_INFO      # 完整版本信息（含编译时间）
├── BUILD_DATE             # 编译日期
└── BUILD_TIME             # 编译时间
```

## 如何更新版本号

### 1. 修改版本号（推荐方式）

只需要修改 `Version.h` 文件中的版本号定义：

```cpp
// 项目版本号定义 - 每次更新只需修改这里
#define VERSION_STRING "v7.1.21"  // 修改这里即可

// 同时更新数字版本（可选，用于版本比较）
#define VERSION_MAJOR 7
#define VERSION_MINOR 1  
#define VERSION_PATCH 13
```

### 2. 自动生效的位置

修改 `Version.h` 后，以下位置会自动使用新版本号：

- ✅ **主程序启动信息** (`ESP32S3_Monitor.ino`)
- ✅ **Web界面系统信息** (`WebServerManager.cpp`)
- ✅ **OTA升级页面显示** (`WebServerManager_OTA.cpp`) 
- ✅ **API接口返回** (所有版本相关API)

## 使用方法

### 在新文件中使用版本号

1. **包含头文件**：
```cpp
#include "Version.h"
```

2. **使用版本号宏**：
```cpp
// 基本版本号
printf("当前版本: %s\n", VERSION_STRING);

// 完整版本信息
printf("版本信息: %s\n", FULL_VERSION_INFO);

// 单独的版本组件
printf("主版本: %d, 次版本: %d, 补丁: %d\n", 
       VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
```

### 在Web页面中使用

```cpp
// 在HTML中嵌入版本号
html += "<p>版本: " + String(VERSION_STRING) + "</p>";

// 在JSON API中返回版本号
doc["version"] = VERSION_STRING;
doc["buildInfo"] = FULL_VERSION_INFO;
```

## 版本号格式说明

### 推荐格式
- **主版本号**: `v7.1.12` （主要功能更新）
- **次版本号**: `v7.2.0` （新功能添加）  
- **补丁版本号**: `v7.1.21` （Bug修复）

### 版本组件说明
- **MAJOR**: 主版本号，重大架构变更或不兼容更新
- **MINOR**: 次版本号，新功能添加但保持向后兼容
- **PATCH**: 补丁版本号，Bug修复和小改进

## 优势

### 🎯 **维护简单**
- 版本更新只需修改一个文件
- 避免版本号不一致的问题
- 减少维护工作量

### 🔄 **自动同步**  
- 所有模块自动使用最新版本号
- 编译时确定版本信息
- 无需手动同步多个文件

### 📊 **功能丰富**
- 支持版本号比较
- 包含编译时间信息
- 提供多种版本格式

### 🛡️ **错误预防**
- 避免版本号遗漏更新
- 确保全项目版本一致性
- 减少发布时的版本错误

## 注意事项

1. **每次版本更新**：只修改 `Version.h` 中的 `VERSION_STRING`
2. **版本格式一致**：建议保持 "vX.Y.Z" 格式
3. **编译依赖**：修改版本号后需要重新编译整个项目
4. **版本同步**：确保 `version.json` 文件也同步更新（用于版本记录）

## 未来扩展

可以进一步扩展版本管理功能：

- 添加Git提交哈希值
- 支持预发布版本标记（如 v7.1.12-beta）
- 自动化版本号生成脚本
- 版本变更历史追踪 
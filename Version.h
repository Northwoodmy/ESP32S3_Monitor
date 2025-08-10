/*
 * ESP32S3监控项目 - 版本号统一管理
 * 文件: Version.h
 * 作者: ESP32S3_Monitor
 * 日期: 2025-01-23
 * 
 * 说明: 
 * - 统一管理项目版本号，避免多处修改
 * - 每次版本更新只需修改此文件中的VERSION_STRING
 * - 所有模块通过包含此头文件获取统一版本号
 */

#ifndef VERSION_H
#define VERSION_H

// 项目版本号定义 - 每次更新只需修改这里
#define VERSION_STRING "v7.5.15"

// 版本号组件分解（可选，用于版本比较等高级功能）
#define VERSION_MAJOR 7
#define VERSION_MINOR 5
#define VERSION_PATCH 15

// 版本信息字符串（包含更多详细信息）
#define VERSION_INFO VERSION_STRING " - ESP32S3监控项目"

// 编译信息
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// 完整版本信息字符串
#define FULL_VERSION_INFO VERSION_STRING " (编译: " BUILD_DATE " " BUILD_TIME ")"

#endif // VERSION_H 
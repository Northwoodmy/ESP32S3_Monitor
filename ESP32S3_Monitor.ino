/*
 * ESP32S3监控项目 - WiFi配置管理器
 * 版本: v5.3.0
 * 作者: ESP32S3_Monitor
 * 日期: 2024
 * 
 * 功能说明:
 * - WiFi自动配置和管理
 * - 用户自定义优先级设置
 * - 现代化Web配置界面
 * - NVS存储WiFi信息
 * - 系统状态监控
 * - 配置重置功能
 * - FreeRTOS多任务架构
 * - 模块化C++设计
 * - LVGL显示驱动和触控按钮
 * - PSRAM内存管理和优化
 * - NTP网络时间同步功能
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "Monitor.h"
#include "ConfigStorage.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "OTAManager.h"
#include "FileManager.h"
#include "LVGL_Driver.h"
#include "DisplayManager.h"
#include "PSRAMManager.h"
#include "TimeManager.h"
#include "I2CBusManager.h"

// 外部变量声明
extern LVGLDriver* lvglDriver;

// LVGL驱动实例
LVGLDriver lvglDriverInstance;

// 全局实例
Monitor monitor;
ConfigStorage configStorage;
WiFiManager wifiManager;
WebServerManager* webServerManager;
OTAManager otaManager;
FileManager fileManager;
DisplayManager displayManager;
PSRAMManager psramManager;
TimeManager timeManager;

// 传感器数据已集成到LVGL驱动中，无需独立任务

void setup() {
  
  printf("=== ESP32S3 WiFi配置管理器启动 ===\n");
  printf("版本: v5.3.0\n");
  printf("编译时间: %s %s\n", __DATE__, __TIME__);
  
  // 初始化PSRAM管理器（优先初始化）
  printf("\n初始化PSRAM管理器...\n");
  if (psramManager.init()) {
    psramManager.start();
    psramManager.setDebugMode(false); // 生产环境关闭调试模式
    printf("PSRAM管理器初始化成功\n");
    psramManager.printStatistics();
  } else {
    printf("PSRAM管理器初始化失败，继续使用内部RAM\n");
  }
  
  // 初始化配置存储
  printf("\n初始化系统组件...\n");
  configStorage.init();
  
  // 启动配置存储任务
  printf("启动配置存储任务...\n");
  if (!configStorage.startTask()) {
    printf("❌ 配置存储任务启动失败\n");
  } else {
    printf("✅ 配置存储任务启动成功\n");
  }
  
  // 初始化WiFi管理器
  wifiManager.setPSRAMManager(&psramManager);
  wifiManager.init(&configStorage);
  
  // 初始化I2C总线管理器（在所有I2C设备初始化之前）
  printf("初始化I2C总线管理器...\n");
  esp_err_t i2c_ret = I2CBus_Init();
  if (i2c_ret != ESP_OK) {
    printf("❌ I2C总线管理器初始化失败，错误码: 0x%x\n", i2c_ret);
  } else {
    printf("✅ I2C总线管理器初始化成功\n");
  }
  
  // 初始化LVGL驱动系统
  printf("开始LVGL驱动系统初始化...\n");
  lvglDriverInstance.init();
  
  // 启动LVGL驱动任务  
  printf("启动LVGL驱动任务...\n");
  lvglDriverInstance.start();
  
  printf("LVGL驱动系统初始化完成\n");

  // 初始化显示管理器
  //printf("开始初始化显示管理器...\n");
  //displayManager.init(&lvglDriverInstance, &wifiManager, &configStorage, &psramManager);
  
  // 启动显示管理器任务
  //printf("启动显示管理器任务...\n");
  //displayManager.start();
  
  printf("显示管理器初始化完成\n");

  // 初始化OTA管理器
  otaManager.init();
  
  // 初始化文件管理器
  fileManager.init();
  
  // 创建Web服务器管理器实例
  webServerManager = new WebServerManager(&wifiManager, &configStorage, &otaManager, &fileManager);
  
  // 初始化并启动Web服务器
  webServerManager->setPSRAMManager(&psramManager);
  webServerManager->setDisplayManager(&displayManager);
  webServerManager->init();
  webServerManager->start();
  
  // 初始化监控器（Hello World任务）
  monitor.init(&psramManager);
  
  // 初始化时间管理器
  printf("开始初始化时间管理器...\n");
  timeManager.init(&psramManager, &wifiManager, &configStorage);
  
  // 启动时间管理器任务
  printf("启动时间管理器任务...\n");
  timeManager.start();  
  
  // 启用时间管理器调试模式，显示详细同步信息
  timeManager.setDebugMode(true);
  
  printf("时间管理器初始化完成\n");
  
  // 显示当前状态
  vTaskDelay(pdMS_TO_TICKS(2000));
  displaySystemStatus();
  
  // 显示PSRAM详细使用分析
  printf("\n=== PSRAM详细使用分析 ===\n");
  if (psramManager.isPSRAMAvailable()) {
    psramManager.printMemoryMap();
  }
  
  // 传感器数据已集成到LVGL驱动的屏幕自动旋转功能中
  
  printf("=== 系统初始化完成 ===\n");
}

void loop() {
  // 根据规则，loop()不做任何任务处理
  // 所有功能通过FreeRTOS任务实现
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void displaySystemStatus() {
  printf("\n=== 系统状态信息 ===\n");
  
  // 分区信息
  printf("=== Flash分区信息 ===\n");
  const esp_partition_t* app0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  const esp_partition_t* app1 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
  const esp_partition_t* running = esp_ota_get_running_partition();
  
  if (app0 && app1) {
    printf("✅ OTA分区配置正确\n");
    printf("APP0 分区: %u MB (0x%x - 0x%x)\n", app0->size / (1024*1024), app0->address, app0->address + app0->size);
    printf("APP1 分区: %u MB (0x%x - 0x%x)\n", app1->size / (1024*1024), app1->address, app1->address + app1->size);
    printf("当前运行: %s\n", running ? running->label : "未知");
    printf("可用固件空间: %u MB\n", ESP.getFreeSketchSpace() / (1024*1024));
  } else {
    printf("❌ OTA分区配置错误或缺失\n");
  }
  printf("==================\n");
  
  // WiFi状态
  if (wifiManager.isConnected()) {
    printf("WiFi状态: 已连接\n");
    printf("SSID: %s\n", WiFi.SSID().c_str());
    printf("IP地址: %s\n", wifiManager.getLocalIP().c_str());
    printf("信号强度: %d dBm\n", WiFi.RSSI());
  } else {
    printf("WiFi状态: AP配置模式\n");
    printf("AP SSID: ESP32S3-Config\n");
    printf("AP密码: 12345678\n");
    printf("配置IP: %s\n", wifiManager.getAPIP().c_str());
    printf("Web配置地址: http://%s\n", wifiManager.getAPIP().c_str());
  }
  
  // 系统信息
  printf("芯片型号: %s Rev.%d\n", ESP.getChipModel(), ESP.getChipRevision());
  printf("CPU频率: %d MHz\n", ESP.getCpuFreqMHz());
  printf("Flash大小: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  printf("内部RAM: %d KB 可用, %d KB 总计\n", ESP.getFreeHeap() / 1024, ESP.getHeapSize() / 1024);
  
  // PSRAM信息
  if (psramManager.isPSRAMAvailable()) {
    printf("PSRAM大小: %d MB\n", psramManager.getTotalSize() / (1024 * 1024));
    printf("PSRAM可用: %d KB (%.1f%%)\n", 
           psramManager.getFreeSize() / 1024, 
           100.0f - psramManager.getUsagePercent());
    printf("PSRAM已用: %d KB (%.1f%%)\n", 
           psramManager.getUsedSize() / 1024, 
           psramManager.getUsagePercent());
    printf("PSRAM碎片率: %.1f%%\n", psramManager.getFragmentationRate());
    printf("PSRAM分配块: %u个\n", psramManager.getBlockCount());
  } else {
    printf("PSRAM: 未检测到或未启用\n");
  }
  
  // 时间信息
  printf("时间状态: ");
  if (timeManager.isTimeValid()) {
    printf("已同步\n");
    printf("当前时间: %s\n", timeManager.getDateTimeString().c_str());
    printf("时区: UTC%+.1f\n", timeManager.getTimezoneOffset());
    printf("同步状态: ");
    switch (timeManager.getSyncStatus()) {
      case TIME_SYNCED: printf("正常\n"); break;
      case TIME_SYNCING: printf("同步中\n"); break;
      case TIME_NOT_SYNCED: printf("未同步\n"); break;
      case TIME_SYNC_FAILED: printf("同步失败\n"); break;
    }
  } else {
    printf("未同步\n");
    printf("NTP服务器: %s\n", timeManager.getNTPConfig().primaryServer.c_str());
  }
  
  printf("==================\n");
} 
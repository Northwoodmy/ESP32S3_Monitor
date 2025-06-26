/*
 * ESP32S3监控项目 - WiFi配置管理器
 * 版本: v3.0.13
 * 作者: ESP32S3_Monitor
 * 日期: 2024
 * 
 * 功能说明:
 * - WiFi自动配置和管理
 * - 现代化Web配置界面
 * - NVS存储WiFi信息
 * - 系统状态监控
 * - 配置重置功能
 * - FreeRTOS多任务架构
 * - 模块化C++设计
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

// 全局实例
Monitor monitor;
ConfigStorage configStorage;
WiFiManager wifiManager;
WebServerManager* webServerManager;
OTAManager otaManager;
FileManager fileManager;

void setup() {
  
      printf("=== ESP32S3 WiFi配置管理器启动 ===\n");
    printf("版本: v3.3.13\n");
    printf("编译时间: %s %s\n", __DATE__, __TIME__);
  
  // 初始化配置存储
  printf("\n初始化系统组件...\n");
  configStorage.init();
  
  // 初始化WiFi管理器
  wifiManager.init();
  
  // 初始化OTA管理器
  otaManager.init();
  
  // 初始化文件管理器
  fileManager.init();
  
  // 创建Web服务器管理器实例
  webServerManager = new WebServerManager(&wifiManager, &configStorage, &otaManager, &fileManager);
  
  // 初始化并启动Web服务器
  webServerManager->init();
  webServerManager->start();
  
  // 初始化监控器（Hello World任务）
  monitor.init();
  
  // 显示当前状态
  vTaskDelay(pdMS_TO_TICKS(2000));
  displaySystemStatus();
  
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
  printf("可用内存: %d KB\n", ESP.getFreeHeap() / 1024);
  printf("总内存: %d KB\n", ESP.getHeapSize() / 1024);
  
  printf("==================\n");
} 
/*
 * ESP32S3监控项目 - WiFi配置管理器
 * 版本: v7.0.0
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
 * - AudioManager音频播放管理
 * - PCM音频文件循环播放功能
 * - I2S音频接口和ES8311编解码器集成
 * - WeatherManager天气管理系统
 * - 高德天气API集成和数据解析
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
#include "AudioManager.h"
#include "WeatherManager.h"
#include "PowerMonitorData.h"

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
AudioManager audioManager;
WeatherManager weatherManager;

// 传感器数据已集成到LVGL驱动中，无需独立任务

// 功率数据回调函数
void powerDataCallback(const PowerMonitorData& data, void* userData) {
  DisplayManager* displayManager = (DisplayManager*)userData;
  if (displayManager) {
    displayManager->updatePowerData(data);
  }
}

void setup() {
  
  printf("=== ESP32S3 WiFi配置管理器启动 ===\n");
  printf("版本: v7.0.0 - 全新SquareLine Studio UI系统\n");
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

  // 初始化OTA管理器
  otaManager.init();
  
  // 初始化文件管理器
  fileManager.init();
  
  // 初始化天气管理器（需要在DisplayManager之前初始化）
  printf("开始初始化天气管理器...\n");
  if (weatherManager.init(&psramManager, &wifiManager, &configStorage)) {
    printf("✅ 天气管理器初始化成功\n");
    
    // 启动天气管理器
    if (weatherManager.start()) {
      printf("✅ 天气管理器启动成功\n");
      
      // 启用调试模式查看详细信息
      weatherManager.setDebugMode(true);
      
      printf("🌤️ 天气管理器配置完成\n");
      printf("💡 请在Web界面设置高德天气API密钥以启用天气功能\n");
      printf("🔗 高德开发者平台: https://console.amap.com/\n");
      
      // 打印天气配置信息
      weatherManager.printConfig();
    } else {
      printf("❌ 天气管理器启动失败\n");
    }
  } else {
    printf("❌ 天气管理器初始化失败\n");
  }
  printf("天气管理器初始化完成\n");

  // 初始化显示管理器（现在包含WeatherManager）
  printf("开始初始化显示管理器...\n");
  displayManager.init(&lvglDriverInstance, &wifiManager, &configStorage, &psramManager, &weatherManager);
  
  // 启动显示管理器任务
  printf("启动显示管理器任务...\n");
  displayManager.start();
  
  // 设置触摸活动回调，将触摸事件传递给DisplayManager
  printf("设置触摸活动回调...\n");
  lvglDriverInstance.setTouchActivityCallback([](void* userdata) {
    DisplayManager* dm = static_cast<DisplayManager*>(userdata);
    if (dm) {
      dm->notifyTouchActivity();
    }
  }, &displayManager);
  
  printf("显示管理器初始化完成\n");
  
  // 初始化音频管理器
  printf("开始初始化音频管理器...\n");
  if (audioManager.init(&psramManager, &fileManager)) {
    printf("✅ 音频管理器初始化成功\n");
    
    // 启动音频管理器
    if (audioManager.start()) {
      printf("✅ 音频管理器启动成功\n");
      
      // 设置初始音量为80%
      audioManager.setVolume(70);
      
      // 启用调试模式查看详细信息
      audioManager.setDebugMode(true);
      
      // 设置正确的音频格式匹配转换后的PCM文件
      printf("🎯 音频文件已成功转换！\n");
      printf("原文件: c3.pcm (ADPCM格式) -> c3_new.pcm (标准PCM)\n");
      printf("格式: 16000Hz, 单声道, 16位, 时长3.09秒\n\n");
      
      // 设置匹配的音频格式
      if (audioManager.setAudioFormat(16000, 1, 16)) {
        printf("✅ 音频格式设置成功: 16000Hz, 单声道, 16位\n");
        
        // 开始播放转换后的PCM文件，使用单次播放模式（播放一次后停止）
        if (audioManager.playPCMFile("/c3.pcm", AUDIO_MODE_ONCE)) {
          printf("🎵 开始播放音频文件: /c3.pcm (单次播放)\n");
          printf("🎧 音频应该清晰无杂音，3.09秒时长\n");
          printf("💡 请确保SPIFFS中已上传 c3_new.pcm 文件\n");
        } else {
          printf("⚠️ 音频文件播放启动失败，请检查文件是否存在\n");
          printf("💡 需要将 c3_new.pcm 上传到SPIFFS文件系统\n");
        }
      } else {
        printf("❌ 音频格式设置失败\n");
      }
    } else {
      printf("❌ 音频管理器启动失败\n");
    }
  } else {
    printf("❌ 音频管理器初始化失败\n");
  }
  printf("音频管理器初始化完成\n");
  
  // 创建Web服务器管理器实例
  webServerManager = new WebServerManager(&wifiManager, &configStorage, &otaManager, &fileManager);
  
  // 初始化并启动Web服务器
  webServerManager->setPSRAMManager(&psramManager);
  webServerManager->setDisplayManager(&displayManager);
  webServerManager->setWeatherManager(&weatherManager);
  webServerManager->init();
  webServerManager->start();
  
  // 初始化监控器（Hello World任务）
  monitor.init(&psramManager, &configStorage);
  
  // 设置功率数据回调，将Monitor的数据传递给DisplayManager
  monitor.setPowerDataCallback(powerDataCallback, &displayManager);
  printf("功率数据回调已设置\n");
  
  // 加载并应用屏幕设置配置
  printf("加载屏幕设置配置...\n");
  if (displayManager.loadScreenModeConfig()) {
    printf("屏幕设置配置加载成功\n");
  } else {
    printf("屏幕设置配置加载失败，将使用默认配置\n");
  }
  
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
  
  // 音频系统信息
  printf("音频状态: ");
  if (audioManager.isPlaying()) {
    printf("播放中\n");
    printf("当前文件: %s\n", audioManager.getCurrentFile().c_str());
    printf("音量: %d%%\n", audioManager.getVolume());
    printf("静音: %s\n", audioManager.isMuted() ? "是" : "否");
    
    AudioStatistics stats = audioManager.getStatistics();
    printf("播放统计: 总计%lu次, 成功%lu次, 错误%lu次\n", 
           stats.totalPlayCount, stats.successPlayCount, stats.errorCount);
    printf("播放字节: %zu bytes\n", stats.bytesPlayed);
  } else {
    AudioState state = audioManager.getState();
    switch (state) {
      case AUDIO_STATE_IDLE: printf("空闲\n"); break;
      case AUDIO_STATE_STOPPED: printf("已停止\n"); break;
      case AUDIO_STATE_PAUSED: printf("已暂停\n"); break;
      case AUDIO_STATE_ERROR: printf("错误\n"); break;
      default: printf("未知状态\n"); break;
    }
  }
  
  // 天气系统信息
  printf("天气状态: %s\n", weatherManager.getStateString().c_str());
  if (weatherManager.isWeatherDataValid()) {
    WeatherData weather = weatherManager.getCurrentWeather();
    printf("当前天气: %s\n", weather.city.c_str());
    printf("天气现象: %s\n", weather.weather.c_str());
    printf("温度: %s°C\n", weather.temperature.c_str());
    printf("湿度: %s%%\n", weather.humidity.c_str());
    printf("风向风力: %s %s级\n", weather.winddirection.c_str(), weather.windpower.c_str());
    printf("发布时间: %s\n", weather.reporttime.c_str());
  } else {
    printf("天气数据: 未获取\n");
    WeatherConfig config = weatherManager.getConfig();
    if (config.apiKey.isEmpty()) {
      printf("提示: 请设置高德API密钥\n");
    } else {
      printf("城市: %s (%s)\n", config.cityName.c_str(), config.cityCode.c_str());
    }
  }
  
  WeatherStatistics weatherStats = weatherManager.getStatistics();
  if (weatherStats.totalRequests > 0) {
    printf("天气统计: 总计%lu次, 成功%lu次, 失败%lu次\n", 
           weatherStats.totalRequests, weatherStats.successRequests, weatherStats.failedRequests);
  }
  
  printf("==================\n");
} 
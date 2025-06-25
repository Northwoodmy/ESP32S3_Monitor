/*
 * ESP32S3监控项目 - Hello World输出
 * 版本: v1.0.0
 * 作者: ESP32S3_Monitor
 * 日期: 2024
 * 
 * 功能说明:
 * - 每秒输出一次"hello world"
 * - 使用FreeRTOS任务实现
 * - 模块化设计
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Monitor.h"

// 全局监控器实例
Monitor monitor;

void setup() {
  // 初始化串口用于调试输出
  Serial.begin(115200);
  
  // 等待串口连接
  delay(1000);
  
  printf("ESP32S3监控项目启动中...\n");
  
  // 初始化监控器
  monitor.init();
  
  printf("系统初始化完成\n");
}

void loop() {
  // 根据规则，loop()不做任何任务处理
  // 所有功能通过FreeRTOS任务实现
  vTaskDelay(pdMS_TO_TICKS(1000));
} 
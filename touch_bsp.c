/*
 * 触摸屏板级支持包(BSP) - FT3168控制器
 * 
 * 功能描述：
 * - 实现FT3168电容触摸控制器的I2C通信接口
 * - 提供触摸状态检测和坐标读取功能
 * - 支持单点触摸检测
 * - 适配368x448分辨率的LCD显示屏
 * 
 * 硬件连接：
 * - SCL: GPIO_14 (I2C时钟线)
 * - SDA: GPIO_15 (I2C数据线)  
 * - INT: GPIO_21 (中断引脚，可选)
 * - RST: 未使用
 */

#include <stdio.h>
#include <stdlib.h>
#include "touch_bsp.h"           // 触摸BSP头文件
#include "I2CBusManager.h"       // 使用统一的I2C总线管理器
#include "esp_log.h"             // ESP日志系统

// === 硬件配置定义 ===
#define EXAMPLE_PIN_NUM_TOUCH_INT (GPIO_NUM_21)  // 中断引脚

// === 显示屏参数定义 ===
#define EXAMPLE_LCD_H_RES  368                   // LCD水平分辨率
#define EXAMPLE_LCD_V_RES  448                   // LCD垂直分辨率

// === FT3168触摸控制器参数 ===
#define I2C_ADDR_FT3168 0x38                     // FT3168的I2C设备地址

// === 日志标签 ===
static const char *TAG = "TOUCH_BSP";

// === 内部函数声明 ===
uint8_t I2C_writr_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t I2C_read_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t I2C_master_write_read_device(uint8_t addr, uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen);

/**
 * @brief 初始化触摸屏接口
 * 
 * 使用统一的I2CBusManager进行I2C通信，无需重复配置I2C总线。
 * 将触摸控制器切换到正常工作模式。
 */
void Touch_Init(void)
{
  ESP_LOGI(TAG, "触摸屏初始化开始，使用I2CBusManager统一管理");
  
  // === 检查I2C总线管理器是否已初始化 ===
  if (!I2CBus_IsInitialized()) {
    ESP_LOGE(TAG, "I2C总线管理器未初始化，请先调用I2CBus_Init()");
    return;
  }
  ESP_LOGI(TAG, "✓ I2C总线管理器已初始化，可以进行通信");
  
  // === 验证FT3168设备连接 ===
  if (!I2CBus_ScanDevice(I2C_ADDR_FT3168)) {
    ESP_LOGE(TAG, "FT3168触摸控制器检测失败，地址: 0x%02X", I2C_ADDR_FT3168);
    return;
  }
  ESP_LOGI(TAG, "✓ FT3168触摸控制器检测成功");

  // === 初始化FT3168触摸控制器 ===
  uint8_t data = 0x00;
  uint8_t write_data[2] = {0x00, data};  // 寄存器地址 + 数据
  esp_err_t ret = I2CBus_Write(I2C_ADDR_FT3168, write_data, 2, 1000);
  
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "✓ FT3168触摸控制器初始化成功");
  } else {
    ESP_LOGE(TAG, "FT3168触摸控制器初始化失败: %s", esp_err_to_name(ret));
  }
  
  ESP_LOGI(TAG, "触摸屏初始化完成（使用I2CBusManager）");
}

/**
 * @brief 获取I2C总线互斥锁（兼容接口，转发到I2CBusManager）
 * 
 * @param timeout_ms 超时时间（毫秒）
 * @return true 成功获取锁，false 获取锁失败
 */
bool I2C_Lock(uint32_t timeout_ms)
{
  // 转发到I2CBusManager
  return I2CBus_Lock(timeout_ms);
}

/**
 * @brief 释放I2C总线互斥锁（兼容接口，转发到I2CBusManager）
 */
void I2C_Unlock(void)
{
  // 转发到I2CBusManager
  I2CBus_Unlock();
}

/**
 * @brief 获取I2C互斥锁句柄（兼容接口，转发到I2CBusManager）
 * 
 * @return I2C互斥锁句柄
 */
SemaphoreHandle_t I2C_GetMutex(void)
{
  return I2CBus_GetMutex();
}

/**
 * @brief 读取触摸状态和坐标
 * 
 * 检测是否有触摸事件发生，如果有触摸则读取触摸坐标。
 * FT3168的坐标数据格式：
 * - 寄存器0x02: 触摸点数量
 * - 寄存器0x03-0x06: 第一个触摸点的坐标数据
 *   - 0x03: Y坐标高4位 + 标志位
 *   - 0x04: Y坐标低8位
 *   - 0x05: X坐标高4位 + 标志位  
 *   - 0x06: X坐标低8位
 * 
 * @param x 返回的X坐标指针
 * @param y 返回的Y坐标指针
 * @return 1 有触摸事件，0 无触摸事件
 */
uint8_t getTouch(uint16_t *x, uint16_t *y)
{
  uint8_t data;     // 触摸点数量
  uint8_t buf[4];   // 坐标数据缓冲区
  uint8_t result = 0;

  // === 1. 读取触摸点数量 ===
  esp_err_t ret = I2CBus_WriteRead(I2C_ADDR_FT3168, (uint8_t[]){0x02}, 1, &data, 1, 100);
  if (ret == ESP_OK && data > 0) {  // 如果有触摸点
    // === 2. 读取第一个触摸点的坐标数据 ===
    ret = I2CBus_WriteRead(I2C_ADDR_FT3168, (uint8_t[]){0x03}, 1, buf, 4, 100);
    if (ret == ESP_OK) {
      // === 3. 解析坐标数据 ===
      // Y坐标 = (buf[0]低4位 << 8) | buf[1]
      *y = (((uint16_t)buf[0] & 0x0f) << 8) | (uint16_t)buf[1];
      
      // X坐标 = (buf[2]低4位 << 8) | buf[3]  
      *x = (((uint16_t)buf[2] & 0x0f) << 8) | (uint16_t)buf[3];
      
      // 坐标范围检查和转换（已注释掉的代码，可根据需要启用）
      // if(*x > EXAMPLE_LCD_H_RES)
      // *x = EXAMPLE_LCD_H_RES;
      // if(*y > EXAMPLE_LCD_V_RES)
      // *y = EXAMPLE_LCD_V_RES;
      // *y = EXAMPLE_LCD_V_RES - *y;  // Y轴翻转
      
      result = 1;  // 返回有触摸事件
    } else {
      ESP_LOGW(TAG, "读取触摸坐标失败: %s", esp_err_to_name(ret));
    }
  } else if (ret != ESP_OK) {
    ESP_LOGW(TAG, "读取触摸点数量失败: %s", esp_err_to_name(ret));
  }
  
  return result;
}

/**
 * @brief 向I2C设备写入数据缓冲区（兼容接口，使用I2CBusManager实现）
 * 
 * 向指定I2C设备的寄存器写入多字节数据。
 * 数据格式：[寄存器地址][数据0][数据1]...[数据n-1]
 * 
 * @param addr I2C设备地址
 * @param reg 寄存器地址
 * @param buf 要写入的数据缓冲区
 * @param len 数据长度
 * @return ESP_OK 成功，其他值表示I2C通信错误
 */
uint8_t I2C_writr_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
  // 分配临时缓冲区，大小为数据长度+1（寄存器地址）
  uint8_t *pbuf = (uint8_t*)malloc(len + 1);
  if (pbuf == NULL) {
    ESP_LOGE(TAG, "内存分配失败");
    return ESP_ERR_NO_MEM;
  }
  
  // 构建传输数据：第一个字节是寄存器地址，后面是数据
  pbuf[0] = reg;  // 寄存器地址
  for(uint8_t i = 0; i < len; i++)
  {
    pbuf[i + 1] = buf[i];  // 复制数据
  }
  
  // 使用I2CBusManager执行I2C写操作
  esp_err_t ret = I2CBus_Write(addr, pbuf, len + 1, 1000);
  
  // 释放临时缓冲区
  free(pbuf);
  pbuf = NULL;
  return ret;
}

/**
 * @brief 从I2C设备读取数据缓冲区（兼容接口，使用I2CBusManager实现）
 * 
 * 从指定I2C设备的寄存器读取多字节数据。
 * 操作过程：先写入寄存器地址，然后读取数据。
 * 
 * @param addr I2C设备地址
 * @param reg 寄存器地址
 * @param buf 读取数据的缓冲区
 * @param len 要读取的数据长度
 * @return ESP_OK 成功，其他值表示I2C通信错误
 */
uint8_t I2C_read_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
  // 使用I2CBusManager执行I2C写-读操作
  esp_err_t ret = I2CBus_WriteRead(addr, &reg, 1, buf, len, 1000);
  return ret;
}

/**
 * @brief I2C主机写-读操作（兼容接口，使用I2CBusManager实现）
 * 
 * 执行复合I2C操作：先写入数据，然后从同一设备读取数据。
 * 这是一个通用的I2C通信函数。
 * 
 * @param addr I2C设备地址
 * @param writeBuf 要写入的数据缓冲区
 * @param writeLen 写入数据长度
 * @param readBuf 读取数据的缓冲区
 * @param readLen 读取数据长度
 * @return ESP_OK 成功，其他值表示I2C通信错误
 */
uint8_t I2C_master_write_read_device(uint8_t addr, uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen)
{
  // 使用I2CBusManager执行I2C写-读操作
  esp_err_t ret = I2CBus_WriteRead(addr, writeBuf, writeLen, readBuf, readLen, 1000);
  return ret;
}
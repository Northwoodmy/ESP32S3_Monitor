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
#include "touch_bsp.h"           // 触摸BSP头文件
#include "driver/i2c.h"          // ESP32 I2C驱动

// === 硬件配置定义 ===
#define TOUCH_HOST  I2C_NUM_0                    // 触摸屏使用的I2C接口
#define EXAMPLE_PIN_NUM_TOUCH_SCL (GPIO_NUM_14)  // I2C时钟引脚
#define EXAMPLE_PIN_NUM_TOUCH_SDA (GPIO_NUM_15)  // I2C数据引脚
#define EXAMPLE_PIN_NUM_TOUCH_RST (-1)           // 复位引脚（未使用）
#define EXAMPLE_PIN_NUM_TOUCH_INT (GPIO_NUM_21)  // 中断引脚

// === 显示屏参数定义 ===
#define EXAMPLE_LCD_H_RES  368                   // LCD水平分辨率
#define EXAMPLE_LCD_V_RES  448                   // LCD垂直分辨率

// === FT3168触摸控制器参数 ===
#define I2C_ADDR_FT3168 0x38                     // FT3168的I2C设备地址

// === 内部函数声明 ===
uint8_t I2C_writr_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t I2C_read_buff(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t I2C_master_write_read_device(uint8_t addr, uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen);

/**
 * @brief 初始化触摸屏接口
 * 
 * 配置I2C接口参数并初始化FT3168触摸控制器。
 * 设置I2C为主机模式，配置引脚和时钟频率，然后将触摸控制器切换到正常工作模式。
 */
void Touch_Init(void)
{
  // === I2C接口配置 ===
  const i2c_config_t i2c_conf = 
  {
    .mode = I2C_MODE_MASTER,                     // I2C主机模式
    .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,     // SDA引脚配置
    .sda_pullup_en = GPIO_PULLUP_ENABLE,         // 启用SDA上拉电阻
    .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,     // SCL引脚配置
    .scl_pullup_en = GPIO_PULLUP_ENABLE,         // 启用SCL上拉电阻
    .master.clk_speed = 300 * 1000,              // I2C时钟频率：300kHz
  };
  
  // 配置并安装I2C驱动
  ESP_ERROR_CHECK(i2c_param_config(TOUCH_HOST, &i2c_conf));
  ESP_ERROR_CHECK(i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0));

  // === 初始化FT3168触摸控制器 ===
  uint8_t data = 0x00;
  I2C_writr_buff(I2C_ADDR_FT3168, 0x00, &data, 1);  // 向寄存器0x00写入0x00，切换到正常工作模式
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

  // === 1. 读取触摸点数量 ===
  I2C_read_buff(I2C_ADDR_FT3168, 0x02, &data, 1);
  
  if(data)  // 如果有触摸点
  {
    // === 2. 读取第一个触摸点的坐标数据 ===
    I2C_read_buff(I2C_ADDR_FT3168, 0x03, buf, 4);
    
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
    
    return 1;  // 返回有触摸事件
  }
  return 0;    // 返回无触摸事件
}

/**
 * @brief 向I2C设备写入数据缓冲区
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
  uint8_t ret;
  
  // 分配临时缓冲区，大小为数据长度+1（寄存器地址）
  uint8_t *pbuf = (uint8_t*)malloc(len + 1);
  
  // 构建传输数据：第一个字节是寄存器地址，后面是数据
  pbuf[0] = reg;  // 寄存器地址
  for(uint8_t i = 0; i < len; i++)
  {
    pbuf[i + 1] = buf[i];  // 复制数据
  }
  
  // 执行I2C写操作
  ret = i2c_master_write_to_device(TOUCH_HOST, addr, pbuf, len + 1, 1000);
  
  // 释放临时缓冲区
  free(pbuf);
  pbuf = NULL;
  return ret;
}

/**
 * @brief 从I2C设备读取数据缓冲区
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
  uint8_t ret;
  
  // 执行I2C写-读操作：先写寄存器地址，再读取数据
  ret = i2c_master_write_read_device(TOUCH_HOST, addr, &reg, 1, buf, len, 1000);
  return ret;
}

/**
 * @brief I2C主机写-读操作
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
  uint8_t ret;
  
  // 执行I2C写-读操作，超时时间1000ms
  ret = i2c_master_write_read_device(TOUCH_HOST, addr, writeBuf, writeLen, readBuf, readLen, 1000);
  return ret;
}
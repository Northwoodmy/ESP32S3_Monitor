/**
 * @file I2CBusManager.h
 * @brief I2C总线管理器 - 统一管理ESP32S3_Monitor项目中的I2C总线访问
 * 
 * 功能描述：
 * - 统一管理I2C_NUM_0总线的初始化和访问
 * - 提供线程安全的I2C互斥锁机制
 * - 支持多个设备共享同一I2C总线：TCA9554、QMI8658、FT3168
 * - 提供设备扫描和状态监控功能
 * 
 * 硬件配置：
 * - I2C总线: I2C_NUM_0
 * - SCL引脚: GPIO_14
 * - SDA引脚: GPIO_15
 * - 频率: 400kHz
 * 
 * 支持设备：
 * - TCA9554 IO扩展芯片 (地址: 0x20)
 * - FT3168 触摸控制器 (地址: 0x38)  
 * - QMI8658 陀螺仪加速度计 (地址: 0x6A/0x6B)
 */

#ifndef I2C_BUS_MANAGER_H
#define I2C_BUS_MANAGER_H

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// === I2C总线硬件配置 ===
#define I2C_BUS_NUM         I2C_NUM_0        // 使用I2C_NUM_0总线
#define I2C_BUS_SCL_PIN     GPIO_NUM_14      // SCL时钟引脚
#define I2C_BUS_SDA_PIN     GPIO_NUM_15      // SDA数据引脚
#define I2C_BUS_FREQ_HZ     400000           // I2C总线频率400kHz
#define I2C_BUS_TIMEOUT_MS  1000             // 默认超时时间1秒

// === 已知I2C设备地址定义 ===
#define I2C_ADDR_TCA9554    0x20             // TCA9554 IO扩展芯片
#define I2C_ADDR_FT3168     0x38             // FT3168 触摸控制器
#define I2C_ADDR_QMI8658_L  0x6B             // QMI8658 陀螺仪 (低地址)
#define I2C_ADDR_QMI8658_H  0x6A             // QMI8658 陀螺仪 (高地址)

// === 设备类型枚举 ===
typedef enum {
    I2C_DEVICE_TCA9554 = 0,    // TCA9554 IO扩展芯片
    I2C_DEVICE_FT3168,         // FT3168 触摸控制器
    I2C_DEVICE_QMI8658,        // QMI8658 陀螺仪加速度计
    I2C_DEVICE_UNKNOWN,        // 未知设备
    I2C_DEVICE_MAX             // 设备数量上限
} I2C_DeviceType_t;

// === 设备信息结构体 ===
typedef struct {
    uint8_t address;           // I2C设备地址
    I2C_DeviceType_t type;     // 设备类型
    bool is_detected;          // 是否检测到设备
    char name[32];             // 设备名称
} I2C_DeviceInfo_t;

// === I2C总线状态结构体 ===
typedef struct {
    bool initialized;          // I2C总线是否已初始化
    uint32_t frequency;        // I2C总线频率
    uint8_t scl_pin;          // SCL引脚号
    uint8_t sda_pin;          // SDA引脚号
    uint8_t device_count;     // 检测到的设备数量
    I2C_DeviceInfo_t devices[16]; // 设备信息列表（最多16个设备）
} I2C_BusStatus_t;

// ========================================
// === I2C总线初始化和管理接口 ===
// ========================================

/**
 * @brief 初始化I2C总线
 * 
 * 配置并初始化I2C_NUM_0总线，创建互斥锁，扫描连接的设备。
 * 此函数应该在所有I2C设备初始化之前调用。
 * 
 * @return 
 *   - ESP_OK: 初始化成功
 *   - ESP_ERR_NO_MEM: 内存不足，无法创建互斥锁
 *   - ESP_ERR_INVALID_ARG: I2C配置参数无效
 *   - ESP_FAIL: I2C驱动安装失败
 */
esp_err_t I2CBus_Init(void);

/**
 * @brief 反初始化I2C总线
 * 
 * 删除I2C驱动，释放互斥锁和相关资源。
 * 通常在系统关闭或重启时调用。
 */
void I2CBus_Deinit(void);

/**
 * @brief 检查I2C总线是否已初始化
 * 
 * @return 
 *   - true: I2C总线已初始化
 *   - false: I2C总线未初始化
 */
bool I2CBus_IsInitialized(void);

// ========================================
// === I2C总线互斥锁接口 ===
// ========================================

/**
 * @brief 获取I2C总线互斥锁
 * 
 * 在进行I2C通信之前必须获取此锁，确保多线程环境下的I2C访问安全。
 * 
 * @param timeout_ms 超时时间（毫秒），使用portMAX_DELAY表示永久等待
 * @return 
 *   - true: 成功获取锁
 *   - false: 获取锁失败（超时或I2C未初始化）
 */
bool I2CBus_Lock(uint32_t timeout_ms);

/**
 * @brief 释放I2C总线互斥锁
 * 
 * 在I2C通信完成后必须释放此锁，允许其他线程访问I2C总线。
 */
void I2CBus_Unlock(void);

/**
 * @brief 获取I2C互斥锁句柄
 * 
 * 返回I2C互斥锁句柄，供需要直接操作锁的模块使用。
 * 
 * @return I2C互斥锁句柄，如果I2C未初始化则返回NULL
 */
SemaphoreHandle_t I2CBus_GetMutex(void);

// ========================================
// === I2C通信接口 ===
// ========================================

/**
 * @brief 向I2C设备写入数据
 * 
 * 向指定I2C设备写入数据。此函数会自动处理互斥锁。
 * 
 * @param device_addr I2C设备地址（7位地址）
 * @param data 要写入的数据指针
 * @param len 数据长度
 * @param timeout_ms 通信超时时间（毫秒）
 * @return 
 *   - ESP_OK: 写入成功
 *   - ESP_ERR_INVALID_ARG: 参数无效
 *   - ESP_ERR_TIMEOUT: 通信超时
 *   - ESP_FAIL: 通信失败
 */
esp_err_t I2CBus_Write(uint8_t device_addr, const uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief 从I2C设备读取数据
 * 
 * 从指定I2C设备读取数据。此函数会自动处理互斥锁。
 * 
 * @param device_addr I2C设备地址（7位地址）
 * @param data 读取数据的缓冲区指针
 * @param len 要读取的数据长度
 * @param timeout_ms 通信超时时间（毫秒）
 * @return 
 *   - ESP_OK: 读取成功
 *   - ESP_ERR_INVALID_ARG: 参数无效
 *   - ESP_ERR_TIMEOUT: 通信超时
 *   - ESP_FAIL: 通信失败
 */
esp_err_t I2CBus_Read(uint8_t device_addr, uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief I2C写-读组合操作
 * 
 * 先向I2C设备写入数据，然后从同一设备读取数据。通常用于读取寄存器值。
 * 此函数会自动处理互斥锁。
 * 
 * @param device_addr I2C设备地址（7位地址）
 * @param write_data 要写入的数据指针（通常是寄存器地址）
 * @param write_len 写入数据长度
 * @param read_data 读取数据的缓冲区指针
 * @param read_len 要读取的数据长度
 * @param timeout_ms 通信超时时间（毫秒）
 * @return 
 *   - ESP_OK: 操作成功
 *   - ESP_ERR_INVALID_ARG: 参数无效
 *   - ESP_ERR_TIMEOUT: 通信超时
 *   - ESP_FAIL: 通信失败
 */
esp_err_t I2CBus_WriteRead(uint8_t device_addr, const uint8_t *write_data, size_t write_len, 
                          uint8_t *read_data, size_t read_len, uint32_t timeout_ms);

// ========================================
// === I2C设备扫描和管理接口 ===
// ========================================

/**
 * @brief 扫描指定地址的I2C设备
 * 
 * 检测指定地址是否存在I2C设备。
 * 
 * @param device_addr I2C设备地址（7位地址）
 * @return 
 *   - true: 设备存在并响应
 *   - false: 设备不存在或无响应
 */
bool I2CBus_ScanDevice(uint8_t device_addr);

/**
 * @brief 扫描I2C总线上的所有设备
 * 
 * 扫描地址范围0x08-0x77的所有可能I2C设备，并识别已知设备类型。
 * 扫描结果会存储在内部设备列表中。
 */
void I2CBus_ScanAllDevices(void);

/**
 * @brief 获取设备类型
 * 
 * 根据I2C地址识别设备类型。
 * 
 * @param device_addr I2C设备地址
 * @return 设备类型枚举值
 */
I2C_DeviceType_t I2CBus_GetDeviceType(uint8_t device_addr);

/**
 * @brief 获取设备名称
 * 
 * 根据设备类型返回设备名称字符串。
 * 
 * @param type 设备类型
 * @return 设备名称字符串
 */
const char* I2CBus_GetDeviceName(I2C_DeviceType_t type);

/**
 * @brief 获取I2C总线状态
 * 
 * 获取I2C总线的详细状态信息，包括初始化状态、设备列表等。
 * 
 * @param status 状态信息结构体指针
 * @return 
 *   - ESP_OK: 获取成功
 *   - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t I2CBus_GetStatus(I2C_BusStatus_t *status);

// ========================================
// === 调试和监控接口 ===
// ========================================

/**
 * @brief 打印I2C总线状态信息
 * 
 * 输出I2C总线的详细状态信息，包括配置参数、设备列表、连接状态等。
 * 用于调试和系统监控。
 */
void I2CBus_PrintStatus(void);

/**
 * @brief 打印I2C设备列表
 * 
 * 以表格形式输出检测到的所有I2C设备信息。
 */
void I2CBus_PrintDeviceList(void);

/**
 * @brief 测试I2C总线通信
 * 
 * 对检测到的所有设备进行通信测试，验证I2C总线工作状态。
 * 
 * @return 
 *   - true: 所有设备通信正常
 *   - false: 存在通信异常的设备
 */
bool I2CBus_TestCommunication(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_BUS_MANAGER_H 
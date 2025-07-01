#ifndef TCA9554_BSP_H
#define TCA9554_BSP_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// === TCA9554芯片常量定义 ===
#define TCA9554_IO_COUNT           8
#define TCA9554_INPUT_REG_ADDR     0x00
#define TCA9554_OUTPUT_REG_ADDR    0x01
#define TCA9554_DIRECTION_REG_ADDR 0x03

// TCA9554 I2C地址定义 (A2,A1,A0 = 000~111)
#define TCA9554_I2C_ADDRESS_000    0x20
#define TCA9554_I2C_ADDRESS_001    0x21
#define TCA9554_I2C_ADDRESS_010    0x22
#define TCA9554_I2C_ADDRESS_011    0x23
#define TCA9554_I2C_ADDRESS_100    0x24
#define TCA9554_I2C_ADDRESS_101    0x25
#define TCA9554_I2C_ADDRESS_110    0x26
#define TCA9554_I2C_ADDRESS_111    0x27

// TCA9554A I2C地址定义 (A2,A1,A0 = 000~111)
#define TCA9554A_I2C_ADDRESS_000   0x38
#define TCA9554A_I2C_ADDRESS_001   0x39
#define TCA9554A_I2C_ADDRESS_010   0x3A
#define TCA9554A_I2C_ADDRESS_011   0x3B
#define TCA9554A_I2C_ADDRESS_100   0x3C
#define TCA9554A_I2C_ADDRESS_101   0x3D
#define TCA9554A_I2C_ADDRESS_110   0x3E
#define TCA9554A_I2C_ADDRESS_111   0x3F

// GPIO方向定义
#define TCA9554_GPIO_INPUT         1
#define TCA9554_GPIO_OUTPUT        0

// === 传统C接口（向后兼容） ===
/**
 * @brief 初始化TCA9554芯片
 * @param i2c_address TCA9554的I2C地址
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_Init(uint8_t i2c_address);

/**
 * @brief 设置GPIO方向
 * @param pin_mask 引脚掩码(bit0~bit7对应GPIO0~GPIO7)
 * @param direction 方向(1=输入, 0=输出)
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_SetDirection(uint8_t pin_mask, uint8_t direction);

/**
 * @brief 写GPIO输出
 * @param pin_mask 引脚掩码
 * @param value 输出值(1=高电平, 0=低电平)
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_WriteOutput(uint8_t pin_mask, uint8_t value);

/**
 * @brief 读GPIO输入
 * @param value 读取的输入值指针
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_ReadInput(uint8_t *value);

/**
 * @brief 读GPIO输出寄存器
 * @param value 读取的输出值指针
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_ReadOutput(uint8_t *value);

/**
 * @brief 设置单个GPIO引脚方向
 * @param pin 引脚号(0~7)
 * @param direction 方向(1=输入, 0=输出)
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_SetPinDirection(uint8_t pin, uint8_t direction);

/**
 * @brief 写单个GPIO引脚输出
 * @param pin 引脚号(0~7)
 * @param value 输出值(1=高电平, 0=低电平)
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_WritePinOutput(uint8_t pin, uint8_t value);

/**
 * @brief 读单个GPIO引脚输入
 * @param pin 引脚号(0~7)
 * @param value 读取的输入值指针
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_ReadPinInput(uint8_t pin, uint8_t *value);

/**
 * @brief 切换单个GPIO引脚输出状态
 * @param pin 引脚号(0~7)
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_TogglePinOutput(uint8_t pin);

/**
 * @brief 检查TCA9554连接状态
 * @return 1=已连接, 0=未连接
 */
uint8_t TCA9554_IsConnected(void);

/**
 * @brief 重置TCA9554为默认状态
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_Reset(void);

/**
 * @brief 打印TCA9554状态信息
 */
void TCA9554_PrintStatus(void);

/**
 * @brief 获取I2C总线互斥锁（供其他I2C设备使用）
 * @param timeout_ms 超时时间（毫秒）
 * @return true 成功获取锁，false 获取锁失败
 */
bool TCA9554_I2C_Lock(uint32_t timeout_ms);

/**
 * @brief 释放I2C总线互斥锁
 */
void TCA9554_I2C_Unlock(void);

/**
 * @brief 获取I2C互斥锁句柄（供其他模块使用）
 * @return I2C互斥锁句柄
 */
SemaphoreHandle_t TCA9554_GetI2CMutex(void);

// === 新增C++模块接口 ===
/**
 * @brief 初始化TCA9554模块(C++)
 * @param i2c_address TCA9554的I2C地址
 * @param sda_pin SDA引脚号
 * @param scl_pin SCL引脚号
 * @param i2c_freq I2C频率
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_Module_Init(uint8_t i2c_address, int sda_pin, int scl_pin, uint32_t i2c_freq);

/**
 * @brief 获取TCA9554模块连接状态
 * @return 1=已连接, 0=未连接
 */
uint8_t TCA9554_Module_IsConnected(void);

/**
 * @brief 批量设置GPIO配置
 * @param direction_mask 方向掩码
 * @param output_mask 输出掩码
 * @return 0=成功, 非0=失败
 */
uint8_t TCA9554_Module_ConfigureGPIOs(uint8_t direction_mask, uint8_t output_mask);

#ifdef __cplusplus
}

// === C++接口 ===
#include <memory>

/**
 * @brief TCA9554模块C++类
 */
class TCA9554Module {
private:
    uint8_t i2c_address_;
    int sda_pin_;
    int scl_pin_;
    uint32_t i2c_freq_;
    bool initialized_;
    
    // 寄存器缓存
    uint8_t direction_reg_;
    uint8_t output_reg_;
    
    // 私有I2C操作函数
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t* value);
    
public:
    /**
     * @brief 构造函数
     * @param i2c_address TCA9554的I2C地址
     * @param sda_pin SDA引脚号
     * @param scl_pin SCL引脚号
     * @param i2c_freq I2C频率(默认400kHz)
     */
    TCA9554Module(uint8_t i2c_address, int sda_pin = 15, int scl_pin = 14, uint32_t i2c_freq = 400000);
    
    /**
     * @brief 析构函数
     */
    ~TCA9554Module();
    
    /**
     * @brief 初始化模块
     * @return true=成功, false=失败
     */
    bool begin();
    
    /**
     * @brief 检查连接状态
     * @return true=已连接, false=未连接
     */
    bool isConnected();
    
    /**
     * @brief 设置GPIO方向
     * @param pin_mask 引脚掩码
     * @param direction 方向
     * @return true=成功, false=失败
     */
    bool setDirection(uint8_t pin_mask, uint8_t direction);
    
    /**
     * @brief 写GPIO输出
     * @param pin_mask 引脚掩码
     * @param value 输出值
     * @return true=成功, false=失败
     */
    bool writeOutput(uint8_t pin_mask, uint8_t value);
    
    /**
     * @brief 读GPIO输入
     * @param value 读取值指针
     * @return true=成功, false=失败
     */
    bool readInput(uint8_t& value);
    
    /**
     * @brief 设置单个引脚方向
     * @param pin 引脚号(0~7)
     * @param direction 方向
     * @return true=成功, false=失败
     */
    bool setPinDirection(uint8_t pin, bool direction);
    
    /**
     * @brief 写单个引脚输出
     * @param pin 引脚号(0~7)
     * @param value 输出值
     * @return true=成功, false=失败
     */
    bool writePinOutput(uint8_t pin, bool value);
    
    /**
     * @brief 读单个引脚输入
     * @param pin 引脚号(0~7)
     * @param value 读取值指针
     * @return true=成功, false=失败
     */
    bool readPinInput(uint8_t pin, bool& value);
    
    /**
     * @brief 切换单个引脚输出
     * @param pin 引脚号(0~7)
     * @return true=成功, false=失败
     */
    bool togglePinOutput(uint8_t pin);
    
    /**
     * @brief 重置为默认状态
     * @return true=成功, false=失败
     */
    bool reset();
    
    /**
     * @brief 打印传感器信息
     */
    void printSensorInfo();
    
    /**
     * @brief 获取当前方向寄存器值
     * @return 方向寄存器值
     */
    uint8_t getDirectionRegister() const { return direction_reg_; }
    
    /**
     * @brief 获取当前输出寄存器值
     * @return 输出寄存器值
     */
    uint8_t getOutputRegister() const { return output_reg_; }
};

// 全局C++模块实例声明
extern std::unique_ptr<TCA9554Module> g_tca9554Module;

#endif

#endif // TCA9554_BSP_H 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tca9554_bsp.h"
#include "I2CBusManager.h"    // 使用统一的I2C总线管理器

#ifdef __cplusplus
extern "C" {
#endif

// === ESP-IDF实现（使用I2CBusManager） ===
#include "esp_log.h"

// 默认寄存器值
#define TCA9554_DIR_REG_DEFAULT   0xFF    // 所有引脚默认为输入
#define TCA9554_OUT_REG_DEFAULT   0xFF    // 所有引脚默认输出高电平

static const char* TAG = "TCA9554_BSP";

// 全局变量
static uint8_t g_i2c_address = TCA9554_I2C_ADDRESS_000;
static uint8_t g_direction_reg = TCA9554_DIR_REG_DEFAULT;
static uint8_t g_output_reg = TCA9554_OUT_REG_DEFAULT;
static bool g_initialized = false;

// 传统C函数声明
static uint8_t tca9554_write_reg(uint8_t reg, uint8_t value);
static uint8_t tca9554_read_reg(uint8_t reg, uint8_t *value);

// === C++模块全局变量 ===
#ifdef __cplusplus
std::unique_ptr<TCA9554Module> g_tca9554Module = nullptr;
#endif

// === 传统C接口实现（使用I2CBusManager） ===
uint8_t TCA9554_Init(uint8_t i2c_address)
{
    ESP_LOGI(TAG, "开始初始化TCA9554 (地址: 0x%02X)...", i2c_address);
    
    g_i2c_address = i2c_address;
    
    // === 检查I2C总线管理器是否已初始化 ===
    if (!I2CBus_IsInitialized()) {
        ESP_LOGE(TAG, "I2C总线管理器未初始化，请先调用I2CBus_Init()");
        return 1;
    }
    
    ESP_LOGI(TAG, "使用统一的I2C总线管理器进行通信");
    
    // === 验证TCA9554设备连接 ===
    if (!I2CBus_ScanDevice(g_i2c_address)) {
        ESP_LOGE(TAG, "TCA9554设备检测失败，地址: 0x%02X", g_i2c_address);
        return 1;
    }
    ESP_LOGI(TAG, "✓ TCA9554设备检测成功");
    
    // 重置TCA9554为默认状态
    if (TCA9554_Reset() != 0) {
        ESP_LOGE(TAG, "TCA9554重置失败");
        return 1;
    }
    
    g_initialized = true;
    ESP_LOGI(TAG, "✓ TCA9554初始化成功，地址: 0x%02X", i2c_address);
    return 0;
}

uint8_t TCA9554_SetDirection(uint8_t pin_mask, uint8_t direction)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "TCA9554未初始化");
        return 1;
    }
    
    uint8_t new_direction;
    if (direction) {
        // 设置为输入
        new_direction = g_direction_reg | pin_mask;
    } else {
        // 设置为输出
        new_direction = g_direction_reg & (~pin_mask);
    }
    
    if (tca9554_write_reg(TCA9554_DIRECTION_REG_ADDR, new_direction) != 0) {
        ESP_LOGE(TAG, "写入方向寄存器失败");
        return 1;
    }
    
    g_direction_reg = new_direction;
    return 0;
}

uint8_t TCA9554_WriteOutput(uint8_t pin_mask, uint8_t value)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "TCA9554未初始化");
        return 1;
    }
    
    uint8_t new_output;
    if (value) {
        // 设置为高电平
        new_output = g_output_reg | pin_mask;
    } else {
        // 设置为低电平
        new_output = g_output_reg & (~pin_mask);
    }
    
    if (tca9554_write_reg(TCA9554_OUTPUT_REG_ADDR, new_output) != 0) {
        ESP_LOGE(TAG, "写入输出寄存器失败");
        return 1;
    }
    
    g_output_reg = new_output;
    return 0;
}

uint8_t TCA9554_ReadInput(uint8_t *value)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "TCA9554未初始化");
        return 1;
    }
    
    if (value == NULL) {
        ESP_LOGE(TAG, "参数为空");
        return 1;
    }
    
    return tca9554_read_reg(TCA9554_INPUT_REG_ADDR, value);
}

uint8_t TCA9554_ReadOutput(uint8_t *value)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "TCA9554未初始化");
        return 1;
    }
    
    if (value == NULL) {
        ESP_LOGE(TAG, "参数为空");
        return 1;
    }
    
    *value = g_output_reg;
    return 0;
}

uint8_t TCA9554_SetPinDirection(uint8_t pin, uint8_t direction)
{
    if (pin >= TCA9554_IO_COUNT) {
        ESP_LOGE(TAG, "引脚号无效: %d", pin);
        return 1;
    }
    
    uint8_t pin_mask = 1 << pin;
    return TCA9554_SetDirection(pin_mask, direction);
}

uint8_t TCA9554_WritePinOutput(uint8_t pin, uint8_t value)
{
    if (pin >= TCA9554_IO_COUNT) {
        ESP_LOGE(TAG, "引脚号无效: %d", pin);
        return 1;
    }
    
    uint8_t pin_mask = 1 << pin;
    return TCA9554_WriteOutput(pin_mask, value);
}

uint8_t TCA9554_ReadPinInput(uint8_t pin, uint8_t *value)
{
    if (pin >= TCA9554_IO_COUNT) {
        ESP_LOGE(TAG, "引脚号无效: %d", pin);
        return 1;
    }
    
    if (value == NULL) {
        ESP_LOGE(TAG, "参数为空");
        return 1;
    }
    
    uint8_t input_reg;
    if (TCA9554_ReadInput(&input_reg) != 0) {
        return 1;
    }
    
    *value = (input_reg >> pin) & 0x01;
    return 0;
}

uint8_t TCA9554_TogglePinOutput(uint8_t pin)
{
    if (pin >= TCA9554_IO_COUNT) {
        ESP_LOGE(TAG, "引脚号无效: %d", pin);
        return 1;
    }
    
    uint8_t pin_mask = 1 << pin;
    uint8_t current_state = (g_output_reg >> pin) & 0x01;
    return TCA9554_WriteOutput(pin_mask, !current_state);
}

uint8_t TCA9554_IsConnected(void)
{
    if (!g_initialized) {
        return 0;
    }
    
    // 使用I2CBusManager进行设备检测
    return I2CBus_ScanDevice(g_i2c_address) ? 1 : 0;
}

uint8_t TCA9554_Reset(void)
{
    uint8_t ret = 0;
    
    // 重置方向寄存器（所有引脚为输入）
    if (tca9554_write_reg(TCA9554_DIRECTION_REG_ADDR, TCA9554_DIR_REG_DEFAULT) != 0) {
        ESP_LOGE(TAG, "重置方向寄存器失败");
        ret = 1;
    }
    g_direction_reg = TCA9554_DIR_REG_DEFAULT;
    
    // 重置输出寄存器（所有引脚输出高电平）
    if (tca9554_write_reg(TCA9554_OUTPUT_REG_ADDR, TCA9554_OUT_REG_DEFAULT) != 0) {
        ESP_LOGE(TAG, "重置输出寄存器失败");
        ret = 1;
    }
    g_output_reg = TCA9554_OUT_REG_DEFAULT;
    
    return ret;
}

void TCA9554_PrintStatus(void)
{
    printf("=== TCA9554状态信息 ===\n");
    printf("I2C地址: 0x%02X\n", g_i2c_address);
    printf("初始化状态: %s\n", g_initialized ? "已初始化" : "未初始化");
    printf("连接状态: %s\n", TCA9554_IsConnected() ? "已连接" : "未连接");
    
    if (g_initialized && TCA9554_IsConnected()) {
        printf("方向寄存器: 0x%02X (1=输入, 0=输出)\n", g_direction_reg);
        printf("输出寄存器: 0x%02X\n", g_output_reg);
        
        uint8_t input_reg;
        if (TCA9554_ReadInput(&input_reg) == 0) {
            printf("输入寄存器: 0x%02X\n", input_reg);
        }
        
        printf("GPIO状态:\n");
        for (int i = 0; i < TCA9554_IO_COUNT; i++) {
            uint8_t dir = (g_direction_reg >> i) & 0x01;
            uint8_t out = (g_output_reg >> i) & 0x01;
            uint8_t in = (input_reg >> i) & 0x01;
            printf("  GPIO%d: %s, 输出=%d, 输入=%d\n", 
                   i, dir ? "输入" : "输出", out, in);
        }
    }
    printf("======================\n");
}

/**
 * @brief 获取I2C总线互斥锁（兼容接口，转发到I2CBusManager）
 * @param timeout_ms 超时时间（毫秒）
 * @return true 成功获取锁，false 获取锁失败
 */
bool TCA9554_I2C_Lock(uint32_t timeout_ms)
{
    return I2CBus_Lock(timeout_ms);
}

/**
 * @brief 释放I2C总线互斥锁（兼容接口，转发到I2CBusManager）
 */
void TCA9554_I2C_Unlock(void)
{
    I2CBus_Unlock();
}

/**
 * @brief 获取I2C互斥锁句柄（兼容接口，转发到I2CBusManager）
 * @return I2C互斥锁句柄
 */
SemaphoreHandle_t TCA9554_GetI2CMutex(void)
{
    return I2CBus_GetMutex();
}

// === 私有函数实现（使用I2CBusManager） ===
static uint8_t tca9554_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    esp_err_t ret = I2CBus_Write(g_i2c_address, data, 2, 100);  // 100ms超时
    return (ret == ESP_OK) ? 0 : 1;
}

static uint8_t tca9554_read_reg(uint8_t reg, uint8_t *value)
{
    esp_err_t ret = I2CBus_WriteRead(g_i2c_address, &reg, 1, value, 1, 100);  // 100ms超时
    return (ret == ESP_OK) ? 0 : 1;
}

// === 新增C++模块接口实现 ===
#ifdef __cplusplus
extern "C" {
#endif

uint8_t TCA9554_Module_Init(uint8_t i2c_address, int sda_pin, int scl_pin, uint32_t i2c_freq)
{
#ifdef __cplusplus
    if (g_tca9554Module == nullptr) {
        // 检查I2C总线管理器是否已初始化
        if (!I2CBus_IsInitialized()) {
            ESP_LOGE(TAG, "I2C总线管理器未初始化，请先调用I2CBus_Init()");
            return 1;
        }
        
        // 创建TCA9554Module实例（使用I2CBusManager）
        g_tca9554Module = std::make_unique<TCA9554Module>(i2c_address, sda_pin, scl_pin, i2c_freq);
        
        // 初始化模块
        if (g_tca9554Module->begin()) {
            printf("TCA9554Module 初始化成功（使用I2CBusManager）\n");
            return 0;
        } else {
            printf("TCA9554Module 初始化失败\n");
            g_tca9554Module.reset();
            return 1;
        }
    }
    return 0;
#else
    // 如果不是C++环境，回退到传统初始化
    return TCA9554_Init(i2c_address);
#endif
}

uint8_t TCA9554_Module_IsConnected(void)
{
#ifdef __cplusplus
    if (g_tca9554Module != nullptr) {
        return g_tca9554Module->isConnected() ? 1 : 0;
    }
#endif
    // 回退到传统接口
    return TCA9554_IsConnected();
}

uint8_t TCA9554_Module_ConfigureGPIOs(uint8_t direction_mask, uint8_t output_mask)
{
#ifdef __cplusplus
    if (g_tca9554Module != nullptr) {
        bool success = true;
        success &= g_tca9554Module->setDirection(0xFF, direction_mask);
        success &= g_tca9554Module->writeOutput(0xFF, output_mask);
        return success ? 0 : 1;
    }
#endif
    // 回退到传统接口
    uint8_t ret = 0;
    ret |= TCA9554_SetDirection(0xFF, direction_mask);
    ret |= TCA9554_WriteOutput(0xFF, output_mask);
    return ret;
}

#ifdef __cplusplus
}

// === C++类实现（使用I2CBusManager） ===
TCA9554Module::TCA9554Module(uint8_t i2c_address, int sda_pin, int scl_pin, uint32_t i2c_freq)
    : i2c_address_(i2c_address), sda_pin_(sda_pin), scl_pin_(scl_pin), 
      i2c_freq_(i2c_freq), initialized_(false),
      direction_reg_(TCA9554_DIR_REG_DEFAULT), output_reg_(TCA9554_OUT_REG_DEFAULT)
{
}

TCA9554Module::~TCA9554Module()
{
    // 析构时不需要特殊处理，I2CBusManager会管理I2C资源
}

bool TCA9554Module::begin()
{
    // 检查I2C总线管理器是否已初始化
    if (!I2CBus_IsInitialized()) {
        printf("TCA9554Module: I2C总线管理器未初始化\n");
        return false;
    }
    
    printf("TCA9554Module: 使用I2CBusManager进行初始化\n");
    
    // 验证设备连接
    if (!I2CBus_ScanDevice(i2c_address_)) {
        printf("TCA9554Module: 设备检测失败，地址: 0x%02X\n", i2c_address_);
        return false;
    }
    
    // 重置芯片为默认状态
    if (!reset()) {
        printf("TCA9554重置失败\n");
        return false;
    }
    
    initialized_ = true;
    printf("TCA9554Module初始化成功，地址: 0x%02X（使用I2CBusManager）\n", i2c_address_);
    return true;
}

bool TCA9554Module::isConnected()
{
    if (!initialized_) {
        return false;
    }
    
    return I2CBus_ScanDevice(i2c_address_);
}

bool TCA9554Module::setDirection(uint8_t pin_mask, uint8_t direction)
{
    if (!initialized_) {
        return false;
    }
    
    uint8_t new_direction;
    if (direction) {
        new_direction = direction_reg_ | pin_mask;
    } else {
        new_direction = direction_reg_ & (~pin_mask);
    }
    
    if (!writeRegister(TCA9554_DIRECTION_REG_ADDR, new_direction)) {
        return false;
    }
    
    direction_reg_ = new_direction;
    return true;
}

bool TCA9554Module::writeOutput(uint8_t pin_mask, uint8_t value)
{
    if (!initialized_) {
        return false;
    }
    
    uint8_t new_output;
    if (value) {
        new_output = output_reg_ | pin_mask;
    } else {
        new_output = output_reg_ & (~pin_mask);
    }
    
    if (!writeRegister(TCA9554_OUTPUT_REG_ADDR, new_output)) {
        return false;
    }
    
    output_reg_ = new_output;
    return true;
}

bool TCA9554Module::readInput(uint8_t& value)
{
    if (!initialized_) {
        return false;
    }
    
    return readRegister(TCA9554_INPUT_REG_ADDR, &value);
}

bool TCA9554Module::setPinDirection(uint8_t pin, bool direction)
{
    if (pin >= TCA9554_IO_COUNT) {
        return false;
    }
    
    uint8_t pin_mask = 1 << pin;
    return setDirection(pin_mask, direction ? 1 : 0);
}

bool TCA9554Module::writePinOutput(uint8_t pin, bool value)
{
    if (pin >= TCA9554_IO_COUNT) {
        return false;
    }
    
    uint8_t pin_mask = 1 << pin;
    return writeOutput(pin_mask, value ? 1 : 0);
}

bool TCA9554Module::readPinInput(uint8_t pin, bool& value)
{
    if (pin >= TCA9554_IO_COUNT) {
        return false;
    }
    
    uint8_t input_reg;
    if (!readInput(input_reg)) {
        return false;
    }
    
    value = (input_reg >> pin) & 0x01;
    return true;
}

bool TCA9554Module::togglePinOutput(uint8_t pin)
{
    if (pin >= TCA9554_IO_COUNT) {
        return false;
    }
    
    bool current_state = (output_reg_ >> pin) & 0x01;
    return writePinOutput(pin, !current_state);
}

bool TCA9554Module::reset()
{
    bool success = true;
    
    // 重置方向寄存器
    success &= writeRegister(TCA9554_DIRECTION_REG_ADDR, TCA9554_DIR_REG_DEFAULT);
    direction_reg_ = TCA9554_DIR_REG_DEFAULT;
    
    // 重置输出寄存器
    success &= writeRegister(TCA9554_OUTPUT_REG_ADDR, TCA9554_OUT_REG_DEFAULT);
    output_reg_ = TCA9554_OUT_REG_DEFAULT;
    
    return success;
}

void TCA9554Module::printSensorInfo()
{
    printf("=== TCA9554Module信息 ===\n");
    printf("I2C地址: 0x%02X\n", i2c_address_);
    printf("SDA引脚: %d, SCL引脚: %d\n", sda_pin_, scl_pin_);
    printf("I2C频率: %lu Hz\n", i2c_freq_);
    printf("初始化状态: %s\n", initialized_ ? "已初始化" : "未初始化");
    printf("连接状态: %s\n", isConnected() ? "已连接" : "未连接");
    printf("使用: I2CBusManager统一管理\n");
    
    if (initialized_ && isConnected()) {
        printf("方向寄存器: 0x%02X\n", direction_reg_);
        printf("输出寄存器: 0x%02X\n", output_reg_);
        
        uint8_t input_reg;
        if (readInput(input_reg)) {
            printf("输入寄存器: 0x%02X\n", input_reg);
            
            printf("引脚状态:\n");
            for (int i = 0; i < TCA9554_IO_COUNT; i++) {
                bool dir = (direction_reg_ >> i) & 0x01;
                bool out = (output_reg_ >> i) & 0x01;
                bool in = (input_reg >> i) & 0x01;
                printf("  GPIO%d: %s, 输出=%d, 输入=%d\n", 
                       i, dir ? "输入" : "输出", out ? 1 : 0, in ? 1 : 0);
            }
        }
    }
    printf("========================\n");
}

bool TCA9554Module::writeRegister(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    esp_err_t ret = I2CBus_Write(i2c_address_, data, 2, 100);  // 100ms超时
    return (ret == ESP_OK);
}

bool TCA9554Module::readRegister(uint8_t reg, uint8_t* value)
{
    esp_err_t ret = I2CBus_WriteRead(i2c_address_, &reg, 1, value, 1, 100);  // 100ms超时
    return (ret == ESP_OK);
}

#endif

#ifdef __cplusplus
}
#endif 
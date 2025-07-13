/**
 * @file I2CBusManager.cpp
 * @brief I2C总线管理器实现 - 统一管理ESP32S3_Monitor项目中的I2C总线访问
 */

#include "I2CBusManager.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

// === 全局变量 ===
static bool g_i2c_initialized = false;             // I2C总线初始化状态
static SemaphoreHandle_t g_i2c_mutex = NULL;       // I2C总线互斥锁
static I2C_BusStatus_t g_bus_status;               // I2C总线状态信息

// === 内部函数声明 ===
static void i2c_reset_bus_status(void);
static void i2c_add_device_to_status(uint8_t addr, I2C_DeviceType_t type, bool detected);

// ========================================
// === I2C总线初始化和管理接口实现 ===
// ========================================

esp_err_t I2CBus_Init(void)
{
    if (g_i2c_initialized) {
        printf("[I2CBusManager] 警告：I2C总线已经初始化\n");
        return ESP_OK;
    }
    
    printf("[I2CBusManager] 开始初始化I2C总线管理器...\n");
    
    // === 1. 创建I2C互斥锁 ===
    g_i2c_mutex = xSemaphoreCreateMutex();
    if (g_i2c_mutex == NULL) {
        printf("[I2CBusManager] 错误：创建I2C互斥锁失败\n");
        return ESP_ERR_NO_MEM;
    }
    printf("[I2CBusManager] ✓ I2C互斥锁创建成功\n");
    
    // === 2. 配置I2C总线参数 ===
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_BUS_SDA_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_BUS_SCL_PIN;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_BUS_FREQ_HZ;
    
    // === 3. 应用I2C配置 ===
    esp_err_t ret = i2c_param_config(I2C_BUS_NUM, &conf);
    if (ret != ESP_OK) {
        printf("[I2CBusManager] 错误：I2C参数配置失败: %s\n", esp_err_to_name(ret));
        vSemaphoreDelete(g_i2c_mutex);
        g_i2c_mutex = NULL;
        return ret;
    }
    printf("[I2CBusManager] ✓ I2C参数配置成功\n");
    
    // === 4. 安装I2C驱动 ===
    ret = i2c_driver_install(I2C_BUS_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        printf("[I2CBusManager] 错误：I2C驱动安装失败: %s\n", esp_err_to_name(ret));
        vSemaphoreDelete(g_i2c_mutex);
        g_i2c_mutex = NULL;
        return ret;
    }
    printf("[I2CBusManager] ✓ I2C驱动安装成功\n");
    
    // === 5. 初始化总线状态结构体 ===
    i2c_reset_bus_status();
    g_bus_status.initialized = true;
    g_bus_status.frequency = I2C_BUS_FREQ_HZ;
    g_bus_status.scl_pin = I2C_BUS_SCL_PIN;
    g_bus_status.sda_pin = I2C_BUS_SDA_PIN;
    
    g_i2c_initialized = true;
    
    printf("[I2CBusManager] ✓ I2C总线初始化成功\n");
    printf("[I2CBusManager]   - 总线: I2C_%d\n", I2C_BUS_NUM);
    printf("[I2CBusManager]   - SCL引脚: GPIO_%d\n", I2C_BUS_SCL_PIN);
    printf("[I2CBusManager]   - SDA引脚: GPIO_%d\n", I2C_BUS_SDA_PIN);
    printf("[I2CBusManager]   - 频率: %d Hz\n", I2C_BUS_FREQ_HZ);
    
    // === 6. 扫描连接的设备 ===
    printf("[I2CBusManager] 开始扫描I2C设备...\n");
    I2CBus_ScanAllDevices();
    
    printf("[I2CBusManager] I2C总线管理器初始化完成，检测到 %d 个设备\n", g_bus_status.device_count);
    return ESP_OK;
}

void I2CBus_Deinit(void)
{
    if (!g_i2c_initialized) {
        printf("[I2CBusManager] 警告：I2C总线未初始化，无需反初始化\n");
        return;
    }
    
    printf("[I2CBusManager] 开始反初始化I2C总线管理器...\n");
    
    // 删除I2C驱动
    esp_err_t ret = i2c_driver_delete(I2C_BUS_NUM);
    if (ret != ESP_OK) {
        printf("[I2CBusManager] 错误：I2C驱动删除失败: %s\n", esp_err_to_name(ret));
    }
    
    // 删除互斥锁
    if (g_i2c_mutex != NULL) {
        vSemaphoreDelete(g_i2c_mutex);
        g_i2c_mutex = NULL;
    }
    
    // 重置状态
    i2c_reset_bus_status();
    g_i2c_initialized = false;
    
    printf("[I2CBusManager] I2C总线管理器反初始化完成\n");
}

bool I2CBus_IsInitialized(void)
{
    return g_i2c_initialized;
}

// ========================================
// === I2C总线互斥锁接口实现 ===
// ========================================

bool I2CBus_Lock(uint32_t timeout_ms)
{
    if (!g_i2c_initialized || g_i2c_mutex == NULL) {
        printf("[I2CBusManager] 警告：I2C总线未初始化，无法获取锁\n");
        return false;
    }
    
    TickType_t timeout_ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    if (xSemaphoreTake(g_i2c_mutex, timeout_ticks) == pdTRUE) {
        return true;
    } else {
        printf("[I2CBusManager] 警告：获取I2C互斥锁超时 (%d ms)\n", timeout_ms);
        return false;
    }
}

void I2CBus_Unlock(void)
{
    if (!g_i2c_initialized || g_i2c_mutex == NULL) {
        printf("[I2CBusManager] 警告：I2C总线未初始化，无法释放锁\n");
        return;
    }
    
    xSemaphoreGive(g_i2c_mutex);
}

SemaphoreHandle_t I2CBus_GetMutex(void)
{
    return g_i2c_mutex;
}

// ========================================
// === I2C通信接口实现 ===
// ========================================

esp_err_t I2CBus_Write(uint8_t device_addr, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (!g_i2c_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取互斥锁
    if (!I2CBus_Lock(timeout_ms)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 执行I2C写操作
    esp_err_t ret = i2c_master_write_to_device(I2C_BUS_NUM, device_addr, 
                                              data, len, 
                                              pdMS_TO_TICKS(timeout_ms));
    
    // 释放互斥锁
    I2CBus_Unlock();
    
    if (ret != ESP_OK) {
        printf("[I2CBusManager] 警告：I2C写操作失败，设备地址: 0x%02X, 错误: %s\n", device_addr, esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t I2CBus_Read(uint8_t device_addr, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (!g_i2c_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取互斥锁
    if (!I2CBus_Lock(timeout_ms)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 执行I2C读操作
    esp_err_t ret = i2c_master_read_from_device(I2C_BUS_NUM, device_addr,
                                               data, len,
                                               pdMS_TO_TICKS(timeout_ms));
    
    // 释放互斥锁
    I2CBus_Unlock();
    
    if (ret != ESP_OK) {
        printf("[I2CBusManager] 警告：I2C读操作失败，设备地址: 0x%02X, 错误: %s\n", device_addr, esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t I2CBus_WriteRead(uint8_t device_addr, const uint8_t *write_data, size_t write_len,
                          uint8_t *read_data, size_t read_len, uint32_t timeout_ms)
{
    if (!g_i2c_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (write_data == NULL || write_len == 0 || read_data == NULL || read_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取互斥锁
    if (!I2CBus_Lock(timeout_ms)) {
        return ESP_ERR_TIMEOUT;
    }
    
    // 执行I2C写-读操作
    esp_err_t ret = i2c_master_write_read_device(I2C_BUS_NUM, device_addr,
                                                write_data, write_len,
                                                read_data, read_len,
                                                pdMS_TO_TICKS(timeout_ms));
    
    // 释放互斥锁
    I2CBus_Unlock();
    
    if (ret != ESP_OK) {
        //printf("[I2CBusManager] 警告：I2C写-读操作失败，设备地址: 0x%02X, 错误: %s\n", device_addr, esp_err_to_name(ret));
    }
    
    return ret;
}

// ========================================
// === I2C设备扫描和管理接口实现 ===
// ========================================

bool I2CBus_ScanDevice(uint8_t device_addr)
{
    if (!g_i2c_initialized) {
        return false;
    }
    
    // 获取互斥锁
    if (!I2CBus_Lock(100)) {  // 100ms超时
        return false;
    }
    
    // 创建I2C命令链路进行设备检测
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_BUS_NUM, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    
    // 释放互斥锁
    I2CBus_Unlock();
    
    return (ret == ESP_OK);
}

void I2CBus_ScanAllDevices(void)
{
    if (!g_i2c_initialized) {
        printf("[I2CBusManager] 警告：I2C总线未初始化，无法扫描设备\n");
        return;
    }
    
    printf("[I2CBusManager] ==========================================\n");
    printf("[I2CBusManager] === 开始扫描I2C总线设备 ===\n");
    printf("[I2CBusManager] ==========================================\n");
    
    uint8_t devices_found = 0;
    
    // 重置设备列表
    i2c_reset_bus_status();
    
    // 扫描地址范围 0x08 到 0x77
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (I2CBus_ScanDevice(addr)) {
            I2C_DeviceType_t device_type = I2CBus_GetDeviceType(addr);
            const char* device_name = I2CBus_GetDeviceName(device_type);
            
            printf("[I2CBusManager] >>> 发现I2C设备，地址: 0x%02X (%s)\n", addr, device_name);
            
            // 添加到设备列表
            i2c_add_device_to_status(addr, device_type, true);
            devices_found++;
            
            // 短暂延时，避免总线过载
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    
    g_bus_status.device_count = devices_found;
    
    printf("[I2CBusManager] ==========================================\n");
    printf("[I2CBusManager] I2C总线扫描完成，共发现 %d 个设备\n", devices_found);
    printf("[I2CBusManager] ==========================================\n");
}

I2C_DeviceType_t I2CBus_GetDeviceType(uint8_t device_addr)
{
    switch (device_addr) {
        case I2C_ADDR_TCA9554:
            return I2C_DEVICE_TCA9554;
        case I2C_ADDR_FT3168:
            return I2C_DEVICE_FT3168;
        case I2C_ADDR_QMI8658_L:
        case I2C_ADDR_QMI8658_H:
            return I2C_DEVICE_QMI8658;
        case I2C_ADDR_ES8311_0:
        case I2C_ADDR_ES8311_1:
            return I2C_DEVICE_ES8311;
        default:
            return I2C_DEVICE_UNKNOWN;
    }
}

const char* I2CBus_GetDeviceName(I2C_DeviceType_t type)
{
    switch (type) {
        case I2C_DEVICE_TCA9554:
            return "TCA9554 IO扩展芯片";
        case I2C_DEVICE_FT3168:
            return "FT3168 触摸控制器";
        case I2C_DEVICE_QMI8658:
            return "QMI8658 陀螺仪加速度计";
        case I2C_DEVICE_ES8311:
            return "ES8311 音频编解码器";
        case I2C_DEVICE_UNKNOWN:
        default:
            return "未知设备";
    }
}

esp_err_t I2CBus_GetStatus(I2C_BusStatus_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(status, &g_bus_status, sizeof(I2C_BusStatus_t));
    return ESP_OK;
}

// ========================================
// === 调试和监控接口实现 ===
// ========================================

void I2CBus_PrintStatus(void)
{
    printf("=== I2C总线管理器状态信息 ===\n");
    printf("初始化状态: %s\n", g_i2c_initialized ? "已初始化" : "未初始化");
    
    if (!g_i2c_initialized) {
        printf("===============================\n");
        return;
    }
    
    printf("总线配置:\n");
    printf("  - I2C总线: I2C_%d\n", I2C_BUS_NUM);
    printf("  - SCL引脚: GPIO_%d\n", g_bus_status.scl_pin);
    printf("  - SDA引脚: GPIO_%d\n", g_bus_status.sda_pin);
    printf("  - 频率: %lu Hz\n", g_bus_status.frequency);
    printf("  - 互斥锁: %s\n", (g_i2c_mutex != NULL) ? "已创建" : "未创建");
    
    printf("设备列表: (共 %d 个设备)\n", g_bus_status.device_count);
    if (g_bus_status.device_count > 0) {
        I2CBus_PrintDeviceList();
    } else {
        printf("  无设备\n");
    }
    
    printf("===============================\n");
}

void I2CBus_PrintDeviceList(void)
{
    if (g_bus_status.device_count == 0) {
        printf("  无设备连接\n");
        return;
    }
    
    printf("  +--------+----------+--------------------+\n");
    printf("  | 地址   | 状态     | 设备类型           |\n");
    printf("  +--------+----------+--------------------+\n");
    
    for (uint8_t i = 0; i < g_bus_status.device_count && i < 16; i++) {
        const I2C_DeviceInfo_t *device = &g_bus_status.devices[i];
        printf("  | 0x%02X   | %-8s | %-18s |\n",
               device->address,
               device->is_detected ? "已连接" : "未连接",
               device->name);
    }
    
    printf("  +--------+----------+--------------------+\n");
}

bool I2CBus_TestCommunication(void)
{
    if (!g_i2c_initialized) {
        printf("[I2CBusManager] 错误：I2C总线未初始化，无法测试通信\n");
        return false;
    }
    
    printf("[I2CBusManager] 开始测试I2C设备通信...\n");
    
    bool all_devices_ok = true;
    uint8_t tested_devices = 0;
    
    for (uint8_t i = 0; i < g_bus_status.device_count && i < 16; i++) {
        const I2C_DeviceInfo_t *device = &g_bus_status.devices[i];
        
        if (device->is_detected) {
            printf("[I2CBusManager] 测试设备 0x%02X (%s)...\n", device->address, device->name);
            
            // 简单的设备存在性测试
            bool device_ok = I2CBus_ScanDevice(device->address);
            
            if (device_ok) {
                printf("[I2CBusManager] ✓ 设备 0x%02X 通信正常\n", device->address);
            } else {
                printf("[I2CBusManager] ✗ 设备 0x%02X 通信异常\n", device->address);
                all_devices_ok = false;
            }
            
            tested_devices++;
            vTaskDelay(pdMS_TO_TICKS(10)); // 短暂延时
        }
    }
    
    printf("[I2CBusManager] I2C通信测试完成，测试了 %d 个设备\n", tested_devices);
    
    if (all_devices_ok) {
        printf("[I2CBusManager] ✓ 所有设备通信正常\n");
    } else {
        printf("[I2CBusManager] ✗ 存在通信异常的设备\n");
    }
    
    return all_devices_ok;
}

// ========================================
// === 内部辅助函数实现 ===
// ========================================

static void i2c_reset_bus_status(void)
{
    memset(&g_bus_status, 0, sizeof(I2C_BusStatus_t));
    g_bus_status.initialized = false;
    g_bus_status.device_count = 0;
}

static void i2c_add_device_to_status(uint8_t addr, I2C_DeviceType_t type, bool detected)
{
    if (g_bus_status.device_count >= 16) {
        printf("[I2CBusManager] 警告：设备列表已满，无法添加设备 0x%02X\n", addr);
        return;
    }
    
    I2C_DeviceInfo_t *device = &g_bus_status.devices[g_bus_status.device_count];
    device->address = addr;
    device->type = type;
    device->is_detected = detected;
    strncpy(device->name, I2CBus_GetDeviceName(type), sizeof(device->name) - 1);
    device->name[sizeof(device->name) - 1] = '\0';
} 
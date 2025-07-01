#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "qmi8658_bsp.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "QMI8658_BSP";

// === I2C 配置定义 ===
#define QMI8658_I2C_HOST            I2C_NUM_0
#define QMI8658_I2C_SDA_PIN         (15)        // 根据硬件配置调整
#define QMI8658_I2C_SCL_PIN         (14)        // 根据硬件配置调整
#define QMI8658_I2C_CLK_SPEED       (400000)    // 400kHz
#define QMI8658_I2C_TIMEOUT_MS      (1000)

// === 全局变量 ===
static bool g_qmi8658_initialized = false;
static uint8_t g_qmi8658_addr = QMI8658_L_SLAVE_ADDRESS;
static float g_accel_scale = 1.0f;
static float g_gyro_scale = 1.0f;
static bool g_accel_enabled = false;
static bool g_gyro_enabled = false;
static QMI8658_AccelRange_t g_accel_range = QMI8658_ACC_RANGE_4G;
static QMI8658_GyroRange_t g_gyro_range = QMI8658_GYR_RANGE_64DPS;
static uint8_t g_usid[6] = {0};
static uint32_t g_revision_id = 0;
static bool g_fifo_enabled = false;
static QMI8658_FifoMode_t g_fifo_mode = QMI8658_FIFO_MODE_BYPASS;

// === 内部函数声明 ===
static esp_err_t qmi8658_i2c_init(void);
static esp_err_t qmi8658_write_reg(uint8_t reg, uint8_t data);
static esp_err_t qmi8658_read_reg(uint8_t reg, uint8_t *data);
static esp_err_t qmi8658_read_regs(uint8_t reg, uint8_t *data, uint8_t len);
static esp_err_t qmi8658_write_regs(uint8_t reg, const uint8_t *data, uint8_t len);
static void qmi8658_update_accel_scale(QMI8658_AccelRange_t range);
static void qmi8658_update_gyro_scale(QMI8658_GyroRange_t range);
static int16_t qmi8658_bytes_to_int16(uint8_t lsb, uint8_t msb);
static esp_err_t qmi8658_wait_for_ready(uint32_t timeout_ms);
static esp_err_t qmi8658_write_command(QMI8658_CommandTable_t cmd, uint32_t wait_ms);
static uint8_t qmi8658_mg_to_bytes(float mg);

// === I2C 底层实现 ===
static esp_err_t qmi8658_i2c_init(void)
{
    printf("[QMI8658_BSP] === qmi8658_i2c_init() 开始执行 ===\n");
    
    // 检查I2C是否已经初始化（由触摸屏驱动初始化）
    // 如果I2C已经初始化，则直接返回成功
    // 这里我们假设触摸屏驱动会先初始化I2C
    
    printf("[QMI8658_BSP] QMI8658使用已初始化的I2C总线（与触摸屏共用）\n");
    
    // 添加I2C总线扫描功能，查看总线上有哪些设备
    printf("[QMI8658_BSP] ==========================================\n");
    printf("[QMI8658_BSP] === 开始扫描I2C总线设备 ===\n");
    printf("[QMI8658_BSP] ==========================================\n");
    uint8_t devices_found = 0;
    
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (I2C_Lock(50)) {
            // 创建I2C命令链路进行设备检测
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
            i2c_master_stop(cmd);
            
            esp_err_t ret = i2c_master_cmd_begin(QMI8658_I2C_HOST, cmd, pdMS_TO_TICKS(50));
            i2c_cmd_link_delete(cmd);
            I2C_Unlock();
            
            if (ret == ESP_OK) {
                printf("[QMI8658_BSP] >>> 发现I2C设备，地址: 0x%02X <<<\n", addr);
                devices_found++;
                
                // 特殊设备识别
                if (addr == 0x38) {
                    printf("[QMI8658_BSP]     -> 这是触摸屏设备(FT3168)\n");
                } else if (addr == 0x6A || addr == 0x6B) {
                    printf("[QMI8658_BSP]     -> 这可能是QMI8658陀螺仪\n");
                } else {
                    printf("[QMI8658_BSP]     -> 未知设备\n");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    printf("[QMI8658_BSP] ==========================================\n");
    printf("[QMI8658_BSP] I2C总线扫描完成，共发现 %d 个设备\n", devices_found);
    printf("[QMI8658_BSP] ==========================================\n");
    
    // 尝试读取芯片ID来验证I2C通信是否正常
    uint8_t chip_id = 0;
    
    // 尝试主地址 (0x6B)
    g_qmi8658_addr = QMI8658_L_SLAVE_ADDRESS;
    esp_err_t ret = qmi8658_read_reg(QMI8658_REG_WHOAMI, &chip_id);
    
    if (ret == ESP_OK && chip_id == QMI8658_REG_WHOAMI_DEFAULT) {
        printf("[QMI8658_BSP] QMI8658检测成功，地址: 0x%02X，芯片ID: 0x%02X\n", g_qmi8658_addr, chip_id);
        return ESP_OK;
    }
    
    // 尝试备用地址 (0x6A)
    g_qmi8658_addr = QMI8658_H_SLAVE_ADDRESS;
    ret = qmi8658_read_reg(QMI8658_REG_WHOAMI, &chip_id);
    
    if (ret == ESP_OK && chip_id == QMI8658_REG_WHOAMI_DEFAULT) {
        printf("[QMI8658_BSP] QMI8658检测成功，地址: 0x%02X，芯片ID: 0x%02X\n", g_qmi8658_addr, chip_id);
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "QMI8658检测失败，可能原因：");
    ESP_LOGE(TAG, "1. QMI8658硬件未连接或未上电");
    ESP_LOGE(TAG, "2. I2C地址配置错误");
    ESP_LOGE(TAG, "3. I2C总线时序问题");
    ESP_LOGE(TAG, "4. 与触摸屏的I2C总线冲突");
    
    return ESP_FAIL;
}

static esp_err_t qmi8658_write_reg(uint8_t reg, uint8_t data)
{
    // 获取I2C总线互斥锁
    if (!I2C_Lock(QMI8658_I2C_TIMEOUT_MS)) {
        ESP_LOGW(TAG, "获取I2C锁失败，写寄存器0x%02X被跳过", reg);
        return ESP_ERR_TIMEOUT;
    }
    
    uint8_t write_buf[2] = {reg, data};
    esp_err_t ret = i2c_master_write_to_device(QMI8658_I2C_HOST, g_qmi8658_addr, 
                                              write_buf, 2, 
                                              pdMS_TO_TICKS(QMI8658_I2C_TIMEOUT_MS));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "写寄存器0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    // 释放I2C总线互斥锁
    I2C_Unlock();
    
    return ret;
}

static esp_err_t qmi8658_read_reg(uint8_t reg, uint8_t *data)
{
    // 获取I2C总线互斥锁
    if (!I2C_Lock(QMI8658_I2C_TIMEOUT_MS)) {
        ESP_LOGW(TAG, "获取I2C锁失败，读寄存器0x%02X被跳过", reg);
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = i2c_master_write_read_device(QMI8658_I2C_HOST, g_qmi8658_addr,
                                                &reg, 1, data, 1,
                                                pdMS_TO_TICKS(QMI8658_I2C_TIMEOUT_MS));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读寄存器0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    // 释放I2C总线互斥锁
    I2C_Unlock();
    
    return ret;
}

static esp_err_t qmi8658_read_regs(uint8_t reg, uint8_t *data, uint8_t len)
{
    // 获取I2C总线互斥锁
    if (!I2C_Lock(QMI8658_I2C_TIMEOUT_MS)) {
        ESP_LOGW(TAG, "获取I2C锁失败，读多个寄存器0x%02X被跳过", reg);
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t ret = i2c_master_write_read_device(QMI8658_I2C_HOST, g_qmi8658_addr,
                                                &reg, 1, data, len,
                                                pdMS_TO_TICKS(QMI8658_I2C_TIMEOUT_MS));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读多个寄存器0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    // 释放I2C总线互斥锁
    I2C_Unlock();
    
    return ret;
}

static esp_err_t qmi8658_write_regs(uint8_t reg, const uint8_t *data, uint8_t len)
{
    // 获取I2C总线互斥锁
    if (!I2C_Lock(QMI8658_I2C_TIMEOUT_MS)) {
        ESP_LOGW(TAG, "获取I2C锁失败，写多个寄存器0x%02X被跳过", reg);
        return ESP_ERR_TIMEOUT;
    }
    
    uint8_t *write_buf = malloc(len + 1);
    if (!write_buf) {
        I2C_Unlock();
        return ESP_ERR_NO_MEM;
    }
    
    write_buf[0] = reg;
    memcpy(&write_buf[1], data, len);
    
    esp_err_t ret = i2c_master_write_to_device(QMI8658_I2C_HOST, g_qmi8658_addr,
                                              write_buf, len + 1,
                                              pdMS_TO_TICKS(QMI8658_I2C_TIMEOUT_MS));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "写多个寄存器0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    free(write_buf);
    // 释放I2C总线互斥锁
    I2C_Unlock();
    
    return ret;
}

// === 内部辅助函数实现 ===
static void qmi8658_update_accel_scale(QMI8658_AccelRange_t range)
{
    switch (range) {
        case QMI8658_ACC_RANGE_2G:
            g_accel_scale = 2.0f / 32768.0f * 9.80665f;  // 转换为 m/s²
            break;
        case QMI8658_ACC_RANGE_4G:
            g_accel_scale = 4.0f / 32768.0f * 9.80665f;
            break;
        case QMI8658_ACC_RANGE_8G:
            g_accel_scale = 8.0f / 32768.0f * 9.80665f;
            break;
        case QMI8658_ACC_RANGE_16G:
            g_accel_scale = 16.0f / 32768.0f * 9.80665f;
            break;
        default:
            g_accel_scale = 4.0f / 32768.0f * 9.80665f;
            break;
    }
    g_accel_range = range;
}

static void qmi8658_update_gyro_scale(QMI8658_GyroRange_t range)
{
    switch (range) {
        case QMI8658_GYR_RANGE_16DPS:
            g_gyro_scale = 16.0f / 32768.0f;
            break;
        case QMI8658_GYR_RANGE_32DPS:
            g_gyro_scale = 32.0f / 32768.0f;
            break;
        case QMI8658_GYR_RANGE_64DPS:
            g_gyro_scale = 64.0f / 32768.0f;
            break;
        case QMI8658_GYR_RANGE_128DPS:
            g_gyro_scale = 128.0f / 32768.0f;
            break;
        case QMI8658_GYR_RANGE_256DPS:
            g_gyro_scale = 256.0f / 32768.0f;
            break;
        case QMI8658_GYR_RANGE_512DPS:
            g_gyro_scale = 512.0f / 32768.0f;
            break;
        case QMI8658_GYR_RANGE_1024DPS:
            g_gyro_scale = 1024.0f / 32768.0f;
            break;
        default:
            g_gyro_scale = 64.0f / 32768.0f;
            break;
    }
    g_gyro_range = range;
}

static int16_t qmi8658_bytes_to_int16(uint8_t lsb, uint8_t msb)
{
    return (int16_t)((msb << 8) | lsb);
}

static esp_err_t qmi8658_wait_for_ready(uint32_t timeout_ms)
{
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint8_t status;
    
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < timeout_ms) {
        if (qmi8658_read_reg(QMI8658_REG_STATUS0, &status) == ESP_OK) {
            if (status != 0) {
                return ESP_OK;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    return ESP_ERR_TIMEOUT;
}

// 命令处理函数实现
static esp_err_t qmi8658_write_command(QMI8658_CommandTable_t cmd, uint32_t wait_ms)
{
    uint32_t start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint8_t status;
    
    // 写入命令到CTRL9寄存器
    if (qmi8658_write_reg(QMI8658_REG_CTRL9, (uint8_t)cmd) != ESP_OK) {
        return ESP_FAIL;
    }
    
    // 等待命令完成标志
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < wait_ms) {
        if (qmi8658_read_reg(QMI8658_REG_STATUS_INT, &status) == ESP_OK) {
            if (status & 0x80) {  // 检查bit7（命令完成标志）
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) >= wait_ms) {
        ESP_LOGE(TAG, "等待命令0x%02X完成超时，状态: 0x%02X", cmd, status);
        return ESP_ERR_TIMEOUT;
    }
    
    // 发送ACK确认
    if (qmi8658_write_reg(QMI8658_REG_CTRL9, QMI8658_CTRL_CMD_ACK) != ESP_OK) {
        return ESP_FAIL;
    }
    
    // 等待ACK完成
    start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - start_time) < wait_ms) {
        if (qmi8658_read_reg(QMI8658_REG_STATUS_INT, &status) == ESP_OK) {
            if (!(status & 0x80)) {  // 检查bit7被清除
                return ESP_OK;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    ESP_LOGE(TAG, "清除命令完成标志超时，命令: 0x%02X，状态: 0x%02X", cmd, status);
    return ESP_ERR_TIMEOUT;
}

static uint8_t qmi8658_mg_to_bytes(float mg)
{
    float g = mg / 1000.0f;      // 转换为g
    int units = (int)round(g / 0.03125f); // 转换为指定分辨率(1/32)的单位
    return (units & 0x1F) << 3; // 将5位小数左移3位，因为只有3位整数位
}

// === 公共接口实现 ===

uint8_t QMI8658_Init(void)
{
    printf("[QMI8658_BSP] 初始化QMI8658传感器...\n");
    
    if (g_qmi8658_initialized) {
        printf("[QMI8658_BSP] 警告：QMI8658已经初始化\n");
        return 0;
    }
    
    // 初始化I2C并检测传感器
    esp_err_t init_result = qmi8658_i2c_init();
    
    if (init_result != ESP_OK) {
        ESP_LOGE(TAG, "I2C初始化失败或传感器检测失败");
        return 1;
    }
    
    // 软重置传感器
    if (QMI8658_Reset() != 0) {
        ESP_LOGE(TAG, "传感器重置失败");
        return 1;
    }
    
    // 等待传感器稳定（重置后需要时间稳定）
    vTaskDelay(pdMS_TO_TICKS(50));  // 等待50ms让传感器稳定
    
    // 验证传感器是否可以正常通信
    uint8_t test_id;
    if (qmi8658_read_reg(QMI8658_REG_WHOAMI, &test_id) != ESP_OK || test_id != QMI8658_REG_WHOAMI_DEFAULT) {
        ESP_LOGE(TAG, "传感器重置后通信异常，芯片ID: 0x%02X", test_id);
        return 1;
    }
    
    // 使能地址自动递增和使用STATUS_INT.bit7作为CTRL9握手
    qmi8658_write_reg(QMI8658_REG_CTRL1, 0x40);  // Little-Endian / 地址自动递增
    qmi8658_write_reg(QMI8658_REG_CTRL8, 0x80);  // 使用STATUS_INT.bit7作为CTRL9握手
    
    // 获取固件版本和USID
    if (qmi8658_write_command(QMI8658_CTRL_CMD_COPY_USID, 1000) == ESP_OK) {
        uint8_t buffer[3];
        if (qmi8658_read_regs(QMI8658_REG_DQW_L, buffer, 3) == ESP_OK) {
            g_revision_id = buffer[0] | (uint32_t)(buffer[1] << 8) | (uint32_t)(buffer[2] << 16);
            ESP_LOGI(TAG, "固件版本: 0x%02X%02X%02X", buffer[0], buffer[1], buffer[2]);
        }
        
        if (qmi8658_read_regs(QMI8658_REG_DVX_L, g_usid, 6) == ESP_OK) {
            ESP_LOGI(TAG, "USID: %02X%02X%02X%02X%02X%02X",
                     g_usid[0], g_usid[1], g_usid[2],
                     g_usid[3], g_usid[4], g_usid[5]);
        }
    }
    
    // 默认配置加速度计和陀螺仪
    QMI8658_ConfigAccelerometer(QMI8658_ACC_RANGE_4G, 
                               QMI8658_ACC_ODR_1000Hz, 
                               QMI8658_LPF_MODE_0);
    
    QMI8658_ConfigGyroscope(QMI8658_GYR_RANGE_64DPS,
                           QMI8658_GYR_ODR_896_8Hz,
                           QMI8658_LPF_MODE_3);
    
    // 使能加速度计和陀螺仪
    QMI8658_EnableAccelerometer();
    QMI8658_EnableGyroscope();
    
    // 等待传感器开始产生数据
    vTaskDelay(pdMS_TO_TICKS(100));  // 额外等待100ms
    
    g_qmi8658_initialized = true;
    ESP_LOGI(TAG, "QMI8658初始化完成");
    
    return 0;
}

void QMI8658_Deinit(void)
{
    if (!g_qmi8658_initialized) {
        return;
    }
    
    // 禁用传感器
    QMI8658_DisableAccelerometer();
    QMI8658_DisableGyroscope();
    
    // 卸载I2C驱动
    i2c_driver_delete(QMI8658_I2C_HOST);
    
    g_qmi8658_initialized = false;
    ESP_LOGI(TAG, "QMI8658反初始化完成");
}

uint8_t QMI8658_IsConnected(void)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    uint8_t chip_id;
    if (qmi8658_read_reg(QMI8658_REG_WHOAMI, &chip_id) != ESP_OK) {
        return 0;
    }
    
    return (chip_id == QMI8658_REG_WHOAMI_DEFAULT) ? 1 : 0;
}

uint8_t QMI8658_GetChipID(void)
{
    uint8_t chip_id = 0;
    qmi8658_read_reg(QMI8658_REG_WHOAMI, &chip_id);
    return chip_id;
}

uint8_t QMI8658_GetDeviceInfo(QMI8658_DeviceInfo_t *info)
{
    if (!info || !g_qmi8658_initialized) {
        return 1;
    }
    
    info->chip_id = QMI8658_GetChipID();
    info->revision_id = g_revision_id;
    memcpy(info->usid, g_usid, 6);
    info->accel_enabled = g_accel_enabled;
    info->gyro_enabled = g_gyro_enabled;
    info->accel_scale = g_accel_scale;
    info->gyro_scale = g_gyro_scale;
    info->accel_range = g_accel_range;
    info->gyro_range = g_gyro_range;
    
    return 0;
}

uint8_t QMI8658_Reset(void)
{
    ESP_LOGI(TAG, "重置QMI8658传感器");
    
    if (qmi8658_write_reg(QMI8658_REG_RESET, QMI8658_REG_RESET_DEFAULT) != ESP_OK) {
        return 1;
    }
    
    // 等待重置完成
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return 0;
}

uint8_t QMI8658_ConfigAccelerometer(QMI8658_AccelRange_t range, 
                                   QMI8658_AccelODR_t odr, 
                                   QMI8658_LpfMode_t lpf)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 配置CTRL2寄存器 (加速度计配置)
    uint8_t ctrl2 = (range << 4) | odr;
    if (qmi8658_write_reg(QMI8658_REG_CTRL2, ctrl2) != ESP_OK) {
        return 1;
    }
    
    // 配置CTRL5寄存器 (低通滤波器)
    uint8_t ctrl5;
    if (qmi8658_read_reg(QMI8658_REG_CTRL5, &ctrl5) != ESP_OK) {
        return 1;
    }
    
    ctrl5 = (ctrl5 & 0xF0) | lpf;  // 保留其他位，只设置低4位
    if (qmi8658_write_reg(QMI8658_REG_CTRL5, ctrl5) != ESP_OK) {
        return 1;
    }
    
    // 更新量程
    qmi8658_update_accel_scale(range);
    
    ESP_LOGI(TAG, "加速度计配置完成: 量程=%d, ODR=%d, LPF=%d", range, odr, lpf);
    return 0;
}

uint8_t QMI8658_ConfigGyroscope(QMI8658_GyroRange_t range, 
                               QMI8658_GyroODR_t odr, 
                               QMI8658_LpfMode_t lpf)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 配置CTRL3寄存器 (陀螺仪配置)
    uint8_t ctrl3 = (range << 4) | odr;
    if (qmi8658_write_reg(QMI8658_REG_CTRL3, ctrl3) != ESP_OK) {
        return 1;
    }
    
    // 配置CTRL5寄存器 (低通滤波器)
    uint8_t ctrl5;
    if (qmi8658_read_reg(QMI8658_REG_CTRL5, &ctrl5) != ESP_OK) {
        return 1;
    }
    
    ctrl5 = (ctrl5 & 0x0F) | (lpf << 4);  // 保留其他位，只设置高4位
    if (qmi8658_write_reg(QMI8658_REG_CTRL5, ctrl5) != ESP_OK) {
        return 1;
    }
    
    // 更新量程
    qmi8658_update_gyro_scale(range);
    
    ESP_LOGI(TAG, "陀螺仪配置完成: 量程=%d, ODR=%d, LPF=%d", range, odr, lpf);
    return 0;
}

uint8_t QMI8658_EnableAccelerometer(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 |= QMI8658_ACCEL_EN_MASK;  // CTRL7 bit 0
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    g_accel_enabled = true;
    ESP_LOGI(TAG, "加速度计已使能（CTRL7）");
    return 0;
}

uint8_t QMI8658_DisableAccelerometer(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 &= ~QMI8658_ACCEL_EN_MASK;  // CTRL7 bit 0
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    g_accel_enabled = false;
    ESP_LOGI(TAG, "加速度计已禁用（CTRL7）");
    return 0;
}

uint8_t QMI8658_EnableGyroscope(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 |= QMI8658_GYRO_EN_MASK;  // CTRL7 bit 1
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    g_gyro_enabled = true;
    ESP_LOGI(TAG, "陀螺仪已使能（CTRL7）");
    return 0;
}

uint8_t QMI8658_DisableGyroscope(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 &= ~QMI8658_GYRO_EN_MASK;  // CTRL7 bit 1
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    g_gyro_enabled = false;
    ESP_LOGI(TAG, "陀螺仪已禁用（CTRL7）");
    return 0;
}

uint8_t QMI8658_GetAccelerometerData(QMI8658_IMUData_t *data)
{
    if (!g_qmi8658_initialized || !g_accel_enabled || !data) {
        return 0;
    }
    
    uint8_t raw_data[6];
    if (qmi8658_read_regs(QMI8658_REG_AX_L, raw_data, 6) != ESP_OK) {
        return 0;
    }
    
    int16_t x_raw = qmi8658_bytes_to_int16(raw_data[0], raw_data[1]);
    int16_t y_raw = qmi8658_bytes_to_int16(raw_data[2], raw_data[3]);
    int16_t z_raw = qmi8658_bytes_to_int16(raw_data[4], raw_data[5]);
    
    data->x = x_raw * g_accel_scale;
    data->y = y_raw * g_accel_scale;
    data->z = z_raw * g_accel_scale;
    
    return 1;
}

uint8_t QMI8658_GetGyroscopeData(QMI8658_IMUData_t *data)
{
    if (!g_qmi8658_initialized || !g_gyro_enabled || !data) {
        return 0;
    }
    
    uint8_t raw_data[6];
    if (qmi8658_read_regs(QMI8658_REG_GX_L, raw_data, 6) != ESP_OK) {
        return 0;
    }
    
    int16_t x_raw = qmi8658_bytes_to_int16(raw_data[0], raw_data[1]);
    int16_t y_raw = qmi8658_bytes_to_int16(raw_data[2], raw_data[3]);
    int16_t z_raw = qmi8658_bytes_to_int16(raw_data[4], raw_data[5]);
    
    data->x = x_raw * g_gyro_scale;
    data->y = y_raw * g_gyro_scale;
    data->z = z_raw * g_gyro_scale;
    
    return 1;
}

uint8_t QMI8658_GetAcceleration(float *x, float *y, float *z)
{
    QMI8658_IMUData_t data;
    if (QMI8658_GetAccelerometerData(&data)) {
        if (x) *x = data.x;
        if (y) *y = data.y;
        if (z) *z = data.z;
        return 1;
    }
    return 0;
}

uint8_t QMI8658_GetAngularVelocity(float *x, float *y, float *z)
{
    QMI8658_IMUData_t data;
    if (QMI8658_GetGyroscopeData(&data)) {
        if (x) *x = data.x;
        if (y) *y = data.y;
        if (z) *z = data.z;
        return 1;
    }
    return 0;
}

uint8_t QMI8658_GetAccelRaw(int16_t *raw_data)
{
    if (!g_qmi8658_initialized || !g_accel_enabled || !raw_data) {
        return 0;
    }
    
    uint8_t raw_buf[6];
    if (qmi8658_read_regs(QMI8658_REG_AX_L, raw_buf, 6) != ESP_OK) {
        return 0;
    }
    
    raw_data[0] = qmi8658_bytes_to_int16(raw_buf[0], raw_buf[1]); // X
    raw_data[1] = qmi8658_bytes_to_int16(raw_buf[2], raw_buf[3]); // Y
    raw_data[2] = qmi8658_bytes_to_int16(raw_buf[4], raw_buf[5]); // Z
    
    return 1;
}

uint8_t QMI8658_GetGyroRaw(int16_t *raw_data)
{
    if (!g_qmi8658_initialized || !g_gyro_enabled || !raw_data) {
        return 0;
    }
    
    uint8_t raw_buf[6];
    if (qmi8658_read_regs(QMI8658_REG_GX_L, raw_buf, 6) != ESP_OK) {
        return 0;
    }
    
    raw_data[0] = qmi8658_bytes_to_int16(raw_buf[0], raw_buf[1]); // X
    raw_data[1] = qmi8658_bytes_to_int16(raw_buf[2], raw_buf[3]); // Y
    raw_data[2] = qmi8658_bytes_to_int16(raw_buf[4], raw_buf[5]); // Z
    
    return 1;
}

float QMI8658_GetTemperature(void)
{
    if (!g_qmi8658_initialized) {
        return -999.0f;
    }
    
    uint8_t raw_data[2];
    if (qmi8658_read_regs(QMI8658_REG_TEMPERATURE_L, raw_data, 2) != ESP_OK) {
        return -999.0f;
    }
    
    int16_t temp_raw = qmi8658_bytes_to_int16(raw_data[0], raw_data[1]);
    // 温度转换公式：T = temp_raw / 256.0 + 25.0
    return (temp_raw / 256.0f) + 25.0f;
}

uint32_t QMI8658_GetTimestamp(void)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    uint8_t raw_data[3];
    if (qmi8658_read_regs(QMI8658_REG_TIMESTAMP_L, raw_data, 3) != ESP_OK) {
        return 0;
    }
    
    return (uint32_t)(raw_data[0] | (raw_data[1] << 8) | (raw_data[2] << 16));
}

uint8_t QMI8658_GetDataReady(void)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    uint8_t status;
    if (qmi8658_read_reg(QMI8658_REG_STATUS0, &status) != ESP_OK) {
        return 0;
    }
    
    return status & (QMI8658_STATUS0_ACCEL_AVAIL | QMI8658_STATUS0_GYRO_AVAIL);
}

uint8_t QMI8658_CalibrateGyro(void)
{
    if (!g_qmi8658_initialized || !g_gyro_enabled) {
        return 0;
    }
    
    ESP_LOGI(TAG, "开始陀螺仪校准...");
    
    // 简单的静态校准：收集一定数量的样本并计算偏移
    const int samples = 100;
    float sum_x = 0, sum_y = 0, sum_z = 0;
    
    for (int i = 0; i < samples; i++) {
        QMI8658_IMUData_t data;
        if (QMI8658_GetGyroscopeData(&data)) {
            sum_x += data.x;
            sum_y += data.y;
            sum_z += data.z;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    float offset_x = sum_x / samples;
    float offset_y = sum_y / samples;
    float offset_z = sum_z / samples;
    
    ESP_LOGI(TAG, "陀螺仪校准完成，偏移值: X=%.3f, Y=%.3f, Z=%.3f", 
             offset_x, offset_y, offset_z);
    
    // 注意：这里的偏移值需要在后续的数据读取中应用
    // 为了简化，这里只是打印出来，实际应用中需要存储并使用
    
    return 1;
}

// 简化版本的自检函数已删除，使用完整版本的实现

// === FIFO接口函数实现 ===

uint8_t QMI8658_ConfigFIFO(QMI8658_FifoMode_t mode, QMI8658_FifoSamples_t samples)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t fifo_ctrl = ((uint8_t)mode << 6) | (samples << 2);
    if (qmi8658_write_reg(QMI8658_REG_FIFO_CTRL, fifo_ctrl) != ESP_OK) {
        return 1;
    }
    
    // 设置FIFO水位标记
    uint8_t watermark = 0;
    switch (samples) {
        case QMI8658_FIFO_SAMPLES_16:  watermark = 16; break;
        case QMI8658_FIFO_SAMPLES_32:  watermark = 32; break;
        case QMI8658_FIFO_SAMPLES_64:  watermark = 64; break;
        case QMI8658_FIFO_SAMPLES_128: watermark = 128; break;
    }
    
    if (qmi8658_write_reg(QMI8658_REG_FIFO_WTM_TH, watermark) != ESP_OK) {
        return 1;
    }
    
    g_fifo_mode = mode;
    g_fifo_enabled = (mode != QMI8658_FIFO_MODE_BYPASS);
    
    ESP_LOGI(TAG, "FIFO配置完成: 模式=%d, 采样数=%d", mode, watermark);
    return 0;
}

uint8_t QMI8658_EnableFIFO(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 使能FIFO映射到INT1
    uint8_t ctrl1;
    if (qmi8658_read_reg(QMI8658_REG_CTRL1, &ctrl1) != ESP_OK) {
        return 1;
    }
    
    ctrl1 |= QMI8658_FIFO_MAP_INT1;
    if (qmi8658_write_reg(QMI8658_REG_CTRL1, ctrl1) != ESP_OK) {
        return 1;
    }
    
    g_fifo_enabled = true;
    ESP_LOGI(TAG, "FIFO已使能");
    return 0;
}

uint8_t QMI8658_DisableFIFO(void)
{
    return QMI8658_ConfigFIFO(QMI8658_FIFO_MODE_BYPASS, QMI8658_FIFO_SAMPLES_16);
}

uint8_t QMI8658_ResetFIFO(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    return (qmi8658_write_command(QMI8658_CTRL_CMD_RST_FIFO, 1000) == ESP_OK) ? 0 : 1;
}

uint16_t QMI8658_GetFIFOCount(void)
{
    if (!g_qmi8658_initialized || !g_fifo_enabled) {
        return 0;
    }
    
    uint8_t count;
    if (qmi8658_read_reg(QMI8658_REG_FIFO_COUNT, &count) != ESP_OK) {
        return 0;
    }
    
    return count;
}

uint16_t QMI8658_ReadFromFIFO(QMI8658_IMUData_t *acc_data, uint16_t acc_length,
                              QMI8658_IMUData_t *gyro_data, uint16_t gyro_length)
{
    if (!g_qmi8658_initialized || !g_fifo_enabled) {
        return 0;
    }
    
    uint16_t fifo_count = QMI8658_GetFIFOCount();
    if (fifo_count == 0) {
        return 0;
    }
    
    // 计算实际要读取的样本数量（考虑加速度计和陀螺仪都启用的情况）
    uint16_t max_samples = acc_length;
    if (gyro_data && gyro_length < max_samples) {
        max_samples = gyro_length;
    }
    
    uint16_t samples_to_read = (fifo_count < max_samples) ? fifo_count : max_samples;
    uint16_t samples_read = 0;
    
    // 根据传感器启用状态确定每个样本的字节数
    uint8_t bytes_per_sample = 0;
    if (g_accel_enabled) bytes_per_sample += 6;  // 加速度计数据
    if (g_gyro_enabled) bytes_per_sample += 6;   // 陀螺仪数据
    
    if (bytes_per_sample == 0) {
        ESP_LOGW(TAG, "没有传感器启用，无法读取FIFO数据");
        return 0;
    }
    
    for (uint16_t i = 0; i < samples_to_read; i++) {
        uint8_t fifo_data[12];  // 最大 6 bytes accel + 6 bytes gyro
        if (qmi8658_read_regs(QMI8658_REG_FIFO_DATA, fifo_data, bytes_per_sample) != ESP_OK) {
            break;
        }
        
        uint8_t offset = 0;
        
        // 解析加速度计数据（如果启用）
        if (g_accel_enabled && acc_data) {
            int16_t ax = qmi8658_bytes_to_int16(fifo_data[offset], fifo_data[offset + 1]);
            int16_t ay = qmi8658_bytes_to_int16(fifo_data[offset + 2], fifo_data[offset + 3]);
            int16_t az = qmi8658_bytes_to_int16(fifo_data[offset + 4], fifo_data[offset + 5]);
            
            acc_data[i].x = ax * g_accel_scale;
            acc_data[i].y = ay * g_accel_scale;
            acc_data[i].z = az * g_accel_scale;
            offset += 6;
        }
        
        // 解析陀螺仪数据（如果启用）
        if (g_gyro_enabled && gyro_data) {
            int16_t gx = qmi8658_bytes_to_int16(fifo_data[offset], fifo_data[offset + 1]);
            int16_t gy = qmi8658_bytes_to_int16(fifo_data[offset + 2], fifo_data[offset + 3]);
            int16_t gz = qmi8658_bytes_to_int16(fifo_data[offset + 4], fifo_data[offset + 5]);
            
            gyro_data[i].x = gx * g_gyro_scale;
            gyro_data[i].y = gy * g_gyro_scale;
            gyro_data[i].z = gz * g_gyro_scale;
        }
        
        samples_read++;
    }
    
    ESP_LOGI(TAG, "从FIFO读取了%d个样本", samples_read);
    return samples_read;
}

// === 高级功能接口函数实现 ===

uint8_t QMI8658_ConfigTap(const QMI8658_TapConfig_t *config)
{
    if (!g_qmi8658_initialized || !config) {
        return 1;
    }
    
    // 准备配置数据
    uint8_t tap_config[8];
    tap_config[0] = config->priority;
    tap_config[1] = config->peak_window;
    tap_config[2] = config->tap_window & 0xFF;
    tap_config[3] = (config->tap_window >> 8) & 0xFF;
    tap_config[4] = config->dtap_window & 0xFF;
    tap_config[5] = (config->dtap_window >> 8) & 0xFF;
    tap_config[6] = (uint8_t)(config->alpha * 32);      // 转换为寄存器格式
    tap_config[7] = (uint8_t)(config->gamma * 32);      // 转换为寄存器格式
    
    // 写入配置到CAL寄存器
    if (qmi8658_write_regs(QMI8658_REG_CAL1_L, tap_config, 8) != ESP_OK) {
        return 1;
    }
    
    // 发送配置命令
    if (qmi8658_write_command(QMI8658_CTRL_CMD_CONFIGURE_TAP, 1000) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "敲击检测配置完成");
    return 0;
}

uint8_t QMI8658_EnableTap(QMI8658_IntPin_t pin)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 配置中断引脚映射
    if (pin != QMI8658_INT_PIN_DISABLE) {
        uint8_t ctrl7;
        if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
            return 1;
        }
        
        if (pin == QMI8658_INT_PIN_1) {
            ctrl7 |= 0x02;  // 映射到INT1
        } else {
            ctrl7 |= 0x20;  // 映射到INT2
        }
        
        if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
            return 1;
        }
    }
    
    ESP_LOGI(TAG, "敲击检测已使能，中断引脚: %d", pin);
    return 0;
}

uint8_t QMI8658_DisableTap(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 清除中断引脚映射
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 &= ~(0x02 | 0x20);  // 清除INT1和INT2的敲击映射
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "敲击检测已禁用");
    return 0;
}

QMI8658_TapEvent_t QMI8658_GetTapStatus(void)
{
    if (!g_qmi8658_initialized) {
        return QMI8658_TAP_EVENT_INVALID;
    }
    
    uint8_t tap_status;
    if (qmi8658_read_reg(QMI8658_REG_TAP_STATUS, &tap_status) != ESP_OK) {
        return QMI8658_TAP_EVENT_INVALID;
    }
    
    if (tap_status & 0x20) {
        return QMI8658_TAP_EVENT_DOUBLE;
    } else if (tap_status & 0x10) {
        return QMI8658_TAP_EVENT_SINGLE;
    }
    
    return QMI8658_TAP_EVENT_INVALID;
}

// === 计步器功能实现 ===

uint8_t QMI8658_ConfigPedometer(const QMI8658_PedometerConfig_t *config)
{
    if (!g_qmi8658_initialized || !config) {
        return 1;
    }
    
    // 准备配置数据
    uint8_t ped_config[8];
    ped_config[0] = config->ped_sample_cnt & 0xFF;
    ped_config[1] = (config->ped_sample_cnt >> 8) & 0xFF;
    ped_config[2] = config->ped_fix_peak2peak & 0xFF;
    ped_config[3] = (config->ped_fix_peak2peak >> 8) & 0xFF;
    ped_config[4] = config->ped_fix_peak & 0xFF;
    ped_config[5] = (config->ped_fix_peak >> 8) & 0xFF;
    ped_config[6] = config->ped_time_up & 0xFF;
    ped_config[7] = (config->ped_time_up >> 8) & 0xFF;
    
    // 写入配置
    if (qmi8658_write_regs(QMI8658_REG_CAL1_L, ped_config, 8) != ESP_OK) {
        return 1;
    }
    
    // 发送配置命令
    if (qmi8658_write_command(QMI8658_CTRL_CMD_CONFIGURE_PEDOMETER, 1000) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "计步器配置完成");
    return 0;
}

uint8_t QMI8658_EnablePedometer(QMI8658_IntPin_t pin)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 配置中断引脚映射
    if (pin != QMI8658_INT_PIN_DISABLE) {
        uint8_t ctrl7;
        if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
            return 1;
        }
        
        if (pin == QMI8658_INT_PIN_1) {
            ctrl7 |= 0x10;  // 映射到INT1
        } else {
            ctrl7 |= 0x08;  // 映射到INT2
        }
        
        if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
            return 1;
        }
    }
    
    ESP_LOGI(TAG, "计步器已使能，中断引脚: %d", pin);
    return 0;
}

uint8_t QMI8658_DisablePedometer(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 清除中断引脚映射
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 &= ~(0x10 | 0x08);  // 清除计步器映射
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "计步器已禁用");
    return 0;
}

uint32_t QMI8658_GetPedometerCounter(void)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    uint8_t step_data[3];
    if (qmi8658_read_regs(QMI8658_REG_STEP_CNT_LOW, step_data, 3) != ESP_OK) {
        return 0;
    }
    
    return step_data[0] | (step_data[1] << 8) | (step_data[2] << 16);
}

uint8_t QMI8658_ClearPedometerCounter(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    return (qmi8658_write_command(QMI8658_CTRL_CMD_RESET_PEDOMETER, 1000) == ESP_OK) ? 0 : 1;
}

// === 运动检测功能实现 ===

uint8_t QMI8658_ConfigWakeOnMotion(uint8_t threshold, uint8_t blank_time)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 准备WOM配置数据
    uint8_t wom_config[8] = {0};
    wom_config[0] = threshold;
    wom_config[1] = blank_time;
    
    // 写入配置
    if (qmi8658_write_regs(QMI8658_REG_CAL1_L, wom_config, 8) != ESP_OK) {
        return 1;
    }
    
    // 发送配置命令
    if (qmi8658_write_command(QMI8658_CTRL_CMD_WRITE_WOM_SETTING, 1000) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "运动唤醒配置完成，阈值: %d，空白时间: %d", threshold, blank_time);
    return 0;
}

uint8_t QMI8658_EnableWakeOnMotion(QMI8658_IntPin_t pin)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 配置中断引脚映射
    if (pin != QMI8658_INT_PIN_DISABLE) {
        uint8_t ctrl7;
        if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
            return 1;
        }
        
        if (pin == QMI8658_INT_PIN_1) {
            ctrl7 |= 0x04;  // 映射到INT1
        } else {
            ctrl7 |= 0x40;  // 映射到INT2
        }
        
        if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
            return 1;
        }
    }
    
    ESP_LOGI(TAG, "运动唤醒已使能，中断引脚: %d", pin);
    return 0;
}

uint8_t QMI8658_DisableWakeOnMotion(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 清除中断引脚映射
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 &= ~(0x04 | 0x40);  // 清除WOM映射
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "运动唤醒已禁用");
    return 0;
}

void QMI8658_PrintStatus(void)
{
    printf("=== QMI8658 状态信息 ===\n");
    printf("初始化状态: %s\n", g_qmi8658_initialized ? "已初始化" : "未初始化");
    
    if (!g_qmi8658_initialized) {
        return;
    }
    
    printf("设备地址: 0x%02X\n", g_qmi8658_addr);
    printf("芯片ID: 0x%02X\n", QMI8658_GetChipID());
    printf("固件版本: 0x%06X\n", g_revision_id);
    printf("USID: %02X%02X%02X%02X%02X%02X\n", 
           g_usid[0], g_usid[1], g_usid[2], g_usid[3], g_usid[4], g_usid[5]);
    printf("连接状态: %s\n", QMI8658_IsConnected() ? "已连接" : "未连接");
    printf("加速度计: %s (量程: %d)\n", g_accel_enabled ? "已使能" : "已禁用", g_accel_range);
    printf("陀螺仪: %s (量程: %d)\n", g_gyro_enabled ? "已使能" : "已禁用", g_gyro_range);
    printf("FIFO: %s (模式: %d)\n", g_fifo_enabled ? "已使能" : "已禁用", g_fifo_mode);
    printf("温度: %.2f°C\n", QMI8658_GetTemperature());
    
    if (g_accel_enabled) {
        QMI8658_IMUData_t acc_data;
        if (QMI8658_GetAccelerometerData(&acc_data)) {
            printf("加速度: X=%.3f, Y=%.3f, Z=%.3f (m/s²)\n", 
                   acc_data.x, acc_data.y, acc_data.z);
        }
    }
    
    if (g_gyro_enabled) {
        QMI8658_IMUData_t gyro_data;
        if (QMI8658_GetGyroscopeData(&gyro_data)) {
            printf("角速度: X=%.3f, Y=%.3f, Z=%.3f (°/s)\n", 
                   gyro_data.x, gyro_data.y, gyro_data.z);
        }
    }
    
    printf("数据就绪状态: 0x%02X\n", QMI8658_GetDataReady());
    if (g_fifo_enabled) {
        printf("FIFO计数: %d\n", QMI8658_GetFIFOCount());
    }
    printf("========================\n");
}

// === 校准和自检接口函数实现 ===

uint8_t QMI8658_OnDemandCalibration(uint16_t *gX_gain, uint16_t *gY_gain, uint16_t *gZ_gain)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    ESP_LOGI(TAG, "开始在线校准...");
    
    // 发送在线校准命令
    if (qmi8658_write_command(QMI8658_CTRL_CMD_ON_DEMAND_CALIBRATION, 5000) != ESP_OK) {
        ESP_LOGE(TAG, "在线校准命令失败");
        return 0;
    }
    
    // 读取校准结果
    uint8_t cal_data[6];
    if (qmi8658_read_regs(QMI8658_REG_CAL1_L, cal_data, 6) == ESP_OK) {
        if (gX_gain) *gX_gain = cal_data[0] | (cal_data[1] << 8);
        if (gY_gain) *gY_gain = cal_data[2] | (cal_data[3] << 8);
        if (gZ_gain) *gZ_gain = cal_data[4] | (cal_data[5] << 8);
        
        ESP_LOGI(TAG, "在线校准完成，增益值: X=%d, Y=%d, Z=%d", 
                 *gX_gain, *gY_gain, *gZ_gain);
        return 1;
    }
    
    return 0;
}

uint8_t QMI8658_WriteCalibration(uint16_t gX_gain, uint16_t gY_gain, uint16_t gZ_gain)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    uint8_t cal_data[6];
    cal_data[0] = gX_gain & 0xFF;
    cal_data[1] = (gX_gain >> 8) & 0xFF;
    cal_data[2] = gY_gain & 0xFF;
    cal_data[3] = (gY_gain >> 8) & 0xFF;
    cal_data[4] = gZ_gain & 0xFF;
    cal_data[5] = (gZ_gain >> 8) & 0xFF;
    
    // 写入校准数据
    if (qmi8658_write_regs(QMI8658_REG_CAL1_L, cal_data, 6) != ESP_OK) {
        return 0;
    }
    
    // 应用陀螺仪增益
    if (qmi8658_write_command(QMI8658_CTRL_CMD_APPLY_GYRO_GAINS, 1000) != ESP_OK) {
        return 0;
    }
    
    ESP_LOGI(TAG, "校准参数已写入并应用");
    return 1;
}

void QMI8658_SetAccelOffset(int16_t offset_x, int16_t offset_y, int16_t offset_z)
{
    if (!g_qmi8658_initialized) {
        return;
    }
    
    uint8_t data[6];
    data[0] = offset_x & 0xFF;
    data[1] = (offset_x >> 8) & 0xFF;
    data[2] = offset_y & 0xFF;
    data[3] = (offset_y >> 8) & 0xFF;
    data[4] = offset_z & 0xFF;
    data[5] = (offset_z >> 8) & 0xFF;
    
    qmi8658_write_regs(QMI8658_REG_CAL1_L, data, 6);
    qmi8658_write_command(QMI8658_CTRL_CMD_ACCEL_HOST_DELTA_OFFSET, 1000);
    
    ESP_LOGI(TAG, "加速度计偏移已设置: X=%d, Y=%d, Z=%d", offset_x, offset_y, offset_z);
}

void QMI8658_SetGyroOffset(int16_t offset_x, int16_t offset_y, int16_t offset_z)
{
    if (!g_qmi8658_initialized) {
        return;
    }
    
    uint8_t data[6];
    data[0] = offset_x & 0xFF;
    data[1] = (offset_x >> 8) & 0xFF;
    data[2] = offset_y & 0xFF;
    data[3] = (offset_y >> 8) & 0xFF;
    data[4] = offset_z & 0xFF;
    data[5] = (offset_z >> 8) & 0xFF;
    
    qmi8658_write_regs(QMI8658_REG_CAL1_L, data, 6);
    qmi8658_write_command(QMI8658_CTRL_CMD_GYRO_HOST_DELTA_OFFSET, 1000);
    
    ESP_LOGI(TAG, "陀螺仪偏移已设置: X=%d, Y=%d, Z=%d", offset_x, offset_y, offset_z);
}

uint8_t QMI8658_SelfTestAccel(void)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    ESP_LOGI(TAG, "执行加速度计自检...");
    
    // 保存当前配置
    uint8_t orig_ctrl2;
    qmi8658_read_reg(QMI8658_REG_CTRL2, &orig_ctrl2);
    
    // 设置自检配置：2G量程，1000Hz
    uint8_t self_test_ctrl2 = (QMI8658_ACC_RANGE_2G << 4) | QMI8658_ACC_ODR_1000Hz;
    qmi8658_write_reg(QMI8658_REG_CTRL2, self_test_ctrl2);
    
    vTaskDelay(pdMS_TO_TICKS(50));  // 等待稳定
    
    // 读取无自检的数据
    QMI8658_IMUData_t data_normal;
    if (!QMI8658_GetAccelerometerData(&data_normal)) {
        qmi8658_write_reg(QMI8658_REG_CTRL2, orig_ctrl2);
        return 0;
    }
    
    // 使能自检
    uint8_t ctrl2_st = self_test_ctrl2 | 0x80;  // 设置自检位
    qmi8658_write_reg(QMI8658_REG_CTRL2, ctrl2_st);
    
    vTaskDelay(pdMS_TO_TICKS(50));  // 等待稳定
    
    // 读取自检数据
    QMI8658_IMUData_t data_selftest;
    if (!QMI8658_GetAccelerometerData(&data_selftest)) {
        qmi8658_write_reg(QMI8658_REG_CTRL2, orig_ctrl2);
        return 0;
    }
    
    // 恢复原始配置
    qmi8658_write_reg(QMI8658_REG_CTRL2, orig_ctrl2);
    
    // 计算差值
    float diff_x = fabs(data_selftest.x - data_normal.x);
    float diff_y = fabs(data_selftest.y - data_normal.y);
    float diff_z = fabs(data_selftest.z - data_normal.z);
    
    ESP_LOGI(TAG, "加速度计自检差值: X=%.3f, Y=%.3f, Z=%.3f", diff_x, diff_y, diff_z);
    
    // 检查差值是否在期望范围内（0.5g ~ 1.5g）
    const float min_diff = 0.5f * 9.80665f;  // 0.5g
    const float max_diff = 1.5f * 9.80665f;  // 1.5g
    
    if (diff_x >= min_diff && diff_x <= max_diff &&
        diff_y >= min_diff && diff_y <= max_diff &&
        diff_z >= min_diff && diff_z <= max_diff) {
        ESP_LOGI(TAG, "加速度计自检通过");
        return 1;
    } else {
        ESP_LOGE(TAG, "加速度计自检失败");
        return 0;
    }
}

uint8_t QMI8658_SelfTestGyro(void)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    ESP_LOGI(TAG, "执行陀螺仪自检...");
    
    // 保存当前配置
    uint8_t orig_ctrl3;
    qmi8658_read_reg(QMI8658_REG_CTRL3, &orig_ctrl3);
    
    // 设置自检配置：64dps量程，896.8Hz
    uint8_t self_test_ctrl3 = (QMI8658_GYR_RANGE_64DPS << 4) | QMI8658_GYR_ODR_896_8Hz;
    qmi8658_write_reg(QMI8658_REG_CTRL3, self_test_ctrl3);
    
    vTaskDelay(pdMS_TO_TICKS(50));  // 等待稳定
    
    // 读取无自检的数据
    QMI8658_IMUData_t data_normal;
    if (!QMI8658_GetGyroscopeData(&data_normal)) {
        qmi8658_write_reg(QMI8658_REG_CTRL3, orig_ctrl3);
        return 0;
    }
    
    // 使能自检
    uint8_t ctrl3_st = self_test_ctrl3 | 0x80;  // 设置自检位
    qmi8658_write_reg(QMI8658_REG_CTRL3, ctrl3_st);
    
    vTaskDelay(pdMS_TO_TICKS(50));  // 等待稳定
    
    // 读取自检数据
    QMI8658_IMUData_t data_selftest;
    if (!QMI8658_GetGyroscopeData(&data_selftest)) {
        qmi8658_write_reg(QMI8658_REG_CTRL3, orig_ctrl3);
        return 0;
    }
    
    // 恢复原始配置
    qmi8658_write_reg(QMI8658_REG_CTRL3, orig_ctrl3);
    
    // 计算差值
    float diff_x = fabs(data_selftest.x - data_normal.x);
    float diff_y = fabs(data_selftest.y - data_normal.y);
    float diff_z = fabs(data_selftest.z - data_normal.z);
    
    ESP_LOGI(TAG, "陀螺仪自检差值: X=%.3f, Y=%.3f, Z=%.3f", diff_x, diff_y, diff_z);
    
    // 检查差值是否在期望范围内（10dps ~ 50dps）
    const float min_diff = 10.0f;
    const float max_diff = 50.0f;
    
    if (diff_x >= min_diff && diff_x <= max_diff &&
        diff_y >= min_diff && diff_y <= max_diff &&
        diff_z >= min_diff && diff_z <= max_diff) {
        ESP_LOGI(TAG, "陀螺仪自检通过");
        return 1;
    } else {
        ESP_LOGE(TAG, "陀螺仪自检失败");
        return 0;
    }
}

// === 中断和状态接口函数实现 ===

void QMI8658_EnableINT(QMI8658_IntPin_t pin, bool enable)
{
    if (!g_qmi8658_initialized) {
        return;
    }
    
    uint8_t ctrl1;
    if (qmi8658_read_reg(QMI8658_REG_CTRL1, &ctrl1) != ESP_OK) {
        return;
    }
    
    if (pin == QMI8658_INT_PIN_1) {
        if (enable) {
            ctrl1 |= 0x08;  // 使能INT1
        } else {
            ctrl1 &= ~0x08; // 禁用INT1
        }
    } else if (pin == QMI8658_INT_PIN_2) {
        if (enable) {
            ctrl1 |= 0x10;  // 使能INT2
        } else {
            ctrl1 &= ~0x10; // 禁用INT2
        }
    }
    
    qmi8658_write_reg(QMI8658_REG_CTRL1, ctrl1);
    ESP_LOGI(TAG, "中断引脚%d %s", pin, enable ? "已使能" : "已禁用");
}

void QMI8658_EnableDataReadyINT(bool enable)
{
    if (!g_qmi8658_initialized) {
        return;
    }
    
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return;
    }
    
    if (enable) {
        ctrl7 |= 0x80;  // 使能数据就绪中断
    } else {
        ctrl7 &= ~0x80; // 禁用数据就绪中断
    }
    
    qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7);
    ESP_LOGI(TAG, "数据就绪中断%s", enable ? "已使能" : "已禁用");
}

uint8_t QMI8658_GetIrqStatus(void)
{
    if (!g_qmi8658_initialized) {
        return 0;
    }
    
    uint8_t status;
    if (qmi8658_read_reg(QMI8658_REG_STATUS_INT, &status) != ESP_OK) {
        return 0;
    }
    
    return status;
}

int QMI8658_GetStatusRegister(void)
{
    if (!g_qmi8658_initialized) {
        return -1;
    }
    
    uint8_t status0, status1;
    if (qmi8658_read_reg(QMI8658_REG_STATUS0, &status0) != ESP_OK) {
        return -1;
    }
    if (qmi8658_read_reg(QMI8658_REG_STATUS1, &status1) != ESP_OK) {
        return -1;
    }
    
    return (status1 << 8) | status0;
}

// === 电源管理接口函数实现 ===

void QMI8658_PowerDown(void)
{
    if (!g_qmi8658_initialized) {
        return;
    }
    
    // 禁用所有传感器（CTRL7寄存器）
    qmi8658_write_reg(QMI8658_REG_CTRL7, 0x00);
    
    ESP_LOGI(TAG, "传感器已掉电");
}

void QMI8658_PowerOn(void)
{
    if (!g_qmi8658_initialized) {
        return;
    }
    
    // 重新使能传感器（CTRL7寄存器）
    uint8_t ctrl7 = 0x00;
    if (g_accel_enabled) ctrl7 |= QMI8658_ACCEL_EN_MASK;  // bit 0
    if (g_gyro_enabled) ctrl7 |= QMI8658_GYRO_EN_MASK;    // bit 1
    
    qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7);
    
    // 确保CTRL1寄存器配置正确（地址自动递增等）
    qmi8658_write_reg(QMI8658_REG_CTRL1, 0x40);  // 地址自动递增
    
    ESP_LOGI(TAG, "传感器已上电");
}

// === 调试和信息接口函数实现 ===

void QMI8658_DumpCtrlRegister(void)
{
    if (!g_qmi8658_initialized) {
        printf("QMI8658未初始化\n");
        return;
    }
    
    printf("=== QMI8658 控制寄存器转储 ===\n");
    
    uint8_t regs[] = {
        QMI8658_REG_CTRL1, QMI8658_REG_CTRL2, QMI8658_REG_CTRL3,
        QMI8658_REG_CTRL5, QMI8658_REG_CTRL7, QMI8658_REG_CTRL8, QMI8658_REG_CTRL9
    };
    
    for (int i = 0; i < sizeof(regs); i++) {
        uint8_t value;
        if (qmi8658_read_reg(regs[i], &value) == ESP_OK) {
            printf("CTRL%d (0x%02X): 0x%02X\n", 
                   (regs[i] == QMI8658_REG_CTRL1) ? 1 :
                   (regs[i] == QMI8658_REG_CTRL2) ? 2 :
                   (regs[i] == QMI8658_REG_CTRL3) ? 3 :
                   (regs[i] == QMI8658_REG_CTRL5) ? 5 :
                   (regs[i] == QMI8658_REG_CTRL7) ? 7 :
                   (regs[i] == QMI8658_REG_CTRL8) ? 8 : 9,
                   regs[i], value);
        }
    }
    
    printf("=============================\n");
}

void QMI8658_GetChipUsid(uint8_t *buffer, uint8_t length)
{
    if (!buffer || length == 0) {
        return;
    }
    
    uint8_t copy_len = (length < 6) ? length : 6;
    memcpy(buffer, g_usid, copy_len);
}

uint32_t QMI8658_GetChipFirmwareVersion(void)
{
    return g_revision_id;
}

// === 内部命令接口函数实现 ===

uint8_t QMI8658_WriteCommand(QMI8658_CommandTable_t cmd, uint32_t wait_ms)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    return (qmi8658_write_command(cmd, wait_ms) == ESP_OK) ? 0 : 1;
}

// === 运动检测功能实现 ===

uint8_t QMI8658_ConfigMotion(const QMI8658_MotionConfig_t *config)
{
    if (!g_qmi8658_initialized || !config) {
        return 1;
    }
    
    // 准备运动检测配置数据
    uint8_t motion_config[16];
    motion_config[0] = config->mode_ctrl;
    motion_config[1] = qmi8658_mg_to_bytes(config->any_motion_x_thr);
    motion_config[2] = qmi8658_mg_to_bytes(config->any_motion_y_thr);
    motion_config[3] = qmi8658_mg_to_bytes(config->any_motion_z_thr);
    motion_config[4] = config->any_motion_window;
    motion_config[5] = qmi8658_mg_to_bytes(config->no_motion_x_thr);
    motion_config[6] = qmi8658_mg_to_bytes(config->no_motion_y_thr);
    motion_config[7] = qmi8658_mg_to_bytes(config->no_motion_z_thr);
    motion_config[8] = config->no_motion_window;
    motion_config[9] = config->sig_motion_wait_window & 0xFF;
    motion_config[10] = (config->sig_motion_wait_window >> 8) & 0xFF;
    motion_config[11] = config->sig_motion_confirm_window & 0xFF;
    motion_config[12] = (config->sig_motion_confirm_window >> 8) & 0xFF;
    motion_config[13] = 0; // 预留
    motion_config[14] = 0; // 预留
    motion_config[15] = 0; // 预留
    
    // 写入配置到CAL寄存器（使用前16字节）
    if (qmi8658_write_regs(QMI8658_REG_CAL1_L, motion_config, 16) != ESP_OK) {
        return 1;
    }
    
    // 发送配置命令
    if (qmi8658_write_command(QMI8658_CTRL_CMD_CONFIGURE_MOTION, 1000) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "运动检测配置完成");
    return 0;
}

uint8_t QMI8658_EnableMotionDetect(QMI8658_IntPin_t pin)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 配置中断引脚映射
    if (pin != QMI8658_INT_PIN_DISABLE) {
        uint8_t ctrl7;
        if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
            return 1;
        }
        
        if (pin == QMI8658_INT_PIN_1) {
            ctrl7 |= 0x01;  // 映射运动检测到INT1
        } else {
            ctrl7 |= 0x80;  // 映射运动检测到INT2
        }
        
        if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
            return 1;
        }
    }
    
    ESP_LOGI(TAG, "运动检测已使能，中断引脚: %d", pin);
    return 0;
}

uint8_t QMI8658_DisableMotionDetect(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    // 清除中断引脚映射
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 &= ~(0x01 | 0x80);  // 清除运动检测映射
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "运动检测已禁用");
    return 0;
}

// === 采样模式功能实现 ===

uint8_t QMI8658_EnableSyncSampleMode(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 |= 0x80;  // 使能同步采样模式
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "同步采样模式已使能");
    return 0;
}

uint8_t QMI8658_DisableSyncSampleMode(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl7;
    if (qmi8658_read_reg(QMI8658_REG_CTRL7, &ctrl7) != ESP_OK) {
        return 1;
    }
    
    ctrl7 &= ~0x80;  // 禁用同步采样模式
    if (qmi8658_write_reg(QMI8658_REG_CTRL7, ctrl7) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "同步采样模式已禁用");
    return 0;
}

uint8_t QMI8658_EnableLockingMechanism(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl8;
    if (qmi8658_read_reg(QMI8658_REG_CTRL8, &ctrl8) != ESP_OK) {
        return 1;
    }
    
    ctrl8 |= 0x02;  // 使能锁定机制
    if (qmi8658_write_reg(QMI8658_REG_CTRL8, ctrl8) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "锁定机制已使能");
    return 0;
}

uint8_t QMI8658_DisableLockingMechanism(void)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl8;
    if (qmi8658_read_reg(QMI8658_REG_CTRL8, &ctrl8) != ESP_OK) {
        return 1;
    }
    
    ctrl8 &= ~0x02;  // 禁用锁定机制
    if (qmi8658_write_reg(QMI8658_REG_CTRL8, ctrl8) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "锁定机制已禁用");
    return 0;
}

uint8_t QMI8658_ConfigActivityInterruptMap(QMI8658_IntPin_t pin)
{
    if (!g_qmi8658_initialized) {
        return 1;
    }
    
    uint8_t ctrl1;
    if (qmi8658_read_reg(QMI8658_REG_CTRL1, &ctrl1) != ESP_OK) {
        return 1;
    }
    
    // 清除现有的中断映射
    ctrl1 &= ~(0x08 | 0x10);
    
    if (pin == QMI8658_INT_PIN_1) {
        ctrl1 |= 0x08;  // 映射活动检测到INT1
    } else if (pin == QMI8658_INT_PIN_2) {
        ctrl1 |= 0x10;  // 映射活动检测到INT2
    }
    
    if (qmi8658_write_reg(QMI8658_REG_CTRL1, ctrl1) != ESP_OK) {
        return 1;
    }
    
    ESP_LOGI(TAG, "活动中断映射配置完成，引脚: %d", pin);
    return 0;
} 
/*
 * ES8311 音频编解码器驱动程序
 * 
 * 版权所有 (C) 2015-2022 乐鑫信息科技（上海）股份有限公司
 * 遵循 Apache-2.0 许可证
 * 
 * 功能描述：
 * - 支持8KHz到96KHz采样率
 * - 支持16/18/20/24/32位分辨率
 * - 支持模拟和数字麦克风
 * - 支持音量控制和静音功能
 * - 支持淡入淡出效果
 * - 通过I2C接口进行配置和控制
 * - 集成统一的I2C总线管理器
 */

#include <string.h>
#include "es8311_bsp.h"
#include "es8311_reg.h"
#include "I2CBusManager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/task.h"

/**
 * @brief ES8311设备结构体
 * 
 * 存储ES8311设备的基本信息，用于I2C通信
 */
typedef struct {
    uint8_t dev_addr;      // 设备I2C地址
    bool initialized;      // 设备初始化状态
} es8311_dev_t;

/**
 * @brief 时钟系数配置结构体
 * 
 * 用于存储不同采样率和主时钟频率下的分频器配置参数
 * 这些参数决定了ES8311内部各个时钟的分频比例
 */
struct _coeff_div {
    uint32_t mclk;        // 主时钟频率 (MCLK frequency in Hz)
    uint32_t rate;        // 采样率 (Sample rate in Hz)
    uint8_t pre_div;      // 预分频器，范围1-8 (Pre-divider: 1 to 8)
    uint8_t pre_multi;    // 预倍频器: 0=1x, 1=2x, 2=4x, 3=8x (Pre-multiplier selection)
    uint8_t adc_div;      // ADC时钟分频器 (ADC clock divider)
    uint8_t dac_div;      // DAC时钟分频器 (DAC clock divider)
    uint8_t fs_mode;      // 速度模式: 0=单倍速, 1=双倍速 (0=single speed, 1=double speed)
    uint8_t lrck_h;       // 左右声道时钟分频器高位 (LRCK divider high byte)
    uint8_t lrck_l;       // 左右声道时钟分频器低位 (LRCK divider low byte)
    uint8_t bclk_div;     // 位时钟分频器 (Bit clock divider)
    uint8_t adc_osr;      // ADC过采样率 (ADC oversampling rate)
    uint8_t dac_osr;      // DAC过采样率 (DAC oversampling rate)
};

/**
 * @brief ES8311高保真音频主时钟分频系数表
 * 
 * 该表包含了不同采样率和主时钟频率组合下的最优分频配置
 * 每一行对应一种MCLK和采样率的组合，包含相应的分频器设置
 * 
 * 表格列说明：
 * - mclk: 主时钟频率 (Hz)
 * - rate: 采样率 (Hz) 
 * - pre_div: 预分频器 (1-8)
 * - pre_multi: 预倍频器 (0=1x, 1=2x, 2=4x, 3=8x)
 * - adc_div: ADC分频器
 * - dac_div: DAC分频器  
 * - fs_mode: 速度模式 (0=单倍速, 1=双倍速)
 * - lrck_h: LRCK分频器高位
 * - lrck_l: LRCK分频器低位
 * - bclk_div: 位时钟分频器
 * - adc_osr: ADC过采样率
 * - dac_osr: DAC过采样率
 */
static const struct _coeff_div coeff_div[] = {
    /*   MCLK     采样率  预分频 倍频  ADC分频 DAC分频 速度模式 LRCK_H LRCK_L 位时钟分频 ADC_OSR DAC_OSR */
    /* 8kHz 采样率配置 */
    {12288000, 8000, 0x06, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 8000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x10},
    {16384000, 8000, 0x08, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 8000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 8000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 8000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 8000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000, 8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 11.025kHz 采样率配置 */
    {11289600, 11025, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 11025, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 11025, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 12kHz 采样率配置 */
    {12288000, 12000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 12000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 12000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 16kHz 采样率配置 */
    {12288000, 16000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 16000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 16000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 16000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 16000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 16000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000, 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 22.05kHz 采样率配置 */
    {11289600, 22050, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 22050, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {705600, 22050, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 24kHz 采样率配置 */
    {12288000, 24000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 24000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 24000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 32kHz 采样率配置 */
    {12288000, 32000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 32000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 32000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 32000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 32000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 32000, 0x03, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
    {1024000, 32000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 44.1kHz 采样率配置 (CD音质) */
    {11289600, 44100, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 44100, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 48kHz 采样率配置 (专业音频标准) */
    {12288000, 48000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 48000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 48000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 64kHz 采样率配置 */
    {12288000, 64000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 64000, 0x03, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {16384000, 64000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 64000, 0x01, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {4096000, 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 64000, 0x01, 0x03, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {2048000, 64000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
    {1024000, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 88.2kHz 采样率配置 (高保真音频) */
    {11289600, 88200, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 88200, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 88200, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 96kHz 采样率配置 (专业级高保真) */
    {12288000, 96000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 96000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 96000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 96000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
};

// 日志标签
static const char *TAG = "ES8311";

/**
 * @brief 向ES8311寄存器写入数据
 * 
 * @param dev ES8311设备句柄
 * @param reg_addr 寄存器地址
 * @param data 要写入的数据
 * @return esp_err_t 操作结果
 *         - ESP_OK: 成功
 *         - ESP_FAIL: I2C通信失败
 */
static inline esp_err_t es8311_write_reg(es8311_handle_t dev, uint8_t reg_addr, uint8_t data)
{
    es8311_dev_t *es = (es8311_dev_t *) dev;
    const uint8_t write_buf[2] = {reg_addr, data};
    
    // 使用统一的I2C总线管理器接口
    return I2CBus_Write(es->dev_addr, write_buf, sizeof(write_buf), I2C_BUS_TIMEOUT_MS);
}

/**
 * @brief 从ES8311寄存器读取数据
 * 
 * @param dev ES8311设备句柄
 * @param reg_addr 寄存器地址
 * @param reg_value 读取到的数据存储位置
 * @return esp_err_t 操作结果
 *         - ESP_OK: 成功
 *         - ESP_FAIL: I2C通信失败
 */
static inline esp_err_t es8311_read_reg(es8311_handle_t dev, uint8_t reg_addr, uint8_t *reg_value)
{
    es8311_dev_t *es = (es8311_dev_t *) dev;
    
    // 使用统一的I2C总线管理器接口进行写-读操作
    return I2CBus_WriteRead(es->dev_addr, &reg_addr, 1, reg_value, 1, I2C_BUS_TIMEOUT_MS);
}

/**
 * @brief 在时钟系数表中查找匹配的配置
 * 
 * 根据给定的主时钟频率和采样率，在预定义的系数表中查找匹配的配置
 * 
 * @param mclk 主时钟频率 (Hz)
 * @param rate 采样率 (Hz)
 * @return int 匹配的配置索引
 *         - >=0: 找到匹配配置的索引
 *         - -1: 未找到匹配的配置
 */
static int get_coeff(uint32_t mclk, uint32_t rate)
{
    // 遍历系数表查找匹配的MCLK和采样率组合
    for (int i = 0; i < (sizeof(coeff_div) / sizeof(coeff_div[0])); i++) {
        if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk) {
            return i;
        }
    }

    return -1;  // 未找到匹配的配置
}

/**
 * @brief 配置ES8311的采样频率
 * 
 * 根据指定的主时钟频率和采样频率，配置ES8311的时钟分频器
 * 该函数会从预定义的系数表中查找匹配的配置，并设置相应的寄存器
 * 
 * @param dev ES8311设备句柄
 * @param mclk_frequency 主时钟频率 (Hz)
 * @param sample_frequency 目标采样频率 (Hz)
 * @return esp_err_t 操作结果
 *         - ESP_OK: 配置成功
 *         - ESP_ERR_INVALID_ARG: 不支持的频率组合
 */
esp_err_t es8311_sample_frequency_config(es8311_handle_t dev, int mclk_frequency, int sample_frequency)
{
    uint8_t regv;

    // 从系数表中获取时钟配置参数
    int coeff = get_coeff(mclk_frequency, sample_frequency);

    if (coeff < 0) {
        ESP_LOGE(TAG, "无法为%dHz MCLK配置%dHz采样率", mclk_frequency, sample_frequency);
        return ESP_ERR_INVALID_ARG;
    }

    // 获取选中的时钟系数配置
    const struct _coeff_div *const selected_coeff = &coeff_div[coeff];

    // 配置寄存器0x02: 时钟预分频和预倍频设置
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG02, &regv), TAG, "I2C读写错误");
    regv &= 0x07;                                          // 保留低3位
    regv |= (selected_coeff->pre_div - 1) << 5;           // 设置预分频器 (bit7-5)
    regv |= selected_coeff->pre_multi << 3;               // 设置预倍频器 (bit4-3)
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG02, regv), TAG, "I2C读写错误");

    // 配置寄存器0x03: ADC速度模式和过采样率
    const uint8_t reg03 = (selected_coeff->fs_mode << 6) | selected_coeff->adc_osr;
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG03, reg03), TAG, "I2C读写错误");

    // 配置寄存器0x04: DAC过采样率
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG04, selected_coeff->dac_osr), TAG, "I2C读写错误");

    // 配置寄存器0x05: ADC和DAC分频器设置
    const uint8_t reg05 = ((selected_coeff->adc_div - 1) << 4) | (selected_coeff->dac_div - 1);
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG05, reg05), TAG, "I2C读写错误");

    // 配置寄存器0x06: 位时钟分频器设置
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG06, &regv), TAG, "I2C读写错误");
    regv &= 0xE0;                                          // 保留高3位

    // 根据分频值设置位时钟分频器
    if (selected_coeff->bclk_div < 19) {
        regv |= (selected_coeff->bclk_div - 1) << 0;       // 小于19时减1
    } else {
        regv |= (selected_coeff->bclk_div) << 0;           // 大于等于19时直接使用
    }

    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG06, regv), TAG, "I2C读写错误");

    // 配置寄存器0x07: 左右声道时钟分频器高位
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG07, &regv), TAG, "I2C读写错误");
    regv &= 0xC0;                                          // 保留高2位
    regv |= selected_coeff->lrck_h << 0;                   // 设置LRCK分频器高位
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG07, regv), TAG, "I2C读写错误");

    // 配置寄存器0x08: 左右声道时钟分频器低位
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG08, selected_coeff->lrck_l), TAG, "I2C读写错误");

    return ESP_OK;
}

static esp_err_t es8311_clock_config(es8311_handle_t dev, const es8311_clock_config_t *const clk_cfg, es8311_resolution_t res)
{
    uint8_t reg06;
    uint8_t reg01 = 0x3F; // Enable all clocks
    int mclk_hz;

    /* Select clock source for internal MCLK and determine its frequency */
    if (clk_cfg->mclk_from_mclk_pin) {
        mclk_hz = clk_cfg->mclk_frequency;
    } else {
        mclk_hz = clk_cfg->sample_frequency * (int)res * 2;
        reg01 |= BIT(7); // Select BCLK (a.k.a. SCK) pin
    }

    if (clk_cfg->mclk_inverted) {
        reg01 |= BIT(6); // Invert MCLK pin
    }
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG01, reg01), TAG, "I2C read/write error");

    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_CLK_MANAGER_REG06, &reg06), TAG, "I2C read/write error");
    if (clk_cfg->sclk_inverted) {
        reg06 |= BIT(5);
    } else {
        reg06 &= ~BIT(5);
    }
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_CLK_MANAGER_REG06, reg06), TAG, "I2C read/write error");

    /* Configure clock dividers */
    return es8311_sample_frequency_config(dev, mclk_hz, clk_cfg->sample_frequency);
}

static esp_err_t es8311_resolution_config(const es8311_resolution_t res, uint8_t *reg)
{
    switch (res) {
    case ES8311_RESOLUTION_16:
        *reg |= (3 << 2);
        break;
    case ES8311_RESOLUTION_18:
        *reg |= (2 << 2);
        break;
    case ES8311_RESOLUTION_20:
        *reg |= (1 << 2);
        break;
    case ES8311_RESOLUTION_24:
        *reg |= (0 << 2);
        break;
    case ES8311_RESOLUTION_32:
        *reg |= (4 << 2);
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static esp_err_t es8311_fmt_config(es8311_handle_t dev, const es8311_resolution_t res_in, const es8311_resolution_t res_out)
{
    uint8_t reg09 = 0; // SDP In
    uint8_t reg0a = 0; // SDP Out

    ESP_LOGI(TAG, "ES8311 in Slave mode and I2S format");
    uint8_t reg00;
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_RESET_REG00, &reg00), TAG, "I2C read/write error");
    reg00 &= 0xBF;
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_RESET_REG00, reg00), TAG, "I2C read/write error"); // Slave serial port - default

    /* Setup SDP In and Out resolution */
    es8311_resolution_config(res_in, &reg09);
    es8311_resolution_config(res_out, &reg0a);

    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_SDPIN_REG09, reg09), TAG, "I2C read/write error");
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_SDPOUT_REG0A, reg0a), TAG, "I2C read/write error");

    return ESP_OK;
}

/**
 * @brief 配置ES8311麦克风类型
 * 
 * 配置ES8311使用模拟麦克风或数字PDM麦克风
 * 
 * @param dev ES8311设备句柄
 * @param digital_mic 麦克风类型选择
 *                    - true: 使用数字PDM麦克风
 *                    - false: 使用模拟麦克风
 * @return esp_err_t 操作结果
 *         - ESP_OK: 配置成功
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t es8311_microphone_config(es8311_handle_t dev, bool digital_mic)
{
    uint8_t reg14 = 0x1A; // 启用模拟麦克风并设置最大PGA增益

    // PDM数字麦克风启用或禁用
    if (digital_mic) {
        reg14 |= BIT(6);                                   // 启用数字麦克风
    }
    
    // 设置ADC增益 (待移动到ADC配置部分)
    es8311_write_reg(dev, ES8311_ADC_REG17, 0xC8);

    return es8311_write_reg(dev, ES8311_SYSTEM_REG14, reg14);
}

/**
 * @brief 初始化ES8311音频编解码器
 * 
 * 该函数完成ES8311的完整初始化配置，包括复位、时钟配置、音频格式设置和基本参数配置
 * 
 * @param dev ES8311设备句柄
 * @param clk_cfg 时钟配置参数指针
 * @param res_in 输入分辨率(ADC)
 * @param res_out 输出分辨率(DAC)
 * @return esp_err_t 操作结果
 *         - ESP_OK: 初始化成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t es8311_init(es8311_handle_t dev, const es8311_clock_config_t *const clk_cfg, const es8311_resolution_t res_in, const es8311_resolution_t res_out)
{
    // 检查采样频率是否在支持范围内 (8kHz - 96kHz)
    ESP_RETURN_ON_FALSE(
        (clk_cfg->sample_frequency >= 8000) && (clk_cfg->sample_frequency <= 96000),
        ESP_ERR_INVALID_ARG, TAG, "ES8311初始化需要采样频率在[8000; 96000] Hz范围内"
    );
    
    // 如果主时钟来自SCK引脚，输入输出分辨率必须相同
    if (!clk_cfg->mclk_from_mclk_pin) {
        ESP_RETURN_ON_FALSE(res_out == res_in, ESP_ERR_INVALID_ARG, TAG, "当MCLK来自SCK引脚时，输入输出分辨率必须相同");
    }


    // 执行ES8311复位，恢复到默认状态
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_RESET_REG00, 0x1F), TAG, "I2C读写错误");
    vTaskDelay(20);                                        // 等待复位完成
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_RESET_REG00, 0x00), TAG, "I2C读写错误");
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_RESET_REG00, 0x80), TAG, "I2C读写错误"); // 上电命令

    // 配置时钟：时钟源、极性和分频器设置
    ESP_RETURN_ON_ERROR(es8311_clock_config(dev, clk_cfg, res_out), TAG, "时钟配置失败");

    // 配置音频格式：主从模式、分辨率、I2S格式
    ESP_RETURN_ON_ERROR(es8311_fmt_config(dev, res_in, res_out), TAG, "音频格式配置失败");

    // 启动模拟电路 - 非默认设置
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_SYSTEM_REG0D, 0x01), TAG, "I2C读写错误");
    
    // 启用模拟PGA和ADC调制器 - 非默认设置
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_SYSTEM_REG0E, 0x02), TAG, "I2C读写错误");
    
    // 启动DAC - 非默认设置
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_SYSTEM_REG12, 0x00), TAG, "I2C读写错误");
    
    // 启用耳机驱动输出 - 非默认设置
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_SYSTEM_REG13, 0x10), TAG, "I2C读写错误");
    
    // ADC均衡器旁路，消除数字域直流偏移
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_ADC_REG1C, 0x6A), TAG, "I2C读写错误");
    
    // 旁路DAC均衡器 - 非默认设置
    ESP_RETURN_ON_ERROR(es8311_write_reg(dev, ES8311_DAC_REG37, 0x08), TAG, "I2C读写错误");

    return ESP_OK;
}

/**
 * @brief 删除ES8311设备句柄
 * 
 * 释放设备句柄分配的内存资源
 * 
 * @param dev ES8311设备句柄
 */
void es8311_delete(es8311_handle_t dev)
{
    free(dev);
}

/**
 * @brief 设置ES8311输出音量
 * 
 * 设置DAC输出音量，音量范围为0-100%
 * 
 * @param dev ES8311设备句柄
 * @param volume 目标音量 (0-100)
 * @param volume_set 实际设置的音量值存储位置 (可为NULL)
 * @return esp_err_t 操作结果
 *         - ESP_OK: 设置成功
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t es8311_voice_volume_set(es8311_handle_t dev, int volume, int *volume_set)
{
    // 限制音量范围在0-100之间
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    int reg32;
    if (volume == 0) {
        reg32 = 0;                                         // 静音
    } else {
        reg32 = ((volume) * 256 / 100) - 1;              // 将百分比转换为寄存器值
    }

    // 返回实际设置的音量值
    if (volume_set != NULL) {
        *volume_set = volume;
    }
    return es8311_write_reg(dev, ES8311_DAC_REG32, reg32);
}

/**
 * @brief 获取ES8311当前输出音量
 * 
 * 读取DAC寄存器获取当前设置的音量值
 * 
 * @param dev ES8311设备句柄
 * @param volume 当前音量值存储位置 (0-100)
 * @return esp_err_t 操作结果
 *         - ESP_OK: 获取成功
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t es8311_voice_volume_get(es8311_handle_t dev, int *volume)
{
    uint8_t reg32;
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_DAC_REG32, &reg32), TAG, "I2C读写错误");

    if (reg32 == 0) {
        *volume = 0;                                       // 静音状态
    } else {
        *volume = ((reg32 * 100) / 256) + 1;             // 将寄存器值转换为百分比
    }
    return ESP_OK;
}

/**
 * @brief 控制ES8311输出静音
 * 
 * 通过设置DAC静音位来控制输出静音状态
 * 
 * @param dev ES8311设备句柄
 * @param mute 静音控制
 *             - true: 启用静音
 *             - false: 取消静音
 * @return esp_err_t 操作结果
 *         - ESP_OK: 设置成功
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t es8311_voice_mute(es8311_handle_t dev, bool mute)
{
    uint8_t reg31;
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_DAC_REG31, &reg31), TAG, "I2C读写错误");

    if (mute) {
        reg31 |= BIT(6) | BIT(5);                         // 设置左右声道静音位
    } else {
        reg31 &= ~(BIT(6) | BIT(5));                      // 清除左右声道静音位
    }

    return es8311_write_reg(dev, ES8311_DAC_REG31, reg31);
}

/**
 * @brief 设置ES8311麦克风增益
 * 
 * 设置ADC的输入增益以调节麦克风的信号强度
 * 
 * @param dev ES8311设备句柄
 * @param gain_db 增益值 (参考es8311_mic_gain_t枚举)
 * @return esp_err_t 操作结果
 *         - ESP_OK: 设置成功
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t es8311_microphone_gain_set(es8311_handle_t dev, es8311_mic_gain_t gain_db)
{
    return es8311_write_reg(dev, ES8311_ADC_REG16, gain_db); // 设置ADC增益放大
}

/**
 * @brief 设置ES8311输出淡入淡出效果
 * 
 * 控制DAC输出的淡入淡出时间，可以避免音频切换时的爆音
 * 
 * @param dev ES8311设备句柄
 * @param fade 淡入淡出时间配置 (参考es8311_fade_t枚举)
 * @return esp_err_t 操作结果
 *         - ESP_OK: 设置成功
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t es8311_voice_fade(es8311_handle_t dev, const es8311_fade_t fade)
{
    uint8_t reg37;
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_DAC_REG37, &reg37), TAG, "I2C读写错误");
    reg37 &= 0x0F;                                         // 保留低4位
    reg37 |= (fade << 4);                                  // 设置淡入淡出时间
    return es8311_write_reg(dev, ES8311_DAC_REG37, reg37);
}

/**
 * @brief 设置ES8311麦克风淡入淡出效果
 * 
 * 控制ADC输入的淡入淡出时间，可以避免录音开始/结束时的爆音
 * 
 * @param dev ES8311设备句柄
 * @param fade 淡入淡出时间配置 (参考es8311_fade_t枚举)
 * @return esp_err_t 操作结果
 *         - ESP_OK: 设置成功
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t es8311_microphone_fade(es8311_handle_t dev, const es8311_fade_t fade)
{
    uint8_t reg15;
    ESP_RETURN_ON_ERROR(es8311_read_reg(dev, ES8311_ADC_REG15, &reg15), TAG, "I2C读写错误");
    reg15 &= 0x0F;                                         // 保留低4位
    reg15 |= (fade << 4);                                  // 设置淡入淡出时间
    return es8311_write_reg(dev, ES8311_ADC_REG15, reg15);
}

/**
 * @brief 调试功能：打印ES8311所有寄存器值
 * 
 * 遍历读取并打印ES8311所有寄存器的当前值，用于调试和故障排除
 * 
 * @param dev ES8311设备句柄
 */
void es8311_register_dump(es8311_handle_t dev)
{
    // 遍历所有寄存器地址 (0x00 - 0x49)
    for (int reg = 0; reg < 0x4A; reg++) {
        uint8_t value;
        ESP_ERROR_CHECK(es8311_read_reg(dev, reg, &value));
        printf("REG:%02x: %02x", reg, value);
    }
}

/**
 * @brief 创建ES8311设备句柄
 * 
 * 分配内存并初始化ES8311设备结构体，设置I2C设备地址
 * 
 * @param dev_addr ES8311设备的I2C地址
 * @return es8311_handle_t 设备句柄
 *         - 非NULL: 创建成功
 *         - NULL: 内存分配失败
 */
es8311_handle_t es8311_create(const uint8_t dev_addr)
{
    // 检查I2C总线是否已初始化
    if (!I2CBus_IsInitialized()) {
        ESP_LOGE(TAG, "I2C总线未初始化，请先调用 I2CBus_Init()");
        return NULL;
    }
    
    // 验证设备地址是否有效
    if (dev_addr != I2C_ADDR_ES8311_0 && dev_addr != I2C_ADDR_ES8311_1) {
        ESP_LOGE(TAG, "无效的ES8311设备地址: 0x%02X", dev_addr);
        return NULL;
    }
    
    // 分配设备结构体内存
    es8311_dev_t *sensor = (es8311_dev_t *) calloc(1, sizeof(es8311_dev_t));
    if (sensor == NULL) {
        ESP_LOGE(TAG, "ES8311设备内存分配失败");
        return NULL;
    }
    
    sensor->dev_addr = dev_addr;                          // 设置设备地址
    sensor->initialized = false;                          // 设置初始化状态
    
    // 验证设备是否存在
    if (!I2CBus_ScanDevice(dev_addr)) {
        ESP_LOGE(TAG, "ES8311设备未找到，地址: 0x%02X", dev_addr);
        free(sensor);
        return NULL;
    }
    
    ESP_LOGI(TAG, "ES8311设备创建成功，地址: 0x%02X", dev_addr);
    return (es8311_handle_t) sensor;
}
